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
    "8/8/8/p1k4p/P2R3P/2P5/1K6/5q2 w - -"
};

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
    string name = "UCI Chess Engine";
    string version = "0";
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
            // make variables for default, min, and max values for hash in MB
            cout << "option name Hash type spin default " << 16 << " min " << 1 << " max " << 1024 << endl;
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
                int len = inputVector.size();
                bool isInc = (len == 9) || (len == 11);
                bool isRecur = (len == 7) || (len == 11);
                
                value = (color == WHITE) ? stoi(inputVector.at(2))
                    : stoi(inputVector.at(4));
                value -= BUFFER_TIME;
                
                if (isInc) value += (int)(MOVE_HORIZON / MAX_TIME_FACTOR)
                    * ((color == WHITE) ? stoi(inputVector.at(6))
                    : stoi(inputVector.at(8)));
                
                if (isRecur) {
                    int movesToGo = (len == 7) ? stoi(inputVector.at(6))
                        : stoi(inputVector.at(10));
                    value = (int)(value / (movesToGo * MAX_TIME_FACTOR));
                }
                else value /= MOVE_HORIZON;
            }
            
            bestMove = NULL_MOVE;
            isStop = false;
            searchThread = thread(getBestMove, &board, mode, value, &bestMove);
            searchThread.detach();
        }
        
        else if (input == "stop") isStop = true;
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
                
                searchThread = thread(getBestMove, &board, DEPTH, 9, &bestMove);
                searchThread.join();
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
            
            int *mailbox = board.getMailbox();
            int piece = mailbox[startSq] % 6;
            bool isCapture = (mailbox[endSq] != -1);
            bool isEP = (piece == PAWNS && (mailbox[endSq] == -1) && ((endSq - startSq) & 1));
            bool isDoublePawn = (piece == PAWNS && abs(endSq - startSq) == 16);
            delete[] mailbox;
            
            bool isCastle = (piece == KINGS && abs(endSq - startSq) == 2);
            string promotionString = " nbrq";
            int promotion = (moveStr.length() == 5)
                ? promotionString.find(moveStr.at(4)) : 0;
            
            Move m = encodeMove(startSq, endSq);
            m = setCapture(m, isCapture);
            m = setCastle(m, isCastle);
            if (isEP)
                m = setFlags(m, MOVE_EP);
            else if (promotion)
                m = setPromotion(m, promotion);
            else if (isDoublePawn)
                m = setFlags(m, MOVE_DOUBLE_PAWN);
            
            board.doMove(m, board.getPlayerToMove());
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
    int sqCounter = 0;
    string pieceString = "PNBRQKpnbrqk";
    
    // iterate through rows backwards (because mailbox goes a1 -> h8), converting into mailbox format
    for (int elem = 7; elem >= 0; elem--) {
        string rowAtElem = rows.at(elem);
        
        for (unsigned col = 0; col < rowAtElem.length(); col++) {
            char sq = rowAtElem.at(col);
            do {
                mailbox[sqCounter] = pieceString.find(sq);
                sqCounter++;
                sq--;
            }
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
            boardString += pieceString[mailbox[8 * i + j] + 1];
        }
        boardString += "|\n";
    }
    boardString += "  abcdefgh\n";
    delete[] mailbox;
    return boardString;
}

void clearAll(Board &board) {
    clearTranspositionTable();
    board = fenToBoard(STARTPOS);
}
