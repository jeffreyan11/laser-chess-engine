#ifndef __SEARCHSPACE_H__
#define __SEARCHSPACE_H__

#include "board.h"
#include "common.h"

struct SearchSpace {
	Board *b;
	int color;
	int depth;
    // For PVS, the node is a PV node if beta - alpha > 1 (i.e. not a null window)
    // We do not want to do most pruning techniques on PV nodes
	bool isPVNode;
    // Similarly, we do not want to prune if we are in check
	bool isInCheck;

	SearchSpace(Board *_b, int _color, int _depth, int _alpha, int _beta);
};

#endif