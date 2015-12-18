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

/*
 * This struct is a simple stack implementation that stores Zobrist keys to
 * check for two-fold repetition.
 * Each time before a move is made, the board position is pushed onto the stack.
 * When the move is unmade or we return back up the search tree, the positions
 * are popped off the stack one by one.
 * The find() function loops through to look for a two-fold repetition. We must
 * only find the first repeat from the top of the stack since we have the
 * condition that the branch terminates immediately returning 0 if two-fold
 * repetition occurs in that branch.
 */
struct TwoFoldStack {
public:
    uint64_t keys[128];
    unsigned int length;

    TwoFoldStack() {
        length = 0;
    }
    ~TwoFoldStack() {}

    unsigned int size() { return length; }

    void push(uint64_t pos) {
        keys[length] = pos;
        length++;
    }

    void pop() { length--; }

    void clear() {
        length = 0;
    }

    bool find(uint64_t pos) {
        for (unsigned int i = length; i > 0; i--) {
            if (keys[i-1] == pos)
                return true;
        }
        return false;
    }
};

void getBestMove(Board *b, int mode, int value, Move *bestMove);
void clearTables();
void setHashSize(uint64_t MB);
void setEvalCacheSize(uint64_t MB);
uint64_t getNodes();
void setMultiPV(unsigned int n);
void setNumThreads(int n);

int getBestMoveForSort(Board *b, MoveList &legalMoves, int depth);

// Search modes
const int TIME = 1;
const int DEPTH = 2;
// const int NODES = 3;
const int MOVETIME = 4;

// Time constants
const uint64_t ONE_SECOND = 1000;
const uint64_t MAX_TIME = (1ULL << 63) - 1;

// Search parameters
const int MAX_POS_SCORE = 120;

// Time management constants
const int MOVE_HORIZON = 40; // expect this many moves left in the game
const int BUFFER_TIME = 100; // try to leave this much time in case of an emergency
const double TIME_FACTOR = 0.7; // timeFactor = log b / (b - 1) where b is branch factor
const double MAX_TIME_FACTOR = 2.5; // do not spend more than this multiple of time over the limit

#endif