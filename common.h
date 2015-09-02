/*
    Laser, a UCI chess engine written in C++11.
    Copyright 2015 Jeffrey An and Michael An

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

#ifndef __COMMON_H__
#define __COMMON_H__

#include <cstdint>
#include <chrono>
#include <string>

#define USE_INLINE_ASM true

const int WHITE = 0;
const int BLACK = 1;
const int PAWNS = 0;
const int KNIGHTS = 1;
const int BISHOPS = 2;
const int ROOKS = 3;
const int QUEENS = 4;
const int KINGS = 5;

// Material constants
const int PAWN_VALUE = 100, PAWN_VALUE_EG = 125;
const int KNIGHT_VALUE = 420;
const int BISHOP_VALUE = 430;
const int ROOK_VALUE = 660;
const int QUEEN_VALUE = 1250;
const int MATE_SCORE = 32766;
const int INFTY = 32767;

// Other values
const int MAX_DEPTH = 127;
const int MAX_MOVES = 256;

// Stuff for timing
typedef std::chrono::high_resolution_clock ChessClock;
typedef std::chrono::high_resolution_clock::time_point ChessTime;

double getTimeElapsed(ChessTime startTime);

// Bitboard methods
int bitScanForward(uint64_t bb);
int bitScanReverse(uint64_t bb);
int count(uint64_t bb);
uint64_t flipAcrossRanks(uint64_t bb);

/*
 * Moves are represented as an unsigned 16-bit integer.
 * Bits 0-5: start square
 * Bits 6-11: end square
 * Bits 12-15: flags for special move types
 *   Bit 14: capture
 *   Bit 15: promotion
 * Based on the butterfly index from http://chessprogramming.wikispaces.com/Encoding+Moves
 */
typedef uint16_t Move;

const Move NULL_MOVE = 0;
const uint16_t MOVE_DOUBLE_PAWN = 0x1;
const uint16_t MOVE_EP = 0x5;
const uint16_t MOVE_PROMO_N = 0x8;
const uint16_t MOVE_PROMO_B = 0x9;
const uint16_t MOVE_PROMO_R = 0xA;
const uint16_t MOVE_PROMO_Q = 0xB;
const int PROMO[16] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 1, 2, 3, 4};

inline Move encodeMove(int startSq, int endSq) {
    return (endSq << 6) | startSq;
}

inline Move setCapture(Move m, bool isCapture) {
    return m | (isCapture << 14);
}

inline Move setCastle(Move m, bool isCastle) {
    return m | (isCastle << 13);
}

inline Move setFlags(Move m, uint16_t f) {
    return m | (f << 12);
}

inline int getStartSq(Move m) {
    return (int) (m & 0x3F);
}

inline int getEndSq(Move m) {
    return (int) ((m >> 6) & 0x3F);
}

inline int getPromotion(Move m) {
    return PROMO[m >> 12];
}

inline bool isPromotion(Move m) {
    return (bool) (m >> 15);
}

inline bool isCapture(Move m) {
    return (bool) ((m >> 14) & 1);
}

inline bool isCastle(Move m) {
    return ((m >> 13) == 1);
}

inline bool isEP(Move m) {
    return ((m >> 12) == MOVE_EP);
}

inline uint16_t getFlags(Move m) {
    return (m >> 12);
}

std::string moveToString(Move m);

/**
 * @brief A simple implementation of an ArrayList, used for storing
 * lists of moves and search parameters. We have the luxury of using
 * an array because the number of legal moves in any given position
 * has a hard limit of about 218 (we use an array of 256).
 */
template <class T> class SearchArrayList {
public:
    T arrayList[MAX_MOVES];
    unsigned int length;

    SearchArrayList() {
        length = 0;
    }
    ~SearchArrayList() {}

    unsigned int size() { return length; }

    void add(T o) {
        arrayList[length] = o;
        length++;
    }

    T get(int i) { return arrayList[i]; }

    void set(int i, T o) { arrayList[i] = o; }

    T remove(int i) {
        T deleted = arrayList[i];
        for(unsigned int j = i; j < length-1; j++) {
            arrayList[j] = arrayList[j+1];
        }
        length--;
        return deleted;
    }

    void swap(int i, int j) {
        T temp = arrayList[i];
        arrayList[i] = arrayList[j];
        arrayList[j] = temp;
    }

    void clear() {
        length = 0;
    }
};

typedef SearchArrayList<Move> MoveList;
typedef SearchArrayList<int> ScoreList;

#endif