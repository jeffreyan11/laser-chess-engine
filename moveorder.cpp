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
#include "moveorder.h"

const int SCORE_IID_MOVE = (1 << 20);
const int SCORE_WINNING_CAPTURE = (1 << 18);
const int SCORE_QUEEN_PROMO = (1 << 17);
const int SCORE_EVEN_CAPTURE = (1 << 16);
const int SCORE_LOSING_CAPTURE = 0;
const int SCORE_QUIET_MOVE = -(1 << 30);

MoveOrder::MoveOrder(Board *_b, int _color, int _depth, bool _isPVNode, bool _isInCheck,
	SearchParameters *_searchParams, Move _hashed, PieceMoveList &_pml) {
	b = _b;
	color = _color;
	depth = _depth;
	isPVNode = _isPVNode;
	isInCheck = _isInCheck;
	searchParams = _searchParams;
    mgStage = STAGE_NONE;
    quietStart = 0;
    index = 0;
    hashed = _hashed;
    pml = &_pml;
}

bool MoveOrder::nodeIsReducible() {
	return !isPVNode && !isInCheck;
}

// Returns true if there are still moves remaining, false if we have
// generated all moves already
void MoveOrder::generateMoves() {
    switch (mgStage) {
        case STAGE_NONE:
            if (hashed != NULL_MOVE) {
                mgStage = STAGE_HASH_MOVE;
                break;
            }
            // else fallthrough

        case STAGE_HASH_MOVE:
            if (isInCheck) {
                legalMoves = b->getPseudoLegalCheckEscapes(color, *pml);
                findQuietStart();

                if (hashed == NULL_MOVE && doIID()) {
                    mgStage = STAGE_IID_MOVE;
                    scoreIIDMove();
                }
                else {
                    mgStage = STAGE_SCORE_QUIETS;
                    scoreCaptures(false);
                }
            }
            else if (hashed == NULL_MOVE && doIID()) {
                mgStage = STAGE_IID_MOVE;
                legalMoves = b->getAllPseudoLegalMoves(color, *pml);
                findQuietStart();

                scoreIIDMove();
            }
            else {
                mgStage = STAGE_CAPTURES;
                legalMoves = b->getPseudoLegalCaptures(color, *pml, true);
                quietStart = legalMoves.size();

                scoreCaptures(false);
            }
            break;

        case STAGE_IID_MOVE:
            mgStage = STAGE_SCORE_QUIETS;
            scoreCaptures(true);
            break;

        case STAGE_CAPTURES:
            generateQuiets();
            break;

        case STAGE_SCORE_QUIETS:
            mgStage = STAGE_QUIETS;
            scoreQuiets();
            break;

        case STAGE_QUIETS:
            break;
    }
}

// Sort captures using SEE and MVV/LVA
void MoveOrder::scoreCaptures(bool isIIDMove) {
    // Remove the hash move from the list, since it has already been tried
    if (hashed != NULL_MOVE) {
        for (unsigned int i = isIIDMove; i < quietStart; i++) {
            if (legalMoves.get(i) == hashed) {
                legalMoves.remove(i);
                quietStart--;
                break;
            }
        }
    }

    for (unsigned int i = isIIDMove; i < quietStart; i++) {
        Move m = legalMoves.get(i);

        // We want the best move first for PV nodes
        if (isPVNode) {
            int see = b->getSEEForMove(color, m);
            if (see > 0)
                scores.add(SCORE_WINNING_CAPTURE + see);
            else if (see == 0)
                scores.add(SCORE_EVEN_CAPTURE);
            else
                scores.add(see);
        }
        // Otherwise, MVV/LVA for cheaper cutoffs might help
        else {
            int exchange = b->getExchangeScore(color, m);
            if (exchange > 0)
                scores.add(SCORE_WINNING_CAPTURE + b->getMVVLVAScore(color, m));
            else if (exchange == 0)
                scores.add(SCORE_EVEN_CAPTURE + b->getMVVLVAScore(color, m));
            else {
                int see = b->getSEEForMove(color, m);
                if (see > 0)
                    scores.add(SCORE_WINNING_CAPTURE + b->getMVVLVAScore(color, m));
                else if (see == 0)
                    scores.add(SCORE_EVEN_CAPTURE + b->getMVVLVAScore(color, m));
                else
                    scores.add(see);
            }
        }
    }
}

void MoveOrder::scoreQuiets() {
    // Remove the hash move from the list, since it has already been tried
    if (hashed != NULL_MOVE) {
        for (unsigned int i = quietStart; i < legalMoves.size(); i++) {
            if (legalMoves.get(i) == hashed) {
                legalMoves.remove(i);
                break;
            }
        }
    }

    for (unsigned int i = quietStart; i < legalMoves.size(); i++) {
        Move m = legalMoves.get(i);
        // Score killers below even captures but above losing captures
        if (m == searchParams->killers[searchParams->ply][0])
            scores.add(SCORE_EVEN_CAPTURE - 1);
        // Order queen promotions somewhat high
        else if (getPromotion(m) == QUEENS)
            scores.add(SCORE_QUEEN_PROMO);
        // Sort all other quiet moves by history
        else {
            int startSq = getStartSq(m);
            int endSq = getEndSq(m);
            int pieceID = b->getPieceOnSquare(color, startSq);
            scores.add(SCORE_QUIET_MOVE + searchParams->historyTable[color][pieceID][endSq]);
        }
    }
}

bool MoveOrder::doIID() {
    return depth >= (isPVNode ? 5 : 6);
}

// IID: get a best move (hoping for a first move cutoff) if we don't
// have a hash move available
void MoveOrder::scoreIIDMove() {
    // Sort the moves with what we have so far
    /*for (Move m = nextMove(); m != NULL_MOVE;
              m = nextMove());
    index = 0;*/

    int iidDepth = isPVNode ? depth-2 : (depth - 3) / 3;
    int bestIndex = getBestMoveForSort(b, legalMoves, iidDepth);
    // Mate check to prevent crashes
    if (bestIndex == -1) {
        legalMoves.clear();
        mgStage = STAGE_QUIETS;
    }
    else {
        scores.add(SCORE_IID_MOVE);
        if (isCapture(legalMoves.get(bestIndex))) {
            legalMoves.swap(0, bestIndex);
        }
        else {
            legalMoves.swap(quietStart, bestIndex);
            legalMoves.swap(0, quietStart);
            quietStart++;
        }
    }
}

void MoveOrder::generateQuiets() {
    mgStage = STAGE_QUIETS;
    MoveList legalQuiets = b->getPseudoLegalQuiets(color, *pml);

    for (unsigned int i = 0; i < legalQuiets.size(); i++)
        legalMoves.add(legalQuiets.get(i));
    scoreQuiets();
}

// Retrieves the next move with the highest score, starting from index using a
// partial selection sort. This way, the entire list does not have to be sorted
// if an early cutoff occurs.
Move MoveOrder::nextMove() {
    // Special case when we have a hash move available
    if (mgStage == STAGE_HASH_MOVE)
        return hashed;
    if (mgStage == STAGE_IID_MOVE) {
        generateMoves();
        index++;
        return legalMoves.get(0);
    }
    // If we are the end of our generated list, generate more.
    // If there are no moves left, return NULL_MOVE to indicate so.
    while (index >= legalMoves.size()) {
        if (mgStage == STAGE_QUIETS)
            return NULL_MOVE;
        else {
            generateMoves();
        }
    }

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

    // Once we've gotten to even captures, we need to generate quiets since
    // some quiets (killers, promotions) should be searched first.
    if ((mgStage == STAGE_CAPTURES || mgStage == STAGE_SCORE_QUIETS)
     && bestScore < SCORE_WINNING_CAPTURE)
        generateMoves();

    return legalMoves.get(index++);
}

// When a PV or cut move is found, the histories of all
// quiet moves searched prior to the best move are reduced
void MoveOrder::reduceBadHistories(Move bestMove) {
    // If we searched only the hash move, return to prevent crashes
    if (index <= 0)
        return;
    for (unsigned int i = 0; i < index-1; i++) {
        if (legalMoves.get(i) == bestMove)
            break;
        if (isCapture(legalMoves.get(i)))
            continue;
        searchParams->historyTable[color][b->getPieceOnSquare(color, getStartSq(legalMoves.get(i)))][getEndSq(legalMoves.get(i))] -= depth;
    }
}

void MoveOrder::findQuietStart() {
    for (unsigned int i = 0; i < legalMoves.size(); i++) {
        if (!isCapture(legalMoves.get(i))) {
            quietStart = i;
            return;
        }
    }

    // If there are no quiets
    quietStart = legalMoves.size();
}