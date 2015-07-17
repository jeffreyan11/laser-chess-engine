#ifndef __HASH_H__
#define __HASH_H__

#include "board.h"
#include "common.h"

struct BoardData {
    uint64_t zobristKey;
    Move m;
    int score;
    uint16_t age;
    uint8_t depth;

    BoardData() {
        zobristKey = 0;
        m = NULL_MOVE;
        score = -MATE_SCORE;
        age = 0;
        depth = 0;
    }

    ~BoardData() {}
};

class HashLL {

public:
    HashLL *next;
    BoardData cargo;

    HashLL(Board &b, int depth, Move m, int score) {
        next = NULL;
        cargo.zobristKey = b.getZobristKey();
        cargo.m = m;
        cargo.score = score;
        cargo.age = (uint16_t) (b.getMoveNumber());
        cargo.depth = (uint8_t) (depth);
    }

    ~HashLL() {}
};

class Hash {

private:
    HashLL **table;
    uint64_t size;

    uint64_t hash(Board &b);

public:
    int keys;
    void test();

    Hash(uint64_t MB);
    ~Hash();

    void add(Board &b, int depth, Move m, int score);
    Move get(Board &b, int &depth, int &score);
    void clean(int moveNumber);
    void clear();
};

#endif
