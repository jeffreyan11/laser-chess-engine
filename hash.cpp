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
 * @brief Adds key (b,ptm) and item move into the hashtable.
 * Assumes that this key has been checked with get and is not in the table.
*/
void Hash::add(Board &b, int depth, Move *m) {
    uint64_t h = hash(b);
    uint64_t index = h % size;
    HashLL *node = table[index];
    if(node == NULL) {
        keys++;
        table[index] = new HashLL(b, depth, m);
        return;
    }

    while(node->next != NULL) {
        if(node->cargo.whitePieces == b.getWhitePieces()
        && node->cargo.blackPieces == b.getBlackPieces()
        && node->cargo.ptm == (uint16_t) (b.getPlayerToMove()))
            return;
        node = node->next;
    }
    keys++;
    node->next = new HashLL(b, depth, m);
}

/**
 * @brief Get the move, if any, associated with a board b and player to move.
*/
Move *Hash::get(Board &b) {
    uint64_t h = hash(b);
    uint64_t index = h % size;
    HashLL *node = table[index];

    if(node == NULL)
        return NULL;

    do {
        if(node->cargo.whitePieces == b.getWhitePieces()
        && node->cargo.blackPieces == b.getBlackPieces()
        && node->cargo.ptm == (uint16_t) (b.getPlayerToMove()))
            return node->cargo.m;
        node = node->next;
    }
    while(node != NULL);

    return NULL;
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

// TODO Make this use Zobrist hashing...
/**
 * @brief Hashes a board position using the FNV hashing algorithm.
*/
uint64_t Hash::hash(Board &b) {
    uint64_t h = 14695981039346656037ULL;
    h ^= b.getWhitePieces();
    h *= 1099511628211;
    h ^= b.getBlackPieces();
    h *= 1099511628211;
    return h;
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