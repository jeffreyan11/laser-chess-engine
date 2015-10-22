/*
    Laser, a UCI chess engine written in C++11.
    Copyright 2015 Jeffrey An and Michael An

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
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <thread>
#include <random>

#include "uci.h"
using namespace std;

const string STARTPOS = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
const vector<string> positions = {
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

const int MIN_HASH_SIZE = 1;
const int MAX_HASH_SIZE = 1024;

void setPosition(string &input, vector<string> &inputVector, Board &board);
vector<string> split(const string &s, char d);
Board fenToBoard(string s);
string boardToString(Board &board);
void clearAll(Board &board);

volatile bool isStop = true;

int main() {
    initMagicTables(2563762638929852183ULL);
    initZobristTable();
    initInBetweenTable();

    string input;
    vector<string> inputVector;
    string name = "Laser";
    string version = "0.2 beta";
    string author = "Jeffrey An and Michael An";
    thread searchThread;
    Move bestMove = NULL_MOVE;
    
    Board board = fenToBoard(STARTPOS);
    
    cout << name << " " << version << " by " << author << endl;
    
    while (input != "quit") {
        getline(cin, input);
        inputVector = split(input, ' ');
        cin.clear();
        
        if (input == "uci") {
            cout << "id name " << name << " " << version << endl;
            cout << "id author " << author << endl;
            cout << "option name Hash type spin default " << 16
                 << " min " << MIN_HASH_SIZE << " max " << MAX_HASH_SIZE << endl;
            cout << "option name EvalCache type spin default " << 16
                 << " min " << MIN_HASH_SIZE << " max " << MAX_HASH_SIZE << endl;
            cout << "uciok" << endl;
        }
        else if (input == "isready") cout << "readyok" << endl;
        else if (input == "ucinewgame") clearAll(board);
        else if (input.substr(0, 8) == "position") setPosition(input, inputVector, board);
        else if (input.substr(0, 2) == "go" && isStop) {
            int mode = DEPTH, value = 1;
            
            if (input.find("movetime") != string::npos && inputVector.size() > 2) {
                mode = TIME;
                value = stoi(inputVector.at(2));
            }
            else if (input.find("depth") != string::npos && inputVector.size() > 2) {
                mode = DEPTH;
                value = min(MAX_DEPTH, stoi(inputVector.at(2)));
            }
            else if (input.find("infinite") != string::npos) {
                mode = DEPTH;
                value = MAX_DEPTH;
            }
            else if (input.find("wtime") != string::npos) {
                mode = TIME;
                int color = board.getPlayerToMove();
                vector<string>::iterator it;

                it = find(inputVector.begin(), inputVector.end(),
                        (color == WHITE) ? "wtime" : "btime");
                it++;
                value = stoi(*it);
                value -= BUFFER_TIME;
                
                int maxValue = value / MAX_TIME_FACTOR;

                // recurring time controls
                it = find(inputVector.begin(), inputVector.end(), "movestogo");
                if (it != inputVector.end()) {
                    it++;
                    int movesToGo = stoi(*it);
                    value /= max(movesToGo, (int)MAX_TIME_FACTOR + 1);
                }
                else value /= MOVE_HORIZON;
                
                // increment time controls
                it = find(inputVector.begin(), inputVector.end(),
                        (color == WHITE) ? "winc" : "binc");
                if (it != inputVector.end()) {
                    it++;
                    value += stoi(*it);
                    value = min(value, maxValue);
                }
            }
            
            bestMove = NULL_MOVE;
            isStop = false;
            searchThread = thread(getBestMove, &board, mode, value, &bestMove);
            searchThread.detach();
        }
        
        else if (input == "stop") isStop = true;
        else if (input.substr(0, 9) == "setoption" && inputVector.size() == 5) {
            if (inputVector.at(1) != "name" || inputVector.at(3) != "value") {
                cout << "Invalid option format." << endl;
            }
            else {
                if (inputVector.at(2) == "Hash" || inputVector.at(2) == "hash") {
                    int MB = stoi(inputVector.at(4));
                    if (MB < MIN_HASH_SIZE)
                        MB = MIN_HASH_SIZE;
                    if (MB > MAX_HASH_SIZE)
                        MB = MAX_HASH_SIZE;
                    setHashSize((uint64_t) MB);
                }
                else if (inputVector.at(2) == "EvalCache" || inputVector.at(2) == "evalcache") {
                    int MB = stoi(inputVector.at(4));
                    if (MB < MIN_HASH_SIZE)
                        MB = MIN_HASH_SIZE;
                    if (MB > MAX_HASH_SIZE)
                        MB = MAX_HASH_SIZE;
                    setEvalCacheSize((uint64_t) MB);
                }
                else
                    cout << "Invalid option." << endl;
            }
        }

        //----------------------------Non-UCI Commands--------------------------
        else if (input == "board") cerr << boardToString(board);
        else if (input.substr(0, 5) == "perft" && inputVector.size() == 2) {
            int depth = stoi(inputVector.at(1));

            Board b;
            uint64_t captures = 0;
            auto startTime = ChessClock::now();
            
            uint64_t nodes = perft(b, WHITE, depth, captures);
            
            double time = getTimeElapsed(startTime);
            
            cerr << "Nodes: " << nodes << endl;
            cerr << "Captures: " << captures << endl;
            cerr << "Time: " << (int)(time * ONE_SECOND) << endl;
            cerr << "Nodes/second: " << (uint64_t)(nodes / time) << endl;
        }
        else if (input == "bench") {
            auto startTime = ChessClock::now();
            uint64_t totalNodes = 0;
            
            for (unsigned int i = 0; i < positions.size(); i++) {
                clearAll(board);
                board = fenToBoard(positions.at(i));
                bestMove = NULL_MOVE;
                isStop = false;
                
                getBestMove(&board, DEPTH, 10, &bestMove);
                totalNodes += getNodes();
            }
            
            double time = getTimeElapsed(startTime);
                
            clearAll(board);
            
            cerr << "Nodes: " << totalNodes << endl;
            cerr << "Time: " << (int)(time * ONE_SECOND) << endl;
            cerr << "Nodes/second: " << (uint64_t)(totalNodes / time) << endl;
        }
        else if (input.substr(0, 9) == "tunemagic" && inputVector.size() == 3) {
            int iters = stoi(inputVector.at(1));
            uint64_t seed = stoull(inputVector.at(2));
            mt19937_64 rng (seed);
            double minTime = 999;
            uint64_t bestSeed = 0;
            for (int i = 0; i < iters; i++) {
                if ((i & 0xFF) == 0xFF) cerr << "Trial " << i+1 << endl;
                auto initStart = ChessClock::now();
                uint64_t test = rng();
                initMagicTables(test);
                double initTime = getTimeElapsed(initStart);
                if (initTime < minTime) {
                    bestSeed = test;
                    minTime = initTime;
                    cerr << "New best! Seed: " << bestSeed << " Time: " << initTime << endl;
                }
            }
            cerr << "Best seed: " << bestSeed << endl;
            cerr << "Time: " << minTime << endl;
        }
        
        // According to UCI protocol, inputs that do not make sense are ignored
    }
}

void setPosition(string &input, vector<string> &inputVector, Board &board) {
    string pos;

    if (input.find("startpos") != string::npos)
        pos = STARTPOS;
    
    if (input.find("fen") != string::npos) {
        if (inputVector.size() < 7 || inputVector.at(6) == "moves") {
            pos = inputVector.at(2) + ' ' + inputVector.at(3) + ' ' + inputVector.at(4) + ' '
                + inputVector.at(5);
        }
        else {
            pos = inputVector.at(2) + ' ' + inputVector.at(3) + ' ' + inputVector.at(4) + ' '
                + inputVector.at(5) + ' ' + inputVector.at(6) + ' ' + inputVector.at(7);
        }
    }
    
    board = fenToBoard(pos);
    
    if (input.find("moves") != string::npos) {
        string moveList = input.substr(input.find("moves") + 6);
        vector<string> moveVector = split(moveList, ' ');
        
        for (unsigned i = 0; i < moveVector.size(); i++) {
            // moveStr contains the move in long algebraic notation
            string moveStr = moveVector.at(i);
            
            int startSq = 8 * (moveStr.at(1) - '1') + (moveStr.at(0) - 'a');
            int endSq = 8 * (moveStr.at(3) - '1') + (moveStr.at(2) - 'a');
            
            int color = board.getPlayerToMove();
            bool isCapture = (bool)(INDEX_TO_BIT[endSq] & board.getAllPieces(color ^ 1));
            bool isPawnMove = (bool)(INDEX_TO_BIT[startSq] & board.getPieces(color, PAWNS));
            bool isKingMove = (bool)(INDEX_TO_BIT[startSq] & board.getPieces(color, KINGS));
            
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
            
            board.doMove(m, color);
        }
    }
}

// Splits a string s with delimiter d.
vector<string> split(const string &s, char d) {
    vector<string> v;
    stringstream ss(s);
    string item;
    while (getline(ss, item, d)) {
        v.push_back(item);
    }
    return v;
}

Board fenToBoard(string s) {
    vector<string> components = split(s, ' ');
    vector<string> rows = split(components.at(0), '/');
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
    int fiftyMoveCounter = (components.size() == 6) ? stoi(components.at(4)) : 0;
    int moveNumber = (components.size() == 6) ? stoi(components.at(5)) : 1;
    return Board(mailbox, whiteCanKCastle, blackCanKCastle, whiteCanQCastle,
            blackCanQCastle, epCaptureFile, fiftyMoveCounter, moveNumber,
            playerToMove);
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

void clearAll(Board &board) {
    clearTables();
    board = fenToBoard(STARTPOS);
}
