#include "hash.h"

Hash::Hash(uint64_t MB) {
    // Convert to bytes
    uint64_t arrSize = MB << 20;
    // Calculate how many array slots we can use
    arrSize /= 2 * sizeof(HashEntry);
    // Create the array
    table = new HashNode *[arrSize];
    size = arrSize;
    for(uint64_t i = 0; i < size; i++) {
        table[i] = NULL;
    }
    keys = 0;
    #if HASH_DEBUG_OUTPUT
    replacements = 0;
    collisions = 0;
    #endif
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
void Hash::add(Board &b, int depth, Move m, int score, uint8_t nodeType, uint8_t searchGen) {
    // Use the lower 32 bits of the hash key to index the array
    uint32_t h = (uint32_t) (b.getZobristKey() & 0xFFFFFFFF);
    uint32_t index = h % size;
    HashNode *node = table[index];
    if (node == NULL) {
        keys++;
        table[index] = new HashNode(b, depth, m, score, nodeType, searchGen);
        return;
    }
    else if (node->slot2.zobristKey == 0) {
        keys++;
        node->slot2.setEntry(b, depth, m, score, nodeType, searchGen);
        return;
    }
    else { // Decide whether to replace the entry
        #if HASH_DEBUG_OUTPUT
        collisions++;
        #endif
        // A more recent update to the same position should always be chosen
        if (node->slot1.zobristKey == (uint32_t) (b.getZobristKey() >> 32)) {
            node->slot1.clearEntry();
            node->slot1.setEntry(b, depth, m, score, nodeType, searchGen);
        }
        else if (node->slot2.zobristKey == (uint32_t) (b.getZobristKey() >> 32)) {
            node->slot2.clearEntry();
            node->slot2.setEntry(b, depth, m, score, nodeType, searchGen);
        }
        // Replace cut/all nodes with PV nodes
        // Replace older PV nodes with newer ones
        // Keep PV nodes whenever possible
        // Otherwise, replace an entry from a previous search space, or the lowest
        // depth entry with the new entry if the new entry's depth is higher
        else {
            HashEntry *toReplace = NULL;
            int score1 = 4*(searchGen - node->slot1.getAge())
                       - 4*(node->slot1.getNodeType() == PV_NODE) + depth - node->slot1.depth;
            int score2 = 4*(searchGen - node->slot2.getAge())
                       - 4*(node->slot2.getNodeType() == PV_NODE) + depth - node->slot2.depth;
            if (score1 >= score2)
                toReplace = &(node->slot1);
            else
                toReplace = &(node->slot2);
            // If new node is PV, almost always put it in
            // Otherwise, the node must be from a newer search space or be a
            // higher depth. Each move in the search space is worth 4 depth.
            if (score1 <= -8*(nodeType == PV_NODE) && score2 <= -8*(nodeType == PV_NODE))
                toReplace = NULL;

            if (toReplace != NULL) {
                toReplace->clearEntry();
                toReplace->setEntry(b, depth, m, score, nodeType, searchGen);
            }
            else return;
        }
        
        #if HASH_DEBUG_OUTPUT
        replacements++;
        #endif
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
