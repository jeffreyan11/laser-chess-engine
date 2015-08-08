#ifndef __SEARCH_H__
#define __SEARCH_H__

#include "board.h"
#include "common.h"
#include "hash.h"

/*
 * @brief Records a bunch of useful statistics from the search,
 * which are printed to std error at the end of the search.
 */
struct SearchStatistics {
    uint64_t nodes;
    uint64_t hashProbes;
    uint64_t hashHits;
    uint64_t hashScoreCuts;
    uint64_t hashMoveAttempts;
    uint64_t hashMoveCuts;
    uint64_t searchSpaces;
    uint64_t failHighs;
    uint64_t firstFailHighs;
    uint64_t qsNodes;
    uint64_t qsSearchSpaces;
    uint64_t qsFailHighs;
    uint64_t qsFirstFailHighs;

    SearchStatistics() {
        reset();
    }

    void reset() {
        nodes = 0;
        hashProbes = 0;
        hashHits = 0;
        hashScoreCuts = 0;
        hashMoveAttempts = 0;
        hashMoveCuts = 0;
        searchSpaces = 0;
        failHighs = 0;
        firstFailHighs = 0;
        qsNodes = 0;
        qsSearchSpaces = 0;
        qsFailHighs = 0;
        qsFirstFailHighs = 0;
    }
};

void getBestMove(Board *b, int mode, int value, SearchStatistics *stats, Move *bestMove);
void clearTranspositionTable();

// Search modes:
const int TIME = 1;
const int DEPTH = 2;
// const int NODES = 3;

const int ONE_SECOND = 1000;

// Search parameters
const int MAX_POS_SCORE = 200;

#endif