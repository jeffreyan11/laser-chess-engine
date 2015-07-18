#ifndef __HASH_H__
#define __HASH_H__

#include "board.h"
#include "common.h"

const uint8_t PV_NODE = 0;
const uint8_t CUT_NODE = 1;
const uint8_t ALL_NODE = 2;

struct BoardData {
    uint64_t zobristKey;
    Move m;
    int score;
    uint16_t age;
    uint8_t depth;
    uint8_t nodeType;

    BoardData() {
        zobristKey = 0;
        m = NULL_MOVE;
        score = -MATE_SCORE;
        age = 0;
        depth = 0;
        nodeType = 0;
    }

    ~BoardData() {}
};

class HashLL {

public:
    HashLL *next;
    BoardData cargo;

    HashLL(Board &b, int depth, Move m, int score, uint8_t nodeType) {
        next = NULL;
        cargo.zobristKey = b.getZobristKey();
        cargo.m = m;
        cargo.score = score;
        cargo.age = (uint16_t) (b.getMoveNumber());
        cargo.depth = (uint8_t) (depth);
        cargo.nodeType = (uint8_t) (nodeType);
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

    void add(Board &b, int depth, Move m, int score, uint8_t nodeType);
    Move get(Board &b, int &depth, int &score, uint8_t &nodeType);
    void clean(int moveNumber);
    void clear();
};

#endif
