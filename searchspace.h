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

#ifndef __SEARCHSPACE_H__
#define __SEARCHSPACE_H__

#include <cmath>
#include "board.h"
#include "common.h"

struct SearchParameters {
    int ply;
    int nullMoveCount;
    ChessTime startTime;
    uint64_t timeLimit;
    Move killers[MAX_DEPTH][2];
    int historyTable[2][6][64];
    uint8_t rootMoveNumber;

    SearchParameters() {
        reset();
        resetHistoryTable();
    }

    void reset() {
        ply = 0;
        nullMoveCount = 0;
        for (int i = 0; i < MAX_DEPTH; i++) {
            killers[i][0] = NULL_MOVE;
            killers[i][1] = NULL_MOVE;
        }
        //resetHistoryTable();
    }
    
    void resetHistoryTable() {
        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < 6; j++) {
                for (int k = 0; k < 64; k++)
                    historyTable[i][j][k] = 0;
            }
        }
    }

    void ageHistoryTable(int depth) {
        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < 6; j++) {
                for (int k = 0; k < 64; k++) {
                    if (historyTable[i][j][k] > 0)
                        historyTable[i][j][k] /= depth;
                    else
                        historyTable[i][j][k] /= (int) std::sqrt(depth);
                }
            }
        }
    }
};

struct SearchSpace {
	Board *b;
	int color;
	int depth;
	bool isPVNode;
	bool isInCheck;
	SearchParameters *searchParams;
	MoveList legalMoves;
	ScoreList scores;
	unsigned int index;

	SearchSpace(Board *_b, int _color, int _depth, bool _isPVNode, bool _isInCheck,
		SearchParameters *_searchParams);

	// Node is reducible if not PV node and not in check
	bool nodeIsReducible();

	void generateMoves(Move hashed, PieceMoveList &pml);
	Move nextMove();
    void reduceBadHistories(Move bestMove);
};

#endif