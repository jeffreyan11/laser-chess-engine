#ifndef __SEARCHSPACE_H__
#define __SEARCHSPACE_H__

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
    }

    void reset() {
        ply = 0;
        nullMoveCount = 0;
        for (int i = 0; i < MAX_DEPTH; i++) {
            killers[i][0] = NULL_MOVE;
            killers[i][1] = NULL_MOVE;
        }
        resetHistoryTable();
    }
    
    void resetHistoryTable() {
        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < 6; j++) {
                for (int k = 0; k < 64; k++)
                    historyTable[i][j][k] = 0;
            }
        }
    }
};

struct SearchSpace {
	Board *b;
	int color;
	int depth;
    // For PVS, the node is a PV node if beta - alpha > 1 (i.e. not a null window)
    // We do not want to do most pruning techniques on PV nodes
	bool isPVNode;
    // Similarly, we do not want to prune if we are in check
	bool isInCheck;
	SearchParameters *searchParams;
	MoveList legalMoves;
	ScoreList scores;

	SearchSpace(Board *_b, int _color, int _depth, int _alpha, int _beta,
		SearchParameters *_searchParams);

	// Node is reducible if not PV node and not in check
	bool nodeIsReducible();

	void generateMoves(Move hashed);
};

#endif