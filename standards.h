#ifndef __STANDARDS_H__
#define __STANDARDS_H__

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

// These values uniquely determine indices for every piece type from 0 to 11
// For example, white pawns get the index 1 + 1 = 2, and black rooks get the
// index -1 + 6 = 5
const int WHITE = 1;
const int BLACK = -1;
const int PAWNS = 1;
const int KNIGHTS = 2;
const int BISHOPS = 5;
const int ROOKS = 6;
const int QUEENS = 9;
const int KINGS = 10;

class Move {

public:
	int piece;
	bool isCapture;
	int startsq;
	int endsq;
	bool isCastle;
	int promotion;
	
	Move(int _piece, bool _isCapture, int _startsq, int _endsq) {
		piece = _piece;
		isCapture = _isCapture;
		startsq = _startsq;
		endsq = _endsq;
		isCastle = false;
		promotion = -1;
	}

    ~Move() {}
	
	string toString() {
    	string pieceName = "";
    	string row = "";
    	switch(piece) {
        	case PAWNS:
        		break;
        	case KNIGHTS:
        		pieceName = "N";
        		break;
        	case BISHOPS:
        		pieceName = "B";
        		break;
        	case ROOKS:
        		pieceName = "R";
        		break;
        	case QUEENS:
        		pieceName = "Q";
        		break;
        	case KINGS:
        		pieceName = "K";
        		break;
		    default:
			    cout << "Error. Invalid piece code in printMove()." << endl;
			    break;
    	}
    	
    	if(isCapture) {
    		if(piece != PAWNS)
    			pieceName += "x";
    		else {
    			int ix = startsq % 8;
    			if(ix == 0)
    	    		pieceName = "a";
    	    	if(ix == 1)
    	    		pieceName = "b";
    	    	if(ix == 2)
    	    		pieceName = "c";
    	    	if(ix == 3)
    	    		pieceName = "d";
    	    	if(ix == 4)
    	    		pieceName = "e";
    	    	if(ix == 5)
    	    		pieceName = "f";
    	    	if(ix == 6)
    	    		pieceName = "g";
    	    	if(ix == 7)
    	    		pieceName = "h";
    	    	
    	    	pieceName += "x";
    		}
    	}
    	
    	int x = endsq % 8;
    	int y = endsq / 8;
    	if(x == 0)
    		row = "a";
    	if(x == 1)
    		row = "b";
    	if(x == 2)
    		row = "c";
    	if(x == 3)
    		row = "d";
    	if(x == 4)
    		row = "e";
    	if(x == 5)
    		row = "f";
    	if(x == 6)
    		row = "g";
    	if(x == 7)
    		row = "h";

    	string resultStr = pieceName;
        resultStr += row;
        resultStr += to_string(y + 1);
        return resultStr;
	}
};

#endif
