#ifndef __SEARCH_H__
#define __SEARCH_H__

#include "board.h"
#include "common.h"

Move *getBestMove(Board *b, int mode, int value);
Move *getBestMoveAtDepth(Board *b, int depth);

// Search modes:
const int TIME = 1;
const int DEPTH = 2;
// const int NODES = 3;

const int ONE_SECOND = 1000;

#endif
