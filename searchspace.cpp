#include "search.h"
#include "searchspace.h"

const int IID_DEPTHS[MAX_DEPTH+1] = {0,
     0,  0,  0,  0,  1,  1,  1,  2,  2,  2,
     3,  3,  3,  4,  4,  4,  5,  5,  5,  6,
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

SearchSpace::SearchSpace(Board *_b, int _color, int _depth, int _alpha, int _beta,
	SearchParameters *_searchParams) {
	b = _b;
	color = _color;
	depth = _depth;
	isPVNode = (_beta - _alpha != 1);
	isInCheck = b->isInCheck(color);
	searchParams = _searchParams;
}

bool SearchSpace::nodeIsReducible() {
	return !isPVNode && !isInCheck;
}

void SearchSpace::generateMoves(Move hashed) {
    legalMoves = isInCheck ? b->getPseudoLegalCheckEscapes(color)
                           : b->getAllPseudoLegalMoves(color);

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

    // Internal iterative deepening, SEE, and MVV/LVA move ordering
    // The scoring relies partially on the fact that our selection sort is stable
    // TODO make this cleaner, probably when captures and moves become generated separately
    if (depth >= 3 || isPVNode) { // sort by SEE
        // ---------------Captures----------------
        unsigned int index = 0;
        for (index = 0; index < legalMoves.size(); index++) {
            Move m = legalMoves.get(index);
            if (!isCapture(m))
                break;
            // If anything wins at least a rook, it should go above this anyways
            /*if (b.getExchangeScore(color, m) > 0)
                scores.add(ROOK_VALUE + b.getMVVLVAScore(color, legalMoves.get(index)));
            else*/
                scores.add(b->getSEE(color, getEndSq(m)));
        }
        // ---------------Non-captures----------------
        // Score killers below even captures but above losing captures
        for (unsigned int i = index; i < legalMoves.size(); i++) {
            if (legalMoves.get(i) == searchParams->killers[searchParams->ply][0])
                scores.add(0);
            else if (legalMoves.get(i) == searchParams->killers[searchParams->ply][1])
                scores.add(-1);
            // Order queen promotions somewhat high
            else if (getPromotion(legalMoves.get(i)) == QUEENS)
                scores.add(MAX_POS_SCORE);
            else
                scores.add(-MATE_SCORE + searchParams->historyTable[color][b->getPieceOnSquare(color, getStartSq(legalMoves.get(i)))][getEndSq(legalMoves.get(i))]);
        }

        // IID: get a best move (hoping for a first move cutoff) if we don't
        // have a hash move available
        if (depth >= 5 && hashed == NULL_MOVE) {
            int bestIndex = getBestMoveForSort(b, legalMoves, IID_DEPTHS[depth]);
            // Mate check to prevent crashes
            if (bestIndex == -1)
                legalMoves.clear();
            else
            	scores.set(bestIndex, INFTY);
        }
    }
    else { // MVV/LVA
        unsigned int index = 0;
        for (index = 0; index < legalMoves.size(); index++) {
            if (!isCapture(legalMoves.get(index)))
                break;
            scores.add(b->getMVVLVAScore(color, legalMoves.get(index)));
        }
        // For MVV/LVA, order killers above capturing a pawn with a minor piece
        // or greater
        for (unsigned int i = index; i < legalMoves.size(); i++) {
            if (legalMoves.get(i) == searchParams->killers[searchParams->ply][0])
                scores.add(PAWNS - KNIGHTS);
            else if (legalMoves.get(i) == searchParams->killers[searchParams->ply][1])
                scores.add(PAWNS - KNIGHTS - 1);
            // Order queen promotions above capturing pawns and minor pieces
            else if (getPromotion(legalMoves.get(i)) == QUEENS)
                scores.add(8*ROOKS);
            else
                scores.add(-MATE_SCORE + searchParams->historyTable[color][b->getPieceOnSquare(color, getStartSq(legalMoves.get(i)))][getEndSq(legalMoves.get(i))]);
        }
    }
}