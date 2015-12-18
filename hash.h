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

#ifndef __HASH_H__
#define __HASH_H__

#include "board.h"
#include "common.h"

const uint8_t PV_NODE = 0;
const uint8_t CUT_NODE = 1;
const uint8_t ALL_NODE = 2;
const uint8_t NO_NODE_INFO = 3;

uint64_t packHashData(int depth, Move m, int score, uint8_t nodeType, uint8_t age);

inline int getHashDepth(uint64_t data) {
    return (int8_t) ((data >> 48) & 0xFF);
}

inline Move getHashMove(uint64_t data) {
    return (data >> 16) & 0xFFFF;
}

inline int getHashScore(uint64_t data) {
    return (int16_t) (data & 0xFFFF);
}

inline uint8_t getHashAge(uint64_t data) {
    return (data >> 40) & 0xFF;
}

inline uint8_t getHashNodeType(uint64_t data) {
    return (data >> 32) & 0x3;
}

/*
 * @brief Struct storing hashed search information
 * Size: 12 bytes
 */
struct HashEntry {
    uint64_t zobristKey;
    uint64_t data;

    HashEntry() {
        clearEntry();
    }

    void setEntry(Board &b, uint64_t _data) {
        zobristKey = b.getZobristKey() ^ _data;
        data = _data;
    }

    void clearEntry() {
        zobristKey = 0;
        data = 0;
    }

    ~HashEntry() {}
};

// This contains each of the hash table entries, in a two-bucket system.
class HashNode {
public:
    HashEntry slot1;
    HashEntry slot2;

    HashNode() {}

    /*HashNode(Board &b, int depth, Move m, int score, uint8_t nodeType, uint8_t age) {
        slot1.setEntry(b, depth, m, score, nodeType, age);
    }*/

    ~HashNode() {}
};

class Hash {
private:
    HashNode *table;
    uint64_t size;

    // Prevent direct copying and assignment
    Hash(const Hash &other);
    Hash& operator=(const Hash &other);

public:
    uint64_t keys;

    Hash(uint64_t MB);
    ~Hash();

    void add(Board &b, uint64_t data, int depth, uint8_t age);
    void addPV(Board &b, uint64_t data, int depth, uint8_t age);
    uint64_t get(Board &b);
    uint64_t getSize();
    void setSize(uint64_t MB);
    void clear();
};

#endif
