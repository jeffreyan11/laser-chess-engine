/*
    Laser, a UCI chess engine written in C++11.
    Copyright 2015-2018 Jeffrey An and Michael An

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

#include "common.h"

// Move promotion flags
const int PROMO[16] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 1, 2, 3, 4};

// Used for bit-scan reverse
constexpr int index64[64] = {
0,  47,  1, 56, 48, 27,  2, 60,
57, 49, 41, 37, 28, 16,  3, 61,
54, 58, 35, 52, 50, 42, 21, 44,
38, 32, 29, 23, 17, 11,  4, 62,
46, 55, 26, 59, 40, 36, 15, 53,
34, 51, 20, 43, 31, 22, 10, 45,
25, 39, 14, 33, 19, 30,  9, 24,
13, 18,  8, 12,  7,  6,  5, 63 };

// BSF and BSR algorithms from https://chessprogramming.wikispaces.com/BitScan
// GCC compiler intrinsics seem to be slower in some cases, even the compiled
// with optimization / intrinsic flags...
int bitScanForward(uint64_t bb) {
    #if USE_INLINE_ASM
        asm ("bsfq %1, %0" : "=r" (bb) : "r" (bb));
        return (int) bb;
    #else
        uint64_t debruijn64 = 0x03f79d71b4cb0a89;
        int i = (int)(((bb ^ (bb-1)) * debruijn64) >> 58);
        return index64[i];
    #endif
}

int bitScanReverse(uint64_t bb) {
    #if USE_INLINE_ASM
        asm ("bsrq %1, %0" : "=r" (bb) : "r" (bb));
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
        asm ("popcntq %1, %0" : "=r" (bb) : "r" (bb));
        return (int) bb;
    #else
        bb = bb - ((bb >> 1) & 0x5555555555555555);
        bb = (bb & 0x3333333333333333) + ((bb >> 2) & 0x3333333333333333);
        bb = (((bb + (bb >> 4)) & 0x0F0F0F0F0F0F0F0F) * 0x0101010101010101) >> 56;
        return (int) bb;
    #endif
}

// Flips a board across the middle ranks, the way perspective is
// flipped from white to black and vice versa
// Parallel-prefix algorithm from
// http://chessprogramming.wikispaces.com/Flipping+Mirroring+and+Rotating
uint64_t flipAcrossRanks(uint64_t bb) {
    #if USE_INLINE_ASM
        asm ("bswapq %0" : "=r" (bb) : "0" (bb));
        return bb;
    #else
        bb = ((bb >>  8) & 0x00FF00FF00FF00FF) | ((bb & 0x00FF00FF00FF00FF) <<  8);
        bb = ((bb >> 16) & 0x0000FFFF0000FFFF) | ((bb & 0x0000FFFF0000FFFF) << 16);
        bb =  (bb >> 32) | (bb << 32);
        return bb;
    #endif
}

// Given a start time_point, returns the seconds elapsed using C++11's
// std::chrono::high_resolution_clock
uint64_t getTimeElapsed(ChessTime startTime) {
    auto endTime = ChessClock::now();
    std::chrono::milliseconds timeSpan =
        std::chrono::duration_cast<std::chrono::milliseconds>(
        endTime-startTime);
    return (uint64_t) timeSpan.count() + 1;
}

std::string moveToString(Move m) {
    char startFile = 'a' + (getStartSq(m) & 7);
    char startRank = '1' + (getStartSq(m) >> 3);
	char endFile = 'a' + (getEndSq(m) & 7);
	char endRank = '1' + (getEndSq(m) >> 3);
    std::string moveStr = {startFile, startRank, endFile, endRank};
    if (getPromotion(m)) moveStr += " nbrq"[getPromotion(m)];
    return moveStr;
}
