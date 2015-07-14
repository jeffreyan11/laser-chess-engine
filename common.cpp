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

string moveToString(Move m) {
    char startFile = 'a' + (getStartSq(m) & 7);
    string startRank = to_string((getStartSq(m) >> 3) + 1);
	char endFile = 'a' + (getEndSq(m) & 7);
	string endRank = to_string((getEndSq(m) >> 3) + 1);
    string moveStr = startFile + startRank + endFile + endRank;
    
    int promotion = getPromotion(m);
    if (promotion) {
        if (promotion == 2)
            moveStr += 'n';
        if (promotion == 5)
            moveStr += 'b';
        if (promotion == 6)
            moveStr += 'r';
        if (promotion == 9)
            moveStr += 'q';
    }
    
    return moveStr;
}
