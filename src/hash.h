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

#include "board.h"
#include "common.h"

constexpr uint8_t PV_NODE = 0;
constexpr uint8_t CUT_NODE = 1;
constexpr uint8_t ALL_NODE = 2;
constexpr uint8_t NO_NODE_INFO = 3;


// Struct storing hashed search information and corresponding hash key.
// Size: 16 bytes
struct HashEntry {
    uint64_t zobristKey;
    int16_t score;
    Move move;
    int16_t eval;
    int8_t depth;
    uint8_t ageNodeType;

    HashEntry() = default;
    ~HashEntry() = default;

    void setEntry(Board &b, int _score, Move _move, int _eval, int _depth, uint8_t _nodeType, uint8_t _age) {
        zobristKey = b.getZobristKey();
        score = (int16_t) _score;
        move = _move;
        eval = (int16_t) _eval;
        depth = (int8_t) _depth;
        ageNodeType = (_age << 2) | _nodeType;
    }
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

    void add(Board &b, int score, Move move, int eval, int depth, uint8_t nodeType);
    HashEntry *get(Board &b);

    uint64_t getSize() const;
    void setSize(uint64_t MB);

    void incrementAge();

    void clear();
    int estimateHashfull() const;
};

#endif
