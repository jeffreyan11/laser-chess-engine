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
const int MATE_SCORE = (1 << 15) - 1;
const int INFTY = (1 << 15) - 1;

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

inline Move encodeMove(int startSq, int endSq) {
    return (endSq << 6) | startSq;
}

inline Move setPromotion(Move m, int promotion) {
    return m | ((promotion + 7) << 12);
}

inline Move setCapture(Move m, bool isCapture) {
    return m | (isCapture << 14);
}

inline Move setCastle(Move m, bool isCastle) {
    return m | (isCastle << 13);
}

inline int getStartSq(Move m) {
    return (int) (m & 0x3F);
}

inline int getEndSq(Move m) {
    return (int) ((m >> 6) & 0x3F);
}

inline int getPromotion(Move m) {
    return (int) ((m >> 15) * (((m >> 12) & 0x3) + 1));
}

inline bool isCapture(Move m) {
    return (bool) ((m >> 14) & 1);
}

inline bool isCastle(Move m) {
    return ((m >> 13) == 1);
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