/*
    Laser, a UCI chess engine written in C++11.
    Copyright 2015-2016 Jeffrey An and Michael An

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

#ifndef __EVALHASH_H__
#define __EVALHASH_H__

#include "board.h"
#include "common.h"


// Offset so that we can always store a positive value in EvalHashEntry
const int EVAL_HASH_OFFSET = (1 << 20);

/*
 * @brief Struct storing hashed eval information
 * Size: 8 bytes
 */
struct EvalHashEntry {
    uint32_t zobristKey;
    uint32_t score;

    EvalHashEntry() {
        clearEntry();
    }

    void setEntry(Board &b, int _score) {
        score = (uint32_t) (_score + EVAL_HASH_OFFSET);
        zobristKey = ((uint32_t) (b.getZobristKey() >> 32)) ^ score;
    }

    void clearEntry() {
        zobristKey = 0;
        score = 0;
    }

    ~EvalHashEntry() {}
};

class EvalHash {
private:
    EvalHashEntry *table;
    uint64_t size;

    void init(uint64_t MB);

public:
    uint64_t keys;

    EvalHash(uint64_t MB);
    EvalHash(const EvalHash &other) = delete;
    EvalHash& operator=(const EvalHash &other) = delete;
    ~EvalHash();

    void add(Board &b, int score);
    int get(Board &b);
    void setSize(uint64_t MB);
    void clear();
};

#endif
