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

#include "hash.h"

Hash::Hash(uint64_t MB) {
    // Convert to bytes
    uint64_t arrSize = MB << 20;
    // Calculate how many array slots we can use
    arrSize /= sizeof(HashNode);
    // Create the array
    table = new HashNode *[arrSize];
    size = arrSize;
    for(uint64_t i = 0; i < size; i++) {
        table[i] = NULL;
    }
    keys = 0;
}

Hash::~Hash() {
    for(uint64_t i = 0; i < size; i++) {
        if (table[i] != NULL)
            delete table[i];
    }
    delete[] table;
}

// Adds key and move into the hashtable. This function assumes that the key has
// been checked with get and is not in the table.
void Hash::add(Board &b, int depth, Move m, int score, uint8_t nodeType, uint8_t age) {
    // Use the lower 32 bits of the hash key to index the array
    uint32_t h = (uint32_t) (b.getZobristKey() & 0xFFFFFFFF);
    uint32_t index = h % size;
    HashNode *node = table[index];
    if (node == NULL) {
        keys++;
        table[index] = new HashNode(b, depth, m, score, nodeType, age);
        return;
    }
    else if (node->slot2.zobristKey == 0) {
        keys++;
        node->slot2.setEntry(b, depth, m, score, nodeType, age);
        return;
    }
    else { // Decide whether to replace the entry
        // A more recent update to the same position should always be chosen
        if (node->slot1.zobristKey == (uint32_t) (b.getZobristKey() >> 32)) {
            node->slot1.clearEntry();
            node->slot1.setEntry(b, depth, m, score, nodeType, age);
        }
        else if (node->slot2.zobristKey == (uint32_t) (b.getZobristKey() >> 32)) {
            node->slot2.clearEntry();
            node->slot2.setEntry(b, depth, m, score, nodeType, age);
        }
        // Replace an entry from a previous search space, or the lowest
        // depth entry with the new entry if the new entry's depth is higher
        else {
            HashEntry *toReplace = NULL;
            int score1 = 4*(age - node->slot1.getAge()) + depth - node->slot1.depth;
            int score2 = 4*(age - node->slot2.getAge()) + depth - node->slot2.depth;
            if (score1 >= score2)
                toReplace = &(node->slot1);
            else
                toReplace = &(node->slot2);
            // The node must be from a newer search space or be a
            // higher depth. Each move forward in the search space is worth 4 depth.
            if (score1 < 0 && score2 < 0)
                toReplace = NULL;

            if (toReplace != NULL) {
                toReplace->clearEntry();
                toReplace->setEntry(b, depth, m, score, nodeType, age);
            }
        }
    }
}

// Get the hash entry, if any, associated with a board b.
HashEntry *Hash::get(Board &b) {
    // Use the lower 32 bits of the hash key to index the array
    uint32_t h = (uint32_t) (b.getZobristKey() & 0xFFFFFFFF);
    uint32_t index = h % size;
    HashNode *node = table[index];

    if(node == NULL)
        return NULL;

    if(node->slot1.zobristKey == (uint32_t) (b.getZobristKey() >> 32))
        return &(node->slot1);
    else if (node->slot2.zobristKey == (uint32_t) (b.getZobristKey() >> 32))
        return &(node->slot2);

    return NULL;
}

uint64_t Hash::getSize() {
    return (2 * size);
}

void Hash::clear() {
    for(uint64_t i = 0; i < size; i++) {
        if (table[i] != NULL)
            delete table[i];
    }
    delete[] table;
    
    table = new HashNode *[size];
    for (uint64_t i = 0; i < size; i++) {
        table[i] = NULL;
    }
    keys = 0;
}
