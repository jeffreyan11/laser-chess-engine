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

#include <assert.h>

#include "common.h"

// Move promotion flags
const int PROMO[16] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 1, 2, 3, 4};

int bitScanForward(uint64_t bb) {
    assert(bb);  // lsb(0) is undefined
    return __builtin_ctzll(bb);
}

int bitScanReverse(uint64_t bb) {
    assert(bb);  // msb(0) is undefined
    return __builtin_clzll(bb) ^ 63;
}

int count(uint64_t bb) {
    return __builtin_popcountll(bb);
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
