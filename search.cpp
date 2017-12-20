/*
    Laser, a UCI chess engine written in C++11.
    Copyright 2015-2017 Jeffrey An and Michael An

    Laser is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Laser is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Laser.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <algorithm>
#include <atomic>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>
#include "eval.h"
#include "evalhash.h"
#include "hash.h"
#include "search.h"
#include "moveorder.h"
#include "searchparams.h"
#include "uci.h"
#include "syzygy/tbprobe.h"

using std::cout;
using std::cerr;
using std::endl;


// Records useful statistics which are printed to std::err at the end of each search
struct SearchStatistics {
    uint64_t nodes;
    uint64_t tbhits;
    uint64_t hashProbes, hashHits, hashScoreCuts;
    uint64_t hashMoveAttempts, hashMoveCuts;
    uint64_t failHighs, firstFailHighs;
    uint64_t qsNodes;
    uint64_t qsFailHighs, qsFirstFailHighs;
    uint64_t evalCacheProbes, evalCacheHits;

    SearchStatistics() {
        reset();
    }

    void reset() {
        nodes = 0;
        tbhits = 0;
        hashProbes = hashHits = hashScoreCuts = 0;
        hashMoveAttempts = hashMoveCuts = 0;
        failHighs = firstFailHighs = 0;
        qsNodes = 0;
        qsFailHighs = qsFirstFailHighs = 0;
        evalCacheProbes = evalCacheHits = 0;
    }
};

// Records the PV found by the search.
struct SearchPV {
    int pvLength;
    Move pv[MAX_DEPTH+1];

    SearchPV() {
        pvLength = 0;
    }
};

// Stores information about candidate easymoves.
struct EasyMove {
    Move prevBest;
    Move streakBest;
    int pvStreak;

    void reset() {
        prevBest = NULL_MOVE;
        streakBest = NULL_MOVE;
        pvStreak = 0;
    }
};

//-------------------------------Search Constants-------------------------------
// Futility pruning margins indexed by depth. If static eval is at least this
// amount below alpha, we skip quiet moves for this position.
const int FUTILITY_MARGIN[7] = {0,
    80,
    170,
    270,
    380,
    500,
    630
};

// Reverse futility pruning margins indexed by depth. If static eval is at least
// this amount above beta, we skip searching the position entirely.
const int REVERSE_FUTILITY_MARGIN[7] = {0,
    60,
    130,
    210,
    300,
    400,
    510
};

// Razor margins indexed by depth. If static eval is far below alpha, use a
// qsearch to confirm fail low and then return.
const int RAZOR_MARGIN[4] = {0,
    240,
    280,
    300
};

// Move count pruning
const unsigned int LMP_MOVE_COUNTS[2][8] = {{0,
    3, 5, 8, 13, 20, 30, 45},
{0, 5, 8, 13, 21, 33, 50, 72}
};


//-----------------------------Global variables---------------------------------
static Hash transpositionTable(DEFAULT_HASH_SIZE);
static EvalHash evalCache(DEFAULT_HASH_SIZE);
static std::vector<SearchParameters *> searchParamsArray;
static std::vector<SearchStatistics *> searchStatsArray;
static EasyMove easyMoveInfo;
static SearchStackInfo **ssInfo;

// Accessible from uci.cpp
TwoFoldStack twoFoldPositions[MAX_THREADS];

// Used to break out of the search thread if the stop command is given
std::atomic<bool> isStop(true);
// Additional stop signal to stop helper threads during SMP
std::atomic<bool> stopSignal(true);
// Used to ensure all threads have terminated
std::atomic<int> threadsRunning;
std::mutex threadsRunningMutex;

// Dummy variables for lazy SMP since we don't care about these results
int dummyBestIndex[MAX_THREADS-1];
int dummyBestScore[MAX_THREADS-1];

// Values for UCI options
unsigned int multiPV;
int numThreads;
bool isPonderSearch = false;

// Accessible from tbcore.c
int TBlargest = 0;
static int probeLimit = 0;


// Search functions
void getBestMoveAtDepth(Board *b, MoveList *legalMoves, int depth, int alpha,
    int beta, int *bestMoveIndex, int *bestScore, unsigned int startMove,
    int threadID, SearchPV *pvLine);
void getBestMoveAtDepthHelper(Board *b, MoveList *legalMoves, int depth, int alpha,
    int beta, int *bestMoveIndex, int *bestScore, unsigned int startMove,
    int threadID, SearchPV *pvLine);
int PVS(Board &b, int depth, int alpha, int beta, int threadID, bool isCutNode, SearchStackInfo *ssi, SearchPV *pvLine);
int quiescence(Board &b, int plies, int alpha, int beta, int threadID);
int checkQuiescence(Board &b, int plies, int alpha, int beta, int threadID);

// Search helpers
int scoreMate(bool isInCheck, int plies);
int adjustHashScore(int score, int plies);

// Other utility functions
Move nextMove(MoveList &moves, ScoreList &scores, unsigned int index);
uint64_t getTBHits();
void changePV(Move best, SearchPV *parent, SearchPV *child);
std::string retrievePV(SearchPV *pvLine);
int getSelectiveDepth();
double getPercentage(uint64_t numerator, uint64_t denominator);
void printStatistics();


/**
 * @brief Finds a best move for a position according to the given search parameters.
 * @param mode The search mode: time or depth
 * @param value The time limit if in time mode, or the depth to search
 */
void getBestMove(Board *b, int mode, int value, Move *bestMove, MoveList *movesToSearch) {
    for (int i = 0; i < numThreads; i++) {
        searchParamsArray[i]->reset();
        searchStatsArray[i]->reset();
        searchParamsArray[i]->rootMoveNumber = (uint8_t) (b->getMoveNumber());
        searchParamsArray[i]->selectiveDepth = 0;
    }

    int color = b->getPlayerToMove();
    MoveList legalMoves = b->getAllLegalMoves(color);
    Move ponder = NULL_MOVE;

    // Special case if we are given a mate/stalemate position
    if (legalMoves.size() <= 0) {
        *bestMove = NULL_MOVE;
        stopSignal = true;
        isStop = true;
        cout << "bestmove none" << endl;
        return;
    }

    *bestMove = legalMoves.get(0);

    // Set up timing
    searchParamsArray[0]->timeLimit = (mode == TIME) ? (uint64_t) (MAX_TIME_FACTOR * value)
                                                    : (mode == MOVETIME) ? value
                                                                         : MAX_TIME;
    searchParamsArray[0]->startTime = ChessClock::now();
    uint64_t timeSoFar = getTimeElapsed(searchParamsArray[0]->startTime);

    // Special case if there is only one legal move: use less search time,
    // only to get a rough PV/score
    if (legalMoves.size() == 1 && mode == TIME) {
        searchParamsArray[0]->timeLimit = std::min(searchParamsArray[0]->timeLimit / 32, ONE_SECOND);
    }


    // Root probe Syzygy
    probeLimit = TBlargest;
    int tbScore = 0;
    bool tbProbeSuccess = false;
    unsigned int prevLMSize = legalMoves.size();
    if (TBlargest && count(b->getAllPieces(WHITE) | b->getAllPieces(BLACK)) <= TBlargest) {
        ScoreList scores;
        // Try probing with DTZ tables first
        int tbProbeResult = root_probe(b, legalMoves, scores, tbScore);
        if (tbProbeResult) {
            // With DTZ table filtering, we have guaranteed that we will not
            // make a mistake so do not probe TBs in search
            probeLimit = 0;
            tbProbeSuccess = true;
            searchStatsArray[0]->tbhits += prevLMSize;
        }
        // If unsuccessful, try WDL tables
        else {
            scores.clear();
            tbScore = 0;
            tbProbeResult = root_probe_wdl(b, legalMoves, scores, tbScore);
            if (tbProbeResult) {
                tbProbeSuccess = true;
                searchStatsArray[0]->tbhits += prevLMSize;
                // Only probe to maintain a win
                if (tbScore <= 0)
                    probeLimit = 0;
            }
        }
    }


    // If we were told to search specific moves, filter them here.
    if (movesToSearch->size() > 0) {
        MoveList temp;
        for (unsigned int i = 0; i < legalMoves.size(); i++) {
            for (unsigned int j = 0; j < movesToSearch->size(); j++) {
                if (legalMoves.get(i) == movesToSearch->get(j))
                    temp.add(legalMoves.get(i));
            }
        }
        legalMoves = temp;
    }


    int bestScore, bestMoveIndex;
    int rootDepth = 1;
    Move prevBest = NULL_MOVE;
    int pvStreak = 0;

    // Iterative deepening loop
    do {
        // For recording the PV
        SearchPV pvLine;

        // Handle multi PV (if multiPV = 1 then the loop is only run once)
        for (unsigned int multiPVNum = 1;
                          multiPVNum <= multiPV
                            && multiPVNum <= legalMoves.size();
                          multiPVNum++) {
            int aspAlpha = -MATE_SCORE;
            int aspBeta = MATE_SCORE;
            // Initial aspiration window based on depth and score
            int deltaAlpha = 20 - std::min(rootDepth/3, 10) + abs(bestScore) / 20;
            int deltaBeta = deltaAlpha;

            // Set up aspiration windows
            if (rootDepth >= 6 && multiPV == 1 && abs(bestScore) < NEAR_MATE_SCORE) {
                aspAlpha = bestScore - deltaAlpha;
                aspBeta = bestScore + deltaBeta;
            }

            deltaAlpha *= 2;
            deltaBeta *= 2;

            // Aspiration loop
            while (!isStop) {
                // Reset all search parameters (killers, plies, etc)
                for (int i = 0; i < numThreads; i++)
                    searchParamsArray[i]->reset();
                pvLine.pvLength = 0;
                threadsRunning = numThreads-1;

                // Get the index of the best move
                // If depth >= 7 create threads for SMP
                if (rootDepth >= 7 && numThreads > 1) {
                    std::thread *threadPool = new std::thread[numThreads-1];

                    // Start secondary threads
                    // Start even threads with depth = rootDepth+1
                    //   (idea from Dan Homan, author of ExChess)
                    for (int i = 1; i < numThreads; i++) {
                        // Copy over the two-fold stack to use
                        twoFoldPositions[i] = twoFoldPositions[0];
                        threadPool[i-1] = std::thread(getBestMoveAtDepthHelper, b,
                            &legalMoves, rootDepth + (i % 2), aspAlpha, aspBeta,
                            dummyBestIndex+i-1, dummyBestScore+i-1,
                            multiPVNum-1, i, nullptr);
                        // Detach secondary threads
                        threadPool[i-1].detach();
                    }

                    // Start the primary result thread
                    getBestMoveAtDepth(b, &legalMoves, rootDepth, aspAlpha, aspBeta,
                        &bestMoveIndex, &bestScore, multiPVNum-1, 0, &pvLine);

                    stopSignal = true;
                    // Wait for all other threads to finish
                    while (threadsRunning > 0)
                        std::this_thread::yield();
                    stopSignal = false;

                    delete[] threadPool;
                }
                // Otherwise, just search with one thread
                else {
                    getBestMoveAtDepth(b, &legalMoves, rootDepth, aspAlpha, aspBeta,
                        &bestMoveIndex, &bestScore, multiPVNum-1, 0, &pvLine);
                }

                timeSoFar = getTimeElapsed(searchParamsArray[0]->startTime);
                // Calculate values for printing
                uint64_t nps = 1000 * getNodes() / timeSoFar;
                std::string pvStr = retrievePV(&pvLine);
                if (pvLine.pvLength > 1)
                    ponder = pvLine.pv[1];
                else if (bestMoveIndex != 0)
                    ponder = NULL_MOVE;

                // Handle fail highs and fail lows
                // Fail low: no best move found
                if (bestMoveIndex == -1 && !isStop) {
                    cout << "info depth " << rootDepth;
                    cout << " seldepth " << getSelectiveDepth();
                    cout << " score";
                    cout << " cp " << (tbProbeSuccess ? (tbScore == 0 ? 0 : (bestScore/10 + tbScore)) : bestScore) * 100 / PIECE_VALUES[EG][PAWNS] << " upperbound";

                    cout << " time " << timeSoFar
                         << " nodes " << getNodes() << " nps " << nps
                         << " tbhits " << getTBHits()
                         << " hashfull " << 1000 * transpositionTable.keys
                                                 / transpositionTable.getSize()
                         << " pv " << pvStr << endl;

                    aspAlpha = bestScore - deltaAlpha;
                    deltaAlpha *= 2;
                    if (aspAlpha < -NEAR_MATE_SCORE)
                        aspAlpha = -MATE_SCORE;
                }
                // Fail high: best score is at least beta
                else if (bestScore >= aspBeta) {
                    cout << "info depth " << rootDepth;
                    cout << " seldepth " << getSelectiveDepth();
                    cout << " score";
                    cout << " cp " << (tbProbeSuccess ? (tbScore == 0 ? 0 : (bestScore/10 + tbScore)) : bestScore) * 100 / PIECE_VALUES[EG][PAWNS] << " lowerbound";

                    cout << " time " << timeSoFar
                         << " nodes " << getNodes() << " nps " << nps
                         << " tbhits " << getTBHits()
                         << " hashfull " << 1000 * transpositionTable.keys
                                                 / transpositionTable.getSize()
                         << " pv " << pvStr << endl;

                    aspBeta = bestScore + deltaBeta;
                    deltaBeta *= 2;
                    if (aspBeta > NEAR_MATE_SCORE)
                        aspBeta = MATE_SCORE;

                    // If the best move is still the same, do not necessarily
                    // resolve the fail high
                    if ((int) multiPVNum - 1 == bestMoveIndex
                     && *bestMove == prevBest
                     && mode == TIME
                     && (timeSoFar >= value * TIME_FACTOR))
                        break;

                    legalMoves.swap(multiPVNum-1, bestMoveIndex);
                    *bestMove = legalMoves.get(0);
                }
                // If no fails, we are done
                else break;
            }
            // End aspiration loop

            // Calculate values for printing
            timeSoFar = getTimeElapsed(searchParamsArray[0]->startTime);
            uint64_t nps = 1000 * getNodes() / timeSoFar;
            std::string pvStr = retrievePV(&pvLine);

            // If we broke out before getting any new results, end the search
            if (bestMoveIndex == -1) {
                cout << "info depth " << rootDepth-1;
                cout << " seldepth " << getSelectiveDepth();
                cout << " time " << timeSoFar
                     << " nodes " << getNodes() << " nps " << nps
                     << " tbhits " << getTBHits()
                     << " hashfull " << 1000 * transpositionTable.keys
                                             / transpositionTable.getSize()
                     << endl;
                break;
            }

            // Swap the PV to be searched first next iteration
            legalMoves.swap(multiPVNum-1, bestMoveIndex);
            *bestMove = legalMoves.get(0);

            // Output info using UCI protocol
            cout << "info depth " << rootDepth;
            cout << " seldepth " << getSelectiveDepth();
            if (multiPV > 1)
                cout << " multipv " << multiPVNum;
            cout << " score";

            // Print score in mate or centipawns
            if (bestScore >= MATE_SCORE - MAX_DEPTH)
                // If it is our mate, it takes plies / 2 + 1 moves to mate since
                // our move ends the game
                cout << " mate " << (MATE_SCORE - bestScore) / 2 + 1;
            else if (bestScore <= -MATE_SCORE + MAX_DEPTH)
                // If we are being mated, it takes plies / 2 moves since our
                // opponent's move ends the game
                cout << " mate " << (-MATE_SCORE - bestScore) / 2;
            else
                // Scale score into centipawns using our internal pawn value
                cout << " cp " << (tbProbeSuccess ? (tbScore == 0 ? 0 : (bestScore/10 + tbScore)) : bestScore) * 100 / PIECE_VALUES[EG][PAWNS];

            cout << " time " << timeSoFar
                 << " nodes " << getNodes() << " nps " << nps
                 << " tbhits " << getTBHits()
                 << " hashfull " << 1000 * transpositionTable.keys
                                         / transpositionTable.getSize()
                 << " pv " << pvStr << endl;
        }
        // End multiPV loop

        // Record candidate easymoves
        if (multiPV == 1 && pvLine.pvLength >= 3) {
            if (pvLine.pv[2] == easyMoveInfo.streakBest) {
                easyMoveInfo.pvStreak++;
            }
            else {
                easyMoveInfo.streakBest = pvLine.pv[2];
                easyMoveInfo.pvStreak = 1;
            }
        }

        if (*bestMove == prevBest) {
            pvStreak++;
        }
        else {
            prevBest = *bestMove;
            pvStreak = 1;
        }

        // Easymove confirmation
        if (!isPonderSearch && mode == TIME && multiPV == 1
         && timeSoFar > (uint64_t) value / 16
         && timeSoFar < (uint64_t) value / 2
         && abs(bestScore) < MATE_SCORE - MAX_DEPTH) {
            if ((*bestMove == easyMoveInfo.prevBest && pvStreak >= 7)
                || pvStreak >= 10) {
                int secondBestMove;
                int secondBestScore;
                int easymoveWindow = bestScore - EASYMOVE_MARGIN - abs(bestScore) / 3;

                getBestMoveAtDepth(b, &legalMoves, rootDepth-5, easymoveWindow - 1, easymoveWindow,//-MATE_SCORE, MATE_SCORE,
                    &secondBestMove, &secondBestScore, 1, 0, &pvLine);

                if (secondBestScore < easymoveWindow)
                    break;
                else
                    pvStreak = -128;
            }
        }

        rootDepth++;
    }
    // Conditions for iterative deepening loop
    while (!isStop
        && ((((mode == TIME && timeSoFar < (uint64_t) value * TIME_FACTOR) || isPonderSearch)
                          && rootDepth <= MAX_DEPTH)
         || (mode == MOVETIME && timeSoFar < (uint64_t) value
                              && rootDepth <= MAX_DEPTH)
         || (mode == DEPTH && rootDepth <= value)));

    // If we found a candidate easymove for the next ply this search
    if (easyMoveInfo.pvStreak >= 8) {
        easyMoveInfo.prevBest = easyMoveInfo.streakBest;
    }
    else
        easyMoveInfo.reset();

    // When pondering, we must continue "searching" until given a stop or ponderhit command.
    while (isPonderSearch && !isStop)
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

    printStatistics();

    // Aging for the history heuristic table
    for (int i = 0; i < numThreads; i++)
        searchParamsArray[i]->ageHistoryTable(rootDepth);

    // Reset the hashfull counter
    transpositionTable.keys = 0;

    // Output best move to UCI interface
    stopSignal = true;
    isStop = true;
    if (ponder != NULL_MOVE)
        cout << "bestmove " << moveToString(*bestMove) << " ponder " << moveToString(ponder) << endl;
    else
        cout << "bestmove " << moveToString(*bestMove) << endl;
    return;
}

void getBestMoveAtDepthHelper(Board *b, MoveList *legalMoves, int depth, int alpha,
        int beta, int *bestMoveIndex, int *bestScore, unsigned int startMove,
        int threadID, SearchPV *pvLine) {
    while (depth <= MAX_DEPTH && !stopSignal) {
        getBestMoveAtDepth(b, legalMoves, depth, alpha, beta,
                           bestMoveIndex, bestScore, startMove, threadID, pvLine);
        depth++;
    }

    // This thread is finished running.
    threadsRunningMutex.lock();
    threadsRunning--;
    threadsRunningMutex.unlock();
}

/**
 * @brief Returns the index of the best move in legalMoves.
 */
void getBestMoveAtDepth(Board *b, MoveList *legalMoves, int depth, int alpha,
        int beta, int *bestMoveIndex, int *bestScore, unsigned int startMove,
        int threadID, SearchPV *pvLine) {
    // SearchParameters *searchParams = searchParamsArray[threadID];
    SearchStatistics *searchStats = searchStatsArray[threadID];
    SearchPV line;
    int color = b->getPlayerToMove();
    int tempMove = -1;
    int score = -MATE_SCORE;
    *bestScore = -INFTY;
    SearchStackInfo *ssi = &(ssInfo[threadID][1]);

    // Push current position to two fold stack
    twoFoldPositions[threadID].push(b->getZobristKey());

    // Helps prevent stalls when using more threads than physical cores,
    // presumably by preventing this function from completing before the thread
    // is able to detach.
    std::this_thread::yield();

    for (unsigned int i = startMove; i < legalMoves->size(); i++) {
        // Output current move info to the GUI. Only do so if 5 seconds of
        // search have elapsed to avoid clutter
        uint64_t timeSoFar = getTimeElapsed(searchParamsArray[0]->startTime);
        uint64_t nps = 1000 * getNodes() / timeSoFar;
        if (threadID == 0 && timeSoFar > 5 * ONE_SECOND)
            cout << "info depth " << depth << " currmove " << moveToString(legalMoves->get(i))
                 << " currmovenumber " << i+1 << " nodes " << getNodes() << " nps " << nps << endl;

        Board copy = b->staticCopy();
        copy.doMove(legalMoves->get(i), color);
        searchStats->nodes++;

        int startSq = getStartSq(legalMoves->get(i));
        int endSq = getEndSq(legalMoves->get(i));
        int pieceID = b->getPieceOnSquare(color, startSq);
        ssi->counterMoveHistory = searchParamsArray[threadID]->counterMoveHistory[pieceID][endSq];

        if (i != 0) {
            score = -PVS(copy, depth-1, -alpha-1, -alpha, threadID, true, ssi, &line);
            if (alpha < score && score < beta) {
                score = -PVS(copy, depth-1, -beta, -alpha, threadID, false, ssi, &line);
            }
        }
        else {
            score = -PVS(copy, depth-1, -beta, -alpha, threadID, false, ssi, &line);
        }

        // Stop condition. If stopping, return search results from incomplete
        // search, if any.
        if (stopSignal.load(std::memory_order_seq_cst))
            break;

        if (score > *bestScore) {
            *bestScore = score;
            if (score > alpha) {
                alpha = score;
                tempMove = (int) i;
                if (pvLine != nullptr)
                    changePV(legalMoves->get(i), pvLine, &line);
            }
            // To get a PV if failing low
            else if (pvLine != nullptr && i == 0)
                changePV(legalMoves->get(i), pvLine, &line);
        }

        if (score >= beta)
            break;
    }

    twoFoldPositions[threadID].pop();
    *bestMoveIndex = tempMove;
}

// Gets a best move to try first when a hash move is not available.
int getBestMoveForSort(Board *b, MoveList &legalMoves, int depth, int threadID, SearchStackInfo *ssi) {
    // SearchParameters *searchParams = searchParamsArray[threadID];
    SearchStatistics *searchStats = searchStatsArray[threadID];
    SearchPV line;
    int color = b->getPlayerToMove();
    int tempMove = -1;
    int score = -MATE_SCORE;
    int alpha = -MATE_SCORE;
    int beta = MATE_SCORE;

    // Push current position to two fold stack
    twoFoldPositions[threadID].push(b->getZobristKey());

    for (unsigned int i = 0; i < legalMoves.size(); i++) {
        Board copy = b->staticCopy();
        if (!copy.doPseudoLegalMove(legalMoves.get(i), color))
            continue;
        searchStats->nodes++;

        int startSq = getStartSq(legalMoves.get(i));
        int endSq = getEndSq(legalMoves.get(i));
        int pieceID = b->getPieceOnSquare(color, startSq);
        (ssi+1)->counterMoveHistory = searchParamsArray[threadID]->counterMoveHistory[pieceID][endSq];

        if (i != 0) {
            score = -PVS(copy, depth-1, -alpha-1, -alpha, threadID, true, ssi+1, &line);
            if (alpha < score && score < beta) {
                score = -PVS(copy, depth-1, -beta, -alpha, threadID, false, ssi+1, &line);
            }
        }
        else {
            score = -PVS(copy, depth-1, -beta, -alpha, threadID, false, ssi+1, &line);
        }

        // Stop condition to break out as quickly as possible
        if (stopSignal.load(std::memory_order_relaxed))
            return i;

        if (score > alpha) {
            alpha = score;
            tempMove = i;
        }
    }

    twoFoldPositions[threadID].pop();

    return tempMove;
}

//------------------------------------------------------------------------------
//------------------------------Search functions--------------------------------
//------------------------------------------------------------------------------
// The standard implementation of a fail-soft PVS search.
int PVS(Board &b, int depth, int alpha, int beta, int threadID, bool isCutNode, SearchStackInfo *ssi, SearchPV *pvLine) {
    SearchParameters *searchParams = searchParamsArray[threadID];
    SearchStatistics *searchStats = searchStatsArray[threadID];
    // Reset the PV line
    pvLine->pvLength = 0;
    // When the standard search is done, enter quiescence search.
    if (depth <= 0 || ssi->ply >= MAX_DEPTH) {
        // Update selective depth if necessary
        if (ssi->ply > searchParams->selectiveDepth)
            searchParams->selectiveDepth = ssi->ply;
        searchParams->ply = ssi->ply;

        return quiescence(b, 0, alpha, beta, threadID);
    }

    // Draw check
    if (b.isDraw())
        return 0;
    if (twoFoldPositions[threadID].find(b.getZobristKey()))
        return 0;


    // Mate distance pruning
    int matingScore = MATE_SCORE - ssi->ply;
    if (matingScore < beta) {
        beta = matingScore;
        if (alpha >= matingScore)
            return alpha;
    }

    int matedScore = -MATE_SCORE + ssi->ply;
    if (matedScore > alpha) {
        alpha = matedScore;
        if (beta <= matedScore)
            return beta;
    }


    int prevAlpha = alpha;
    int color = b.getPlayerToMove();
    // For PVS, the node is a PV node if beta - alpha != 1 (not a null window)
    // We do not want to do most pruning techniques on PV nodes
    bool isPVNode = (beta - alpha != 1);


    // Transposition table probe
    // If a cutoff or exact score hit occurred, probeTT will return a value
    // other than -INFTY
    Move hashed = NULL_MOVE;
    int hashScore = -INFTY;
    int hashDepth = 0;
    uint8_t nodeType = NO_NODE_INFO;
    searchStats->hashProbes++;

    uint64_t hashEntry = transpositionTable.get(b);
    if (hashEntry != 0) {
        searchStats->hashHits++;
        hashScore = getHashScore(hashEntry);
        nodeType = getHashNodeType(hashEntry);
        hashDepth = getHashDepth(hashEntry);
        hashed = getHashMove(hashEntry);

        // Count hashed tb hits
        if (nodeType == PV_NODE && hashed == NULL_MOVE)
            searchStats->tbhits++;

        // Adjust the hash score to mate distance from root if necessary
        if (hashScore >= MATE_SCORE - MAX_DEPTH)
            hashScore -= ssi->ply;
        else if (hashScore <= -MATE_SCORE + MAX_DEPTH)
            hashScore += ssi->ply;

        // Return hash score failing soft if hash depth >= current depth and:
        //   The node is a hashed all node and score <= alpha
        //   The node is a hashed cut node and score >= beta
        //   The node is a hashed PV node and we are searching on a null window
        //    (we do not return immediately on full PVS windows since this cuts
        //     short the PV line)
        if (!isPVNode && hashDepth >= depth) {
            if ((nodeType == ALL_NODE && hashScore <= alpha)
             || (nodeType == CUT_NODE && hashScore >= beta)
             || (nodeType == PV_NODE)) {
                searchStats->hashScoreCuts++;
                return hashScore;
            }
        }
    }


    // Tablebase probe
    // We use Stockfish's strategy of only probing WDL tables in the main search
    if (probeLimit
     && count(b.getAllPieces(WHITE) | b.getAllPieces(BLACK)) <= probeLimit
     && b.getFiftyMoveCounter() == 0
     && !b.getAnyCanCastle()) {
        int tbProbeResult;
        int tbValue = probe_wdl(b, &tbProbeResult);

        // Probe was successful
        if (tbProbeResult != 0) {
            searchStats->tbhits++;

            int tbScore = tbValue < -1 ? -TB_WIN - MAX_DEPTH + ssi->ply
                        : tbValue >  1 ?  TB_WIN + MAX_DEPTH - ssi->ply
                                       : 2 * tbValue;

            // Hash the TB result
            int tbDepth = std::min(depth+4, MAX_DEPTH);
            uint64_t hashData = packHashData(tbDepth, NULL_MOVE,
                adjustHashScore(tbScore, ssi->ply), PV_NODE,
                searchParams->rootMoveNumber);
            transpositionTable.add(b, hashData, tbDepth, searchParams->rootMoveNumber);

            return tbScore;
        }
    }


    // Keeps track of the PV to propagate up to root
    SearchPV line;
    // We do not want to do pruning if we are in check
    bool isInCheck = b.isInCheck(color);
    // A static evaluation, used to make numerous pruning decisions
    int staticEval = INFTY;
    ssi->staticEval = INFTY;
    if (!isInCheck) {
        searchStats->evalCacheProbes++;
        // Probe the eval cache for a saved evaluation
        int ehe = evalCache.get(b);
        if (ehe != 0) {
            searchStats->evalCacheHits++;
            ssi->staticEval = staticEval = ehe - EVAL_HASH_OFFSET;
        }
        else {
            Eval e;
            ssi->staticEval = staticEval = (color == WHITE) ? e.evaluate(b)
                                                            : -e.evaluate(b);
            evalCache.add(b, staticEval);
        }
    }

    // Use the TT score as a better "static" eval, if available.
    if (hashScore != -INFTY) {
        if ((nodeType == ALL_NODE && hashScore < staticEval)
         || (nodeType == CUT_NODE && hashScore > staticEval)
         || (nodeType == PV_NODE))
            staticEval = hashScore;
    }


    // Is static eval improving across plies? Used to make pruning decisions. Idea from Stockfish
    bool evalImproving = (ssi->ply >= 3 && (ssi->staticEval >= (ssi-2)->staticEval || (ssi-2)->staticEval == INFTY))
                      || (ssi->ply < 3);


    // Reverse futility pruning
    // If we are already doing really well and it's our turn, our opponent
    // probably wouldn't have let us get here (a form of the null-move observation
    // adapted to low depths, also called static null move pruning)
    if (!isPVNode && !isInCheck
     && (depth <= 6 && staticEval - REVERSE_FUTILITY_MARGIN[depth] >= beta)
     && b.getNonPawnMaterial(color))
        return staticEval;


    // Razoring
    // If static eval is a good amount below alpha, we are probably at an all-node.
    // Do a qsearch just to confirm. If the qsearch fails high, a capture gained back
    // the material and trust its result since a quiet move probably can't gain
    // as much.
    if (!isPVNode && !isInCheck
     && nodeType != CUT_NODE && nodeType != PV_NODE && abs(alpha) < NEAR_MATE_SCORE
     && depth <= 3 && staticEval <= alpha - RAZOR_MARGIN[depth]) {
        searchParams->ply = ssi->ply;
        if (depth == 1)
            return quiescence(b, 0, alpha, beta, threadID);

        int rWindow = alpha - RAZOR_MARGIN[depth];
        int value = quiescence(b, 0, rWindow, rWindow+1, threadID);
        // Fail hard here to be safe
        if (value <= rWindow)
            return value;
    }


    // Null move pruning
    // If we are in a position good enough that even after passing and giving
    // our opponent a free turn, we still exceed beta, then simply return beta
    // Only if doing a null move does not leave player in check
    // Do not do if the side to move has only pawns
    // Do not do more than 2 null moves in a row
    if (!isPVNode && !isInCheck
     && depth >= 2 && staticEval >= beta
     && searchParams->nullMoveCount < 2
     && b.getNonPawnMaterial(color)) {
        // Reduce more if we are further ahead
        int reduction = 2 + (32 * depth + std::min(staticEval - beta, 512)) / 128;

        uint16_t epCaptureFile = b.getEPCaptureFile();
        b.doNullMove();
        searchParams->nullMoveCount++;
        (ssi+1)->counterMoveHistory = nullptr;
        int nullScore = -PVS(b, depth-1-reduction, -beta, -alpha, threadID, !isCutNode, ssi+1, &line);

        // Undo the null move
        b.undoNullMove(epCaptureFile);
        searchParams->nullMoveCount = 0;

        if (nullScore >= beta) {
            if (depth >= 10) {
                int verifyScore = PVS(b, depth-1-reduction, alpha, beta, threadID, false, ssi, &line);
                if (verifyScore >= beta)
                    return verifyScore;
            }
            else return nullScore;
        }
    }


    // Create list of legal moves
    MoveList legalMoves;
    if (isInCheck)
        b.getPseudoLegalCheckEscapes(legalMoves, color);
    else
        b.getAllPseudoLegalMoves(legalMoves, color);
    // Initialize the module for move ordering
    MoveOrder moveSorter(&b, color, depth, threadID, isPVNode,
        isCutNode, staticEval, beta, searchParams, ssi, hashed, legalMoves);
    moveSorter.generateMoves();

    // Keeps track of the best move for storing into the TT
    Move toHash = NULL_MOVE;
    // separate counter only incremented when valid move is searched
    unsigned int movesSearched = 0;
    int bestScore = -INFTY;
    int score = -INFTY;


    //----------------------------Main search loop------------------------------
    for (Move m = moveSorter.nextMove(); m != NULL_MOVE;
              m = moveSorter.nextMove()) {
        // Check for a timeout
        if (!isPonderSearch) {
            uint64_t timeSoFar = getTimeElapsed(searchParamsArray[0]->startTime);
            if (timeSoFar > searchParamsArray[0]->timeLimit) {
                isStop = true;
                stopSignal = true;
            }
        }
        // Stop condition to help break out as quickly as possible
        if (stopSignal.load(std::memory_order_relaxed))
            return INFTY;

        // Conditions for whether to do futility and move count pruning
        bool moveIsPrunable = !isInCheck
                           && !isCapture(m)
                           && !isPromotion(m)
                           && m != hashed
                           && abs(alpha) < NEAR_MATE_SCORE
                           && !b.isCheckMove(color, m);

        // For accessing history tables
        int startSq = getStartSq(m);
        int endSq = getEndSq(m);
        int pieceID = b.getPieceOnSquare(color, startSq);

        // Used to adjust pruning amount so that PV nodes are pruned slightly less
        int pruneDepth = isPVNode ? depth+1 : depth;


        // Futility pruning
        // If we are already a decent amount of material below alpha, a quiet
        // move probably won't raise our prospects much, so don't bother
        // q-searching it.
        if (moveIsPrunable && bestScore > -INFTY
         && pruneDepth <= 6 && staticEval <= alpha - FUTILITY_MARGIN[pruneDepth])
            continue;


        // Move count based pruning / Late move pruning
        // At low depths, moves late in the list with poor history are pruned
        // As used in Fruit/Stockfish:
        // https://chessprogramming.wikispaces.com/Futility+Pruning#MoveCountBasedPruning
        if (moveIsPrunable
         && depth <= 7
         && movesSearched > LMP_MOVE_COUNTS[evalImproving][depth] + (isPVNode ? depth : 0))
            continue;


        // Futility pruning using SEE
        if (!isPVNode && !isInCheck && abs(alpha) < NEAR_MATE_SCORE
         && bestScore > -INFTY && depth <= 5
         && b.getSEEForMove(color, m) < -100*depth)
            continue;


        // Copy the board and do the move
        Board copy = b.staticCopy();
        // If we are searching the hash move, we must use to a special
        // move generator for extra verification.
        if (m == hashed) {
            if (!copy.doHashMove(m, color)) {
                hashed = NULL_MOVE;
                moveSorter.hashed = NULL_MOVE;
                moveSorter.generateMoves();
                continue;
            }
            moveSorter.generateMoves();
        }
        else if (!copy.doPseudoLegalMove(m, color))
            continue;
        searchStats->nodes++;


        int reduction = 0;
        // Late move reduction
        // With good move ordering, later moves are less likely to increase
        // alpha, so we search them to a shallower depth hoping for a quick
        // fail-low.
        if (depth >= 3 && movesSearched > (isPVNode ? 4 : 2) + (unsigned int) isInCheck
         && !isCapture(m) && !isPromotion(m)
         && !copy.isInCheck(color^1)) {
            // Increase reduction with higher depth and later moves
            // Idea for log-based formula from Stockfish
            reduction = (int) (0.5 + log(depth) * log(movesSearched) / 2.1);
            // Reduce less for killers
            if (m == searchParams->killers[ssi->ply][0]
             || m == searchParams->killers[ssi->ply][1])
                reduction--;
            // Reduce less when in check
            if (isInCheck)
                reduction--;
            // Reduce more for moves with poor history
            int historyValue = searchParams->historyTable[color][pieceID][endSq];
            reduction -= historyValue / 256;
            // Reduce more for expected cut nodes
            if (isCutNode)
                reduction++;
            // Reduce less at PV nodes
            if (isPVNode)
                reduction--;

            // Do not let search descend directly into q-search
            reduction = std::max(0, std::min(reduction, depth - 2));
        }


        int extension = 0;
        // Check extensions
        if (reduction == 0
         && copy.isInCheck(color^1)
         && b.getSEEForMove(color, m) >= 0) {
            extension++;
        }

        // Record two-fold stack since we may do a search for singular extensions
        twoFoldPositions[threadID].push(b.getZobristKey());

        // Singular extensions
        // If the TT move appears to be much better than all others, extend the move
        if (depth >= 7 && reduction == 0 && extension == 0
         && m == hashed
         && abs(hashScore) < NEAR_MATE_SCORE
         && (nodeType == CUT_NODE || nodeType == PV_NODE)
         && hashDepth >= depth - 3) {
            bool isSingular = true;

            // Do a reduced depth search with a lowered window for a fail low check
            for (unsigned int i = 0; i < legalMoves.size(); i++) {
                Move seMove = legalMoves.get(i);
                Board seCopy = b.staticCopy();
                // Search every move except the hash move
                if (seMove == hashed)
                    continue;
                if (!seCopy.doPseudoLegalMove(seMove, color))
                    continue;

                (ssi+1)->counterMoveHistory = searchParams->counterMoveHistory
                    [b.getPieceOnSquare(color, getStartSq(seMove))][getEndSq(seMove)];

                // The window is lowered more for higher depths
                int SEWindow = hashScore - 10 - depth;
                // Do a reduced search for fail-low confirmation
                int SEDepth = depth / 2 - 1;

                score = -PVS(seCopy, SEDepth, -SEWindow - 1, -SEWindow, threadID, !isCutNode, ssi+1, &line);

                // If a move did not fail low, no singular extension
                if (score > SEWindow) {
                    isSingular = false;
                    break;
                }
            }

            // If all moves other than the hash move failed low, we extend for
            // the singular move
            if (isSingular)
                extension++;
        }


        (ssi+1)->counterMoveHistory = searchParams->counterMoveHistory[pieceID][endSq];

        // Null-window search, with re-search if applicable
        if (movesSearched != 0) {
            score = -PVS(copy, depth-1-reduction+extension, -alpha-1, -alpha, threadID, true, ssi+1, &line);

            // LMR re-search if the reduced search did not fail low
            if (reduction > 0 && score > alpha) {
                score = -PVS(copy, depth-1+extension, -alpha-1, -alpha, threadID, !isCutNode, ssi+1, &line);
            }

            // Re-search for a scout window at PV nodes
            if (alpha < score && score < beta) {
                score = -PVS(copy, depth-1+extension, -beta, -alpha, threadID, false, ssi+1, &line);
            }
        }

        // The first move is always searched at a normal depth
        else {
            score = -PVS(copy, depth-1+extension, -beta, -alpha, threadID, (isPVNode ? false : !isCutNode), ssi+1, &line);
        }

        // Pop the position in case we return early from this search
        twoFoldPositions[threadID].pop();

        // Stop condition to help break out as quickly as possible
        if (stopSignal.load(std::memory_order_relaxed))
            return INFTY;

        // Beta cutoff
        if (score >= beta) {
            searchStats->failHighs++;
            if (movesSearched == 0)
                searchStats->firstFailHighs++;
            if (hashed != NULL_MOVE && nodeType != ALL_NODE) {
                searchStats->hashMoveAttempts++;
                if (m == hashed)
                    searchStats->hashMoveCuts++;
            }

            // Hash the cut move and score
            uint64_t hashData = packHashData(depth, m,
                adjustHashScore(score, ssi->ply), CUT_NODE,
                searchParams->rootMoveNumber);
            transpositionTable.add(b, hashData, depth, searchParams->rootMoveNumber);

            // Record killer if applicable
            if (!isCapture(m)) {
                // Ensure the same killer does not fill both slots
                if (m != searchParams->killers[ssi->ply][0]) {
                    searchParams->killers[ssi->ply][1] =
                        searchParams->killers[ssi->ply][0];
                    searchParams->killers[ssi->ply][0] = m;
                }
                // Update the history table
                int histDepth = std::min(depth, 12);
                searchParams->historyTable[color][pieceID][endSq] -=
                    histDepth * searchParams->historyTable[color][pieceID][endSq] / 64;
                searchParams->historyTable[color][pieceID][endSq] += histDepth * histDepth;
                // Update countermove history
                if (ssi->counterMoveHistory != nullptr) {
                    ssi->counterMoveHistory[pieceID][endSq] -=
                        histDepth * ssi->counterMoveHistory[pieceID][endSq] / 64;
                    ssi->counterMoveHistory[pieceID][endSq] += histDepth * histDepth;
                }
                moveSorter.reduceBadHistories(m);
            }

            changePV(m, pvLine, &line);

            return score;
        }

        // If alpha was raised, we have a new PV
        if (score > bestScore) {
            bestScore = score;
            if (score > alpha) {
                alpha = score;
                toHash = m;
                changePV(m, pvLine, &line);
            }
        }

        movesSearched++;
    }
    // End main search loop


    // If there were no legal moves
    if (bestScore == -INFTY && movesSearched == 0)
        return scoreMate(isInCheck, ssi->ply);

    // Exact scores indicate a principal variation
    if (prevAlpha < alpha && alpha < beta) {
        if (hashed != NULL_MOVE && nodeType != ALL_NODE) {
            searchStats->hashMoveAttempts++;
            if (toHash == hashed)
                searchStats->hashMoveCuts++;
        }

        uint64_t hashData = packHashData(depth, toHash,
            adjustHashScore(alpha, ssi->ply), PV_NODE,
            searchParams->rootMoveNumber);
        transpositionTable.add(b, hashData, depth, searchParams->rootMoveNumber);

        if (!isCapture(toHash)) {
            int startSq = getStartSq(toHash);
            int endSq = getEndSq(toHash);
            int pieceID = b.getPieceOnSquare(color, startSq);

            // Update the history table
            int histDepth = std::min(depth, 12);
            searchParams->historyTable[color][pieceID][endSq] -=
                histDepth * searchParams->historyTable[color][pieceID][endSq] / 64;
            searchParams->historyTable[color][pieceID][endSq] += histDepth * histDepth;
            // Update countermove history
            if (ssi->counterMoveHistory != nullptr) {
                ssi->counterMoveHistory[pieceID][endSq] -=
                    histDepth * ssi->counterMoveHistory[pieceID][endSq] / 64;
                ssi->counterMoveHistory[pieceID][endSq] += histDepth * histDepth;
            }
            moveSorter.reduceBadHistories(toHash);
        }
    }

    // Record all-nodes
    else if (alpha <= prevAlpha) {
        // If we would have done IID, save the hash/IID move in case the node
        // becomes a PV or cut node next time
        if (!isPVNode && moveSorter.doIID()) {
            uint64_t hashData = packHashData(depth,
                (hashed == NULL_MOVE) ? moveSorter.legalMoves.get(0) : hashed,
                adjustHashScore(bestScore, ssi->ply), ALL_NODE,
                searchParams->rootMoveNumber);
            transpositionTable.add(b, hashData, depth, searchParams->rootMoveNumber);
        }
        // Otherwise, just store no best move as expected
        else {
            uint64_t hashData = packHashData(depth, NULL_MOVE,
                adjustHashScore(bestScore, ssi->ply), ALL_NODE,
                searchParams->rootMoveNumber);
            transpositionTable.add(b, hashData, depth, searchParams->rootMoveNumber);
        }
    }

    return bestScore;
}


/* Quiescence search, which completes all capture and check lines (thus reaching
 * a "quiet" position.)
 * This diminishes the horizon effect and greatly improves playing strength.
 * Delta pruning and static-exchange evaluation are used to reduce the time
 * spent here.
 * The search is a fail-soft PVS.
 */
int quiescence(Board &b, int plies, int alpha, int beta, int threadID) {
    SearchParameters *searchParams = searchParamsArray[threadID];
    SearchStatistics *searchStats = searchStatsArray[threadID];
    int color = b.getPlayerToMove();

    // If in check, we must consider all legal check evasions
    if (b.isInCheck(color))
        return checkQuiescence(b, plies, alpha, beta, threadID);

    if (b.isInsufficientMaterial())
        return 0;
    // Check for repetition draws while we are still considering checks
    if (b.getFiftyMoveCounter() >= 2 && twoFoldPositions[threadID].find(b.getZobristKey()))
        return 0;

    // Qsearch hash table probe
    int hashScore = -INFTY;
    uint64_t hashEntry = transpositionTable.get(b);
    uint8_t nodeType = NO_NODE_INFO;
    if (hashEntry != 0) {
        hashScore = getHashScore(hashEntry);

        // Adjust the hash score to mate distance from root if necessary
        if (hashScore >= MATE_SCORE - MAX_DEPTH)
            hashScore -= searchParams->ply + plies;
        else if (hashScore <= -MATE_SCORE + MAX_DEPTH)
            hashScore += searchParams->ply + plies;

        nodeType = getHashNodeType(hashEntry);
        // Only used a hashed score if the search depth was at least
        // the current depth
        if (getHashDepth(hashEntry) >= -plies) {
            // Check for the correct node type and bounds
            if ((nodeType == ALL_NODE && hashScore <= alpha)
             || (nodeType == CUT_NODE && hashScore >= beta)
             || (nodeType == PV_NODE))
                return hashScore;
        }
    }


    // Stand pat: if our current position is already way too good or way too bad
    // we can simply stop the search here.
    int standPat;
    // Probe the eval cache for a saved calculation
    searchStats->evalCacheProbes++;
    int ehe = evalCache.get(b);
    if (ehe != 0) {
        searchStats->evalCacheHits++;
        standPat = ehe - EVAL_HASH_OFFSET;
    }
    else {
        Eval e;
        standPat = (color == WHITE) ? e.evaluate(b) : -e.evaluate(b);
        evalCache.add(b, standPat);
    }

    // Use the TT score as a better "static" eval, if available.
    if (hashScore != -INFTY) {
        if ((nodeType == ALL_NODE && hashScore < standPat)
         || (nodeType == CUT_NODE && hashScore > standPat)
         || (nodeType == PV_NODE))
            standPat = hashScore;
    }

    // The stand pat cutoff
    if (standPat >= beta)
        return standPat;

    if (alpha < standPat)
        alpha = standPat;


    // Generate captures and order by MVV/LVA
    MoveList legalMoves;
    b.getPseudoLegalCaptures(legalMoves, color, false);
    ScoreList scores;
    for (unsigned int i = 0; i < legalMoves.size(); i++) {
        scores.add(b.getMVVLVAScore(color, legalMoves.get(i)));
    }

    int bestScore = -INFTY;
    int score = -INFTY;
    unsigned int i = 0;
    unsigned int j = 0; // separate counter only incremented when valid move is searched
    for (Move m = nextMove(legalMoves, scores, i); m != NULL_MOVE;
              m = nextMove(legalMoves, scores, ++i)) {
        // Delta prune
        if (standPat + b.valueOfPiece(b.getPieceOnSquare(color^1, getEndSq(m))) < alpha - MAX_POS_SCORE)
            continue;
        // Futility pruning
        if (standPat < alpha - 70 && b.getSEEForMove(color, m) <= 0)
            continue;
        // Static exchange evaluation pruning
        if (b.getExchangeScore(color, m) < 0 && b.getSEEForMove(color, m) < 0)
            continue;


        Board copy = b.staticCopy();
        if (!copy.doPseudoLegalMove(m, color))
            continue;

        searchStats->nodes++;
        searchStats->qsNodes++;
        score = -quiescence(copy, plies+1, -beta, -alpha, threadID);

        // Stop condition to help break out as quickly as possible
        if (stopSignal.load(std::memory_order_relaxed))
            return INFTY;

        if (score >= beta) {
            searchStats->qsFailHighs++;
            if (j == 0)
                searchStats->qsFirstFailHighs++;

            uint64_t hashData = packHashData(-plies, m,
                adjustHashScore(score, searchParams->ply + plies), CUT_NODE,
                searchParams->rootMoveNumber);
            transpositionTable.add(b, hashData, -plies, searchParams->rootMoveNumber);

            return score;
        }

        if (score > bestScore) {
            bestScore = score;
            if (score > alpha)
                alpha = score;
        }

        j++;
    }

    // Generate and search promotions
    legalMoves.clear();
    b.getPseudoLegalPromotions(legalMoves, color);
    for (unsigned int i = 0; i < legalMoves.size(); i++) {
        Move m = legalMoves.get(i);

        // Static exchange evaluation pruning
        if (!isCapture(m) && b.getSEEForMove(color, m) < 0)
            continue;

        Board copy = b.staticCopy();
        if (!copy.doPseudoLegalMove(m, color))
            continue;

        searchStats->nodes++;
        searchStats->qsNodes++;
        score = -quiescence(copy, plies+1, -beta, -alpha, threadID);

        if (score >= beta) {
            searchStats->qsFailHighs++;
            if (j == 0)
                searchStats->qsFirstFailHighs++;

            uint64_t hashData = packHashData(-plies, m,
                adjustHashScore(score, searchParams->ply + plies), CUT_NODE,
                searchParams->rootMoveNumber);
            transpositionTable.add(b, hashData, -plies, searchParams->rootMoveNumber);

            return score;
        }

        if (score > bestScore) {
            bestScore = score;
            if (score > alpha)
                alpha = score;
        }

        j++;
    }

    // Checks: only on the first two plies of q-search
    // Only do checks if a futility pruning condition is met
    if (plies <= 1 && standPat >= alpha - 100) {
        legalMoves.clear();
        b.getPseudoLegalChecks(legalMoves, color);

        for (unsigned int i = 0; i < legalMoves.size(); i++) {
            Move m = legalMoves.get(i);

            // Futility pruning
            if (standPat < alpha - 100)
                continue;
            // Static exchange evaluation pruning
            if (b.getSEEForMove(color, m) < 0)
                continue;

            Board copy = b.staticCopy();
            if (!copy.doPseudoLegalMove(m, color))
                continue;

            searchStats->nodes++;
            searchStats->qsNodes++;
            twoFoldPositions[threadID].push(b.getZobristKey());

            int score = -checkQuiescence(copy, plies+1, -beta, -alpha, threadID);

            twoFoldPositions[threadID].pop();

            if (score >= beta) {
                searchStats->qsFailHighs++;
                if (j == 0)
                    searchStats->qsFirstFailHighs++;

                uint64_t hashData = packHashData(-plies, m,
                    adjustHashScore(score, searchParams->ply + plies), CUT_NODE,
                    searchParams->rootMoveNumber);
                transpositionTable.add(b, hashData, -plies, searchParams->rootMoveNumber);

                return score;
            }

            if (score > bestScore) {
                bestScore = score;
                if (score > alpha)
                    alpha = score;
            }

            j++;
        }
    }

    // Fail low and hard if there are no captures
    if (bestScore == -INFTY)
        bestScore = alpha;

    return bestScore;
}

/*
 * When checks are considered in quiescence, the responses must include all moves,
 * not just captures, necessitating this function.
 */
int checkQuiescence(Board &b, int plies, int alpha, int beta, int threadID) {
    if (b.getFiftyMoveCounter() >= 2 && twoFoldPositions[threadID].find(b.getZobristKey()))
        return 0;

    SearchParameters *searchParams = searchParamsArray[threadID];
    SearchStatistics *searchStats = searchStatsArray[threadID];
    int color = b.getPlayerToMove();
    MoveList legalMoves;
    b.getPseudoLegalCheckEscapes(legalMoves, color);

    int bestScore = -INFTY;
    int score = -INFTY;
    unsigned int j = 0; // separate counter only incremented when valid move is searched
    for (unsigned int i = 0; i < legalMoves.size(); i++) {
        Move m = legalMoves.get(i);

        if (bestScore > -INFTY && b.getSEEForMove(color, m) < 0)
            continue;

        Board copy = b.staticCopy();
        if (!copy.doPseudoLegalMove(m, color))
            continue;

        searchStats->nodes++;
        searchStats->qsNodes++;
        twoFoldPositions[threadID].push(b.getZobristKey());

        score = -quiescence(copy, plies+1, -beta, -alpha, threadID);

        twoFoldPositions[threadID].pop();

        if (score >= beta) {
            searchStats->qsFailHighs++;
            if (j == 0)
                searchStats->qsFirstFailHighs++;
            return score;
        }

        if (score > bestScore) {
            bestScore = score;
            if (score > alpha)
                alpha = score;
        }

        j++;
    }

    // If there were no legal moves
    if (bestScore == -INFTY) {
        // We already know we are in check, so it must be a checkmate
        // Adjust score so that quicker mates are better
        return (-MATE_SCORE + searchParams->ply + plies);
    }

    return bestScore;
}


//------------------------------------------------------------------------------
//-----------------------------Search Helpers-----------------------------------
//------------------------------------------------------------------------------

// Used to get a score when we have realized that we have no legal moves.
int scoreMate(bool isInCheck, int plies) {
    // If we are in check, then it is a checkmate
    if (isInCheck)
        // Adjust score so that quicker mates are better
        return (-MATE_SCORE + plies);

    // Else, it is a stalemate
    else return 0;
}

// Adjust a mate score to accurately reflect distance to mate from the
// current position, if necessary.
int adjustHashScore(int score, int plies) {
    if (score >= MATE_SCORE - MAX_DEPTH)
        return score + plies;
    if (score <= -MATE_SCORE + MAX_DEPTH)
        return score - plies;
    return score;
}


// Pondering
void startPonder() {
    isPonderSearch = true;
}

void stopPonder() {
    isPonderSearch = false;
}


//------------------------------------------------------------------------------
//------------------------------Other functions---------------------------------
//------------------------------------------------------------------------------

// These functions help to communicate with uci.cpp
void clearTables() {
    transpositionTable.clear();
    evalCache.clear();
    for (int i = 0; i < numThreads; i++)
        searchParamsArray[i]->resetHistoryTable();
}

void setHashSize(uint64_t MB) {
    transpositionTable.setSize(MB);
}

void setEvalCacheSize(uint64_t MB) {
    evalCache.setSize(MB);
}

uint64_t getNodes() {
    uint64_t total = 0;
    for (int i = 0; i < numThreads; i++) {
        total += searchStatsArray[i]->nodes;
    }
    return total;
}

uint64_t getTBHits() {
    uint64_t total = 0;
    for (int i = 0; i < numThreads; i++) {
        total += searchStatsArray[i]->tbhits;
    }
    return total;
}

void setMultiPV(unsigned int n) {
    multiPV = n;
}

void setNumThreads(int n) {
    numThreads = n;

    while ((int) searchParamsArray.size() < n)
        searchParamsArray.push_back(new SearchParameters());
    while ((int) searchParamsArray.size() > n) {
        delete searchParamsArray.back();
        searchParamsArray.pop_back();
    }

    while ((int) searchStatsArray.size() < n)
        searchStatsArray.push_back(new SearchStatistics());
    while ((int) searchStatsArray.size() > n) {
        delete searchStatsArray.back();
        searchStatsArray.pop_back();
    }
}

void initPerThreadMemory() {
    searchParamsArray.push_back(new SearchParameters());
    searchStatsArray.push_back(new SearchStatistics());
}


// Retrieves the next move with the highest score, starting from index using a
// partial selection sort. This way, the entire list does not have to be sorted
// if an early cutoff occurs.
Move nextMove(MoveList &moves, ScoreList &scores, unsigned int index) {
    if (index >= moves.size())
        return NULL_MOVE;
    // Find the index of the next best move
    int bestIndex = index;
    int bestScore = scores.get(index);
    for (unsigned int i = index + 1; i < moves.size(); i++) {
        if (scores.get(i) > bestScore) {
            bestIndex = i;
            bestScore = scores.get(bestIndex);
        }
    }
    // Swap the best move to the correct position
    moves.swap(bestIndex, index);
    scores.swap(bestIndex, index);
    return moves.get(index);
}

// Copies the new PV line when alpha is raised
void changePV(Move best, SearchPV *parent, SearchPV *child) {
    parent->pv[0] = best;
    for (int i = 0; i < child->pvLength; i++) {
        parent->pv[i+1] = child->pv[i];
    }
    parent->pvLength = child->pvLength + 1;
}

// Recover PV for outputting to terminal / GUI
std::string retrievePV(SearchPV *pvLine) {
    std::string pvStr = moveToString(pvLine->pv[0]);
    for (int i = 1; i < pvLine->pvLength; i++) {
        pvStr += " " + moveToString(pvLine->pv[i]);
    }

    return pvStr;
}

// The selective depth in a parallel search is the max selective depth reached
// by any of the threads
int getSelectiveDepth() {
    int max = 0;
    for (int i = 0; i < numThreads; i++)
        if (searchParamsArray[i]->selectiveDepth > max)
            max = searchParamsArray[i]->selectiveDepth;
    return max;
}

void initSSI() {
    ssInfo = new SearchStackInfo *[MAX_THREADS];
    for (int i = 0; i < MAX_THREADS; i++) {
        ssInfo[i] = new SearchStackInfo[128];
        for (int j = 0; j < 128; j++)
            ssInfo[i][j].ply = j;
    }
}

// Formats a fraction into a percentage value (0 to 100) for printing
double getPercentage(uint64_t numerator, uint64_t denominator) {
    if (denominator == 0)
        return 0;
    uint64_t tenThousandths = (numerator * 10000) / denominator;
    double percent = ((double) tenThousandths) / 100.0;
    return percent;
}

// Prints the statistics gathered during search
void printStatistics() {
    // Aggregate statistics over all threads
    SearchStatistics searchStats;
    for (int i = 0; i < numThreads; i++) {
        searchStats.nodes +=            searchStatsArray[i]->nodes;
        searchStats.hashProbes +=       searchStatsArray[i]->hashProbes;
        searchStats.hashHits +=         searchStatsArray[i]->hashHits;
        searchStats.hashScoreCuts +=    searchStatsArray[i]->hashScoreCuts;
        searchStats.hashMoveAttempts += searchStatsArray[i]->hashMoveAttempts;
        searchStats.hashMoveCuts +=     searchStatsArray[i]->hashMoveCuts;
        searchStats.failHighs +=        searchStatsArray[i]->failHighs;
        searchStats.firstFailHighs +=   searchStatsArray[i]->firstFailHighs;
        searchStats.qsNodes +=          searchStatsArray[i]->qsNodes;
        searchStats.qsFailHighs +=      searchStatsArray[i]->qsFailHighs;
        searchStats.qsFirstFailHighs += searchStatsArray[i]->qsFirstFailHighs;
        searchStats.evalCacheProbes +=  searchStatsArray[i]->evalCacheProbes;
        searchStats.evalCacheHits +=    searchStatsArray[i]->evalCacheHits;
    }

    cerr << std::setw(22) << "Hash hit rate: " << getPercentage(searchStats.hashHits, searchStats.hashProbes)
         << '%' << " of " << searchStats.hashProbes << " probes" << endl;
    cerr << std::setw(22) << "Hash score cut rate: " << getPercentage(searchStats.hashScoreCuts, searchStats.hashHits)
         << '%' << " of " << searchStats.hashHits << " hash hits" << endl;
    cerr << std::setw(22) << "Hash move cut rate: " << getPercentage(searchStats.hashMoveCuts, searchStats.hashMoveAttempts)
         << '%' << " of " << searchStats.hashMoveAttempts << " hash moves" << endl;
    cerr << std::setw(22) << "First fail high rate: " << getPercentage(searchStats.firstFailHighs, searchStats.failHighs)
         << '%' << " of " << searchStats.failHighs << " fail highs" << endl;
    cerr << std::setw(22) << "QS Nodes: " << searchStats.qsNodes << " ("
         << getPercentage(searchStats.qsNodes, searchStats.nodes) << '%' << ")" << endl;
    cerr << std::setw(22) << "QS FFH rate: " << getPercentage(searchStats.qsFirstFailHighs, searchStats.qsFailHighs)
         << '%' << " of " << searchStats.qsFailHighs << " qs fail highs" << endl;
    cerr << std::setw(22) << "Eval cache hit rate: " << getPercentage(searchStats.evalCacheHits, searchStats.evalCacheProbes)
         << '%' << " of " << searchStats.evalCacheProbes << " probes" << endl;
}