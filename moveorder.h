/*
    Laser, a UCI chess engine written in C++11.
    Copyright 2015-2016 Jeffrey An and Michael An

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

#ifndef __MOVEORDER_H__
#define __MOVEORDER_H__

#include <cmath>
#include "board.h"
#include "common.h"
#include "searchparams.h"

enum MoveGenStage {
    STAGE_NONE, STAGE_HASH_MOVE, STAGE_IID_MOVE, STAGE_CAPTURES, STAGE_QUIETS
};

struct MoveOrder {
	Board *b;
	int color;
	int depth;
    int threadID;
	bool isPVNode;
	bool isInCheck;
	SearchParameters *searchParams;
    MoveGenStage mgStage;
    Move hashed;
	MoveList legalMoves;
	ScoreList scores;
    unsigned int quietStart;
	unsigned int index;

	MoveOrder(Board *_b, int _color, int _depth, int _threadID, bool _isPVNode, bool _isInCheck,
		SearchParameters *_searchParams, Move _hashed, MoveList _legalMoves);

    bool doIID();

	void generateMoves();
	Move nextMove();
    void reduceBadHistories(Move bestMove);

private:
    void scoreCaptures(bool isIIDMove);
    void scoreQuiets();
    void scoreIIDMove();
    void findQuietStart();
};

#endif