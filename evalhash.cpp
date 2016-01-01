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

#include "evalhash.h"

EvalHash::EvalHash(uint64_t MB) {
    // Convert to bytes
    uint64_t arrSize = MB << 20;
    // Calculate how many array slots we can use
    arrSize /= sizeof(EvalHashEntry);
    // Create the array
    table = new EvalHashEntry[arrSize];
    size = arrSize;
    keys = 0;
}

EvalHash::~EvalHash() {
    delete[] table;
}

// Adds key and move into the hashtable. This function assumes that the key has
// been checked with get and is not in the table.
void EvalHash::add(Board &b, int score) {
    // Use the lower 32 bits of the hash key to index the array
    uint32_t h = (uint32_t) (b.getZobristKey() & 0xFFFFFFFF);
    uint32_t index = h % size;

    table[index].setEntry(b, score);
}

// Get the hash entry, if any, associated with a board b.
int EvalHash::get(Board &b) {
    // Use the lower 32 bits of the hash key to index the array
    uint32_t h = (uint32_t) (b.getZobristKey() & 0xFFFFFFFF);
    uint32_t index = h % size;

    if((table[index].zobristKey ^ table[index].score) == (uint32_t) (b.getZobristKey() >> 32))
        return table[index].score;

    // Because of the offset, 0 will not be a valid returned score. Thus we can use
    // this to indicate no match found.
    return 0;
}

void EvalHash::setSize(uint64_t MB) {
    delete[] table;
    
    // Convert to bytes
    uint64_t arrSize = MB << 20;
    // Calculate how many array slots we can use
    arrSize /= sizeof(EvalHashEntry);
    size = arrSize;

    table = new EvalHashEntry[size];
    keys = 0;
}

void EvalHash::clear() {
    delete[] table;
    table = new EvalHashEntry[size];
    keys = 0;
}