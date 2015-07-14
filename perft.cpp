#include <chrono>
#include <sstream>
#include <vector>
#include "board.h"
#include "common.h"

uint64_t perft(Board &b, int color, int depth);
Board fenToBoard(string s);
void split(const string &s, char d, vector<string> &v);
vector<string> split(const string &s, char d);

int captures = 0;

int main(int argc, char **argv) {
    int depth = 5;
    if (argc == 2)
        depth = atoi(argv[1]);

    Board b = fenToBoard("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1");
    using namespace std::chrono;
    auto start_time = high_resolution_clock::now();

    cout << "Nodes: " << perft(b, 1, depth) << endl;
    cout << "Captures: " << captures << endl;

    auto end_time = high_resolution_clock::now();
    duration<double> time_span = duration_cast<duration<double>>(
        end_time-start_time);

    cerr << time_span.count() << endl;

    return 0;
}

/*
 * Performs a PERFT.
 * 7/8/15: PERFT 5, 1.46 s (i5-2450m)
 * 7/11/15: PERFT 5, 1.22 s (i5-2450m)
 * 7/13/15: PERFT 5, 1.27/1.08 s (i5-2450m) before/after pass Board by reference
 */
uint64_t perft(Board &b, int color, int depth) {
	if(depth == 0)
		return 1;
	
	MoveList pl = b.getPseudoLegalMoves(color);
	MoveList pc = b.getPLCaptures(color);
	uint64_t nodes = 0;
	
	for(unsigned int i = 0; i < pl.size(); i++) {
		Board copy = b.staticCopy();
		if(!copy.doPLMove(pl.get(i), color))
			continue;
        cerr << "move " << depth << ", " << moveToString(pl.get(i)) << endl;
		/*if(!b->doPLMove(pl.get(i), color)) {
			b->undoMove();
			continue;
		}*/
		
		nodes += perft(copy, -color, depth-1);
		
		//b.undoMove();
	}
	
	for(unsigned int i = 0; i < pc.size(); i++) {
		Board copy = b.staticCopy();
		if(!copy.doPLMove(pc.get(i), color))
			continue;
        cerr << "capture " << depth << ", " << moveToString(pc.get(i)) << endl;
		
		captures++;
		
		nodes += perft(copy, -color, depth-1);
	}
	
	return nodes;
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
                    mailbox[sqCounter] = WHITE + PAWNS;
                    break;
                case 'N':
                    mailbox[sqCounter] = WHITE + KNIGHTS;
                    break;
                case 'B':
                    mailbox[sqCounter] = WHITE + BISHOPS;
                    break;
                case 'R':
                    mailbox[sqCounter] = WHITE + ROOKS;
                    break;
                case 'Q':
                    mailbox[sqCounter] = WHITE + QUEENS;
                    break;
                case 'K':
                    mailbox[sqCounter] = WHITE + KINGS;
                    break;
                case 'p':
                    mailbox[sqCounter] = BLACK + PAWNS;
                    break;
                case 'n':
                    mailbox[sqCounter] = BLACK + KNIGHTS;
                    break;
                case 'b':
                    mailbox[sqCounter] = BLACK + BISHOPS;
                    break;
                case 'r':
                    mailbox[sqCounter] = BLACK + ROOKS;
                    break;
                case 'q':
                    mailbox[sqCounter] = BLACK + QUEENS;
                    break;
                case 'k':
                    mailbox[sqCounter] = BLACK + KINGS;
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
    
    uint64_t whiteEPCaptureSq = 0, blackEPCaptureSq = 0;
    if (components.at(3).find("6") != string::npos)
        whiteEPCaptureSq = MOVEMASK[8 * (5 - 1) + (components.at(3).at(0) - 'a')];
    if (components.at(3).find("3") != string::npos)
        blackEPCaptureSq = MOVEMASK[8 * (4 - 1) + (components.at(3).at(0) - 'a')];
    int fiftyMoveCounter = stoi(components.at(4));
    int moveNumber = stoi(components.at(5));
    return Board(mailbox, whiteCanKCastle, blackCanKCastle, whiteCanQCastle,
            blackCanQCastle, whiteEPCaptureSq, blackEPCaptureSq, fiftyMoveCounter,
            moveNumber, playerToMove);
}

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
