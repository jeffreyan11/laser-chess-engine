#ifndef __HASH_H__
#define __HASH_H__

#include "board.h"
#include "common.h"

const uint8_t PV_NODE = 0;
const uint8_t CUT_NODE = 1;
const uint8_t ALL_NODE = 2;
const uint8_t NO_NODE_INFO = 3;

/*
 * @brief Struct storing hashed search information
 * Size: 12 bytes
 */
struct HashEntry {
    uint32_t zobristKey;
    Move m;
    int16_t score;
    uint8_t ageNT;
    int8_t depth;

    HashEntry() {
        clearEntry();
    }

    void setEntry(Board &b, int _depth, Move _m, int _score, uint8_t nodeType, uint8_t age) {
        zobristKey = (uint32_t) (b.getZobristKey() >> 32);
        m = _m;
        score = (int16_t) _score;
        ageNT = (age << 2) | nodeType;
        depth = (int8_t) (_depth);
    }

    void clearEntry() {
        zobristKey = 0;
        m = NULL_MOVE;
        score = 0;
        ageNT = NO_NODE_INFO;
        depth = 0;
    }

    uint8_t getAge() {
        return ageNT >> 2;
    }

    uint8_t getNodeType() {
        return ageNT & 0x3;
    }

    ~HashEntry() {}
};

// This contains each of the hash table entries, in a two-bucket system.
class HashNode {
public:
    HashEntry slot1;
    HashEntry slot2;

    HashNode() {}

    HashNode(Board &b, int depth, Move m, int score, uint8_t nodeType, uint8_t age) {
        slot1.setEntry(b, depth, m, score, nodeType, age);
    }

    ~HashNode() {}
};

class Hash {
private:
    HashNode **table;
    uint64_t size;

public:
    int keys;

    Hash(uint64_t MB);
    ~Hash();

    void add(Board &b, int depth, Move m, int score, uint8_t nodeType, uint8_t age);
    HashEntry *get(Board &b);
    uint64_t getSize();
    void clear();
};

#endif
