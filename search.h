#ifndef __SEARCH_H__
#define __SEARCH_H__

#include "board.h"
#include "common.h"
#include "hash.h"

void getBestMove(Board *b, int mode, int value, Move *bestMove);
void clearTranspositionTable();
void clearHistoryTable();
uint64_t getNodes();

// Search modes:
const int TIME = 1;
const int DEPTH = 2;
// const int NODES = 3;

const int ONE_SECOND = 1000;

// Search parameters
const int MAX_POS_SCORE = 200;

#endif