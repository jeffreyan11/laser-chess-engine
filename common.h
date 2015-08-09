#ifndef __COMMON_H__
#define __COMMON_H__

#include <cstdint>
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
const int MAX_MOVES = 256;

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

inline Move encodeMove(int startSq, int endSq, bool isCapture) {
    Move result = 0;
    result |= isCapture;
    result <<= 10;
    result |= endSq;
    result <<= 6;
    result |= startSq;
    return result;
}

inline Move setPromotion(Move m, int promotion) {
    return m | (promotion << 12);
}

inline Move setCastle(Move m, bool isCastle) {
    return m | (isCastle << 17);
}

inline int getStartSq(Move m) {
    return (int) (m & 0x3F);
}

inline int getEndSq(Move m) {
    return (int) ((m >> 6) & 0x3F);
}

inline int getPromotion(Move m) {
    return (int) ((m >> 12) & 0xF);
}

inline bool isCapture(Move m) {
    return (m >> 16) & 1;
}

inline bool isCastle(Move m) {
    return (m >> 17) & 1;
}

std::string moveToString(Move m);

class MoveList {
public:
    Move *moves;
    unsigned int length;

    MoveList() {
        moves = new Move[MAX_MOVES];
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
        scores = new int[MAX_MOVES];
        length = 0;
    }
    ~ScoreList() { delete[] scores; }

    unsigned int size() { return length; }

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

    void clear() {
        length = 0;
    }
};

#endif
