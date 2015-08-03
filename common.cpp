#include "common.h"

// Used for bit-scan reverse
const int index64[64] = {
0,  47,  1, 56, 48, 27,  2, 60,
57, 49, 41, 37, 28, 16,  3, 61,
54, 58, 35, 52, 50, 42, 21, 44,
38, 32, 29, 23, 17, 11,  4, 62,
46, 55, 26, 59, 40, 36, 15, 53,
34, 51, 20, 43, 31, 22, 10, 45,
25, 39, 14, 33, 19, 30,  9, 24,
13, 18,  8, 12,  7,  6,  5, 63 };

// BSF and BSR algorithms from https://chessprogramming.wikispaces.com/BitScan
int bitScanForward(uint64_t bb) {
    #if USE_INLINE_ASM
        asm ("bsf %1, %0" : "=r" (bb) : "r" (bb));
        return (int) bb;
    #else
        uint64_t debruijn64 = 0x03f79d71b4cb0a89;
        int i = (int)(((bb ^ (bb-1)) * debruijn64) >> 58);
        return index64[i];
    #endif
}

int bitScanReverse(uint64_t bb) {
    #if USE_INLINE_ASM
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
    #if USE_INLINE_ASM
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
    char startRank = '1' + (getStartSq(m) >> 3);
	char endFile = 'a' + (getEndSq(m) & 7);
	char endRank = '1' + (getEndSq(m) >> 3);
    string moveStr = {startFile, startRank, endFile, endRank};
    if (getPromotion(m)) moveStr += " nbrq"[getPromotion(m)];
    return moveStr;
}
