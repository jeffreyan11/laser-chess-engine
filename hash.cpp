#include "hash.h"

Hash::Hash(uint64_t MB) {
    uint64_t arrSize = MB << 20;
    arrSize /= sizeof(HashNode);
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

/**
 * @brief Adds key and move into the hashtable.
 * Assumes that this key has been checked with get and is not in the table.
*/
void Hash::add(Board &b, int depth, Move m, int score, uint8_t nodeType) {
    uint64_t h = b.getZobristKey();
    uint64_t index = h % size;
    HashNode *node = table[index];
    if(node == NULL) {
        keys++;
        table[index] = new HashNode(b, depth, m, score, nodeType);
        return;
    }
    else { // Decide whether to replace the entry
        #if HASH_DEBUG_OUTPUT
        collisions++;
        #endif
        // TODO figure out why this doesn't work
        /*if (node->cargo.zobristKey == b.getZobristKey()) {
            delete node;
            table[index] = new HashNode(b, depth, m, score, nodeType);
            #if HASH_DEBUG_OUTPUT
            replacements++;
            #endif
        }
        else */if (node->cargo.nodeType == PV_NODE) { // Always keep PV nodes
            return;
        }
        else if (nodeType == PV_NODE) { // and replace cut/all nodes with PV nodes
            delete node;
            table[index] = new HashNode(b, depth, m, score, nodeType);
            #if HASH_DEBUG_OUTPUT
            replacements++;
            #endif
        }
        // Otherwise, give priority to higher depths
        // Also, replace very low depth searches with newer searches regardless of
        // the new search's depth.
        else if (depth >= node->cargo.depth || (node->cargo.depth <= 3)) {
            delete node;
            table[index] = new HashNode(b, depth, m, score, nodeType);
            #if HASH_DEBUG_OUTPUT
            replacements++;
            #endif
        }
    }
}

/**
 * @brief Get the hash entry, if any, associated with a board b.
*/
HashEntry *Hash::get(Board &b) {
    uint64_t h = b.getZobristKey();
    uint64_t index = h % size;
    HashNode *node = table[index];

    if(node == NULL)
        return NULL;

    if(node->cargo.zobristKey == b.getZobristKey()) {
        return &(node->cargo);
    }

    return NULL;
}

void Hash::clean(int moveNumber) {
    for(uint64_t i = 0; i < size; i++) {
        HashNode *node = table[i];
        // TODO choose aging policy
        if (node != NULL) {
            if(moveNumber > node->cargo.age) {
                keys--;
                delete node;
                table[i] = NULL;
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
