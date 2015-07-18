#include <chrono>
#include "search.h"

Hash transpositionTable(16);
uint64_t nodes;

int getBestMoveAtDepth(Board *b, MoveList &legalMoves, int depth,
    int &bestScore, bool &isMate);
void sortSearch(Board *b, MoveList &pseudoLegalMoves, ScoreList &scores, int depth);
int PVS(Board &b, int color, int depth, int alpha, int beta);
int quiescence(Board &b, int color, int plies, int alpha, int beta, bool isCheckLine);
int checkQuiescence(Board &b, int color, int plies, int alpha, int beta);
void sort(MoveList &moves, ScoreList &scores, int left, int right);
void swap(MoveList &moves, ScoreList &scores, int i, int j);
int partition(MoveList &moves, ScoreList &scores, int left, int right,
    int pindex);

string retrievePV(Board *b, Move bestMove, int plies);

// Iterative deepening search
Move getBestMove(Board *b, int mode, int value) {
    using namespace std::chrono;
    nodes = 0;

    // test if only 1 legal move
    int color = b->getPlayerToMove();
    MoveList legalMoves = b->getAllLegalMoves(color);
    if (legalMoves.size() == 1) return legalMoves.get(0);
    
    auto start_time = high_resolution_clock::now();
    bool isMate = false;
    int currentBestMove;
    int bestScore;
    
    if (mode == TIME) {
        double timeFactor = 0.4; // timeFactor = log b / (b - 1) where b is branch factor
        int i = 1;
        do {
            currentBestMove = getBestMoveAtDepth(b, legalMoves, i, bestScore, isMate);
            Move temp = legalMoves.get(0);
            legalMoves.set(0, legalMoves.get(currentBestMove));
            legalMoves.set(currentBestMove, temp);

            double timeSoFar = duration_cast<duration<double>>(
                    high_resolution_clock::now() - start_time).count();
            uint64_t nps = (uint64_t) ((double) nodes / timeSoFar);

            string pvStr = retrievePV(b, legalMoves.get(0), i);

            cout << "info depth " << i << " score cp " << bestScore << " time "
                << (int)(timeSoFar * ONE_SECOND) << " nodes " << nodes
                << " nps " << nps << " pv " << pvStr << endl;

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
            Move temp = legalMoves.get(0);
            legalMoves.set(0, legalMoves.get(currentBestMove));
            legalMoves.set(currentBestMove, temp);

            double timeSoFar = duration_cast<duration<double>>(
                    high_resolution_clock::now() - start_time).count();
            uint64_t nps = (uint64_t) ((double) nodes / timeSoFar);

            string pvStr = retrievePV(b, legalMoves.get(0), i);

            cout << "info depth " << i << " score cp " << bestScore << " time "
                 << (int)(timeSoFar * ONE_SECOND) << " nodes " << nodes
                 << " nps " << nps << " pv " << pvStr << endl;
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
        nodes++;
        
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
        else if (copy.isStalemate(color^1)) {
            score = 0;
            if (score > alpha) {
                alpha = score;
                tempMove = i;
            }
            continue;
        }
        
        if (i != 0) {
            score = -PVS(copy, color^1, depth-1, -alpha-1, -alpha);
            if (alpha < score && score < beta) {
                score = -PVS(copy, color^1, depth-1, -beta, -alpha);
            }
        }
        else {
            score = -PVS(copy, color^1, depth-1, -beta, -alpha);
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

void sortSearch(Board *b, MoveList &legalMoves, ScoreList &scores, int depth) {
    int color = b->getPlayerToMove();
    
    for (unsigned int i = 0; i < legalMoves.size(); i++) {
        Board copy = b->staticCopy();
        if(!copy.doPseudoLegalMove(legalMoves.get(i), color)) {
            legalMoves.remove(i);
            i--;
            continue;
        }
        
        if (copy.isWinMate() || copy.isBinMate()) {
            scores.add(MATE_SCORE);
            continue;
        }
        else if (copy.isStalemate(color^1)) {
            scores.add(0);
            continue;
        }
        
        scores.add(-PVS(copy, color^1, depth-1, -MATE_SCORE, MATE_SCORE));
    }
}

// The standard implementation of a null-window PVS search.
// The implementation is fail-hard (score returned must be within [alpha, beta])
int PVS(Board &b, int color, int depth, int alpha, int beta) {
    // When the standard search is done, enter quiescence search.
    // Static board evaluation is done there.
    if (depth <= 0) {
        return quiescence(b, color, 0, alpha, beta, false);
    }
    
    int score = -MATE_SCORE;
    int prevAlpha = alpha;

    Move toHash = NULL_MOVE;
    // See if a hash move exists
    int hashDepth = 0;
    int hashScore = 0;
    uint8_t nodeType = 0;
    Move hashed = transpositionTable.get(b, hashDepth, hashScore, nodeType);

    if (hashed != NULL_MOVE) {
        Board copy = b.staticCopy();
        if (copy.doHashMove(hashed, color)) {
            if (hashDepth >= depth) {
                if (nodeType == CUT_NODE && hashScore >= beta)
                    return beta;
                else if (nodeType == PV_NODE)
                    return hashScore;
                else if (nodeType == ALL_NODE && hashScore <= alpha)
                    return alpha;
            }

            nodes++;
            score = -PVS(copy, color^1, depth-1, -beta, -alpha);

            if (score >= beta)
                return beta;
            if (score > alpha)
                alpha = score;
        }
        else {
            cerr << "Type-1 TT error" << endl;
            hashed = NULL_MOVE;
        }
    }

    // if (hashed != NULL_MOVE)
    //     cerr << "Type-1 TT error" << endl;
    
    // null move pruning
    // only if doing a null move does not leave player in check
    if (depth >= 2) {
        if (b.doPseudoLegalMove(NULL_MOVE, color)) {
            int nullScore = -PVS(b, color^1, depth-4, -beta, -alpha);
            if (nullScore >= beta) {
                b.doMove(NULL_MOVE, color^1);
                return beta;
            }
        }
        // Undo the null move
        b.doMove(NULL_MOVE, color^1);
    }

    MoveList legalMoves = b.getAllPseudoLegalMoves(color);

    if(depth >= 4) {
        ScoreList scores;
        sortSearch(&b, legalMoves, scores, 1);
        sort(legalMoves, scores, 0, legalMoves.size() - 1);
    }

    int reduction = 0;
    for (unsigned int i = 0; i < legalMoves.size(); i++) {
        Move m = legalMoves.get(i);
        if (m == hashed)
            continue;

        reduction = 0;
        Board copy = b.staticCopy();
        if (depth < 4) {
            if (!copy.doPseudoLegalMove(m, color))
                continue;
        }
        else {
            copy.doMove(m, color);
        }

        nodes++;
        if(depth > 1 && b.getSEE(color, getEndSq(m)) < -200)
            reduction = 2;
        //else if(i > 2 && alpha <= prevAlpha)
        //    reduction = 1;

        if (i != 0) {
            score = -PVS(copy, color^1, depth-1-reduction, -alpha-1, -alpha);
            if (alpha < score && score < beta) {
                score = -PVS(copy, color^1, depth-1-reduction, -beta, -alpha);
            }
        }
        else {
            score = -PVS(copy, color^1, depth-1-reduction, -beta, -alpha);
        }
        
        if (score >= beta) {
            if (depth >= 2)
                transpositionTable.add(b, depth, m, score, CUT_NODE);
            return beta;
        }
        if (score > alpha) {
            alpha = score;
            toHash = m;
        }
    }
    
    // Special cases for a mate or stalemate
    if (score == -MATE_SCORE) {
        if (b.isWinMate()) {
            // + 50 - depth adjusts so that quicker mates are better
            score = (color == WHITE) ? (-MATE_SCORE + 50 - depth) : (MATE_SCORE - 50 + depth);
        }
        else if (b.isBinMate()) {
            score = (color == WHITE) ? (MATE_SCORE - 50 + depth) : (-MATE_SCORE + 50 - depth);
        }
        else if (b.isStalemate(color)) {
            score = 0;
        }
        
        if (score >= beta)
            return beta;
        if (score > alpha)
            alpha = score;
    }
    
    // Exact scores indicate a principal variation and should be hashed
    if (prevAlpha < alpha && alpha < beta) {
        transpositionTable.add(b, depth, toHash, alpha, PV_NODE);
    }
    //else if (depth >= 3 && alpha <= prevAlpha) {
    //    transpositionTable.add(b, depth, toHash, alpha, ALL_NODE);
    //}

    return alpha;
}

/* Quiescence search, which completes all capture and check lines (thus reaching
 * a "quiet" position.)
 * This diminishes the horizon effect and greatly improves playing strength.
 * Delta pruning and static-exchange evaluation are used to reduce the time
 * spent here.
 * The search is done within a fail-hard framework (alpha <= score <= beta)
 */
int quiescence(Board &b, int color, int plies, int alpha, int beta, bool isCheckLine) {
    if (b.isStalemate(color))
        return 0;
    if (b.getInCheck(color))
        return checkQuiescence(b, color, plies, alpha, beta);

    // Stand pat: if our current position is already way too good or way too bad
    // we can simply stop the search here
    int standPat = (color == WHITE) ? b.evaluate() : -b.evaluate();
    if (standPat >= beta)
        return beta;
    if (alpha < standPat)
        alpha = standPat;
    
    // delta prune
    if (standPat < alpha - 1200)
        return alpha;
    
//    if (!isCheckLine) {
        MoveList legalCaptures = b.getPseudoLegalCaptures(color);
        
        for (unsigned int i = 0; i < legalCaptures.size(); i++) {
            Move m = legalCaptures.get(i);

            // Static exchange evaluation pruning
            if(b.getSEE(color, getEndSq(m)) < -200)
                continue;

            Board copy = b.staticCopy();
            if (!copy.doPseudoLegalMove(m, color))
                continue;
            
            nodes++;
            int score = -quiescence(copy, color^1, plies+1, -beta, -alpha, false);
            
            if (score >= beta) {
                alpha = beta;
                break;
            }
            if (score > alpha)
                alpha = score;
        }
//    }

/* TODO needs a getChecks function
    // Checks
    else if(plies <= 4) {
        MoveList legalMoves = b.getAllPseudoLegalMoves(color);

        for (unsigned int i = 0; i < legalMoves.size(); i++) {
            Move m = legalMoves.get(i);

            Board copy = b.staticCopy();
            if (!copy.doPseudoLegalMove(m, color))
                continue;
            if (!copy.getInCheck(color^1))
                continue;
            
            int score = -checkQuiescence(copy, color^1, plies+1, -beta, -alpha);
            
            if (score >= beta) {
                alpha = beta;
                break;
            }
            if (score > alpha)
                alpha = score;
        }
    }
*/
    return alpha;
}

/*
 * When checks are considered in quiescence, the responses must include all moves,
 * not just captures, necessitating this function.
 */
int checkQuiescence(Board &b, int color, int plies, int alpha, int beta) {
    MoveList legalMoves = b.getAllPseudoLegalMoves(color);

    for (unsigned int i = 0; i < legalMoves.size(); i++) {
        Move m = legalMoves.get(i);

        Board copy = b.staticCopy();
        if (!copy.doPseudoLegalMove(m, color))
            continue;
        
        nodes++;
        int score = -quiescence(copy, color^1, plies+1, -beta, -alpha, true);
        
        if (score >= beta) {
            alpha = beta;
            break;
        }
        if (score > alpha)
            alpha = score;
    }
    
    return alpha;
}

void clearTranspositionTable() {
    transpositionTable.clear();
}

// Basic quicksort implementation. The lists are both sorted by scores, with
// moves simply changing to mirror the changes to scores.
void sort(MoveList &moves, ScoreList &scores, int left, int right) {
    int pivot = (left + right) / 2;

    if (left < right) {
        pivot = partition(moves, scores, left, right, pivot);
        sort(moves, scores, left, pivot-1);
        sort(moves, scores, pivot+1, right);
    }
}

void swap(MoveList &moves, ScoreList &scores, int i, int j) {
    int temp = moves.get(j);
    moves.set(j, moves.get(i));
    moves.set(i, temp);

    scores.swap(i, j);
}

int partition(MoveList &moves, ScoreList &scores, int left, int right,
    int pindex) {

    int index = left;
    int pivot = scores.get(pindex);

    swap(moves, scores, pindex, right);

    for (int i = left; i < right; i++) {
        if (scores.get(i) > pivot) {
            swap(moves, scores, i, index);
            index++;
        }
    }
    swap(moves, scores, index, right);

    return index;
}

// Recover PV for outputting to terminal / GUI using transposition table entries
string retrievePV(Board *b, Move bestMove, int plies) {
    int hashDepth = 0;
    int hashScore = 0;
    uint8_t nodeType = 0;
    Board copy = b->staticCopy();
    string pvStr = moveToString(bestMove);
    pvStr += " ";
    copy.doMove(bestMove, copy.getPlayerToMove());

    Move hashed = transpositionTable.get(copy, hashDepth, hashScore, nodeType);
    int lineLength = 0;
    while (hashed != NULL_MOVE) {
        pvStr += moveToString(hashed);
        pvStr += " ";
        copy.doMove(hashed, copy.getPlayerToMove());
        hashed = transpositionTable.get(copy, hashDepth, hashScore, nodeType);
        lineLength++;
        if (lineLength > plies)
            break;
    }

    return pvStr;
}