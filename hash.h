#ifndef __HASH_H__
#define __HASH_H__

#include "board.h"
#include "common.h"

struct BoardData {
    uint64_t whitePieces;
    uint64_t blackPieces;
    int ptm;
    int fiftyMoveCounter;
    int depth;
    Move *m;
    int age;

    BoardData() {
        whitePieces = 0;
        blackPieces = 0;
        ptm = 0;
        fiftyMoveCounter = 0;
        depth = 0;
        m = NULL;
        age = 0;
    }
};

class HashLL {

public:
    HashLL *next;
    BoardData cargo;

    HashLL(Board &b, int depth, Move *m) {
        next = NULL;
        cargo.whitePieces = b.getWhitePieces();
        cargo.blackPieces = b.getBlackPieces();
        cargo.ptm = b.getPlayerToMove();
        cargo.fiftyMoveCounter = b.getFiftyMoveCounter();
        cargo.depth = depth;
        Move *copy = new Move(m->piece, m->isCapture, m->startsq, m->endsq);
        copy->promotion = m->promotion;
        copy->isCastle = m->isCastle;
        cargo.m = copy;
    }

    ~HashLL() {}
};

class Hash {

private:
    HashLL **table;
    uint64_t size;

    uint64_t hash(Board &b);

public:
    int keys;
    void test();

    Hash(uint64_t MB);
    ~Hash();

    void add(Board &b, int depth, Move *m);
    Move *get(Board &b);
    void clean();
};

#endif
