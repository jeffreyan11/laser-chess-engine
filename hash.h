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
    uint32_t zobristKey;
    Move m;
    int16_t score;
    uint8_t age;
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

    void setEntry(Board &b, int _depth, Move _m, int _score, uint8_t _nodeType, uint8_t searchGen) {
        zobristKey = (uint32_t) (b.getZobristKey() >> 32);
        m = _m;
        score = (int16_t) _score;
        age = searchGen;
        depth = (uint8_t) (_depth);
        nodeType = (uint8_t) (_nodeType);
    }

    void clearEntry() {
        zobristKey = 0;
        m = NULL_MOVE;
        score = 0;
        age = 0;
        depth = 0;
        nodeType = NO_NODE_INFO;
    }

    ~HashEntry() {}
};

// This contains each of the hash table entries, in a two-bucket system.
class HashNode {
public:
    HashEntry slot1;
    HashEntry slot2;

    HashNode() {}

    HashNode(Board &b, int depth, Move m, int score, uint8_t nodeType, uint8_t searchGen) {
        slot1.setEntry(b, depth, m, score, nodeType, searchGen);
    }

    ~HashNode() {}
};

class Hash {

private:
    HashNode **table;
    uint64_t size;

public:
    int keys;
    #if HASH_DEBUG_OUTPUT
    int replacements;
    int collisions;
    #endif
    void test();

    Hash(uint64_t MB);
    ~Hash();

    void add(Board &b, int depth, Move m, int score, uint8_t nodeType, uint8_t searchGen);
    HashEntry *get(Board &b);
    //void clean(uint16_t moveNumber);
    void clear();
};

#endif
