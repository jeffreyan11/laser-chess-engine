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

Board fenToBoard(string s) {
    vector<string> components = split(s, ' ');
    vector<string> rows = split(components.at(0), '/');
    int whoCanMove = (components.at(1) == "b") ? 1 : -1;
    bool whiteCanKCastle = (components.at(2).find("K") != string::npos);
    bool whiteCanQCastle = (components.at(2).find("Q") != string::npos);
    bool blackCanKCastle = (components.at(2).find("k") != string::npos);
    bool blackCanQCastle = (components.at(2).find("q") != string::npos);
    // TODO finish other parameters
    int whiteCanEP = 0;
    int blackCanEP = 0;
    int fiftyMoveCounter = 0;
    int moveNumber = 0;
    Board board;
    return board;
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