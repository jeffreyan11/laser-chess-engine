#include "hash.h"

Hash::Hash(uint64_t MB) {
    uint64_t arrSize = MB << 20;
    arrSize /= 2 * sizeof(HashEntry);
    table = new HashNode *[arrSize];
    size = arrSize;
    for(uint64_t i = 0; i < size; i++) {
        table[i] = NULL;
    }
    keys = 0;
    #if HASH_DEBUG_OUTPUT
    replacements = 0;
    collisions = 0;
    cerr << "Hash size: " << size << endl;
    #endif
}

Hash::~Hash() {
    for(uint64_t i = 0; i < size; i++) {
        delete table[i];
    }
    delete[] table;
}

// Adds key and move into the hashtable. This function assumes that the key has
// been checked with get and is not in the table.
void Hash::add(Board &b, int depth, Move m, int score, uint8_t nodeType) {
    uint64_t h = b.getZobristKey();
    uint64_t index = h % size;
    HashNode *node = table[index];
    if (node == NULL) {
        keys++;
        table[index] = new HashNode(b, depth, m, score, nodeType);
        return;
    }
    else if (node->slot2 == NULL) {
        keys++;
        node->slot2 = new HashEntry(b, depth, m, score, nodeType);
        return;
    }
    else { // Decide whether to replace the entry
        #if HASH_DEBUG_OUTPUT
        collisions++;
        #endif
        // A more recent update to the same position should always be chosen
        if (node->slot1->zobristKey == b.getZobristKey()) {
            delete node->slot1;
            node->slot1 = new HashEntry(b, depth, m, score, nodeType);
        }
        else if (node->slot2->zobristKey == b.getZobristKey()) {
            delete node->slot2;
            node->slot2 = new HashEntry(b, depth, m, score, nodeType);
        }
        // Always keep PV nodes
        else if (node->slot1->nodeType == PV_NODE && node->slot2->nodeType == PV_NODE) {
            return;
        }
        // and replace cut/all nodes with PV nodes
        else if (nodeType == PV_NODE) {
            if (node->slot1->nodeType != PV_NODE) {
                delete node->slot1;
                node->slot1 = new HashEntry(b, depth, m, score, nodeType);
            }
            else {
                delete node->slot2;
                node->slot2 = new HashEntry(b, depth, m, score, nodeType);
            }
        }
        // Otherwise, replace the lowest depth entry with the new entry if the
        // new entry's depth is higher
        else {
            if (node->slot1->depth <= node->slot2->depth) {
                if ((depth >= node->slot1->depth) || (node->slot1->depth <= 2)) {
                    delete node->slot1;
                    node->slot1 = new HashEntry(b, depth, m, score, nodeType);
                }
                else return;
            }
            else if ((depth >= node->slot2->depth) || (node->slot2->depth <= 2)) {
                delete node->slot2;
                node->slot2 = new HashEntry(b, depth, m, score, nodeType);
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
    uint64_t h = b.getZobristKey();
    uint64_t index = h % size;
    HashNode *node = table[index];

    if(node == NULL)
        return NULL;

    if(node->slot1->zobristKey == b.getZobristKey())
        return node->slot1;
    else if (node->slot2 != NULL && node->slot2->zobristKey == b.getZobristKey())
        return node->slot2;

    return NULL;
}

void Hash::clean(uint16_t moveNumber) {
    for(uint64_t i = 0; i < size; i++) {
        HashNode *node = table[i];
        // TODO choose aging policy
        if (node != NULL) {
            // If slot 2 is aged out, delete it and set slot to NULL
            if (node->slot2 != NULL && moveNumber >= node->slot2->age) {
                keys--;
                delete node->slot2;
                node->slot2 = NULL;
            }
            // If slot 1 is aged out, shift over slot 2 or delete node if both
            // slots are empty
            if (moveNumber >= node->slot1->age) {
                keys--;
                if (node->slot2 != NULL) {
                    delete node->slot1;
                    node->slot1 = node->slot2;
                    node->slot2 = NULL;
                }
                else {
                    delete node;
                    table[i] = NULL;
                }
            }
        }
    }
    #if HASH_DEBUG_OUTPUT
    replacements = 0;
    collisions = 0;
    #endif
}

void Hash::clear() {
    for(uint64_t i = 0; i < size; i++) {
        delete table[i];
    }
    delete[] table;
    
    table = new HashNode *[size];
    for (uint64_t i = 0; i < size; i++) {
        table[i] = NULL;
    }
    keys = 0;
}
