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

#include <cstring>
#include "hash.h"

Hash::Hash(uint64_t MB) {
    init(MB);
}

Hash::~Hash() {
    free(table);
}

// Adds key and move into the hashtable. This function assumes that the key has
// been checked with get and is not in the table.
void Hash::add(Board &b, int score, Move move, int eval, int depth, uint8_t nodeType) {
    uint64_t h = b.getZobristKey();
    uint64_t index = h & (size-1);
    HashNode *node = table + index;

    // Decide whether to replace the entry
    // A more recent update to the same position should always be chosen
    if (node->slot1.zobristKey == b.getZobristKey())
        node->slot1.setEntry(b, score, move, eval, depth, nodeType, age);

    else if (node->slot2.zobristKey == b.getZobristKey())
        node->slot2.setEntry(b, score, move, eval, depth, nodeType, age);

    // Replace an entry from a previous search space, or the lowest depth
    // entry with the new entry if the new entry's depth is high enough
    else {
        HashEntry *toReplace = &(node->slot1);
        int score1 = 16 * ((int) ((uint8_t) (age - (node->slot1.ageNodeType >> 2))))
            + depth - node->slot1.depth;
        int score2 = 16 * ((int) ((uint8_t) (age - (node->slot2.ageNodeType >> 2))))
            + depth - node->slot2.depth;
        if (score1 < score2)
            toReplace = &(node->slot2);
        // The node must be from a newer search space or a sufficiently high depth
        if (score1 >= -2 || score2 >= -2)
            toReplace->setEntry(b, score, move, eval, depth, nodeType, age);
    }
}

// Get the hash entry, if any, associated with a board b.
HashEntry *Hash::get(Board &b) {
    uint64_t h = b.getZobristKey();
    uint64_t index = h & (size-1);
    HashNode *node = table + index;

    if (node->slot1.zobristKey == b.getZobristKey())
        return &(node->slot1);
    else if (node->slot2.zobristKey == b.getZobristKey())
        return &(node->slot2);

    return nullptr;
}

uint64_t Hash::getSize() const {
    return (2 * size);
}

void Hash::setSize(uint64_t MB) {
    free(table);
    init(MB);
}

void Hash::init(uint64_t MB) {
    // Convert to bytes
    uint64_t bytes = MB << 20;
    // Calculate how many array slots we can use
    uint64_t maxSize = bytes / sizeof(HashNode);

    size = 1;
    while (size <= maxSize)
        size <<= 1;
    size >>= 1;

    table = (HashNode *) calloc(size,  sizeof(HashNode));
    clear();
}

void Hash::incrementAge() {
    age++;
}

void Hash::clear() {
    std::memset(static_cast<void*>(table), 0, size * sizeof(HashNode));
    age = 0;
}

int Hash::estimateHashfull() const {
    int used = 0;
    // This will never go out of bounds since a 1 MB table has 32768 slots
    for (int i = 0; i < 500; i++) {
        used += ((table + i)->slot1.ageNodeType >> 2) == age;
        used += ((table + i)->slot2.ageNodeType >> 2) == age;
    }
    return used;
}
