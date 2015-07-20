#ifndef __SEARCH_H__
#define __SEARCH_H__

#include "board.h"
#include "common.h"
#include "hash.h"

Move getBestMove(Board *b, int mode, int value);
void clearTranspositionTable();

// Search modes:
const int TIME = 1;
const int DEPTH = 2;
// const int NODES = 3;

const int ONE_SECOND = 1000;

// Search parameters
const int MAX_POS_SCORE = 200;

#endif
