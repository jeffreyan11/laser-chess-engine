/*
    Laser, a UCI chess engine written in C++11.
    Copyright 2015-2018 Jeffrey An and Michael An

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

#include <cstring>
#include "board.h"
#include "common.h"

const uint8_t PV_NODE = 0;
const uint8_t CUT_NODE = 1;
const uint8_t ALL_NODE = 2;
const uint8_t NO_NODE_INFO = 3;


// Size: 8 bytes
struct HashData {
    Move m;
    int16_t score;
    // TODO use eval instead of a separate eval cache
    int16_t eval;
    uint8_t ageNT;
    int8_t depth;

    HashData() = default;
    HashData(Move _m, int _score, uint8_t _nodeType, uint8_t _age, int _depth) {
        m = _m;
        score = _score;
        ageNT = _age | _nodeType;
        depth = _depth;
    }

    // Accessors
    Move getMove() const { return m; }
    int getScore() const { return score; }
    int getEval() const { return eval; }
    uint8_t getAge() const { return ageNT & 0xFC; }
    uint8_t getNodeType() const { return ageNT & 0x3; }
    int getDepth() const { return depth; }
};

// Struct storing hashed search information.
// Size: 16 bytes
struct HashEntry {
    uint64_t zobristKey;
    HashData data;

    HashEntry() {
        clearEntry();
    }

    void setEntry(Board &b, HashData _data) {
        zobristKey = b.getZobristKey();
        data = _data;
    }

    void clearEntry() {
        zobristKey = 0;
        std::memset(&data, 0, sizeof(HashData));
    }

    ~HashEntry() {}
};

// This contains each of the hash table entries, in a two-bucket system.
class HashNode {
public:
    HashEntry slot1;
    HashEntry slot2;

    HashNode() {}
    ~HashNode() {}
};

class Hash {
private:
    HashNode *table;
    uint64_t size;
    uint8_t age;

    void init(uint64_t MB);

public:
    Hash(uint64_t MB);
    Hash(const Hash &other) = delete;
    Hash& operator=(const Hash &other) = delete;
    ~Hash();

    void add(Board &b, HashData data, int depth);
    HashData get(Board &b, bool &isHit);
    uint64_t getSize();
    void setSize(uint64_t MB);
    void clear();
    int estimateHashfull();
    uint8_t getAge() { return age; }
    void nextSearch() { age += 4; }
};

#endif
