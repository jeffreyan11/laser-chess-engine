#ifndef __HASH_H__
#define __HASH_H__

#include "board.h"
#include "common.h"

#define HASH_DEBUG_OUTPUT false

const uint8_t PV_NODE = 1;
const uint8_t CUT_NODE = 2;
const uint8_t ALL_NODE = 4;
const uint8_t NO_NODE_INFO = 8;

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
        score = 0;
        age = 0;
        depth = 0;
        nodeType = NO_NODE_INFO;
    }

    HashEntry(Board &b, int _depth, Move _m, int _score, uint8_t _nodeType) {
        zobristKey = b.getZobristKey();
        m = _m;
        score = _score;
        age = b.getMoveNumber();
        depth = (uint8_t) (_depth);
        nodeType = (uint8_t) (_nodeType);
    }

    ~HashEntry() {}
};

// This contains each of the hash table entries, in a two-bucket system.
class HashNode {
public:
    HashEntry *slot1;
    HashEntry *slot2;

    HashNode() {
        slot1 = NULL;
        slot2 = NULL;
    }

    HashNode(Board &b, int depth, Move m, int score, uint8_t nodeType) {
        slot1 = new HashEntry(b, depth, m, score, nodeType);
        slot2 = NULL;
    }

    ~HashNode() {
        if (slot1 != NULL)
            delete slot1;
        if (slot2 != NULL)
            delete slot2;
    }
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
    void clean(uint16_t moveNumber);
    void clear();
};

#endif
