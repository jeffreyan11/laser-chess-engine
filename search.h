/*
    Laser, a UCI chess engine written in C++11.
    Copyright 2015 Jeffrey An and Michael An

    Laser is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Laser is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Laser.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __SEARCH_H__
#define __SEARCH_H__

#include "board.h"
#include "common.h"
#include "hash.h"

void getBestMove(Board *b, int mode, int value, Move *bestMove);
void clearTables();
void setHashSize(uint64_t MB);
uint64_t getNodes();

int getBestMoveForSort(Board *b, MoveList &legalMoves, int depth);

// Search modes
const int TIME = 1;
const int DEPTH = 2;
// const int NODES = 3;

// Time constants
const int ONE_SECOND = 1000;
const uint64_t MAX_TIME = (1ULL << 63) - 1;

// Search parameters
const int MAX_POS_SCORE = 120;

// Time management constants
const int MOVE_HORIZON = 40; // expect this many moves left in the game
const int BUFFER_TIME = 200; // try to leave this much time in case of an emergency
const double TIME_FACTOR = 0.7; // timeFactor = log b / (b - 1) where b is branch factor
const double MAX_TIME_FACTOR = 2.5; // do not spend more than this multiple of time over the limit

#endif