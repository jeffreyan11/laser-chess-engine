#include <chrono>
#include "search.h"

struct SearchParameters {
    int nullMoveCount;
    Move killers[MAX_DEPTH][2];

    SearchParameters() {
        reset();
    }

    void reset() {
        nullMoveCount = 0;
        for (int i = 0; i < MAX_DEPTH; i++) {
            killers[i][0] = NULL_MOVE;
            killers[i][1] = NULL_MOVE;
        }
    }
};

static Hash transpositionTable(16);
static int rootDepth;
static uint8_t searchGen;
static SearchParameters searchParams;
static SearchStatistics searchStats;

extern bool isStop;

unsigned int getBestMoveAtDepth(Board *b, MoveList &legalMoves, int depth,
    int &bestScore, bool &isMate);
int getBestMoveForSort(Board &b, MoveList &legalMoves, int depth);
void sortSearch(Board &b, MoveList &pseudoLegalMoves, ScoreList &scores, int depth);
int PVS(Board &b, int color, int depth, int alpha, int beta);
int quiescence(Board &b, int color, int plies, int alpha, int beta);
int checkQuiescence(Board &b, int color, int plies, int alpha, int beta);

int probeTT(Board &b, int color, Move &hashed, int depth, int &alpha, int beta);
int scoreMate(bool isInCheck, int depth, int alpha, int beta);

Move nextMove(MoveList &moves, ScoreList &scores, unsigned int index);
string retrievePV(Board *b, Move bestMove, int plies);

void getBestMove(Board *b, int mode, int value, SearchStatistics *stats, Move *bestMove) {
    using namespace std::chrono;
    searchParams.reset();
    searchStats.reset();
    searchGen = (uint8_t) (b->getMoveNumber());

    int color = b->getPlayerToMove();
    MoveList legalMoves = b->getAllLegalMoves(color);
    *bestMove = legalMoves.get(0);

    /*if (legalMoves.size() == 1) {
        isStop = true;
        cout << "bestmove " << moveToString(*bestMove) << endl;
        return;
    }*/
    
    auto start_time = high_resolution_clock::now();
    double timeSoFar = duration_cast<duration<double>>(
            high_resolution_clock::now() - start_time).count();
    bool isMate = false;
    int bestScore, bestMoveIndex;
    
    if (mode == TIME) {
        double timeFactor = 0.4; // timeFactor = log b / (b - 1) where b is branch factor
        rootDepth = 1;
        do {
            bestMoveIndex = getBestMoveAtDepth(b, legalMoves, rootDepth, bestScore, isMate);
            if (bestMoveIndex == -1)
                break;
            legalMoves.swap(0, bestMoveIndex);
            *bestMove = legalMoves.get(0);

            timeSoFar = duration_cast<duration<double>>(
                    high_resolution_clock::now() - start_time).count();
            uint64_t nps = (uint64_t) ((double) searchStats.nodes / timeSoFar);

            string pvStr = retrievePV(b, *bestMove, rootDepth);

            cout << "info depth " << rootDepth << " score cp " << bestScore << " time "
                << (int)(timeSoFar * ONE_SECOND) << " nodes " << searchStats.nodes
                << " nps " << nps << " pv " << pvStr << endl;

            if (isMate)
                break;
            rootDepth++;
        }
        while ((timeSoFar * ONE_SECOND < value * timeFactor) && (rootDepth <= MAX_DEPTH));
    }
    
    else if (mode == DEPTH) {
        for (rootDepth = 1; rootDepth <= min(value, MAX_DEPTH); rootDepth++) {
            bestMoveIndex = getBestMoveAtDepth(b, legalMoves, rootDepth, bestScore, isMate);
            if (bestMoveIndex == -1)
                break;
            legalMoves.swap(0, bestMoveIndex);
            *bestMove = legalMoves.get(0);

            timeSoFar = duration_cast<duration<double>>(
                    high_resolution_clock::now() - start_time).count();
            uint64_t nps = (uint64_t) ((double) searchStats.nodes / timeSoFar);

            string pvStr = retrievePV(b, *bestMove, rootDepth);

            cout << "info depth " << rootDepth << " score cp " << bestScore << " time "
                 << (int)(timeSoFar * ONE_SECOND) << " nodes " << searchStats.nodes
                 << " nps " << nps << " pv " << pvStr << endl;
            // transpositionTable.test();

            if (isMate)
                break;
        }
    }
    
    #if HASH_DEBUG_OUTPUT
    cerr << "collisions: " << transpositionTable.collisions << endl;
    cerr << "replacements: " << transpositionTable.replacements << endl;
    #endif
    //transpositionTable.clean(b->getMoveNumber());
    cerr << "TT occupancy: " << transpositionTable.keys << "/" << transpositionTable.getSize() << endl;
    cerr << "Hash scorecut/hit/probe: " << searchStats.hashScoreCuts << "/"
         << searchStats.hashHits << "/" << searchStats.hashProbes << endl;
    cerr << "Hash movecut rate: " << searchStats.hashMoveCuts << "/"
         << searchStats.hashMoveAttempts << endl;
    cerr << "First fail-high/fail-high rate: " << searchStats.firstFailHighs << "/"
         << searchStats.failHighs << "/" << searchStats.searchSpaces << endl;
    cerr << "QS Nodes: " << searchStats.qsNodes << endl;
    cerr << "QS FFH/FH rate: " << searchStats.qsFirstFailHighs << "/"
         << searchStats.qsFailHighs << "/" << searchStats.qsSearchSpaces << endl;
    
    isStop = true;
    cout << "bestmove " << moveToString(*bestMove) << endl;
    *stats = searchStats;
    return;
}

// returns index of best move in legalMoves
unsigned int getBestMoveAtDepth(Board *b, MoveList &legalMoves, int depth,
        int &bestScore, bool &isMate) {
    int color = b->getPlayerToMove();
    searchParams.reset();
    
    unsigned int tempMove = -1;
    int score = -MATE_SCORE;
    int alpha = -MATE_SCORE;
    int beta = MATE_SCORE;

    // root move ordering
    /*if (depth == 1) {
        ScoreList scores;
        sortSearch(b, legalMoves, scores, 1);
        unsigned int i = 0;
        for (Move m = nextMove(legalMoves, scores, i); m != NULL_MOVE;
                m = nextMove(legalMoves, scores, ++i));
    }*/
    
    for (unsigned int i = 0; i < legalMoves.size(); i++) {
        // Stop condition. If stopping, return search results from incomplete
        // search, if any.
        if (isStop) {
            return tempMove;
        }
        Board copy = b->staticCopy();
        copy.doMove(legalMoves.get(i), color);
        searchStats.nodes++;
        
        if (copy.isWInMate()) {
            isMate = true;
            bestScore = MATE_SCORE - 1;
            return i;
        }
        else if (copy.isBInMate()) {
            isMate = true;
            bestScore = MATE_SCORE - 1;
            return i;
        }
        else if (copy.isStalemate(color^1) || copy.isDraw()) {
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
    if(alpha >= MATE_SCORE - MAX_DEPTH)
        isMate = true;
    bestScore = alpha;

    return tempMove;
}

// Gets a sorting score for every move by searching each with a full window PVS.
void sortSearch(Board &b, MoveList &legalMoves, ScoreList &scores, int depth) {
    int color = b.getPlayerToMove();
    
    for (unsigned int i = 0; i < legalMoves.size(); i++) {
        Board copy = b.staticCopy();
        if(!copy.doPseudoLegalMove(legalMoves.get(i), color)) {
            legalMoves.remove(i);
            i--;
            continue;
        }
        
        scores.add(-PVS(copy, color^1, depth-1, -MATE_SCORE, MATE_SCORE));
    }
}

// Gets a best move to try first when a hash move is not available.
int getBestMoveForSort(Board &b, MoveList &legalMoves, int depth) {
    int color = b.getPlayerToMove();
    
    int tempMove = -1;
    int score = -MATE_SCORE;
    int alpha = -MATE_SCORE;
    int beta = MATE_SCORE;
    
    for (unsigned int i = 0; i < legalMoves.size(); i++) {
        Board copy = b.staticCopy();
        if(!copy.doPseudoLegalMove(legalMoves.get(i), color))
            continue;
        
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

    return tempMove;
}

//------------------------------------------------------------------------------
//------------------------------Search functions--------------------------------
//------------------------------------------------------------------------------

// The standard implementation of a null-window PVS search.
// The implementation is fail-hard (score returned must be within [alpha, beta])
int PVS(Board &b, int color, int depth, int alpha, int beta) {
    // When the standard search is done, enter quiescence search.
    // Static board evaluation is done there.
    if (depth <= 0) {
        return quiescence(b, color, 0, alpha, beta);
    }

    if (b.isDraw()) {
        if (0 >= beta)
            return beta;
        if (0 > alpha)
            return 0;
        else
            return alpha;
    }
    
    int score;
    int prevAlpha = alpha;
    // For PVS, the node is a PV node if beta - alpha > 1 (i.e. not a null window)
    // We do not want to do most pruning techniques on PV nodes
    bool isPVNode = (beta - alpha > 1);
    // Similarly, we do not want to prune if we are in check
    bool isInCheck = b.isInCheck(color);

    Move hashed = NULL_MOVE;
    // Probe the hash table for a match/cutoff
    if (!isPVNode) {
        searchStats.hashProbes++;
        score = probeTT(b, color, hashed, depth, alpha, beta);
        if (score != -INFTY)
            return score;
    }

    // A static evaluation, currently only used for null move pruning
    // May in the future be used for futility pruning/razoring/etc.
    int staticEval = (color == WHITE) ? b.evaluate() : -b.evaluate();
    
    // null move pruning
    // only if doing a null move does not leave player in check
    // Possibly remove staticEval >= beta condition?
    if (depth >= 3 && !isPVNode && searchParams.nullMoveCount < 2 && staticEval >= beta && !isInCheck) {
        int reduction;
        if (depth >= 8)
            reduction = 3;
        else if (depth >= 5)
            reduction = 2;
        else
            reduction = 1;

        b.doMove(NULL_MOVE, color);
        searchParams.nullMoveCount++;
        int nullScore = -PVS(b, color^1, depth-1-reduction, -beta, -alpha);
        if (nullScore >= beta) {
            b.doMove(NULL_MOVE, color^1);
            searchParams.nullMoveCount--;
            return beta;
        }
        
        // Undo the null move
        b.doMove(NULL_MOVE, color^1);
        searchParams.nullMoveCount--;
    }

    MoveList legalMoves = isInCheck ? b.getPseudoLegalCheckEscapes(color)
                                    : b.getAllPseudoLegalMoves(color);

    // If there were no pseudo-legal moves
    // This is an early check to prevent sorting from crashing
    if (legalMoves.size() == 0)
        return scoreMate(isInCheck, depth, alpha, beta);

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

    ScoreList scores;
    // Internal iterative deepening, SEE, and MVV/LVA move ordering
    // The scoring relies partially on the fact that our selection sort is stable
    // Do a full sort on PV nodes since good move ordering is the most important here
    if (depth >= 8 && isPVNode) {
        sortSearch(b, legalMoves, scores, 2);
    }
    else if (depth >= 4 && isPVNode) {
        sortSearch(b, legalMoves, scores, 1);
    }
    // Otherwise, get a best move (hoping for a first move cutoff) if we don't have
    // a hash move available
    else if (depth >= 4 && hashed == NULL_MOVE) {
        int bestIndex = getBestMoveForSort(b, legalMoves, (depth >= 9) ? 2 : 1);
        // Mate check to prevent crashes
        if (bestIndex == -1)
            return scoreMate(isInCheck, depth, alpha, beta);

        unsigned int index;
        for (index = 0; index < legalMoves.size(); index++)
            if (!isCapture(legalMoves.get(index)))
                break;
        for (unsigned int i = 0; i < index; i++) {
            scores.add(b.getSEE(color, getEndSq(legalMoves.get(i))));
        }
        // Score killers below even captures but above losing captures
        for (unsigned int i = index; i < legalMoves.size(); i++) {
            if (legalMoves.get(i) == searchParams.killers[depth][0])
                scores.add(0);
            else if (legalMoves.get(i) == searchParams.killers[depth][1])
                scores.add(-1);
            else
                scores.add(-MATE_SCORE);
        }
        scores.set(bestIndex, (1 << 15));
    }
    else if (depth >= 2) { // sort by SEE
        unsigned int index;
        for (index = 0; index < legalMoves.size(); index++)
            if (!isCapture(legalMoves.get(index)))
                break;
        for (unsigned int i = 0; i < index; i++) {
            scores.add(b.getSEE(color, getEndSq(legalMoves.get(i))));
        }
        for (unsigned int i = index; i < legalMoves.size(); i++) {
            if (legalMoves.get(i) == searchParams.killers[depth][0])
                scores.add(0);
            else if (legalMoves.get(i) == searchParams.killers[depth][1])
                scores.add(-1);
            else
                scores.add(-MATE_SCORE);
        }
    }
    else { // MVV/LVA
        unsigned int index;
        for (index = 0; index < legalMoves.size(); index++)
            if (!isCapture(legalMoves.get(index)))
                break;
        for (unsigned int i = 0; i < index; i++) {
            scores.add(b.getMVVLVAScore(color, legalMoves.get(i)));
        }
        for (unsigned int i = index; i < legalMoves.size(); i++) {
            if (legalMoves.get(i) == searchParams.killers[depth][0])
                scores.add(-64);
            else if (legalMoves.get(i) == searchParams.killers[depth][1])
                scores.add(-65);
            else
                scores.add(-MATE_SCORE);
        }
    }

    Move toHash = NULL_MOVE;
    int reduction = 0;
    unsigned int i = 0;
    unsigned int j = 0; // separate counter only incremented when valid move is searched
    score = -INFTY;
    searchStats.searchSpaces++;
    for (Move m = nextMove(legalMoves, scores, i); m != NULL_MOVE;
              m = nextMove(legalMoves, scores, ++i)) {
        //if (m == hashed)
        //    continue;
        // Stop condition to help break out as quickly as possible
        if (isStop)
            return -INFTY;

        reduction = 0;
        Board copy = b.staticCopy();
        if (!copy.doPseudoLegalMove(m, color))
            continue;
        searchStats.nodes++;

        // Futility pruning
        // Needs better check detection
        if(depth == 1 && !isPVNode && staticEval <= alpha - MAX_POS_SCORE && !isInCheck && !isCapture(m) && !copy.isInCheck(color^1) /*&& b.getSEE(color, getEndSq(m)) < -MAX_POS_SCORE*/) {
            score = alpha;
            continue;
        }

        // Late move reduction
        if(!isPVNode && !isInCheck && !isCapture(m) && depth >= 3 && j > 2 && alpha <= prevAlpha) {
            if (depth >= 6)
                reduction = 2;
            else
                reduction = 1;
        }

        if (j != 0) {
            score = -PVS(copy, color^1, depth-1-reduction, -alpha-1, -alpha);
            // The re-search is always done at normal depth
            if (alpha < score && score < beta) {
                score = -PVS(copy, color^1, depth-1, -beta, -alpha);
            }
        }
        else {
            // The first move is always searched at a normal depth
            score = -PVS(copy, color^1, depth-1, -beta, -alpha);
        }
        
        if (score >= beta) {
            searchStats.failHighs++;
            if (j == 0)
                searchStats.firstFailHighs++;
            // Hash moves that caused a beta cutoff
            transpositionTable.add(b, depth, m, beta, CUT_NODE, searchGen);
            // Record killer if applicable
            if (!isCapture(m)) {
                if (m != searchParams.killers[depth][0]) {
                    searchParams.killers[depth][1] = searchParams.killers[depth][0];
                    searchParams.killers[depth][0] = m;
                }
            }
            return beta;
        }
        if (score > alpha) {
            alpha = score;
            toHash = m;
        }
        j++;
    }

    // If there were no legal moves
    if (score == -INFTY)
        return scoreMate(isInCheck, depth, alpha, beta);
    
    // Exact scores indicate a principal variation and should always be hashed
    if (toHash != NULL_MOVE && prevAlpha < alpha && alpha < beta) {
        transpositionTable.add(b, depth, toHash, alpha, PV_NODE, searchGen);
    }
    // Record all-nodes. The upper bound score can save a lot of search time.
    // No best move can be recorded in a fail-hard framework.
    else if (alpha <= prevAlpha) {
        transpositionTable.add(b, depth, NULL_MOVE, alpha, ALL_NODE, searchGen);
    }

    return alpha;
}

// See if a hash move exists.
int probeTT(Board &b, int color, Move &hashed, int depth, int &alpha, int beta) {
    HashEntry *entry = transpositionTable.get(b);
    if (entry != NULL) {
        searchStats.hashHits++;
        // If the node is a predicted all node and score <= alpha, return alpha
        // since score is an upper bound
        // Vulnerable to Type-1 errors
        int hashScore = entry->score;
        uint8_t nodeType = entry->getNodeType();
        if (nodeType == ALL_NODE) {
            if (entry->depth >= depth && hashScore <= alpha) {
                searchStats.hashScoreCuts++;
                return alpha;
            }
        }
        else {
            hashed = entry->m;
            // Only used a hashed score if the search depth was at least
            // the current depth
            if (entry->depth >= depth) {
                // At cut nodes if hash score >= beta return beta since hash
                // score is a lower bound.
                if (nodeType == CUT_NODE && hashScore >= beta) {
                    searchStats.hashScoreCuts++;
                    return beta;
                }
                // At PV nodes we can simply return the exact score
                else if (nodeType == PV_NODE) {
                    searchStats.hashScoreCuts++;
                    return hashScore;
                }
            }
            Board copy = b.staticCopy();
            if (copy.doHashMove(hashed, color)) {
                // If the hash score is unusable and node is not a predicted
                // all-node, we can search the hash move first.
                if ((/*entry->depth >= 2 && */entry->depth + 2 >= depth) || nodeType == PV_NODE) {
                    searchStats.hashMoveAttempts++;
                    searchStats.nodes++;
                    int score = -PVS(copy, color^1, depth-1, -beta, -alpha);

                    if (score >= beta) {
                        searchStats.hashMoveCuts++;
                        return beta;
                    }
                    if (score > alpha)
                        alpha = score;
                }
                // Otherwise, we aren't using the hash move
                else
                    hashed = NULL_MOVE;
            }
            else {
                cerr << "Type-1 TT error on " << moveToString(hashed) << endl;
                hashed = NULL_MOVE;
            }
        }
    }
    return -INFTY;
}

// Used to get a score when we have realized that we have no legal moves.
int scoreMate(bool isInCheck, int depth, int alpha, int beta) {
    int score;
    // If we are in check, then checkmate
    if (isInCheck) {
        // Adjust score so that quicker mates are better
        score = (-MATE_SCORE + rootDepth - depth);
    }
    else { // else, it is a stalemate
        score = 0;
    }
    if (score >= beta)
        return beta;
    if (score > alpha)
        alpha = score;
    return alpha;
}

/* Quiescence search, which completes all capture and check lines (thus reaching
 * a "quiet" position.)
 * This diminishes the horizon effect and greatly improves playing strength.
 * Delta pruning and static-exchange evaluation are used to reduce the time
 * spent here.
 * The search is done within a fail-hard framework (alpha <= score <= beta)
 */
int quiescence(Board &b, int color, int plies, int alpha, int beta) {
    if (b.isInCheck(color))
        return checkQuiescence(b, color, plies, alpha, beta);
    if (b.isDraw()) {
        if (0 >= beta)
            return beta;
        if (0 > alpha)
            return 0;
        else
            return alpha;
    }

    // Stand pat: if our current position is already way too good or way too bad
    // we can simply stop the search here
    int standPat = (color == WHITE) ? b.evaluate() : -b.evaluate();
    // if (b.getEGFactor() >= EG_FACTOR_RES)
    //     return standPat;
    if (standPat >= beta)
        return beta;
    if (alpha < standPat)
        alpha = standPat;
    
    // delta prune
    if (standPat < alpha - MAX_POS_SCORE - QUEEN_VALUE)
        return alpha;
    
    MoveList legalCaptures = b.getPseudoLegalCaptures(color, false);
    ScoreList scores;
    for (unsigned int i = 0; i < legalCaptures.size(); i++) {
        scores.add(b.getMVVLVAScore(color, legalCaptures.get(i)));
    }
    
    int score = -INFTY;
    unsigned int i = 0;
    unsigned int j = 0; // separate counter only incremented when valid move is searched
    searchStats.qsSearchSpaces++;
    for (Move m = nextMove(legalCaptures, scores, i); m != NULL_MOVE;
              m = nextMove(legalCaptures, scores, ++i)) {
        // Delta prune
        if (standPat + b.valueOfPiece(b.getCapturedPiece(color^1, getEndSq(m))) < alpha - MAX_POS_SCORE)
            continue;
        // Static exchange evaluation pruning
        if (b.getExchangeScore(color, m) < 0 && b.getSEE(color, getEndSq(m)) < -MAX_POS_SCORE)
            continue;

        Board copy = b.staticCopy();
        if (!copy.doPseudoLegalMove(m, color))
            continue;
        
        searchStats.nodes++;
        searchStats.qsNodes++;
        score = -quiescence(copy, color^1, plies+1, -beta, -alpha);
        
        if (score >= beta) {
            searchStats.qsFailHighs++;
            if (j == 0)
                searchStats.qsFirstFailHighs++;
            return beta;
        }
        if (score > alpha)
            alpha = score;
        j++;
    }

    MoveList legalPromotions = b.getPseudoLegalPromotions(color);
    for (unsigned int i = 0; i < legalPromotions.size(); i++) {
        Move m = legalPromotions.get(i);

        // Static exchange evaluation pruning
        if(b.getSEE(color, getEndSq(m)) < 0)
            continue;

        Board copy = b.staticCopy();
        if (!copy.doPseudoLegalMove(m, color))
            continue;
        
        searchStats.nodes++;
        searchStats.qsNodes++;
        score = -quiescence(copy, color^1, plies+1, -beta, -alpha);
        
        if (score >= beta) {
            searchStats.qsFailHighs++;
            if (j == 0)
                searchStats.qsFirstFailHighs++;
            return beta;
        }
        if (score > alpha)
            alpha = score;
        j++;
    }

    // Checks
    if(plies <= 0) {
        MoveList legalMoves = b.getPseudoLegalChecks(color);

        for (unsigned int i = 0; i < legalMoves.size(); i++) {
            Move m = legalMoves.get(i);

            Board copy = b.staticCopy();
            if (!copy.doPseudoLegalMove(m, color))
                continue;
            
            searchStats.nodes++;
            searchStats.qsNodes++;
            int score = -checkQuiescence(copy, color^1, plies+1, -beta, -alpha);
            
            if (score >= beta) {
                searchStats.qsFailHighs++;
                if (j == 0)
                    searchStats.qsFirstFailHighs++;
                alpha = beta;
                return beta;
            }
            if (score > alpha)
                alpha = score;
            j++;
        }
    }

    // TODO This is too slow to be effective
/*    if (score == -INFTY) {
        if (b.isStalemate(color))
            return 0;
    }*/

    return alpha;
}

/*
 * When checks are considered in quiescence, the responses must include all moves,
 * not just captures, necessitating this function.
 */
int checkQuiescence(Board &b, int color, int plies, int alpha, int beta) {
    MoveList legalMoves = b.getPseudoLegalCheckEscapes(color);

    int score = -INFTY;

    searchStats.qsSearchSpaces++;
    unsigned int j = 0; // separate counter only incremented when valid move is searched
    for (unsigned int i = 0; i < legalMoves.size(); i++) {
        Move m = legalMoves.get(i);

        Board copy = b.staticCopy();
        if (!copy.doPseudoLegalMove(m, color))
            continue;
        
        searchStats.nodes++;
        searchStats.qsNodes++;
        score = -quiescence(copy, color^1, plies+1, -beta, -alpha);
        
        if (score >= beta) {
            searchStats.qsFailHighs++;
            if (j == 0)
                searchStats.qsFirstFailHighs++;
            return beta;
        }
        if (score > alpha)
            alpha = score;
        j++;
    }

    // If there were no legal moves
    if (score == -INFTY) {
        // We already know we are in check, so it must be a checkmate
        // Adjust score so that quicker mates are better
        score = (-MATE_SCORE + rootDepth + plies);
        if (score >= beta)
            return beta;
        if (score > alpha)
            alpha = score;
    }
    
    return alpha;
}

//------------------------------------------------------------------------------
//------------------------------Other functions---------------------------------
//------------------------------------------------------------------------------

void clearTranspositionTable() {
    transpositionTable.clear();
}

// Retrieves the next move with the highest score, starting from index using a
// partial selection sort. This way, the entire list does not have to be sorted
// if an early cutoff occurs.
Move nextMove(MoveList &moves, ScoreList &scores, unsigned int index) {
    if (index >= moves.size())
        return NULL_MOVE;
    // Find the index of the next best move
    int bestIndex = index;
    int bestScore = scores.get(index);
    for (unsigned int i = index + 1; i < moves.size(); i++) {
        if (scores.get(i) > bestScore) {
            bestIndex = i;
            bestScore = scores.get(bestIndex);
        }
    }
    // Swap the best move to the correct position
    moves.swap(bestIndex, index);
    scores.swap(bestIndex, index);
    return moves.get(index);
}

// Recover PV for outputting to terminal / GUI using transposition table entries
string retrievePV(Board *b, Move bestMove, int plies) {
    Board copy = b->staticCopy();
    string pvStr = moveToString(bestMove);
    pvStr += " ";
    copy.doMove(bestMove, copy.getPlayerToMove());

    HashEntry *entry = transpositionTable.get(copy);
    int lineLength = 1;
    while (entry != NULL && lineLength < plies) {
        if (entry->m == NULL_MOVE)
            break;
        pvStr += moveToString(entry->m);
        pvStr += " ";
        copy.doMove(entry->m, copy.getPlayerToMove());
        entry = transpositionTable.get(copy);
        lineLength++;
    }

    return pvStr;
}