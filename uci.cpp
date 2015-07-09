#include <iostream>
#include <string>
#include <sstream>
#include <vector>

#include "uci.h"
using namespace std;

// Splits a string s with delimiter d into vector v.
vector<string> &split(const string &s, char d, vector<string> &v) {
    stringstream ss(s);
    string item;
    while (getline(ss, item, d)) {
        v.push_back(item);
    }
    return v;
}

// Splits a string s with delimiter d.
vector<string> split(const string &s, char d) {
    vector<string> v;
    split(s, d, v);
    return v;
}

// int* return type for testing
int* fenToBoard(string s) {
    vector<string> components = split(s, ' ');
    vector<string> rows = split(components.at(0), '/');
    int mailbox[64];
    
    int colCounter, blankSquareCounter;
    bool isBlankSquare;
    
    // iterate through rows, converting into mailbox format
    for (int elem = 0; elem < 8; elem++) {
        string rowAtElem = rows.at(elem);
        
        colCounter = 0;
        
        for (unsigned col = 0; col < rowAtElem.length(); col++) {
            blankSquareCounter = 0;
            isBlankSquare = false;
            
            // determine what piece is on rowAtElem.at(col) and fill out if not blank
            switch (rowAtElem.at(col)) {
                case '1':
                    isBlankSquare = true;
                    blankSquareCounter = 1;
                    break;
                case '2':
                    isBlankSquare = true;
                    blankSquareCounter = 2;
                    break;
                case '3':
                    isBlankSquare = true;
                    blankSquareCounter = 3;
                    break;
                case '4':
                    isBlankSquare = true;
                    blankSquareCounter = 4;
                    break;
                case '5':
                    isBlankSquare = true;
                    blankSquareCounter = 5;
                    break;
                case '6':
                    isBlankSquare = true;
                    blankSquareCounter = 6;
                    break;
                case '7':
                    isBlankSquare = true;
                    blankSquareCounter = 7;
                    break;
                case '8':
                    isBlankSquare = true;
                    blankSquareCounter = 8;
                    break;
                case 'P':
                    mailbox[8 * (7 - elem) + colCounter] = WHITE + PAWNS;
                    colCounter++;
                    break;
                case 'N':
                    mailbox[8 * (7 - elem) + colCounter] = WHITE + KNIGHTS;
                    colCounter++;
                    break;
                case 'B':
                    mailbox[8 * (7 - elem) + colCounter] = WHITE + BISHOPS;
                    colCounter++;
                    break;
                case 'R':
                    mailbox[8 * (7 - elem) + colCounter] = WHITE + ROOKS;
                    colCounter++;
                    break;
                case 'Q':
                    mailbox[8 * (7 - elem) + colCounter] = WHITE + QUEENS;
                    colCounter++;
                    break;
                case 'K':
                    mailbox[8 * (7 - elem) + colCounter] = WHITE + KINGS;
                    colCounter++;
                    break;
                case 'p':
                    mailbox[8 * (7 - elem) + colCounter] = BLACK + PAWNS;
                    colCounter++;
                    break;
                case 'n':
                    mailbox[8 * (7 - elem) + colCounter] = BLACK + KNIGHTS;
                    colCounter++;
                    break;
                case 'b':
                    mailbox[8 * (7 - elem) + colCounter] = BLACK + BISHOPS;
                    colCounter++;
                    break;
                case 'r':
                    mailbox[8 * (7 - elem) + colCounter] = BLACK + ROOKS;
                    colCounter++;
                    break;
                case 'q':
                    mailbox[8 * (7 - elem) + colCounter] = BLACK + QUEENS;
                    colCounter++;
                    break;
                case 'k':
                    mailbox[8 * (7 - elem) + colCounter] = BLACK + KINGS;
                    colCounter++;
                    break;
            }
            
            // fill out blank squares
            if (isBlankSquare) {
                for (int i = 0; i < blankSquareCounter; i++) {
                    mailbox[8 * (7 - elem) + colCounter] = -1; // -1 is blank square
                    colCounter++;
                }
            }
        }
    }
    
    int whoCanMove = (components.at(1) == "w") ? WHITE : BLACK;
    bool whiteCanKCastle = (components.at(2).find("K") != string::npos);
    bool whiteCanQCastle = (components.at(2).find("Q") != string::npos);
    bool blackCanKCastle = (components.at(2).find("k") != string::npos);
    bool blackCanQCastle = (components.at(2).find("q") != string::npos);
    // TODO finish other parameters
    int whiteEPCaptureSq = 0;
    int blackEPCaptureSq = 0;
    int fiftyMoveCounter = stoi(components.at(4));
    int moveNumber = stoi(components.at(5));
    Board board;
    return mailbox;
}

int main() {
    string input;
    string name = "UCI Chess Engine";
    string version = "0";
    string author = "Jeffrey An and Michael An";
    string pos;
    
    const string STARTPOS = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    
    cout << name << " " << version << " by " << author << '\n';
    
    while (input != "quit") {
        getline(cin, input);
        
        if (input == "uci") {
            cout << "id name " << name << " " << version << '\n';
            cout << "id author " << author << '\n';
            // make variables for default, min, and max values for hash in MB
            cout << "option name Hash type spin default " << 16 << " min " << 1 << " max " << 1024 << '\n';
            cout << "uciok\n";
        }
        
        if (input == "isready") {
            // return "readyok" when all initialization of values is done
            // must return "readyok" even when searching
            cout << "readyok\n";
        }
        
        if (input == "ucinewgame") {
            // reset search
            cout << "ucinewgame works\n";
        }
        
        if (input.substr (0, 8) == "position") {
            if (input.find("startpos") != string::npos)
                pos = STARTPOS;
            
            // TODO: make pos = FEN string when not startpos
            
            if (input.find("moves") != string::npos) {
                // TODO: read moves
            }
        }
        
        if (input.substr (0, 2) == "go") {
            // start search
            cout << "go works\n";
        }
        
        if (input == "stop") {
            // must stop search
            cout << "stop works\n";
        }
    }
}
