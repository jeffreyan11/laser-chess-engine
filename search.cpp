#include "search.h"

int PVS(Board b, int color, int depth, int alpha, int beta);
int quiescence(Board b, int color, int alpha, int beta);

Move *getBestMove(Board *b, int depth) {
    int color = b->getPlayerToMove();
    MoveList legalMoves = b->getAllLegalMoves(color);
    
    unsigned int tempMove = 0;
    int score = -99999;
    int alpha = -99999;
    int beta = 99999;
    
    for (unsigned int i = 0; i < legalMoves.size(); i++) {
        Board copy = b->staticCopy();
        copy.doMove(legalMoves.get(i), color);
        
        // debug code
        // cerr << "considering move: " << legalMoves.get(i)->startsq << ", " << legalMoves.get(i)->endsq << endl;
        
        if(copy.isWinMate()) {
            return legalMoves.get(i);
        }
        else if(copy.isBinMate()) {
            return legalMoves.get(i);
        }
        else if(copy.isStalemate(color)) {
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
    // TODO leaks memory from movelist...
    return legalMoves.get(tempMove);
}

// The standard implementation of a null-window PVS search.
int PVS(Board b, int color, int depth, int alpha, int beta) {
    if(depth <= 0) {
        return quiescence(b, color, alpha, beta);
    }
    
    int score = -99999;
    
    /*Move hashed = tTable.get(b);
    if(hashed != null) {
        Board copy = b.getCopy();
        copy.doMove(hashed, color);
        score = -PVS(copy, -color, depth-1, -beta, -alpha);
        if(score > alpha)
            alpha = score;
        if (alpha >= beta)
            return alpha;
    }*/
    
    // null move pruning
    /*
    if((color == WHITE) ? !b.getWinCheck() : !b.getBinCheck()) {
        int nullScore = -PVS(b, -color, depth-4, -beta, -alpha);
        if(nullScore >= beta)
            return beta;
    }
    */
    
    MoveList legalCaptures = b.getPLCaptures(color);
    
    for (unsigned int i = 0; i < legalCaptures.size(); i++) {
        Board copy = b.staticCopy();
        if(!copy.doPLMove(legalCaptures.get(i), color))
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
            //if(hashed != null)
            //  tTable.put(b, hashed);
            return alpha;
        }
    }
    
    MoveList legalMoves = b.getPseudoLegalMoves(color);
    
    for (unsigned int i = 0; i < legalMoves.size(); i++) {
        Board copy = b.staticCopy();
        if(!copy.doPLMove(legalMoves.get(i), color))
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
    
    if(score == -99999) {
        if(b.isWinMate()) {
            score = (-99999 + 50 - depth) * color;
        }
        else if(b.isBinMate()) {
            score = (99999 - 50 + depth) * color;
        }
        else if(b.isStalemate(color)) {
            score = 0;
        }
        
        if (score > alpha) {
            alpha = score;
        }
    }
    
    //if(hashed != null)
    //  tTable.put(b, hashed);
    return alpha;
}

/* Quiescence search, which completes all capture lines.
 * This diminishes the horizon effect and greatly improves playing strength.
 */
int quiescence(Board b, int color, int alpha, int beta) {
    // debug code
    // if (b.getMoveNumber() > 25) cerr << b.getMoveNumber() << endl;
    
    int standPat = color * b.evaluate();
    if(standPat >= beta) {
        return beta;
    }
    if(alpha < standPat)
        alpha = standPat;
    
    // delta prune
    if(standPat < alpha - 1200)
        return alpha;
    
    MoveList legalCaptures = b.getPLCaptures(color);
    
    for (unsigned int i = 0; i < legalCaptures.size(); i++) {
        Board copy = b.staticCopy();
        if(!copy.doPLMove(legalCaptures.get(i), color))
            continue;
        
        int score = -quiescence(copy, -color, -beta, -alpha);
        
        if(score >= beta)
            return beta;
        if(score > alpha)
            alpha = score;
    }
    
    return alpha;
}
