/*
    Laser, a UCI chess engine written in C++11.
    Copyright 2015-2018 Jeffrey An and Michael An

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

constexpr int SCORE_IID_MOVE = (1 << 20);
constexpr int SCORE_WINNING_CAPTURE = (1 << 18);
constexpr int SCORE_QUEEN_PROMO = (1 << 17);
constexpr int SCORE_EVEN_CAPTURE = (1 << 16);
constexpr int SCORE_QUIET_MOVE = -(1 << 30);
constexpr int SCORE_LOSING_CAPTURE = -(1 << 30) - (1 << 28);


MoveOrder::MoveOrder(Board *_b, int _color, int _depth, SearchParameters *_searchParams,
    SearchStackInfo *_ssi, Move _hashed, MoveList _legalMoves) {
	b = _b;
	color = _color;
	depth = _depth;
	searchParams = _searchParams;
    ssi = _ssi;
    mgStage = STAGE_NONE;
    quietStart = 0;
    index = 0;
    hashed = _hashed;
    legalMoves = _legalMoves;
}

MoveOrder::MoveOrder(Board *_b, int _color, int _depth, SearchParameters *_searchParams) {
    b = _b;
    color = _color;
    depth = _depth;
    searchParams = _searchParams;
    ssi = nullptr;
    mgStage = STAGE_QS_CAPTURES;
    quietStart = 0;
    index = 0;
    hashed = NULL_MOVE;
}

// Returns true if there are still moves remaining, false if we have
// generated all moves already
void MoveOrder::generateMoves() {
    switch (mgStage) {
        // The hash move, if any, is handled separately from the rest of the list
        case STAGE_NONE:
            if (hashed != NULL_MOVE) {
                mgStage = STAGE_HASH_MOVE;

                // Remove the hash move from the list, since it has already been tried
                for (unsigned int i = 0; i < legalMoves.size(); i++) {
                    if (legalMoves.get(i) == hashed) {
                        legalMoves.remove(i);
                        break;
                    }
                }

                break;
            }
            // else fallthrough

        // If we just searched the hash move (or there is none), we need to find
        // where the quiet moves start in the list, and then score captures.
        case STAGE_HASH_MOVE:
            findQuietStart();
            mgStage = STAGE_CAPTURES;
            scoreCaptures();
            break;

        // After winnning captures, we score quiets
        case STAGE_CAPTURES:
            mgStage = STAGE_QUIETS;
            scoreQuiets();
            break;

        // We are done
        case STAGE_QUIETS:
            break;


        // QS captures, not including promotions
        case STAGE_QS_CAPTURES:
            mgStage = STAGE_QS_PROMOTIONS;
            b->getPseudoLegalCaptures(legalMoves, color, false);
            for (unsigned int i = 0; i < legalMoves.size(); i++) {
                scores.add(b->getMVVLVAScore(color, legalMoves.get(i)));
            }
            break;

        // QS promotions. We do not sort these, so fill the scores with junk
        case STAGE_QS_PROMOTIONS:
            mgStage = STAGE_QS_CHECKS;
            b->getPseudoLegalPromotions(legalMoves, color);
            for (unsigned int i = index; i < legalMoves.size(); i++)
                scores.add(0);
            break;

        // QS checks: only on the first ply of q-search. We do not sort these, so fill the scores with junk
        case STAGE_QS_CHECKS:
            mgStage = STAGE_QS_DONE;
            if (depth == 0) {
                b->getPseudoLegalChecks(legalMoves, color);
                for (unsigned int i = index; i < legalMoves.size(); i++)
                    scores.add(0);
            }
            break;

        case STAGE_QS_DONE:
            break;
    }
}

// Sort captures using SEE and MVV/LVA
void MoveOrder::scoreCaptures() {
    for (unsigned int i = 0; i < quietStart; i++) {
        Move m = legalMoves.get(i);
        int startSq = getStartSq(m);
        int endSq = getEndSq(m);
        int pieceID = b->getPieceOnSquare(color, startSq);
        int capturedID = b->getPieceOnSquare(color, endSq);
        if (capturedID == -1) capturedID = 0;

        int adjustedMVVLVA = 8 * b->getMVVLVAScore(color, m) / (4 + depth);
        if (b->isSEEAbove(color, m, 1))
            scores.add(SCORE_WINNING_CAPTURE + adjustedMVVLVA + searchParams->captureHistory[color][pieceID][capturedID][endSq]);
        else if (b->isSEEAbove(color, m, 0))
            scores.add(SCORE_EVEN_CAPTURE + adjustedMVVLVA + searchParams->captureHistory[color][pieceID][capturedID][endSq]);
        else
            scores.add(SCORE_LOSING_CAPTURE + adjustedMVVLVA + searchParams->captureHistory[color][pieceID][capturedID][endSq]);
    }
}

void MoveOrder::scoreQuiets() {
    for (unsigned int i = quietStart; i < legalMoves.size(); i++) {
        Move m = legalMoves.get(i);

        // Score killers below even captures but above losing captures
        if (m == searchParams->killers[ssi->ply][0])
            scores.add(SCORE_EVEN_CAPTURE - 1);

        // Order queen promotions somewhat high
        else if (getPromotion(m) == QUEENS)
            scores.add(SCORE_QUEEN_PROMO);

        // Sort all other quiet moves by history
        else {
            int startSq = getStartSq(m);
            int endSq = getEndSq(m);
            int pieceID = b->getPieceOnSquare(color, startSq);

            scores.add(SCORE_QUIET_MOVE
                + searchParams->historyTable[color][pieceID][endSq]
                + ((ssi->counterMoveHistory != nullptr) ? ssi->counterMoveHistory[pieceID][endSq] : 0)
                + ((ssi->followupMoveHistory != nullptr) ? ssi->followupMoveHistory[pieceID][endSq] : 0));
        }
    }
}

// Retrieves the next move with the highest score, starting from index using a
// partial selection sort. This way, the entire list does not have to be sorted
// if an early cutoff occurs.
Move MoveOrder::nextMove() {
    // Special case when we have a hash move available
    if (mgStage == STAGE_HASH_MOVE)
        return hashed;

    // If we are the end of our generated list, generate more.
    // If there are no moves left, return NULL_MOVE to indicate so.
    while (index >= scores.size()) {
        if (mgStage == STAGE_QUIETS || mgStage == STAGE_QS_DONE)
            return NULL_MOVE;
        else {
            generateMoves();
        }
    }

    // Find the index of the next best move
    int bestIndex = index;
    int bestScore = scores.get(index);
    for (unsigned int i = index + 1; i < scores.size(); i++) {
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
    if (mgStage == STAGE_CAPTURES && bestScore < SCORE_WINNING_CAPTURE)
        generateMoves();

    return legalMoves.get(index++);
}

// When a PV or cut move is found, the history of the best move in increased,
// and the histories of all moves searched prior to the best move are reduced.
void MoveOrder::updateHistories(Move bestMove) {
    if (depth > 18) return;
    const int resetFactor = 448;
    int historyChange = depth * depth + 5 * depth - 2;

    int startSq = getStartSq(bestMove);
    int endSq = getEndSq(bestMove);
    int pieceID = b->getPieceOnSquare(color, startSq);

    // Increase history for the best move
    searchParams->historyTable[color][pieceID][endSq] -=
        historyChange * searchParams->historyTable[color][pieceID][endSq] / resetFactor;
    searchParams->historyTable[color][pieceID][endSq] += historyChange;
    if (ssi->counterMoveHistory != nullptr) {
        ssi->counterMoveHistory[pieceID][endSq] -=
            historyChange * ssi->counterMoveHistory[pieceID][endSq] / resetFactor;
        ssi->counterMoveHistory[pieceID][endSq] += historyChange;
    }
    if (ssi->followupMoveHistory != nullptr) {
        ssi->followupMoveHistory[pieceID][endSq] -=
            historyChange * ssi->followupMoveHistory[pieceID][endSq] / resetFactor;
        ssi->followupMoveHistory[pieceID][endSq] += historyChange;
    }

    // If we searched only the hash move, return to prevent crashes
    if (index <= 0)
        return;
    for (unsigned int i = 0; i < index-1; i++) {
        if (legalMoves.get(i) == bestMove)
            break;

        startSq = getStartSq(legalMoves.get(i));
        endSq = getEndSq(legalMoves.get(i));
        pieceID = b->getPieceOnSquare(color, startSq);

        if (isCapture(legalMoves.get(i))) {
            int capturedID = b->getPieceOnSquare(color, endSq);
            if (capturedID == -1) capturedID = 0;

            searchParams->captureHistory[color][pieceID][capturedID][endSq] -=
                historyChange * searchParams->captureHistory[color][pieceID][capturedID][endSq] / resetFactor;
            searchParams->captureHistory[color][pieceID][capturedID][endSq] -= historyChange;
        }
        else {
            searchParams->historyTable[color][pieceID][endSq] -=
                historyChange * searchParams->historyTable[color][pieceID][endSq] / resetFactor;
            searchParams->historyTable[color][pieceID][endSq] -= historyChange;
            if (ssi->counterMoveHistory != nullptr) {
                ssi->counterMoveHistory[pieceID][endSq] -=
                    historyChange * ssi->counterMoveHistory[pieceID][endSq] / resetFactor;
                ssi->counterMoveHistory[pieceID][endSq] -= historyChange;
            }
            if (ssi->followupMoveHistory != nullptr) {
                ssi->followupMoveHistory[pieceID][endSq] -=
                    historyChange * ssi->followupMoveHistory[pieceID][endSq] / resetFactor;
                ssi->followupMoveHistory[pieceID][endSq] -= historyChange;
            }
        }
    }
}

void MoveOrder::updateCaptureHistories(Move bestMove) {
    if (depth > 18) return;
    const int resetFactor = 448;
    int historyChange = depth * depth + 5 * depth - 2;

    int startSq = getStartSq(bestMove);
    int endSq = getEndSq(bestMove);
    int pieceID = b->getPieceOnSquare(color, startSq);
    int capturedID = b->getPieceOnSquare(color, endSq);
    if (capturedID == -1) capturedID = 0;

    searchParams->captureHistory[color][pieceID][capturedID][endSq] -=
        historyChange * searchParams->captureHistory[color][pieceID][capturedID][endSq] / resetFactor;
    searchParams->captureHistory[color][pieceID][capturedID][endSq] += historyChange;

    // If we searched only the hash move, return to prevent crashes
    if (index <= 0)
        return;
    for (unsigned int i = 0; i < index-1; i++) {
        if (legalMoves.get(i) == bestMove)
            break;
        if (!isCapture(legalMoves.get(i)))
            continue;

        startSq = getStartSq(legalMoves.get(i));
        endSq = getEndSq(legalMoves.get(i));
        pieceID = b->getPieceOnSquare(color, startSq);
        capturedID = b->getPieceOnSquare(color, endSq);
        if (capturedID == -1) capturedID = 0;

        searchParams->captureHistory[color][pieceID][capturedID][endSq] -=
            historyChange * searchParams->captureHistory[color][pieceID][capturedID][endSq] / resetFactor;
        searchParams->captureHistory[color][pieceID][capturedID][endSq] -= historyChange;
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