#ifndef __HASH_H__
#define __HASH_H__

#include "board.h"
#include "common.h"

struct BoardData {
    uint64_t whitePieces;
    uint64_t blackPieces;
    Move *m;
    uint16_t ptm;
    uint16_t fiftyMoveCounter;
    uint16_t depth;
    uint16_t age;

    BoardData() {
        whitePieces = 0;
        blackPieces = 0;
        m = NULL;
        ptm = 0;
        fiftyMoveCounter = 0;
        depth = 0;
        age = 0;
    }

    ~BoardData() {
        delete m;
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
        Move *copy = new Move(m->piece, m->isCapture, m->startsq, m->endsq);
        copy->promotion = m->promotion;
        copy->isCastle = m->isCastle;
        cargo.m = copy;
        cargo.ptm = (uint16_t) (b.getPlayerToMove());
        cargo.fiftyMoveCounter = (uint16_t) (b.getFiftyMoveCounter());
        cargo.depth = (uint16_t) (depth);
        cargo.age = (uint16_t) (b.getMoveNumber());
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
    void clean(int moveNumber);
    void clear();
};

#endif
