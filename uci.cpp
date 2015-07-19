#include <iostream>
#include <string>
#include <sstream>
#include <vector>

#include "uci.h"
using namespace std;

// Splits a string s with delimiter d into vector v.
void split(const string &s, char d, vector<string> &v) {
    stringstream ss(s);
    string item;
    while (getline(ss, item, d)) {
        v.push_back(item);
    }
}

// Splits a string s with delimiter d.
vector<string> split(const string &s, char d) {
    vector<string> v;
    split(s, d, v);
    return v;
}

Board fenToBoard(string s) {
    vector<string> components = split(s, ' ');
    vector<string> rows = split(components.at(0), '/');
    int mailbox[64];
    int sqCounter = 0;
    
    // iterate through rows backwards (because mailbox goes a1 -> h8), converting into mailbox format
    for (int elem = 7; elem >= 0; elem--) {
        string rowAtElem = rows.at(elem);
        
        for (unsigned col = 0; col < rowAtElem.length(); col++) {
            // determine what piece is on rowAtElem.at(col) and fill out if not blank
            switch (rowAtElem.at(col)) {
                case 'P':
                    mailbox[sqCounter] = PAWNS;
                    break;
                case 'N':
                    mailbox[sqCounter] = KNIGHTS;
                    break;
                case 'B':
                    mailbox[sqCounter] = BISHOPS;
                    break;
                case 'R':
                    mailbox[sqCounter] = ROOKS;
                    break;
                case 'Q':
                    mailbox[sqCounter] = QUEENS;
                    break;
                case 'K':
                    mailbox[sqCounter] = KINGS;
                    break;
                case 'p':
                    mailbox[sqCounter] = 6 + PAWNS;
                    break;
                case 'n':
                    mailbox[sqCounter] = 6 + KNIGHTS;
                    break;
                case 'b':
                    mailbox[sqCounter] = 6 + BISHOPS;
                    break;
                case 'r':
                    mailbox[sqCounter] = 6 + ROOKS;
                    break;
                case 'q':
                    mailbox[sqCounter] = 6 + QUEENS;
                    break;
                case 'k':
                    mailbox[sqCounter] = 6 + KINGS;
                    break;
            }
            
            if (rowAtElem.at(col) >= 'B') sqCounter++;
            
            // fill out blank squares
            if ('1' <= rowAtElem.at(col) && rowAtElem.at(col) <= '8') {
                for (int i = 0; i < rowAtElem.at(col) - '0'; i++) {
                    mailbox[sqCounter] = -1; // -1 is blank square
                    sqCounter++;
                }
            }
        }
    }
    
    int playerToMove = (components.at(1) == "w") ? WHITE : BLACK;
    bool whiteCanKCastle = (components.at(2).find("K") != string::npos);
    bool whiteCanQCastle = (components.at(2).find("Q") != string::npos);
    bool blackCanKCastle = (components.at(2).find("k") != string::npos);
    bool blackCanQCastle = (components.at(2).find("q") != string::npos);
    
    uint16_t epCaptureFile = NO_EP_POSSIBLE;
    if (components.at(3).find("6") != string::npos
     || components.at(3).find("3") != string::npos)
        epCaptureFile = components.at(3).at(0) - 'a';
    int fiftyMoveCounter = stoi(components.at(4));
    int moveNumber = stoi(components.at(5));
    return Board(mailbox, whiteCanKCastle, blackCanKCastle, whiteCanQCastle,
            blackCanQCastle, epCaptureFile, fiftyMoveCounter, moveNumber,
            playerToMove);
}

int main() {
    initZobristTable();

    string input;
    vector<string> inputVector;
    string name = "UCI Chess Engine";
    string version = "0";
    string author = "Jeffrey An and Michael An";
    string pos;
    
    const string STARTPOS = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    Board board = fenToBoard(STARTPOS);
    
    cout << name << " " << version << " by " << author << endl;
    
    while (input != "quit") {
        getline(cin, input);
        inputVector = split(input, ' ');
        
        if (input == "uci") {
            cout << "id name " << name << " " << version << endl;
            cout << "id author " << author << endl;
            // make variables for default, min, and max values for hash in MB
            cout << "option name Hash type spin default " << 16 << " min " << 1 << " max " << 1024 << endl;
            cout << "uciok" << endl;
        }
        
        if (input == "isready") {
            // return "readyok" when all initialization of values is done
            // must return "readyok" even when searching
            cout << "readyok" << endl;
        }
        
        if (input == "ucinewgame") {
            board = fenToBoard(STARTPOS);
            clearTranspositionTable();
        }
        
        if (input.substr(0, 8) == "position") {
            if (input.find("startpos") != string::npos)
                pos = STARTPOS;
            
            if (input.find("fen") != string::npos) {
                pos = inputVector.at(2) + ' ' + inputVector.at(3) + ' ' + inputVector.at(4) + ' '
                        + inputVector.at(5) + ' ' + inputVector.at(6) + ' ' + inputVector.at(7);
            }
            
            board = fenToBoard(pos);
            
            if (input.find("moves") != string::npos) {
                string moveList = input.substr(input.find("moves") + 6);
                vector<string> moveVector = split(moveList, ' ');
                
                for (unsigned i = 0; i < moveVector.size(); i++) {
                    // moveString contains the move in long algebraic notation
                    string moveString = moveVector.at(i);
                    char startFile = moveString.at(0);
                    char startRank = moveString.at(1);
                    char endFile = moveString.at(2);
                    char endRank = moveString.at(3);
                    
                    int startSq = 8 * (startRank - '0' - 1) + (startFile - 'a');
                    int endSq = 8 * (endRank - '0' - 1) + (endFile - 'a');
                    int *mailbox = board.getMailbox();

                    int piece = mailbox[startSq] % 6;
                    
                    bool isCapture = ((mailbox[endSq] != -1)
                            || (piece == PAWNS && abs(abs(startSq - endSq) - 8) == 1));
                    bool isCastle = (piece == KINGS && abs(endSq - startSq) == 2);
                    delete[] mailbox;
                    
                    int promotion = 0;
                    
                    if (moveString.length() == 5) {
                        switch (moveString.at(4)) {
                            case 'n':
                                promotion = KNIGHTS;
                                break;
                            case 'b':
                                promotion = BISHOPS;
                                break;
                            case 'r':
                                promotion = ROOKS;
                                break;
                            case 'q':
                                promotion = QUEENS;
                                break;
                        }
                    }
                    
                    Move m = encodeMove(startSq, endSq, piece, isCapture);
                    
                    m = setCastle(m, isCastle);
                    m = setPromotion(m, promotion);
                    
                    board.doMove(m, board.getPlayerToMove());
                }
            }
        }
        
        if (input.substr(0, 2) == "go") {
            int mode = DEPTH, value = 1;
            
            if (input.find("movetime") != string::npos && inputVector.size() > 2) {
                mode = TIME;
                value = stoi(inputVector.at(2));
            }
            
            if (input.find("depth") != string::npos && inputVector.size() > 2) {
                mode = DEPTH;
                value = stoi(inputVector.at(2));
            }
            
            if (input.find("infinite") != string::npos) {
                mode = DEPTH;
                value = MAX_DEPTH;
            }
            
            if (input.find("wtime") != string::npos) {
                mode = TIME;
                int color = board.getPlayerToMove();
                
                if (inputVector.size() == 5) {
                    if (color == WHITE) value = stoi(inputVector.at(2));
                    else value = stoi(inputVector.at(4));
                }
                if (inputVector.size() == 9) {
                    if (color == WHITE) value = stoi(inputVector.at(2)) + 40 * stoi(inputVector.at(6));
                    else value = stoi(inputVector.at(4)) + 40 * stoi(inputVector.at(8));
                }
                // Primitive time management: use on average 1/40 of remaining time with a 200 ms buffer zone
                value = (value - 200) / 40;
            }
            
            Move bestMove = getBestMove(&board, mode, value);
            cout << "bestmove " << moveToString(bestMove) << endl;
        }
        
        if (input == "stop") {
            // must stop search
            cerr << "stop works" << endl;
        }
        
        if (input == "mailbox") {
            int *mailbox = board.getMailbox();
            for (unsigned i = 0; i < 64; i++) {
                cerr << mailbox[i] << ' ';
                if (i % 8 == 7) cerr << endl;
            }
            delete[] mailbox;
        }
    }
}
