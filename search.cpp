#include <chrono>
#include "search.h"

Hash transpositionTable(16);

int getBestMoveAtDepth(Board *b, MoveList &legalMoves, int depth,
    int &bestScore, bool &isMate);
int sortSearch(Board *b, MoveList &pseudoLegalMoves, int depth);
int PVS(Board &b, int color, int depth, int alpha, int beta);
int quiescence(Board &b, int color, int alpha, int beta);

// Iterative deepening search
Move *getBestMove(Board *b, int mode, int value) {
    using namespace std::chrono;
    
    // test if only 1 legal move
    int color = b->getPlayerToMove();
    MoveList legalMoves = b->getAllLegalMoves(color);
    if (legalMoves.size() == 1) return legalMoves.get(0);
    
    auto start_time = high_resolution_clock::now();
    bool isMate = false;
    int currentBestMove;
    int bestScore;
    
    // cerr << "value is " << value << endl;
    
    double timeFactor = 0.4; // timeFactor = log b / (b - 1) where b is branch factor
    
    if (mode == TIME) {
        int i = 1;
        do {
            currentBestMove = getBestMoveAtDepth(b, legalMoves, i, bestScore, isMate);
            Move *temp = legalMoves.get(0);
            legalMoves.set(0, legalMoves.get(currentBestMove));
            legalMoves.set(currentBestMove, temp);

            double timeSoFar = duration_cast<duration<double>>(
                    high_resolution_clock::now() - start_time).count();
            cerr << "info depth " << i << " score cp " << bestScore << " time "
                << (int)(timeSoFar * ONE_SECOND) << " nodes 1 nps 1000 pv e2e4" << endl;

            if (isMate)
                break;
            i++;
        }
        while ((duration_cast<duration<double>>(high_resolution_clock::now() - start_time).count() * ONE_SECOND
            < value * timeFactor) && (i <= MAX_DEPTH));
    }
    
    if (mode == DEPTH) {
        for (int i = 1; i <= min(value, MAX_DEPTH); i++) {
            currentBestMove = getBestMoveAtDepth(b, legalMoves, i, bestScore, isMate);
            Move *temp = legalMoves.get(0);
            legalMoves.set(0, legalMoves.get(currentBestMove));
            legalMoves.set(currentBestMove, temp);

            double timeSoFar = duration_cast<duration<double>>(
                    high_resolution_clock::now() - start_time).count();
            cerr << "info depth " << i << " score cp " << bestScore << " time "
                 << (int)(timeSoFar * ONE_SECOND) << " nodes 1 nps 1000 pv e2e4" << endl;
            // transpositionTable.test();

            if (isMate)
                break;
        }
    }
    
    transpositionTable.clean(b->getMoveNumber());
    cerr << "keys: " << transpositionTable.keys << endl;
    return legalMoves.get(0);
}

int getBestMoveAtDepth(Board *b, MoveList &legalMoves, int depth,
        int &bestScore, bool &isMate) {
    int color = b->getPlayerToMove();
    
    unsigned int tempMove = 0;
    int score = -MATE_SCORE;
    int alpha = -MATE_SCORE;
    int beta = MATE_SCORE;
    
    for (unsigned int i = 0; i < legalMoves.size(); i++) {
        Board copy = b->staticCopy();
        copy.doMove(legalMoves.get(i), color);
        
        // debug code
        // cerr << "considering move: " << legalMoves.get(i)->startsq << ", " << legalMoves.get(i)->endsq << endl;
        
        if (copy.isWinMate()) {
            isMate = true;
            bestScore = MATE_SCORE;
            return i;
        }
        else if (copy.isBinMate()) {
            isMate = true;
            bestScore = MATE_SCORE;
            return i;
        }
        else if (copy.isStalemate(color)) {
            score = 0;
            if (score > alpha) {
                alpha = score;
                tempMove = i;
            }
            continue;
        }
        
        if (i != 0) {
            score = -PVS(copy, -color, depth-1, -alpha-1, -alpha);
            if (alpha < score && score < beta) {
                score = -PVS(copy, -color, depth-1, -beta, -alpha);
            }
        }
        else {
            score = -PVS(copy, -color, depth-1, -beta, -alpha);
        }
        
        if (score > alpha) {
            alpha = score;
            tempMove = i;
        }
    }

    // If a mate has been found, indicate so
    if(alpha > MATE_SCORE - 300)
        isMate = true;
    bestScore = alpha;

    return tempMove;
}

int sortSearch(Board *b, MoveList &legalMoves, int depth) {
    int color = b->getPlayerToMove();
    
    unsigned int tempMove = 0;
    int score = -MATE_SCORE;
    int alpha = -MATE_SCORE;
    int beta = MATE_SCORE;
    
    for (unsigned int i = 0; i < legalMoves.size(); i++) {
        Board copy = b->staticCopy();
        if(!copy.doPLMove(legalMoves.get(i), color))
            continue;
        
        if (copy.isWinMate())
            return i;
        else if (copy.isBinMate())
            return i;
        else if (copy.isStalemate(color)) {
            score = 0;
            if (score > alpha) {
                alpha = score;
                tempMove = i;
            }
            continue;
        }
        
        if (i != 0) {
            score = -PVS(copy, -color, depth-1, -alpha-1, -alpha);
            if (alpha < score && score < beta) {
                score = -PVS(copy, -color, depth-1, -beta, -alpha);
            }
        }
        else {
            score = -PVS(copy, -color, depth-1, -beta, -alpha);
        }
        
        if (score > alpha) {
            alpha = score;
            tempMove = i;
        }
    }

    return tempMove;
}

// The standard implementation of a null-window PVS search.
// The implementation is fail-soft (score returned can be outside [alpha, beta])
int PVS(Board &b, int color, int depth, int alpha, int beta) {
    // When the standard search is done, enter quiescence search.
    // Static board evaluation is done there.
    if (depth <= 0) {
        return quiescence(b, color, alpha, beta);
    }
    
    int score = -MATE_SCORE;
    int bestScore = -MATE_SCORE;
    Move *toHash = NULL;
    
    // null move pruning
    // only if doing a null move does not leave player in check
    if (b.doPLMove(NULL, color)) {
        int nullScore = -PVS(b, -color, depth-4, -beta, -alpha);
        if (nullScore >= beta)
            return beta;
    }
    // Undo the null move
    b.doMove(NULL, -color);
    
    // Basic move ordering: check captures first
    MoveList legalCaptures = b.getPLCaptures(color);

    // See if a hash move exists
    Move *hashed = transpositionTable.get(b);
    if (hashed != NULL) {
        for (unsigned int i = 0; i < legalCaptures.size(); i++) {
            Move *test = legalCaptures.get(i);
            // Check legality
            if(test->piece == hashed->piece && test->startsq == hashed->startsq
            && test->endsq == hashed->endsq && test->isCapture == hashed->isCapture) {
                // Search this move first
                Board copy = b.staticCopy();
                if (!copy.doPLMove(hashed, color))
                    break;
                legalCaptures.remove(i);
                score = -PVS(copy, -color, depth-1, -beta, -alpha);
                if (score >= beta)
                    return score;
                if (score > bestScore) {
                    bestScore = score;
                    if (score > alpha) {
                        alpha = score;
                    }
                }
                // Set NULL to indicate we have used the hash move now
                hashed = NULL;
                break;
            }
        }
    }

    // Internal sort search
    if(depth >= 4) {
        int pv = sortSearch(&b, legalCaptures, 1);
        Move *temp = legalCaptures.get(0);
        legalCaptures.set(0, legalCaptures.get(pv));
        legalCaptures.set(pv, temp);
    }

    for (unsigned int i = 0; i < legalCaptures.size(); i++) {
        Board copy = b.staticCopy();
        if (!copy.doPLMove(legalCaptures.get(i), color))
            continue;

        if (i != 0) {
            score = -PVS(copy, -color, depth-1, -alpha-1, -alpha);
            if (alpha < score && score < beta) {
                score = -PVS(copy, -color, depth-1, -beta, -alpha);
            }
        }
        else {
            score = -PVS(copy, -color, depth-1, -beta, -alpha);
        }
        
        if (score >= beta) {
            transpositionTable.add(b, depth, legalCaptures.get(i));
            legalCaptures.free();
            return score;
        }
        if (score > bestScore) {
            bestScore = score;
            if (score > alpha) {
                alpha = score;
                toHash = legalCaptures.get(i);
            }
        }
    }
    
    MoveList legalMoves = b.getPseudoLegalMoves(color);

    if (hashed != NULL) {
        for (unsigned int i = 0; i < legalMoves.size(); i++) {
            Move *test = legalMoves.get(i);
            if(test->piece == hashed->piece && test->startsq == hashed->startsq
            && test->endsq == hashed->endsq && test->isCapture == hashed->isCapture) {
                Board copy = b.staticCopy();
                if (!copy.doPLMove(hashed, color))
                    break;
                legalMoves.remove(i);
                score = -PVS(copy, -color, depth-1, -beta, -alpha);
                if (score >= beta)
                    return score;
                if (score > bestScore) {
                    bestScore = score;
                    if (score > alpha) {
                        alpha = score;
                    }
                }
                hashed = NULL;
                break;
            }
        }
    }

    if(depth >= 4) {
        int pv = sortSearch(&b, legalMoves, 1);
        Move *temp = legalMoves.get(0);
        legalMoves.set(0, legalMoves.get(pv));
        legalMoves.set(pv, temp);
    }

    for (unsigned int i = 0; i < legalMoves.size(); i++) {
        Board copy = b.staticCopy();
        if (!copy.doPLMove(legalMoves.get(i), color))
            continue;

        if (i != 0) {
            score = -PVS(copy, -color, depth-1, -alpha-1, -alpha);
            if (alpha < score && score < beta) {
                score = -PVS(copy, -color, depth-1, -beta, -alpha);
            }
        }
        else {
            score = -PVS(copy, -color, depth-1, -beta, -alpha);
        }
        
        if (score >= beta) {
            transpositionTable.add(b, depth, legalMoves.get(i));
            legalMoves.free();
            return score;
        }
        if (score > bestScore) {
            bestScore = score;
            if (score > alpha) {
                alpha = score;
                toHash = legalMoves.get(i);
            }
        }
    }
    
    // Special cases for a mate or stalemate
    if (score == -MATE_SCORE) {
        if (b.isWinMate()) {
            // + 50 - depth adjusts so that quicker mates are better
            score = (-MATE_SCORE + 50 - depth) * color;
        }
        else if (b.isBinMate()) {
            score = (MATE_SCORE - 50 + depth) * color;
        }
        else if (b.isStalemate(color)) {
            score = 0;
        }
        
        if (score > bestScore) {
            bestScore = score;
            if (score > alpha) {
                alpha = score;
            }
        }
    }
    
    // Exact scores indicate a principal variation and should be hashed
    if (alpha < score && score < beta)
        transpositionTable.add(b, depth, toHash);

    legalCaptures.free();
    legalMoves.free();

    return bestScore;
}

/* Quiescence search, which completes all capture lines.
 * This diminishes the horizon effect and greatly improves playing strength.
 * Delta pruning and static-exchange evaluation are used to reduce the time
 * spent here.
 * The search is done within a fail-hard framework (alpha <= score <= beta)
 */
int quiescence(Board &b, int color, int alpha, int beta) {
    // debug code
    // if (b.getMoveNumber() > 25) cerr << b.getMoveNumber() << endl;
    
    // Stand pat: if our current position is already way too good or way too bad
    // we can simply stop the search here
    int standPat = color * b.evaluate();
    if (standPat >= beta) {
        return beta;
    }
    if (alpha < standPat)
        alpha = standPat;
    
    // delta prune
    if (standPat < alpha - 1200)
        return alpha;
    
    MoveList legalCaptures = b.getPLCaptures(color);
    
    for (unsigned int i = 0; i < legalCaptures.size(); i++) {
        Move *m = legalCaptures.get(i);

        // Static exchange evaluation pruning
        if(b.getSEE(color, m->endsq) < -200)
            continue;

        Board copy = b.staticCopy();
        if (!copy.doPLMove(m, color))
            continue;
        
        int score = -quiescence(copy, -color, -beta, -alpha);
        
        if (score >= beta) {
            alpha = beta;
            break;
        }
        if (score > alpha)
            alpha = score;
    }
    
    legalCaptures.free();

    return alpha;
}
