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

// Time management constants
const int MOVE_HORIZON = 40; // expect this many moves left in the game
const int BUFFER_TIME = 200; // try to leave this much time in case of an emergency
const double TIME_FACTOR = 0.4; // timeFactor = log b / (b - 1) where b is branch factor
const double MAX_TIME_FACTOR = 2.5; // do not spend more than this multiple of time over the limit

#endif