#include "common.h"

// BSF and BSR algorithms from https://chessprogramming.wikispaces.com/BitScan
int bitScanForward(uint64_t bb) {
    #if USE_INLINE_AS
        asm ("bsf %1, %0" : "=r" (bb) : "r" (bb));
        return (int) bb;
    #else
        uint64_t debruijn64 = 0x03f79d71b4cb0a89;
        int i = (int)(((bb ^ (bb-1)) * debruijn64) >> 58);
        return index64[i];
    #endif
}

int bitScanReverse(uint64_t bb) {
    #if USE_INLINE_AS
        asm ("bsr %1, %0" : "=r" (bb) : "r" (bb));
        return (int) bb;
    #else
        uint64_t debruijn64 = 0x03f79d71b4cb0a89;
        bb |= bb >> 1; 
        bb |= bb >> 2;
        bb |= bb >> 4;
        bb |= bb >> 8;
        bb |= bb >> 16;
        bb |= bb >> 32;
        return index64[(int) ((bb * debruijn64) >> 58)];
    #endif
}

int count(uint64_t bb) {
    #if USE_INLINE_AS
        asm ("popcnt %1, %0" : "=r" (bb) : "r" (bb));
        return (int) bb;
    #else
        bb = bb - ((bb >> 1) & 0x5555555555555555);
        bb = (bb & 0x3333333333333333) + ((bb >> 2) & 0x3333333333333333);
        bb = (((bb + (bb >> 4)) & 0x0F0F0F0F0F0F0F0F) * 0x0101010101010101) >> 56;
        return (int) bb;
    #endif
}

Move::Move(int _piece, bool _isCapture, int _startsq, int _endsq) {
	piece = _piece;
	isCapture = _isCapture;
	startsq = _startsq;
	endsq = _endsq;
	isCastle = false;
	promotion = -1;
}

string Move::toString() {
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
