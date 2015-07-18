#include "hash.h"

Hash::Hash(uint64_t MB) {
    uint64_t arrSize = MB << 20;
    arrSize /= 4 * sizeof(HashLL);
    table = new HashLL* [arrSize];
    size = arrSize;
    for(uint64_t i = 0; i < size; i++) {
        table[i] = NULL;
    }
    keys = 0;
}

Hash::~Hash() {
    for(uint64_t i = 0; i < size; i++) {
        HashLL* temp = table[i];
        while(temp != NULL) {
            HashLL *temp2 = temp->next;
            delete temp;
            temp = temp2;
        }
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
    HashLL *node = table[index];
    if(node == NULL) {
        keys++;
        table[index] = new HashLL(b, depth, m, score, nodeType);
        return;
    }

    while(node->next != NULL) {
        if(node->cargo.zobristKey == b.getZobristKey())
            return;
        node = node->next;
    }
    keys++;
    node->next = new HashLL(b, depth, m, score, nodeType);
}

/**
 * @brief Get the move, if any, associated with a board b.
 * Also returns the depth and score.
*/
Move Hash::get(Board &b, int &depth, int &score, uint8_t &nodeType) {
    uint64_t h = b.getZobristKey();
    uint64_t index = h % size;
    HashLL *node = table[index];

    if(node == NULL)
        return NULL_MOVE;

    do {
        if(node->cargo.zobristKey == b.getZobristKey()) {
            depth = (int) node->cargo.depth;
            score = node->cargo.score;
            nodeType = node->cargo.nodeType;
            return node->cargo.m;
        }
        node = node->next;
    }
    while(node != NULL);

    return NULL_MOVE;
}

void Hash::clean(int moveNumber) {
    for(uint64_t i = 0; i < size; i++) {
        HashLL *node = table[i];
        while(node != NULL) {
            // TODO choose aging policy
            if(moveNumber > node->cargo.age) {
                keys--;
                table[i] = node->next;
                delete node;
                node = table[i];
            }
            else
                node = node->next;
        }
    }
}

void Hash::test() {
    int zeros = 0;
    int threes = 0;

    for(uint64_t i = 0; i < size; i++) {
        int linked = 0;
        HashLL* node = table[i];
        if(node == NULL)
            zeros++;
        else {
            linked++;
            while(node->next != NULL) {
                node = node->next;
                linked++;
            }
            if(linked >= 3) threes++;
        }
    }

    cerr << "zeros: " << zeros << endl;
    cerr << "threes: " << threes << endl;
}

void Hash::clear() {
    for(uint64_t i = 0; i < size; i++) {
        HashLL* temp = table[i];
        while(temp != NULL) {
            HashLL *temp2 = temp->next;
            delete temp;
            temp = temp2;
        }
    }
    delete[] table;
    
    table = new HashLL *[size];
    for (uint64_t i = 0; i < size; i++) {
        table[i] = 0;
    }
    keys = 0;
}
