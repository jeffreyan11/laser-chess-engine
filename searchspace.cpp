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

#include "search.h"
#include "searchspace.h"

const int SCORE_IID_MOVE = (1 << 20);
const int SCORE_WINNING_CAPTURE = (1 << 18);
const int SCORE_QUEEN_PROMO = (1 << 17);
const int SCORE_EVEN_CAPTURE = (1 << 16);
const int SCORE_LOSING_CAPTURE = 0;
const int SCORE_QUIET_MOVE = -(1 << 30);

const int IID_DEPTHS[MAX_DEPTH+1] = {0,
     0,  0,  0,  1,  1,  1,  1,  1,  2,  2,
     2,  3,  3,  3,  4,  4,  4,  5,  5,  6,
     6,  6,  7,  7,  7,  8,  8,  8,  9,  9,
     9, 10, 10, 10, 11, 11, 11, 12, 12, 12,
    13, 13, 13, 14, 14, 14, 15, 15, 15, 16,
    16, 16, 17, 17, 17, 18, 18, 18, 19, 19,
    19, 20, 20, 20, 21, 21, 21, 22, 22, 22,
    23, 23, 23, 24, 24, 24, 25, 25, 25, 26,
    26, 26, 27, 27, 27, 28, 28, 28, 29, 29,
    29, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    30, 30, 30, 30, 30, 30, 30
};

SearchSpace::SearchSpace(Board *_b, int _color, int _depth, bool _isPVNode, bool _isInCheck,
	SearchParameters *_searchParams) {
	b = _b;
	color = _color;
	depth = _depth;
	isPVNode = _isPVNode;
	isInCheck = _isInCheck;
	searchParams = _searchParams;
}

bool SearchSpace::nodeIsReducible() {
	return !isPVNode && !isInCheck;
}

void SearchSpace::generateMoves(Move hashed, PieceMoveList &pml) {
	index = 0;
    legalMoves = isInCheck ? b->getPseudoLegalCheckEscapes(color, pml)
                           : b->getAllPseudoLegalMoves(color, pml);

    // Remove the hash move from the list, since it has already been tried
    // TODO make this nicer
    if (hashed != NULL_MOVE) {
        for (unsigned int i = 0; i < legalMoves.size(); i++) {
            if (legalMoves.get(i) == hashed) {
                legalMoves.remove(i);
                break;
            }
        }
    }

    // Internal iterative deepening and SEE move ordering
    // TODO make this cleaner, probably when captures and moves become generated separately

    // ---------------Captures----------------
    unsigned int quietStart = 0;
    for (quietStart = 0; quietStart < legalMoves.size(); quietStart++) {
        Move m = legalMoves.get(quietStart);
        if (!isCapture(m))
            break;

        int see = b->getSEE(color, getEndSq(m));
        // We want the best move first for PV nodes
        if (isPVNode) {
            if (see > 0)
                scores.add(SCORE_WINNING_CAPTURE + see);
            else if (see == 0)
                scores.add(SCORE_EVEN_CAPTURE);
            else
                scores.add(see);
        }
        // Otherwise, MVV/LVA for cheaper cutoffs might help
        else {
            if (see > 0)
                scores.add(SCORE_WINNING_CAPTURE + b->getMVVLVAScore(color, m));
            else if (see == 0)
                scores.add(SCORE_EVEN_CAPTURE + b->getMVVLVAScore(color, m));
            else
                scores.add(see);
        }
    }

    // ---------------Non-captures----------------
    // Score killers below even captures but above losing captures
    for (unsigned int i = quietStart; i < legalMoves.size(); i++) {
        Move m = legalMoves.get(i);
        if (m == searchParams->killers[searchParams->ply][0])
            scores.add(SCORE_EVEN_CAPTURE - 1);
        else if (m == searchParams->killers[searchParams->ply][1])
            scores.add(SCORE_EVEN_CAPTURE - 2);
        // Order queen promotions somewhat high
        else if (getPromotion(m) == QUEENS)
            scores.add(SCORE_QUEEN_PROMO);
        else {
            int startSq = getStartSq(m);
            int endSq = getEndSq(m);
            int pieceID = b->getPieceOnSquare(color, startSq);
            scores.add(SCORE_QUIET_MOVE + searchParams->historyTable[color][pieceID][endSq]);
        }
    }

    // IID: get a best move (hoping for a first move cutoff) if we don't
    // have a hash move available
    if (depth >= (isPVNode ? 5 : 6) && hashed == NULL_MOVE) {
        // Sort the moves with what we have so far
        /*for (Move m = nextMove(); m != NULL_MOVE;
                  m = nextMove());
        index = 0;*/

        int iidDepth = isPVNode ? depth-2 : IID_DEPTHS[depth];
        int bestIndex = getBestMoveForSort(b, legalMoves, iidDepth);
        // Mate check to prevent crashes
        if (bestIndex == -1)
            legalMoves.clear();
        else
        	scores.set(bestIndex, SCORE_IID_MOVE);
    }
}

// Retrieves the next move with the highest score, starting from index using a
// partial selection sort. This way, the entire list does not have to be sorted
// if an early cutoff occurs.
Move SearchSpace::nextMove() {
    if (index >= legalMoves.size())
        return NULL_MOVE;
    // Find the index of the next best move
    int bestIndex = index;
    int bestScore = scores.get(index);
    for (unsigned int i = index + 1; i < legalMoves.size(); i++) {
        if (scores.get(i) > bestScore) {
            bestIndex = i;
            bestScore = scores.get(bestIndex);
        }
    }
    // Swap the best move to the correct position
    legalMoves.swap(bestIndex, index);
    scores.swap(bestIndex, index);
    return legalMoves.get(index++);
}

// When a PV or cut move is found, the histories of all
// quiet moves searched prior to the best move are reduced
void SearchSpace::reduceBadHistories(Move bestMove) {
    for (unsigned int i = 0; i < index-1; i++) {
        if (legalMoves.get(i) == bestMove)
            break;
        if (isCapture(legalMoves.get(i)))
            continue;
        searchParams->historyTable[color][b->getPieceOnSquare(color, getStartSq(legalMoves.get(i)))][getEndSq(legalMoves.get(i))] -= depth;
    }
}