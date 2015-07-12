#include <chrono>
#include "search.h"

Move *getBestMoveAtDepth(Board *b, int depth, bool &isMate);
int PVS(Board b, int color, int depth, int alpha, int beta);
int quiescence(Board b, int color, int alpha, int beta);

// Iterative deepening search
Move *getBestMove(Board *b, int mode, int value) {
    using namespace std::chrono;
    
    // test if only 1 legal move
    int color = b->getPlayerToMove();
    MoveList legalMoves = b->getAllLegalMoves(color);
    if (legalMoves.size() == 1) return legalMoves.get(0);
    
    auto start_time = high_resolution_clock::now();
    bool isMate = false;
    Move *currentBestMove = getBestMoveAtDepth(b, 1, isMate);
    if(isMate) return currentBestMove;
    
    // cerr << "value is " << value << endl;
    // cerr << duration_cast<duration<double>>(high_resolution_clock::now() - start_time).count() << endl;
    
    double timeFactor = 0.25;
    
    if (mode == TIME) {
        int i = 2;
        while ((duration_cast<duration<double>>(high_resolution_clock::now() - start_time).count() * ONE_SECOND
            < value * timeFactor) && (i <= MAX_DEPTH)) {
            isMate = false;
            currentBestMove = getBestMoveAtDepth(b, i, isMate);
            if(isMate)
                return currentBestMove;
            // cerr << duration_cast<duration<double>>(high_resolution_clock::now() - start_time).count() << endl;
            i++;
        }
    }
    
    if (mode == DEPTH) {
        for (int i = 2; i <= min(value, MAX_DEPTH); i++) {
            isMate = false;
            currentBestMove = getBestMoveAtDepth(b, i, isMate);
            if(isMate)
                return currentBestMove;
            // cerr << duration_cast<duration<double>>(high_resolution_clock::now() - start_time).count() << endl;
        }
    }
    
    return currentBestMove;
}

Move *getBestMoveAtDepth(Board *b, int depth, bool &isMate) {
    int color = b->getPlayerToMove();
    MoveList legalMoves = b->getAllLegalMoves(color);
    if (legalMoves.size() == 1) return legalMoves.get(0);
    
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
            return legalMoves.get(i);
        }
        else if (copy.isBinMate()) {
            isMate = true;
            return legalMoves.get(i);
        }
        else if (copy.isStalemate(color)) {
            score = 0;
            if (score > alpha) {
                alpha = score;
                tempMove = i;
            }
            if (alpha >= beta)
                break;
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
        if (alpha >= beta)
            break;
    }

    // If a mate has been found, indicate so
    if(alpha > MATE_SCORE - 300)
        isMate = true;
    // TODO leaks memory from movelist...
    return legalMoves.get(tempMove);
}

// The standard implementation of a null-window PVS search.
int PVS(Board b, int color, int depth, int alpha, int beta) {
    // When the standard search is done, enter quiescence search.
    // Static board evaluation is done there.
    if (depth <= 0) {
        return quiescence(b, color, alpha, beta);
    }
    
    int score = -MATE_SCORE;
    
    /*Move hashed = tTable.get(b);
    if (hashed != null) {
        Board copy = b.getCopy();
        copy.doMove(hashed, color);
        score = -PVS(copy, -color, depth-1, -beta, -alpha);
        if (score > alpha)
            alpha = score;
        if (alpha >= beta)
            return alpha;
    }*/
    
    // null move pruning
    /*
    if ((color == WHITE) ? !b.getWinCheck() : !b.getBinCheck()) {
        int nullScore = -PVS(b, -color, depth-4, -beta, -alpha);
        if (nullScore >= beta)
            return beta;
    }
    */
    
    // Basic move ordering: check captures first
    MoveList legalCaptures = b.getPLCaptures(color);
    
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
        
        if (score > alpha) {
            alpha = score;
            //hashed = legalCaptures.get(i);
        }
        if (alpha >= beta) {
            // if (hashed != null)
            // tTable.put(b, hashed);
            legalCaptures.free();
            return alpha;
        }
    }
    
    MoveList legalMoves = b.getPseudoLegalMoves(color);
    
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
        
        if (score > alpha) {
            alpha = score;
            //hashed = legalMoves.get(i);
        }
        if (alpha >= beta)
            break;
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
        
        if (score > alpha) {
            alpha = score;
        }
    }
    
    // if (hashed != null)
    // tTable.put(b, hashed);

    legalCaptures.free();
    legalMoves.free();

    return alpha;
}

/* Quiescence search, which completes all capture lines.
 * This diminishes the horizon effect and greatly improves playing strength.
 */
int quiescence(Board b, int color, int alpha, int beta) {
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
        Board copy = b.staticCopy();
        if (!copy.doPLMove(legalCaptures.get(i), color))
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
