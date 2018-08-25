/*
    Laser, a UCI chess engine written in C++11.
    Copyright 2015-2018 Jeffrey An and Michael An

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
#include <cstring>
#include <iostream>
#include <sstream>
#include <vector>
#include <thread>
#include <random>

#include "attacks.h"
#include "common.h"
#include "bbinit.h"
#include "board.h"
#include "eval.h"
#include "search.h"
#include "timeman.h"
#include "uci.h"
#include "syzygy/tbprobe.h"

using std::cout;
using std::cerr;
using std::endl;
using std::string;

constexpr char STARTPOS[] = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

void setPosition(string &input, std::vector<string> &inputVector, Board &board);
std::vector<string> split(const string &s, char d);
Move stringToMove(const string &moveStr, Board &b, bool &reversible);
string boardToString(Board &board);
bool equalsIgnoreCase(const std::string &s1, const std::string &s2);
void stringToLowerCase(std::string &s);
void clearAll(Board &board);
uint64_t perft(Board &b, int color, int depth, uint64_t &captures);
void runBenchmark(Board &b, int depth);


static int BUFFER_TIME = DEFAULT_BUFFER_TIME;
MoveList movesToSearch;
TimeManagement timeParams;
// Declared in search.cpp
extern std::atomic<bool> isStop;
extern std::atomic<bool> stopSignal;


int main(int argc, char **argv) {
    initAttacks();
    initEvalTables();
    initDistances();
    initZobristTable();
    initInBetweenTable();
    initPerThreadMemory();
    initReductionTable();

    setMultiPV(DEFAULT_MULTI_PV);
    setNumThreads(DEFAULT_THREADS);

    string input;
    std::vector<string> inputVector;
    string author = "Jeffrey An and Michael An";
    std::thread searchThread;

    Board board = fenToBoard(STARTPOS);

    cout << VERSION_ID << " by " << author << endl;

    // Run benchmark from command line with given depth
    if (argc > 1 && strcmp(argv[1], "bench") == 0) {
        runBenchmark(board, argc > 2 ? atoi(argv[2]) : 0);
        return 0;
    }

    while (true) {
        getline(std::cin, input);
        stringToLowerCase(input);
        inputVector = split(input, ' ');
        std::cin.clear();

        // Ignore all input other than "stop", "quit", and "ponderhit" while running a search.
        if (!isStop && input != "stop" && input != "quit" && input != "ponderhit")
            continue;

        if (input == "uci") {
            cout << "id name " << LASER_VERSION << endl;
            cout << "id author " << author << endl;
            cout << "option name Threads type spin default " << DEFAULT_THREADS
                 << " min " << MIN_THREADS << " max " << MAX_THREADS << endl;
            cout << "option name Hash type spin default " << DEFAULT_HASH_SIZE
                 << " min " << MIN_HASH_SIZE << " max " << MAX_HASH_SIZE << endl;
            cout << "option name EvalCache type spin default " << DEFAULT_HASH_SIZE
                 << " min " << MIN_HASH_SIZE << " max " << MAX_HASH_SIZE << endl;
            cout << "option name Ponder type check default false" << endl;
            cout << "option name MultiPV type spin default " << DEFAULT_MULTI_PV
                 << " min " << MIN_MULTI_PV << " max " << MAX_MULTI_PV << endl;
            cout << "option name BufferTime type spin default " << DEFAULT_BUFFER_TIME
                 << " min " << MIN_BUFFER_TIME << " max " << MAX_BUFFER_TIME << endl;
            cout << "option name SyzygyPath type string default <empty>" << endl;
            cout << "option name ScaleMaterial type spin default " << DEFAULT_EVAL_SCALE
                 << " min " << MIN_EVAL_SCALE << " max " << MAX_EVAL_SCALE << endl;
            cout << "option name ScaleKingSafety type spin default " << DEFAULT_EVAL_SCALE
                 << " min " << MIN_EVAL_SCALE << " max " << MAX_EVAL_SCALE << endl;
            cout << "uciok" << endl;
        }
        else if (input == "isready") cout << "readyok" << endl;
        else if (input == "ucinewgame") clearAll(board);
        else if (input.substr(0, 8) == "position") setPosition(input, inputVector, board);
        else if (input.substr(0, 2) == "go" && isStop) {
            std::vector<string>::iterator it;

            if (input.find("ponder") != string::npos)
                startPonder();

            if (input.find("searchmoves") != string::npos) {
                movesToSearch.clear();
                it = find(inputVector.begin(), inputVector.end(), "searchmoves");
                it++;
                std::locale loc;
                while (it != inputVector.end() && (*it).size() >= 4 && std::isdigit((*it)[1], loc)) {
                    string moveStr = *it;
                    bool reversible;
                    movesToSearch.add(stringToMove(moveStr, board, reversible));
                    it++;
                }
            }

            if (input.find("movetime") != string::npos && inputVector.size() > 2) {
                timeParams.searchMode = MOVETIME;
                it = find(inputVector.begin(), inputVector.end(), "movetime");
                it++;
                timeParams.allotment = std::stoi(*it);
            }
            else if (input.find("depth") != string::npos && inputVector.size() > 2) {
                timeParams.searchMode = DEPTH;
                it = find(inputVector.begin(), inputVector.end(), "depth");
                it++;
                timeParams.allotment = std::min(MAX_DEPTH, std::stoi(*it));
            }
            else if (input.find("infinite") != string::npos) {
                timeParams.searchMode = DEPTH;
                timeParams.allotment = MAX_DEPTH;
            }
            else if (input.find("wtime") != string::npos
                  || input.find("btime") != string::npos) {
                timeParams.searchMode = TIME;
                int color = board.getPlayerToMove();
                int moveNumber = std::min(ENDGAME_HORIZON_LIMIT, (int) board.getMoveNumber());

                it = find(inputVector.begin(), inputVector.end(), (color == WHITE) ? "wtime" : "btime");
                it++;
                int timeRemaining = std::stoi(*it);
                int minValue = std::min(timeRemaining, BUFFER_TIME) / 100;
                timeRemaining -= BUFFER_TIME;
                // We can never have negative time
                timeRemaining = std::max(0, timeRemaining);

                int movesToGo = MOVE_HORIZON - MOVE_HORIZON_DEC * moveNumber / ENDGAME_HORIZON_LIMIT;

                // Parse recurring time controls, and set a different movestogo if necessary
                it = find(inputVector.begin(), inputVector.end(), "movestogo");
                if (it != inputVector.end()) {
                    it++;
                    movesToGo = std::min(movesToGo, std::stoi(*it));
                }

                int value = timeRemaining / movesToGo;

                // Parse the increment if available
                int increment = 0;
                it = find(inputVector.begin(), inputVector.end(), (color == WHITE) ? "winc" : "binc");
                if (it != inputVector.end()) {
                    it++;
                    increment = std::stoi(*it);
                }
                value += increment;

                // Minimum thinking time
                value = std::max(value, minValue);

                // Use special factors for recurring time controls with movestogo < 10
                if (increment == 0 && movesToGo < 10) {
                    timeParams.maxAllotment = (int) std::min(value * MAX_TIME_FACTOR, timeRemaining * MAX_USAGE_FACTORS[movesToGo]);
                    timeParams.allotment = std::max(value, (int) (timeRemaining * ALLOTMENT_FACTORS[movesToGo]));
                }
                else {
                    timeParams.maxAllotment = (int) std::min(value * MAX_TIME_FACTOR, timeRemaining * 0.95);
                    timeParams.allotment = std::min(value, timeParams.maxAllotment / 3);
                }
            }

            isStop = false;
            stopSignal = false;
            if (searchThread.joinable()) searchThread.join();
            searchThread = std::thread(getBestMoveThreader, &board, &timeParams, &movesToSearch);
        }
        else if (input == "ponderhit") {
            stopPonder();
        }

        else if (input == "stop") {
            stopPonder();
            isStop = true;
            stopSignal = true;
            if (searchThread.joinable()) searchThread.join();
        }
        else if (input == "quit") {
            stopPonder();
            isStop = true;
            stopSignal = true;
            if (searchThread.joinable()) searchThread.join();
            break;
        }
        else if (input.substr(0, 9) == "setoption" && inputVector.size() >= 5) {
            if (inputVector.at(1) != "name" || inputVector.at(3) != "value") {
                cout << "info string Invalid option format." << endl;
            }
            else {
                if (inputVector.at(2) == "threads") {
                    int threads = std::stoi(inputVector.at(4));
                    if (threads < MIN_THREADS)
                        threads = MIN_THREADS;
                    if (threads > MAX_THREADS)
                        threads = MAX_THREADS;
                    setNumThreads(threads);
                }
                else if (inputVector.at(2) == "hash") {
                    uint64_t MB = std::stoull(inputVector.at(4));
                    if (MB < MIN_HASH_SIZE)
                        MB = MIN_HASH_SIZE;
                    if (MB > MAX_HASH_SIZE)
                        MB = MAX_HASH_SIZE;
                    setHashSize(MB);
                }
                else if (inputVector.at(2) == "evalcache") {
                    uint64_t MB = std::stoull(inputVector.at(4));
                    if (MB < MIN_HASH_SIZE)
                        MB = MIN_HASH_SIZE;
                    if (MB > MAX_HASH_SIZE)
                        MB = MAX_HASH_SIZE;
                    setEvalCacheSize(MB);
                }
                else if (inputVector.at(2) == "ponder") {
                    // do nothing
                }
                else if (inputVector.at(2) == "multipv") {
                    int multiPV = std::stoi(inputVector.at(4));
                    if (multiPV < MIN_MULTI_PV)
                        multiPV = MIN_MULTI_PV;
                    if (multiPV > MAX_MULTI_PV)
                        multiPV = MAX_MULTI_PV;
                    setMultiPV((unsigned int) multiPV);
                }
                else if (inputVector.at(2) == "buffertime") {
                    BUFFER_TIME = std::stoi(inputVector.at(4));
                    if (BUFFER_TIME < MIN_BUFFER_TIME)
                        BUFFER_TIME = MIN_BUFFER_TIME;
                    if (BUFFER_TIME > MAX_BUFFER_TIME)
                        BUFFER_TIME = MAX_BUFFER_TIME;
                }
                else if (inputVector.at(2) == "syzygypath") {
                    string path = inputVector.at(4);
                    for (unsigned int i = 5; i < inputVector.size(); i++) {
                        path += string(" ") + inputVector.at(i);
                    }
                    char *c_path = (char *) malloc(path.length() + 1);
                    std::strcpy(c_path, path.c_str());
                    init_tablebases(c_path);
                    free(c_path);
                }
                else if (inputVector.at(2) == "scalematerial") {
                    int scale = std::stoi(inputVector.at(4));
                    if (scale < MIN_EVAL_SCALE)
                        scale = MIN_EVAL_SCALE;
                    if (scale > MAX_EVAL_SCALE)
                        scale = MAX_EVAL_SCALE;
                    setMaterialScale(scale);
                }
                else if (inputVector.at(2) == "scalekingsafety") {
                    int scale = std::stoi(inputVector.at(4));
                    if (scale < MIN_EVAL_SCALE)
                        scale = MIN_EVAL_SCALE;
                    if (scale > MAX_EVAL_SCALE)
                        scale = MAX_EVAL_SCALE;
                    setKingSafetyScale(scale);
                }
                else
                    cout << "info string Invalid option." << endl;
            }
        }

        //----------------------------Non-UCI Commands--------------------------
        else if (input == "board") cerr << boardToString(board);
        else if (input.substr(0, 5) == "perft" && inputVector.size() == 2) {
            int depth = std::stoi(inputVector.at(1));

            uint64_t captures = 0;
            auto startTime = ChessClock::now();

            uint64_t nodes = perft(board, board.getPlayerToMove(), depth, captures);

            uint64_t time = getTimeElapsed(startTime);

            cerr << "Nodes: " << nodes << endl;
            cerr << "Captures: " << captures << endl;
            cerr << "Time: " << time << endl;
            cerr << "Nodes/second: " << 1000 * nodes / time << endl;
        }
        else if (input.substr(0, 5) == "bench") {
            int depth = 0;
            if (inputVector.size() == 2)
                depth = std::stoi(inputVector.at(1));
            runBenchmark(board, depth);
        }

        else if (input == "eval") {
            Eval e;
            e.evaluate<true>(board);
        }

        // According to UCI protocol, inputs that do not make sense are ignored
    }

    return 0;
}

void setPosition(string &input, std::vector<string> &inputVector, Board &board) {
    string pos;

    if (input.find("startpos") != string::npos)
        pos = STARTPOS;

    if (input.find("fen") != string::npos) {
        if (inputVector.size() < 5 || inputVector.at(4) == "moves") {
            pos = inputVector.at(2) + ' ' + inputVector.at(3) + " - -";
        }
        else if (inputVector.size() < 7 || inputVector.at(6) == "moves") {
            pos = inputVector.at(2) + ' ' + inputVector.at(3) + ' ' + inputVector.at(4) + ' '
                + inputVector.at(5);
        }
        else {
            pos = inputVector.at(2) + ' ' + inputVector.at(3) + ' ' + inputVector.at(4) + ' '
                + inputVector.at(5) + ' ' + inputVector.at(6) + ' ' + inputVector.at(7);
        }
    }

    board = fenToBoard(pos);
    TwoFoldStack *twoFoldPositions = getTwoFoldStackPointer();
    twoFoldPositions->clear();

    size_t moveListStart = input.find("moves");
    if (moveListStart != string::npos) {
        moveListStart += 6;
        // Check for a non-empty movelist
        if (moveListStart < input.length()) {
            string moveList = input.substr(moveListStart);
            std::istringstream is(moveList);
            // moveStr contains the move in long algebraic notation
            string moveStr;
            while (is >> moveStr) {
                bool reversible;
                Move m = stringToMove(moveStr, board, reversible);

                // Record positions on two fold stack.
                twoFoldPositions->push(board.getZobristKey());
                // The stack is cleared for captures, pawn moves, and castles, which are all
                // irreversible
                if (!reversible)
                    twoFoldPositions->clear();

                board.doMove(m, board.getPlayerToMove());
            }
        }
    }

    twoFoldPositions->setRootEnd();
}

// Splits a string s with delimiter d.
std::vector<string> split(const string &s, char d) {
    std::vector<string> v;
    std::stringstream ss(s);
    string item;
    while (getline(ss, item, d)) {
        v.push_back(item);
    }
    return v;
}

Move stringToMove(const string &moveStr, Board &b, bool &reversible) {
    int startSq = 8 * (moveStr.at(1) - '1') + (moveStr.at(0) - 'a');
    int endSq = 8 * (moveStr.at(3) - '1') + (moveStr.at(2) - 'a');

    int color = b.getPlayerToMove();
    bool isCapture = (bool)(indexToBit(endSq) & b.getAllPieces(color ^ 1));
    bool isPawnMove = (bool)(indexToBit(startSq) & b.getPieces(color, PAWNS));
    bool isKingMove = (bool)(indexToBit(startSq) & b.getPieces(color, KINGS));

    bool isEP = (isPawnMove && !isCapture && ((endSq - startSq) & 1));
    bool isDoublePawn = (isPawnMove && abs(endSq - startSq) == 16);
    bool isCastle = (isKingMove && abs(endSq - startSq) == 2);
    string promotionString = " nbrq";
    int promotion = (moveStr.length() == 5)
        ? promotionString.find(moveStr.at(4)) : 0;

    Move m = encodeMove(startSq, endSq);
    m = setCapture(m, isCapture);
    m = setCastle(m, isCastle);
    if (isEP)
        m = setFlags(m, MOVE_EP);
    else if (promotion) {
        m = setFlags(m, MOVE_PROMO_N + promotion - 1);
    }
    else if (isDoublePawn)
        m = setFlags(m, MOVE_DOUBLE_PAWN);

    reversible = !(isCapture || isPawnMove || isCastle);
    return m;
}

Board fenToBoard(string s) {
    std::vector<string> components = split(s, ' ');
    std::vector<string> rows = split(components.at(0), '/');
    int mailbox[64];
    int sqCounter = -1;
    string pieceString = "PNBRQKpnbrqk";

    // iterate through rows backwards (because mailbox goes a1 -> h8), converting into mailbox format
    for (int elem = 7; elem >= 0; elem--) {
        string rowAtElem = rows.at(elem);

        for (unsigned col = 0; col < rowAtElem.length(); col++) {
            char sq = rowAtElem.at(col);
            do mailbox[++sqCounter] = pieceString.find(sq--);
            while ('0' < sq && sq < '8');
        }
    }

    int playerToMove = (components.at(1) == "w") ? WHITE : BLACK;
    bool whiteCanKCastle = (components.at(2).find("K") != string::npos);
    bool whiteCanQCastle = (components.at(2).find("Q") != string::npos);
    bool blackCanKCastle = (components.at(2).find("k") != string::npos);
    bool blackCanQCastle = (components.at(2).find("q") != string::npos);
    int epCaptureFile = (components.at(3) == "-") ? NO_EP_POSSIBLE
        : components.at(3).at(0) - 'a';
    int fiftyMoveCounter = (components.size() == 6) ? std::stoi(components.at(4)) : 0;
    int moveNumber = (components.size() == 6) ? std::stoi(components.at(5)) : 1;
    return Board(mailbox, whiteCanKCastle, blackCanKCastle, whiteCanQCastle,
            blackCanQCastle, epCaptureFile, fiftyMoveCounter, moveNumber,
            playerToMove);
}

string boardToFEN(Board &board) {
    int *mailbox = board.getMailbox();
    string pieceString = "PNBRQKpnbrqk";
    string fenString;
    int emptyCt = 0;

    for (int r = 7; r >= 0; r--) {
        for (int f = 0; f < 8; f++) {
            int sq = 8*r + f;
            if (mailbox[sq] == -1)
                emptyCt++;
            else {
                if (emptyCt) {
                    fenString += std::to_string(emptyCt);
                    emptyCt = 0;
                }
                fenString += pieceString[mailbox[sq]];
            }
        }

        if (emptyCt) {
            fenString += std::to_string(emptyCt);
            emptyCt = 0;
        }
        if (r != 0)
            fenString += '/';
    }

    fenString += ' ';
    fenString += (board.getPlayerToMove() == WHITE) ? 'w' : 'b';
    fenString += ' ';
    bool hasCastles = false;
    if (board.getWhiteCanKCastle()) { hasCastles = true; fenString += 'K'; }
    if (board.getWhiteCanQCastle()) { hasCastles = true; fenString += 'Q'; }
    if (board.getBlackCanKCastle()) { hasCastles = true; fenString += 'k'; }
    if (board.getBlackCanQCastle()) { hasCastles = true; fenString += 'q'; }
    if (!hasCastles) fenString += '-';
    fenString += ' ';

    uint16_t epCaptureFile = board.getEPCaptureFile();
    if (epCaptureFile == NO_EP_POSSIBLE)
        fenString += '-';
    else {
        fenString += 'a' + epCaptureFile;
        fenString += (board.getPlayerToMove() == WHITE) ? '6' : '3';
    }

    fenString += ' ';
    fenString += std::to_string(board.getFiftyMoveCounter());
    fenString += ' ';
    fenString += std::to_string(board.getMoveNumber());

    return fenString;
}

string boardToString(Board &board) {
    int *mailbox = board.getMailbox();
    string pieceString = " PNBRQKpnbrqk";
    string boardString;
    for (int i = 7; i >= 0; i--) {
        boardString += (char)(i + '1');
        boardString += '|';
        for (int j = 0; j < 8; j++) {
            boardString += pieceString[mailbox[8*i+j] + 1];
        }
        boardString += "|\n";
    }
    boardString += "  abcdefgh\n";
    delete[] mailbox;
    return boardString;
}

bool equalsIgnoreCase(const std::string &s1, const std::string &s2) {
    if (s1.size() != s2.size())
        return false;
    for (std::size_t i = 0; i < s1.size(); i++) {
        if (tolower(s1[i]) != tolower(s2[i]))
            return false;
    }
    return true;
}

void stringToLowerCase(std::string &s) {
    if (equalsIgnoreCase(s.substr(0, 8), "position")) {
        for (std::size_t i = 0; i < 12; i++)
            s[i] = tolower(s[i]);
    }
    else if (equalsIgnoreCase(s.substr(0, 9), "setoption")) {
        std::size_t j = 0;
        int spaceCt = 0;
        // Find the index of the 4th space: right after value
        // setoption _1_ name _2_ <name> _3_ value _4_ <value>
        for (; spaceCt < 4; j++)
            if (s[j] == ' ')
                spaceCt++;
        for (std::size_t i = 0; i < j; i++)
            s[i] = tolower(s[i]);
    }
    else {
        for (std::size_t i = 0; i < s.size(); i++)
            s[i] = tolower(s[i]);
    }
}

void clearAll(Board &board) {
    clearTables();
    board = fenToBoard(STARTPOS);
}

/*
 * Performs a PERFT (performance test). Useful for testing/debugging
 * PERFT n counts the number of possible positions after n moves by either side,
 * ex. PERFT 4 = # of positions after 2 moves from each side
 *
 * 7/8/15: PERFT 5, 1.46 s (i5-2450m)
 * 7/11/15: PERFT 5, 1.22 s (i5-2450m)
 * 7/13/15: PERFT 5, 1.08 s (i5-2450m)
 * 7/14/15: PERFT 5, 0.86 s (i5-2450m)
 * 7/17/15: PERFT 5, 0.32 s (i5-2450m)
 * 8/7/15: PERFT 5, 0.25 s, PERFT 6, 6.17 s (i5-5200u)
 * 8/8/15: PERFT 6, 5.90 s (i5-5200u)
 * 8/11/15: PERFT 6, 5.20 s (i5-5200u)
 */
uint64_t perft(Board &b, int color, int depth, uint64_t &captures) {
    if (depth == 0)
        return 1;

    uint64_t nodes = 0;

    MoveList pl;
    b.getAllPseudoLegalMoves(pl, color);
    for (unsigned int i = 0; i < pl.size(); i++) {
        Board copy = b.staticCopy();
        if (!copy.doPseudoLegalMove(pl.get(i), color))
            continue;

        if (isCapture(pl.get(i)))
            captures++;

        nodes += perft(copy, color^1, depth-1, captures);
    }

    return nodes;
}

void runBenchmark(Board &b, int depth) {
    const std::vector<string> benchPositions = {
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -",
        "r2q4/pp1k1pp1/2p1r1np/5p2/2N5/1P5Q/5PPP/3RR1K1 b - -",
        "5k2/1qr2pp1/2Np1n1r/QB2p3/2R4p/3PPRPb/PP2P2P/6K1 w - -",
        "r2r2k1/2p2pp1/p1n4p/1qbnp3/2Q5/1PPP1RPP/3NN2K/R1B5 b - -",
        "8/3k4/p6Q/pq6/3p4/1P6/P3p1P1/6K1 w - -",
        "8/8/k7/2B5/P1K5/8/8/1r6 w - -",
        "8/8/8/p1k4p/P2R3P/2P5/1K6/5q2 w - -",
        "rnbq1k1r/ppp1ppb1/5np1/1B1pN2p/P2P1P2/2N1P3/1PP3PP/R1BQK2R w KQ -",
        "4r3/6pp/2p1p1k1/4Q2n/1r2Pp2/8/6PP/2R3K1 w - -",
        "8/3k2p1/p2P4/P5p1/8/1P1R1P2/5r2/3K4 w - -",
        "r5k1/1bqnbp1p/r3p1p1/pp1pP3/2pP1P2/P1P2N1P/1P2NBP1/R2Q1RK1 b - -",
        "r1bqk2r/1ppnbppp/p1np4/4p1P1/4PP2/3P1N1P/PPP5/RNBQKBR1 b Qkq -",
        "5nk1/6pp/8/pNpp4/P7/1P1Pp3/6PP/6K1 w - -",
        "2r2rk1/1p2npp1/1q1b1nbp/p2p4/P2N3P/BPN1P3/4BPP1/2RQ1RK1 w - -",
        "8/2b3p1/4knNp/2p4P/1pPp1P2/1P1P1BPK/8/8 w - -"
    };

    auto startTime = ChessClock::now();
    uint64_t totalNodes = 0;
    movesToSearch.clear();
    timeParams.searchMode = DEPTH;
    // Set a default when the given depth is 0.
    timeParams.allotment = depth ? depth : 13;

    for (unsigned int i = 0; i < benchPositions.size(); i++) {
        clearAll(b);
        b = fenToBoard(benchPositions.at(i));

        isStop = false;
        stopSignal = false;
        getBestMoveThreader(&b, &timeParams, &movesToSearch);
        isStop = true;
        stopSignal = true;

        totalNodes += getNodes();
    }

    uint64_t time = getTimeElapsed(startTime);

    clearAll(b);

    cerr << "Time  : " << time << " ms" << endl;
    cerr << "Nodes : " << totalNodes << endl;
    cerr << "NPS   : " << 1000 * totalNodes / time << endl;
}
