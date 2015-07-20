#ifndef __HASH_H__
#define __HASH_H__

#include "board.h"
#include "common.h"

#define HASH_DEBUG_OUTPUT false

const uint8_t PV_NODE = 0;
const uint8_t CUT_NODE = 1;
const uint8_t ALL_NODE = 2;
const uint8_t NO_NODE_INFO = 3;

struct HashEntry {
    uint64_t zobristKey;
    Move m;
    int score;
    uint16_t age;
    uint8_t depth;
    uint8_t nodeType;

    HashEntry() {
        zobristKey = 0;
        m = NULL_MOVE;
        score = -MATE_SCORE;
        age = 0;
        depth = 0;
        nodeType = 0;
    }

    ~HashEntry() {}
};

// This contains each of the hash table entries (currently 1 per array slot).
// The class is here in case a multi-bucket system is implemented in the future.
class HashNode {
public:
    HashEntry cargo;

    HashNode(Board &b, int depth, Move m, int score, uint8_t nodeType) {
        cargo.zobristKey = b.getZobristKey();
        cargo.m = m;
        cargo.score = score;
        cargo.age = b.getMoveNumber();
        cargo.depth = (uint8_t) (depth);
        cargo.nodeType = (uint8_t) (nodeType);
    }

    ~HashNode() {}
};

class Hash {

private:
    HashNode **table;
    uint64_t size;

    uint64_t hash(Board &b);

public:
    int keys;
    #if HASH_DEBUG_OUTPUT
    int replacements;
    int collisions;
    #endif
    void test();

    Hash(uint64_t MB);
    ~Hash();

    void add(Board &b, int depth, Move m, int score, uint8_t nodeType);
    HashEntry *get(Board &b);
    void clean(int moveNumber);
    void clear();
};

#endif
