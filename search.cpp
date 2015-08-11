#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include "search.h"

using namespace std;

struct SearchParameters {
    int ply;
    int nullMoveCount;
    Move killers[MAX_DEPTH][2];
    int historyTable[2][6][64];

    SearchParameters() {
        reset();
        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < 6; j++) {
                for (int k = 0; k < 64; k++)
                    historyTable[i][j][k] = 0;
            }
        }
    }

    void reset() {
        ply = 0;
        nullMoveCount = 0;
        for (int i = 0; i < MAX_DEPTH; i++) {
            killers[i][0] = NULL_MOVE;
            killers[i][1] = NULL_MOVE;
        }
    }

    // TODO tune this
    void ageHistory() {
        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < 6; j++) {
                for (int k = 0; k < 64; k++)
                    historyTable[i][j][k] /= 2;
            }
        }
    }
};

static Hash transpositionTable(16);
static uint8_t rootMoveNumber;
static SearchParameters searchParams;
static SearchStatistics searchStats;

extern bool isStop;

// Search functions
unsigned int getBestMoveAtDepth(Board *b, MoveList &legalMoves, int depth,
    int &bestScore, bool &isMate);
int getBestMoveForSort(Board &b, MoveList &legalMoves, int depth);
int PVS(Board &b, int color, int depth, int alpha, int beta);
int quiescence(Board &b, int color, int plies, int alpha, int beta);
int checkQuiescence(Board &b, int color, int plies, int alpha, int beta);

// Search helpers
int probeTT(Board &b, int color, Move &hashed, int depth, int &alpha, int beta);
int scoreMate(bool isInCheck, int depth, int alpha, int beta);
double getPercentage(uint64_t numerator, uint64_t denominator);
void printStatistics();

// Other utility functions
Move nextMove(MoveList &moves, ScoreList &scores, unsigned int index);
string retrievePV(Board *b, Move bestMove, int plies);

void getBestMove(Board *b, int mode, int value, SearchStatistics *stats, Move *bestMove) {
    using namespace std::chrono;
    searchParams.reset();
    searchStats.reset();
    rootMoveNumber = (uint8_t) (b->getMoveNumber());

    int color = b->getPlayerToMove();
    MoveList legalMoves = b->getAllLegalMoves(color);
    *bestMove = legalMoves.get(0);
    
    auto start_time = high_resolution_clock::now();
    double timeSoFar = duration_cast<duration<double>>(
            high_resolution_clock::now() - start_time).count();
    bool isMate = false;
    int bestScore, bestMoveIndex;
    
    double timeFactor = 0.4; // timeFactor = log b / (b - 1) where b is branch factor
    int rootDepth = 1;
    do {
        bestMoveIndex = getBestMoveAtDepth(b, legalMoves, rootDepth, bestScore, isMate);
        if (bestMoveIndex == -1)
            break;
        legalMoves.swap(0, bestMoveIndex);
        *bestMove = legalMoves.get(0);
        
        timeSoFar = duration_cast<duration<double>>(
                high_resolution_clock::now() - start_time).count();
        uint64_t nps = (uint64_t) ((double) searchStats.nodes / timeSoFar);
        string pvStr = retrievePV(b, *bestMove, rootDepth);
        
        cout << "info depth " << rootDepth << " score cp " << bestScore << " time "
            << (int)(timeSoFar * ONE_SECOND) << " nodes " << searchStats.nodes
            << " nps " << nps << " pv " << pvStr << endl;
        
        if (isMate)
            break;
        rootDepth++;
    }
    while ((mode == TIME && (timeSoFar * ONE_SECOND < value * timeFactor)
        && (rootDepth <= MAX_DEPTH))
        || (mode == DEPTH && rootDepth <= value));
    
    printStatistics();
    // Aging for the history heuristic table
    searchParams.ageHistory();
    
    isStop = true;
    cout << "bestmove " << moveToString(*bestMove) << endl;
    *stats = searchStats;
    return;
}

// returns index of best move in legalMoves
unsigned int getBestMoveAtDepth(Board *b, MoveList &legalMoves, int depth,
        int &bestScore, bool &isMate) {
    int color = b->getPlayerToMove();
    searchParams.reset();
    
    unsigned int tempMove = -1;
    int score = -MATE_SCORE;
    int alpha = -MATE_SCORE;
    int beta = MATE_SCORE;
    
    for (unsigned int i = 0; i < legalMoves.size(); i++) {
        // Stop condition. If stopping, return search results from incomplete
        // search, if any.
        if (isStop) {
            return tempMove;
        }
        Board copy = b->staticCopy();
        copy.doMove(legalMoves.get(i), color);
        searchStats.nodes++;
        
        if (i != 0) {
            searchParams.ply++;
            score = -PVS(copy, color^1, depth-1, -alpha-1, -alpha);
            searchParams.ply--;
            if (alpha < score && score < beta) {
                searchParams.ply++;
                score = -PVS(copy, color^1, depth-1, -beta, -alpha);
                searchParams.ply--;
            }
        }
        else {
            searchParams.ply++;
            score = -PVS(copy, color^1, depth-1, -beta, -alpha);
            searchParams.ply--;
        }

        if (score > alpha) {
            alpha = score;
            tempMove = i;
        }
    }

    // If a mate has been found, indicate so
    if(alpha >= MATE_SCORE - MAX_DEPTH)
        isMate = true;
    bestScore = alpha;

    return tempMove;
}

// Gets a best move to try first when a hash move is not available.
int getBestMoveForSort(Board &b, MoveList &legalMoves, int depth) {
    int color = b.getPlayerToMove();
    
    int tempMove = -1;
    int score = -MATE_SCORE;
    int alpha = -MATE_SCORE;
    int beta = MATE_SCORE;
    
    for (unsigned int i = 0; i < legalMoves.size(); i++) {
        Board copy = b.staticCopy();
        if(!copy.doPseudoLegalMove(legalMoves.get(i), color))
            continue;
        
        if (i != 0) {
            searchParams.ply++;
            score = -PVS(copy, color^1, depth-1, -alpha-1, -alpha);
            searchParams.ply--;
            if (alpha < score && score < beta) {
                searchParams.ply++;
                score = -PVS(copy, color^1, depth-1, -beta, -alpha);
                searchParams.ply--;
            }
        }
        else {
            searchParams.ply++;
            score = -PVS(copy, color^1, depth-1, -beta, -alpha);
            searchParams.ply--;
        }
        
        if (score > alpha) {
            alpha = score;
            tempMove = i;
        }
    }

    return tempMove;
}

//------------------------------------------------------------------------------
//------------------------------Search functions--------------------------------
//------------------------------------------------------------------------------

// The standard implementation of a null-window PVS search.
// The implementation is fail-hard (score returned must be within [alpha, beta])
int PVS(Board &b, int color, int depth, int alpha, int beta) {
    // When the standard search is done, enter quiescence search.
    // Static board evaluation is done there.
    if (depth <= 0) {
        return quiescence(b, color, 0, alpha, beta);
    }

    if (b.isDraw()) {
        if (0 >= beta)
            return beta;
        if (0 > alpha)
            return 0;
        else
            return alpha;
    }
    
    int prevAlpha = alpha;
    // For PVS, the node is a PV node if beta - alpha > 1 (i.e. not a null window)
    // We do not want to do most pruning techniques on PV nodes
    bool isPVNode = (beta - alpha > 1);
    // Similarly, we do not want to prune if we are in check
    bool isInCheck = b.isInCheck(color);

    Move hashed = NULL_MOVE;
    // Probe the hash table for a match/cutoff
    // If a cutoff or exact score hit occurred, probeTT will return a value
    // other than -INFTY
    // alpha is passed by reference in case a hash move raises alpha but does
    // not cause a cutoff
    searchStats.hashProbes++;
    int hashScore = probeTT(b, color, hashed, depth, alpha, beta);
    if (hashScore != -INFTY)
        return hashScore;

    // A static evaluation, used to activate null move pruning and futility
    // pruning
    int staticEval = (color == WHITE) ? b.evaluate() : -b.evaluate();
    
    // Null move reduction/pruning: if we are in a position good enough that
    // even after passing and giving our opponent a free turn, we still exceed
    // beta, then simply return beta
    // Only if doing a null move does not leave player in check
    // Do not do NMR if the side to move has only pawns
    // Do not do more than 2 null moves in a row
    if (depth >= 3 && !isPVNode && searchParams.nullMoveCount < 2
                   && staticEval >= beta && !isInCheck && b.getNonPawnMaterial(color)) {
        int reduction;
        if (depth >= 11)
            reduction = 4;
        else if (depth >= 6)
            reduction = 3;
        else
            reduction = 2;
        // Reduce more if we are further ahead, but do not let NMR descend
        // directly into q-search
        reduction = min(depth - 2, reduction + (staticEval - beta) / MAX_POS_SCORE);

        b.doMove(NULL_MOVE, color);
        searchParams.nullMoveCount++;
        searchParams.ply++;
        int nullScore = -PVS(b, color^1, depth-1-reduction, -beta, -alpha);
        searchParams.ply--;
        if (nullScore >= beta) {
            b.doMove(NULL_MOVE, color^1);
            searchParams.nullMoveCount--;
            return beta;
        }
        
        // Undo the null move
        b.doMove(NULL_MOVE, color^1);
        searchParams.nullMoveCount--;
    }

    // Reverse futility pruning
    // If we are already doing really well and it's our turn, our opponent
    // probably wouldn't have let us get here (a form of the null-move observation
    // adapted to low depths)
    if (!isPVNode && !isInCheck && ((depth == 1 && staticEval - MAX_POS_SCORE >= beta)
                                 || (depth == 2 && staticEval - 3*MAX_POS_SCORE >= beta)))
        return beta;

    MoveList legalMoves = isInCheck ? b.getPseudoLegalCheckEscapes(color)
                                    : b.getAllPseudoLegalMoves(color);

    // If there were no pseudo-legal moves
    // This is an early check to prevent sorting from crashing
    if (legalMoves.size() == 0)
        return scoreMate(isInCheck, depth, alpha, beta);

    // Remove the hash move from the list, since it has already been tried
    // TODO make this nicer
    if (hashed != NULL_MOVE) {
        for (unsigned int i = 0; i < legalMoves.size(); i++) {
            if (legalMoves.get(i) == hashed) {
                legalMoves.remove(i);
                break;
            }
        }
    }

    ScoreList scores;
    // Internal iterative deepening, SEE, and MVV/LVA move ordering
    // The scoring relies partially on the fact that our selection sort is stable
    // TODO make this cleaner, probably when captures and moves become generated separately
    if (depth >= 3 || isPVNode) { // sort by SEE
        // ---------------Captures----------------
        unsigned int index = 0;
        for (index = 0; index < legalMoves.size(); index++) {
            Move m = legalMoves.get(index);
            if (!isCapture(m))
                break;
            // If anything wins at least a rook, it should go above this anyways
            if (b.getExchangeScore(color, m) > 0)
                scores.add(ROOK_VALUE + b.getMVVLVAScore(color, legalMoves.get(index)));
            else
                scores.add(b.getSEE(color, getEndSq(m)));
        }
        // ---------------Non-captures----------------
        // Score killers below even captures but above losing captures
        for (unsigned int i = index; i < legalMoves.size(); i++) {
            if (legalMoves.get(i) == searchParams.killers[searchParams.ply][0])
                scores.add(0);
            else if (legalMoves.get(i) == searchParams.killers[searchParams.ply][1])
                scores.add(-1);
            // Order queen promotions somewhat high
            else if (getPromotion(legalMoves.get(i)) == QUEENS)
                scores.add(MAX_POS_SCORE);
            else
                scores.add(-MATE_SCORE + searchParams.historyTable[color][b.getPieceOnSquare(color, getStartSq(legalMoves.get(i)))][getEndSq(legalMoves.get(i))]);
        }

        // IID: get a best move (hoping for a first move cutoff) if we don't
        // have a hash move available
        if (depth >= 5 && hashed == NULL_MOVE) {
            int bestIndex = getBestMoveForSort(b, legalMoves, (depth >= 9) ? 2 : 1);
            // Mate check to prevent crashes
            if (bestIndex == -1)
                return scoreMate(isInCheck, depth, alpha, beta);
            scores.set(bestIndex, (1 << 15));
        }
    }
    else { // MVV/LVA
        unsigned int index = 0;
        for (index = 0; index < legalMoves.size(); index++) {
            if (!isCapture(legalMoves.get(index)))
                break;
            scores.add(b.getMVVLVAScore(color, legalMoves.get(index)));
        }
        // For MVV/LVA, order killers above capturing a pawn with a minor piece
        // or greater
        for (unsigned int i = index; i < legalMoves.size(); i++) {
            if (legalMoves.get(i) == searchParams.killers[searchParams.ply][0])
                scores.add(PAWNS - KNIGHTS);
            else if (legalMoves.get(i) == searchParams.killers[searchParams.ply][1])
                scores.add(PAWNS - KNIGHTS - 1);
            // Order queen promotions above capturing pawns and minor pieces
            else if (getPromotion(legalMoves.get(i)) == QUEENS)
                scores.add(8*ROOKS);
            else
                scores.add(-MATE_SCORE + searchParams.historyTable[color][b.getPieceOnSquare(color, getStartSq(legalMoves.get(i)))][getEndSq(legalMoves.get(i))]);
        }
    }

    Move toHash = NULL_MOVE;
    int reduction = 0;
    unsigned int i = 0;
    // separate counter only incremented when valid move is searched
    unsigned int movesSearched = (hashed == NULL_MOVE) ? 0 : 1;
    int score = -INFTY;
    for (Move m = nextMove(legalMoves, scores, i); m != NULL_MOVE;
              m = nextMove(legalMoves, scores, ++i)) {
        // Stop condition to help break out as quickly as possible
        if (isStop)
            return -INFTY;

        // Futility pruning
        // If we are already a decent amount of material below alpha, a quiet
        // move probably won't raise our prospects much, so don't bother
        // q-searching it.
        // TODO may fail low in some stalemate cases
        if(!isPVNode && ((depth == 1 && staticEval <= alpha - MAX_POS_SCORE)
                      || (depth == 2 && staticEval <= alpha - ROOK_VALUE)
                      || (depth == 3 && staticEval <= alpha - QUEEN_VALUE - MAX_POS_SCORE))
        && !isInCheck && !isCapture(m) && abs(alpha) < QUEEN_VALUE
        && getPromotion(m) == 0 && !b.isCheckMove(m, color)) {
            score = alpha;
            continue;
        }

        // Futility pruning using SEE
        /*if(!isPVNode && depth == 1 //&& staticEval <= alpha - MAX_POS_SCORE
        && !isInCheck && abs(alpha) < QUEEN_VALUE && !isCapture(m) && getPromotion(m) == 0
        && !b.isCheckMove(m, color) && b.getExchangeScore(color, m) < 0 && b.getSEE(color, getEndSq(m)) < 0) {
            score = alpha;
            continue;
        }*/

        reduction = 0;
        Board copy = b.staticCopy();
        if (!copy.doPseudoLegalMove(m, color))
            continue;
        searchStats.nodes++;

        // Late move reduction
        // If we have not raised alpha in the first few moves, we are probably
        // at an all-node. The later moves are likely worse so we search them
        // to a shallower depth.
        // TODO set up an array for reduction values
        if(!isPVNode && !isInCheck && !isCapture(m) && depth >= 3 && movesSearched > 2 && alpha <= prevAlpha
        && m != searchParams.killers[searchParams.ply][0] && m != searchParams.killers[searchParams.ply][1]
        && getPromotion(m) == 0 && !copy.isInCheck(color^1)) {
            // Increase reduction with higher depth and later moves, but do
            // not let search descend directly into q-search
            reduction = min(depth - 2,
                (int) (((double) depth - 3.0) / 4 + ((double) movesSearched) / 9.5));
        }

        if (movesSearched != 0) {
            searchParams.ply++;
            score = -PVS(copy, color^1, depth-1-reduction, -alpha-1, -alpha);
            searchParams.ply--;
            // The re-search is always done at normal depth
            if (alpha < score && score < beta) {
                searchParams.ply++;
                score = -PVS(copy, color^1, depth-1, -beta, -alpha);
                searchParams.ply--;
            }
        }
        else {
            searchParams.ply++;
            // The first move is always searched at a normal depth
            score = -PVS(copy, color^1, depth-1, -beta, -alpha);
            searchParams.ply--;
        }
        
        if (score >= beta) {
            searchStats.failHighs++;
            if (movesSearched == 0)
                searchStats.firstFailHighs++;
            // Hash moves that caused a beta cutoff
            transpositionTable.add(b, depth, m, beta, CUT_NODE, rootMoveNumber);
            // Record killer if applicable
            if (!isCapture(m)) {
                // Ensure the same killer does not fill both slots
                if (m != searchParams.killers[searchParams.ply][0]) {
                    searchParams.killers[searchParams.ply][1] = searchParams.killers[searchParams.ply][0];
                    searchParams.killers[searchParams.ply][0] = m;
                }
                // Update the history table
                searchParams.historyTable[color][b.getPieceOnSquare(color, getStartSq(m))][getEndSq(m)]
                    += depth * depth;
            }
            return beta;
        }
        if (score > alpha) {
            alpha = score;
            toHash = m;
        }
        movesSearched++;
    }

    // If there were no legal moves
    if (score == -INFTY)
        return scoreMate(isInCheck, depth, alpha, beta);
    
    if (toHash != NULL_MOVE && prevAlpha < alpha && alpha < beta) {
        // Exact scores indicate a principal variation and should always be hashed
        transpositionTable.add(b, depth, toHash, alpha, PV_NODE, rootMoveNumber);
        // Update the history table
        /*if (!isCapture(toHash)) {
            searchParams.historyTable[color][b.getPieceOnSquare(color, getStartSq(toHash))][getEndSq(toHash)]
                += depth * depth;
        }*/
    }
    // Record all-nodes. The upper bound score can save a lot of search time.
    // No best move can be recorded in a fail-hard framework.
    else if (alpha <= prevAlpha) {
        transpositionTable.add(b, depth, NULL_MOVE, alpha, ALL_NODE, rootMoveNumber);
    }

    return alpha;
}

// See if a hash move exists.
int probeTT(Board &b, int color, Move &hashed, int depth, int &alpha, int beta) {
    HashEntry *entry = transpositionTable.get(b);
    if (entry != NULL) {
        searchStats.hashHits++;
        // If the node is a predicted all node and score <= alpha, return alpha
        // since score is an upper bound
        // Vulnerable to Type-1 errors
        int hashScore = entry->score;
        uint8_t nodeType = entry->getNodeType();
        if (nodeType == ALL_NODE) {
            if (entry->depth >= depth && hashScore <= alpha) {
                searchStats.hashScoreCuts++;
                return alpha;
            }
        }
        else {
            hashed = entry->m;
            // Only used a hashed score if the search depth was at least
            // the current depth
            if (entry->depth >= depth) {
                // At cut nodes if hash score >= beta return beta since hash
                // score is a lower bound.
                if (nodeType == CUT_NODE && hashScore >= beta) {
                    searchStats.hashScoreCuts++;
                    searchStats.failHighs++;
                    searchStats.firstFailHighs++;
                    return beta;
                }
                // At PV nodes we can simply return the exact score
                else if (nodeType == PV_NODE) {
                    searchStats.hashScoreCuts++;
                    return hashScore;
                }
            }
            Board copy = b.staticCopy();
            // Sanity check in case of Type-1 hash error
            if (copy.doHashMove(hashed, color)) {
                // If the hash score is unusable and node is not a predicted
                // all-node, we can search the hash move first.
                searchStats.hashMoveAttempts++;
                searchStats.nodes++;
                searchParams.ply++;
                int score = -PVS(copy, color^1, depth-1, -beta, -alpha);
                searchParams.ply--;

                if (score >= beta) {
                    searchStats.hashMoveCuts++;
                    return beta;
                }
                if (score > alpha)
                    alpha = score;
            }
            else {
                cerr << "Type-1 TT error on " << moveToString(hashed) << endl;
                hashed = NULL_MOVE;
            }
        }
    }
    return -INFTY;
}

// Used to get a score when we have realized that we have no legal moves.
int scoreMate(bool isInCheck, int depth, int alpha, int beta) {
    int score;
    // If we are in check, then checkmate
    if (isInCheck) {
        // Adjust score so that quicker mates are better
        score = (-MATE_SCORE + searchParams.ply);
    }
    else { // else, it is a stalemate
        score = 0;
    }
    if (score >= beta)
        return beta;
    if (score > alpha)
        alpha = score;
    return alpha;
}

/* Quiescence search, which completes all capture and check lines (thus reaching
 * a "quiet" position.)
 * This diminishes the horizon effect and greatly improves playing strength.
 * Delta pruning and static-exchange evaluation are used to reduce the time
 * spent here.
 * The search is done within a fail-hard framework (alpha <= score <= beta)
 */
int quiescence(Board &b, int color, int plies, int alpha, int beta) {
    if (b.isInCheck(color))
        return checkQuiescence(b, color, plies, alpha, beta);
    if (b.isDraw()) {
        if (0 >= beta)
            return beta;
        if (0 > alpha)
            return 0;
        else
            return alpha;
    }

    // Stand pat: if our current position is already way too good or way too bad
    // we can simply stop the search here
    int standPat = (color == WHITE) ? b.evaluate() : -b.evaluate();
    if (standPat >= beta)
        return beta;
    if (alpha < standPat)
        alpha = standPat;
    
    // delta prune
    if (standPat < alpha - MAX_POS_SCORE - QUEEN_VALUE)
        return alpha;
    
    MoveList legalCaptures = b.getPseudoLegalCaptures(color, false);
    ScoreList scores;
    for (unsigned int i = 0; i < legalCaptures.size(); i++) {
        scores.add(b.getMVVLVAScore(color, legalCaptures.get(i)));
    }
    
    int score = -INFTY;
    unsigned int i = 0;
    unsigned int j = 0; // separate counter only incremented when valid move is searched
    for (Move m = nextMove(legalCaptures, scores, i); m != NULL_MOVE;
              m = nextMove(legalCaptures, scores, ++i)) {
        // Delta prune
        if (standPat + b.valueOfPiece(b.getPieceOnSquare(color^1, getEndSq(m))) < alpha - MAX_POS_SCORE)
            continue;
        // Static exchange evaluation pruning
        if (b.getExchangeScore(color, m) < 0 && b.getSEE(color, getEndSq(m)) < -MAX_POS_SCORE)
            continue;

        Board copy = b.staticCopy();
        if (!copy.doPseudoLegalMove(m, color))
            continue;
        
        searchStats.nodes++;
        searchStats.qsNodes++;
        score = -quiescence(copy, color^1, plies+1, -beta, -alpha);
        
        if (score >= beta) {
            searchStats.qsFailHighs++;
            if (j == 0)
                searchStats.qsFirstFailHighs++;
            return beta;
        }
        if (score > alpha)
            alpha = score;
        j++;
    }

    MoveList legalPromotions = b.getPseudoLegalPromotions(color);
    for (unsigned int i = 0; i < legalPromotions.size(); i++) {
        Move m = legalPromotions.get(i);

        // Static exchange evaluation pruning
        if(b.getSEE(color, getEndSq(m)) < 0)
            continue;

        Board copy = b.staticCopy();
        if (!copy.doPseudoLegalMove(m, color))
            continue;
        
        searchStats.nodes++;
        searchStats.qsNodes++;
        score = -quiescence(copy, color^1, plies+1, -beta, -alpha);
        
        if (score >= beta) {
            searchStats.qsFailHighs++;
            if (j == 0)
                searchStats.qsFirstFailHighs++;
            return beta;
        }
        if (score > alpha)
            alpha = score;
        j++;
    }

    // Checks
    if(plies <= 0) {
        MoveList legalMoves = b.getPseudoLegalChecks(color);

        for (unsigned int i = 0; i < legalMoves.size(); i++) {
            Move m = legalMoves.get(i);

            Board copy = b.staticCopy();
            if (!copy.doPseudoLegalMove(m, color))
                continue;
            
            searchStats.nodes++;
            searchStats.qsNodes++;
            int score = -checkQuiescence(copy, color^1, plies+1, -beta, -alpha);
            
            if (score >= beta) {
                searchStats.qsFailHighs++;
                if (j == 0)
                    searchStats.qsFirstFailHighs++;
                alpha = beta;
                return beta;
            }
            if (score > alpha)
                alpha = score;
            j++;
        }
    }

    // TODO This is too slow to be effective
/*    if (score == -INFTY) {
        if (b.isStalemate(color))
            return 0;
    }*/

    return alpha;
}

/*
 * When checks are considered in quiescence, the responses must include all moves,
 * not just captures, necessitating this function.
 */
int checkQuiescence(Board &b, int color, int plies, int alpha, int beta) {
    MoveList legalMoves = b.getPseudoLegalCheckEscapes(color);

    int score = -INFTY;

    unsigned int j = 0; // separate counter only incremented when valid move is searched
    for (unsigned int i = 0; i < legalMoves.size(); i++) {
        Move m = legalMoves.get(i);

        Board copy = b.staticCopy();
        if (!copy.doPseudoLegalMove(m, color))
            continue;
        
        searchStats.nodes++;
        searchStats.qsNodes++;
        score = -quiescence(copy, color^1, plies+1, -beta, -alpha);
        
        if (score >= beta) {
            searchStats.qsFailHighs++;
            if (j == 0)
                searchStats.qsFirstFailHighs++;
            return beta;
        }
        if (score > alpha)
            alpha = score;
        j++;
    }

    // If there were no legal moves
    if (score == -INFTY) {
        // We already know we are in check, so it must be a checkmate
        // Adjust score so that quicker mates are better
        score = (-MATE_SCORE + searchParams.ply + plies);
        if (score >= beta)
            return beta;
        if (score > alpha)
            alpha = score;
    }
    
    return alpha;
}

//------------------------------------------------------------------------------
//------------------------------Other functions---------------------------------
//------------------------------------------------------------------------------

void clearTranspositionTable() {
    transpositionTable.clear();
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

// Recover PV for outputting to terminal / GUI using transposition table entries
string retrievePV(Board *b, Move bestMove, int plies) {
    Board copy = b->staticCopy();
    string pvStr = moveToString(bestMove);
    pvStr += " ";
    copy.doMove(bestMove, copy.getPlayerToMove());

    HashEntry *entry = transpositionTable.get(copy);
    int lineLength = 1;
    while (entry != NULL && lineLength < plies) {
        if (entry->m == NULL_MOVE)
            break;
        pvStr += moveToString(entry->m);
        pvStr += " ";
        copy.doMove(entry->m, copy.getPlayerToMove());
        entry = transpositionTable.get(copy);
        lineLength++;
    }

    return pvStr;
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
    cerr << setw(22) << "TT occupancy: " << transpositionTable.keys << " / "
         << transpositionTable.getSize() << endl;
    cerr << setw(22) << "Hash hitrate: " << getPercentage(searchStats.hashHits, searchStats.hashProbes)
         << '%' << " of " << searchStats.hashProbes << " probes" << endl;
    cerr << setw(22) << "Hash score cut rate: " << getPercentage(searchStats.hashScoreCuts, searchStats.hashHits)
         << '%' << " of " << searchStats.hashHits << " hash hits" << endl;
    cerr << setw(22) << "Hash move cut rate: " << getPercentage(searchStats.hashMoveCuts, searchStats.hashMoveAttempts)
         << '%' << " of " << searchStats.hashMoveAttempts << " hash moves" << endl;
    cerr << setw(22) << "First fail high rate: " << getPercentage(searchStats.firstFailHighs, searchStats.failHighs)
         << '%' << " of " << searchStats.failHighs << " fail highs" << endl;
    cerr << setw(22) << "QS Nodes: " << searchStats.qsNodes << " ("
         << getPercentage(searchStats.qsNodes, searchStats.nodes) << '%' << ")" << endl;
    cerr << setw(22) << "QS FFH rate: " << getPercentage(searchStats.qsFirstFailHighs, searchStats.qsFailHighs)
         << '%' << " of " << searchStats.qsFailHighs << " qs fail highs" << endl;
}