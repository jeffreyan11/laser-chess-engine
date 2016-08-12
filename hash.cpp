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

#include <cstring>
#include "hash.h"

/*
 * Packs the data into a single 64-bit integer using the following format:
 * Bits 0-15: score
 * Bits 16-31: move
 * Bits 32-39: node type
 * Bits 40-47: age
 * Bits 48-55: depth
 */
uint64_t packHashData(int depth, Move m, int score, uint8_t nodeType, uint8_t age) {
    uint64_t data = 0;
    data |= (uint8_t) depth;
    data <<= 8;
    data |= age;
    data <<= 8;
    data |= nodeType;
    data <<= 16;
    data |= m;
    data <<= 16;
    data |= (uint16_t) score;

    return data;
}

Hash::Hash(uint64_t MB) {
    init(MB);
}

Hash::~Hash() {
    free(table);
}

// Adds key and move into the hashtable. This function assumes that the key has
// been checked with get and is not in the table.
void Hash::add(Board &b, uint64_t data, int depth, uint8_t age) {
    uint64_t h = b.getZobristKey();
    uint64_t index = h & (size-1);
    HashNode *node = table + index;
    if (node->slot1.zobristKey == 0) {
        keys++;
        node->slot1.setEntry(b, data);
        return;
    }
    else if (node->slot2.zobristKey == 0) {
        keys++;
        node->slot2.setEntry(b, data);
        return;
    }
    else { // Decide whether to replace the entry
        // A more recent update to the same position should always be chosen
        if ((node->slot1.zobristKey ^ node->slot1.data) == b.getZobristKey()) {
            if (getHashAge(node->slot1.data) != age)
                keys++;
            node->slot1.setEntry(b, data);
        }
        else if ((node->slot2.zobristKey ^ node->slot2.data) == b.getZobristKey()) {
            if (getHashAge(node->slot2.data) != age)
                keys++;
            node->slot2.setEntry(b, data);
        }
        // Replace an entry from a previous search space, or the lowest
        // depth entry with the new entry if the new entry's depth is higher
        else {
            HashEntry *toReplace = NULL;
            int score1 = 128*((int) (age - getHashAge(node->slot1.data)))
                + depth - getHashDepth(node->slot1.data);
            int score2 = 128*((int) (age - getHashAge(node->slot2.data)))
                + depth - getHashDepth(node->slot2.data);
            if (score1 >= score2)
                toReplace = &(node->slot1);
            else
                toReplace = &(node->slot2);
            // The node must be from a newer search space or be a
            // higher depth if from the same search space.
            if (score1 < 0 && score2 < 0)
                toReplace = NULL;

            if (toReplace != NULL) {
                if (getHashAge(toReplace->data) != age)
                    keys++;
                toReplace->setEntry(b, data);
            }
        }
    }
}

// Get the hash entry, if any, associated with a board b.
uint64_t Hash::get(Board &b) {
    uint64_t h = b.getZobristKey();
    uint64_t index = h & (size-1);
    HashNode *node = table + index;

    if((node->slot1.zobristKey ^ node->slot1.data) == b.getZobristKey())
        return node->slot1.data;
    else if ((node->slot2.zobristKey ^ node->slot2.data) == b.getZobristKey())
        return node->slot2.data;

    return 0;
}

uint64_t Hash::getSize() {
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
    while (size <= maxSize) {
        size <<= 1;
    }
    size >>= 1;

    table = (HashNode *) calloc(size,  sizeof(HashNode));
    keys = 0;
}

void Hash::clear() {
    std::memset(table, 0, size * sizeof(HashNode));
    keys = 0;
}
