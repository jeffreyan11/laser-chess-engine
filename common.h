#ifndef __COMMON_H__
#define __COMMON_H__

#include <cstdint>
#include <iostream>
#include <string>

using namespace std;

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
const int KNIGHT_VALUE = 340;
const int BISHOP_VALUE = 350;
const int ROOK_VALUE = 520;
const int QUEEN_VALUE = 1000;
const int MATE_SCORE = (1 << 15) - 1;
const int INFTY = (1 << 15) - 1;

const int BISHOP_PAIR_VALUE = 40;
const int TEMPO_VALUE = 10;

const int START_VALUE = 8 * PAWN_VALUE + 2 * KNIGHT_VALUE + 2 * BISHOP_VALUE + 2 * ROOK_VALUE + QUEEN_VALUE;
const int EG_FACTOR_RES = 1000;

// Other values
const int MAX_DEPTH = 127;

// Converts square number to 64-bit integer
const uint64_t MOVEMASK[64] = {
0x0000000000000001, 0x0000000000000002, 0x0000000000000004, 0x0000000000000008,
0x0000000000000010, 0x0000000000000020, 0x0000000000000040, 0x0000000000000080,
0x0000000000000100, 0x0000000000000200, 0x0000000000000400, 0x0000000000000800,
0x0000000000001000, 0x0000000000002000, 0x0000000000004000, 0x0000000000008000,
0x0000000000010000, 0x0000000000020000, 0x0000000000040000, 0x0000000000080000,
0x0000000000100000, 0x0000000000200000, 0x0000000000400000, 0x0000000000800000,
0x0000000001000000, 0x0000000002000000, 0x0000000004000000, 0x0000000008000000,
0x0000000010000000, 0x0000000020000000, 0x0000000040000000, 0x0000000080000000,
0x0000000100000000, 0x0000000200000000, 0x0000000400000000, 0x0000000800000000,
0x0000001000000000, 0x0000002000000000, 0x0000004000000000, 0x0000008000000000,
0x0000010000000000, 0x0000020000000000, 0x0000040000000000, 0x0000080000000000,
0x0000100000000000, 0x0000200000000000, 0x0000400000000000, 0x0000800000000000,
0x0001000000000000, 0x0002000000000000, 0x0004000000000000, 0x0008000000000000,
0x0010000000000000, 0x0020000000000000, 0x0040000000000000, 0x0080000000000000,
0x0100000000000000, 0x0200000000000000, 0x0400000000000000, 0x0800000000000000,
0x1000000000000000, 0x2000000000000000, 0x4000000000000000, 0x8000000000000000
};

int bitScanForward(uint64_t bb);
int bitScanReverse(uint64_t bb);
int count(uint64_t bb);

/*
 * Moves are represented as an unsigned 32-bit integer.
 * Bits 0-5: start square
 * Bits 6-11: end square
 * Bits 12-15: piece
 * Bits 16-19: promotion, 0 is no promotion
 * Bit 20: isCapture
 * Bit 21: isCastle
 * Bits 22-31: junk or 0s
 */
typedef uint32_t Move;

const Move NULL_MOVE = 0;

inline Move encodeMove(int startSq, int endSq, int piece, bool isCapture) {
    Move result = 0;
    result |= isCapture;
    result <<= 8;
    result |= piece;
    result <<= 6;
    result |= endSq;
    result <<= 6;
    result |= startSq;
    return result;
}

inline Move setPromotion(Move m, int promotion) {
    return m | (promotion << 16);
}

inline Move setCastle(Move m, bool isCastle) {
    return m | (isCastle << 21);
}

inline int getStartSq(Move m) {
    return (int) (m & 0x3F);
}

inline int getEndSq(Move m) {
    return (int) ((m >> 6) & 0x3F);
}

inline int getPiece(Move m) {
    return (int) ((m >> 12) & 0xF);
}

inline int getPromotion(Move m) {
    return (int) ((m >> 16) & 0xF);
}

inline bool isCapture(Move m) {
    return (m >> 20) & 1;
}

inline bool isCastle(Move m) {
    return (m >> 21) & 1;
}

string moveToString(Move m);

class MoveList {
public:
    Move *moves;
    unsigned int length;

    MoveList() {
        moves = new Move[128];
        length = 0;
    }
    ~MoveList() { delete[] moves; }

    unsigned int size() { return length; }

    // Adds to the end of the list.
    void add(Move m) {
        moves[length] = m;
        length++;
    }

    Move get(int i) { return moves[i]; }

    Move last() { return moves[length-1]; }

    void set(int i, Move m) { moves[i] = m; }

    Move remove(int i) {
        Move deleted = moves[i];
        for(unsigned int j = i; j < length-1; j++) {
            moves[j] = moves[j+1];
        }
        length--;
        return deleted;
    }

    void swap(int i, int j) {
        Move temp = moves[i];
        moves[i] = moves[j];
        moves[j] = temp;
    }

    // Reset the MoveList
    void clear() {
        length = 0;
    }
};

class ScoreList {
public:
    int *scores;
    unsigned int length;

    ScoreList() {
        scores = new int[128];
        length = 0;
    }
    ~ScoreList() { delete[] scores; }

    unsigned int size() { return length; }

    // Adds to the end of the list.
    void add(int s) {
        scores[length] = s;
        length++;
    }

    int get(int i) { return scores[i]; }

    void set(int i, int s) { scores[i] = s; }

    void swap(int i, int j) {
        int temp = scores[i];
        scores[i] = scores[j];
        scores[j] = temp;
    }

    // Reset the MoveList
    void clear() {
        length = 0;
    }
};

#endif
