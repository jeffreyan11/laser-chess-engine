/*
    Laser, a UCI chess engine written in C++11.
    Copyright 2015-2017 Jeffrey An and Michael An

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

#include <cstring>
#include <iomanip>
#include <iostream>
#include "bbinit.h"
#include "board.h"
#include "common.h"
#include "eval.h"
#include "uci.h"

static Score PSQT[2][6][64];

void initPSQT() {
    #define E(mg, eg) ((Score) ((((int32_t) eg) << 16) + ((int32_t) mg)))
    for (int pieceType = PAWNS; pieceType <= KINGS; pieceType++) {
        for (int sq = 0; sq < 32; sq++) {
            int r = sq / 4;
            int f = sq & 0x3;
            Score sc = E(pieceSquareTable[MG][pieceType][sq], pieceSquareTable[EG][pieceType][sq]);

            PSQT[WHITE][pieceType][8*(7-r) + f] = sc;
            PSQT[WHITE][pieceType][8*(7-r) + (7-f)] = sc;
            PSQT[BLACK][pieceType][8*r + f] = sc;
            PSQT[BLACK][pieceType][8*r + (7-f)] = sc;
        }
    }
    #undef E
}


struct EvalDebug {
    int totalEval;
    int totalMg, totalEg;
    int totalMaterialMg, totalMaterialEg;
    int totalImbalanceMg, totalImbalanceEg;
    Score whitePsqtScore, blackPsqtScore;
    int whiteMobilityMg, whiteMobilityEg, blackMobilityMg, blackMobilityEg;
    int whiteKingSafety, blackKingSafety;
    Score whitePieceScore, blackPieceScore;
    Score whiteThreatScore, blackThreatScore;
    Score whitePawnScore, blackPawnScore;

    EvalDebug() {
        clear();
    }

    void clear() {
        totalEval = 0;
        totalMg = totalEg = 0;
        totalMaterialMg = totalMaterialEg = 0;
        totalImbalanceMg = totalImbalanceEg = 0;
        whitePsqtScore = blackPsqtScore = EVAL_ZERO;
        whiteMobilityMg = whiteMobilityEg = blackMobilityMg = blackMobilityEg = 0;
        whiteKingSafety = blackKingSafety = 0;
        whitePieceScore = blackPieceScore = EVAL_ZERO;
        whiteThreatScore = blackThreatScore = EVAL_ZERO;
        whitePawnScore = blackPawnScore = EVAL_ZERO;
    }

    void print() {
        std::cerr << std::endl;
        std::cerr << "    Component     |      White      |      Black      |      Total" << std::endl;
        std::cerr << "                  |    MG     EG    |    MG     EG    |    MG     EG" << std::endl;
        std::cerr << std::string(70, '-') << std::endl;
        std::cerr << "    Material      |   "
                  << " -- " << "   "
                  << " -- " << "   |   "
                  << " -- " << "   "
                  << " -- " << "   |   "
                  << std::setw(4) << S(totalMaterialMg) << "   "
                  << std::setw(4) << S(totalMaterialEg) << std::endl;
        std::cerr << "    Imbalance     |   "
                  << " -- " << "   "
                  << " -- " << "   |   "
                  << " -- " << "   "
                  << " -- " << "   |   "
                  << std::setw(4) << S(totalImbalanceMg) << "   "
                  << std::setw(4) << S(totalImbalanceEg) << std::endl;
        std::cerr << "    PSQT          |   "
                  << std::setw(4) << S(decEvalMg(whitePsqtScore)) << "   "
                  << std::setw(4) << S(decEvalEg(whitePsqtScore)) << "   |   "
                  << std::setw(4) << S(decEvalMg(blackPsqtScore)) << "   "
                  << std::setw(4) << S(decEvalEg(blackPsqtScore)) << "   |   "
                  << std::setw(4) << S(decEvalMg(whitePsqtScore)) - S(decEvalMg(blackPsqtScore)) << "   "
                  << std::setw(4) << S(decEvalEg(whitePsqtScore)) - S(decEvalEg(blackPsqtScore)) << std::endl;
        std::cerr << "    Mobility      |   "
                  << std::setw(4) << S(whiteMobilityMg) << "   "
                  << std::setw(4) << S(whiteMobilityEg) << "   |   "
                  << std::setw(4) << S(blackMobilityMg) << "   "
                  << std::setw(4) << S(blackMobilityEg) << "   |   "
                  << std::setw(4) << S(whiteMobilityMg) - S(blackMobilityMg) << "   "
                  << std::setw(4) << S(whiteMobilityEg) - S(blackMobilityEg) << std::endl;
        std::cerr << "    King Safety   |   "
                  << std::setw(4) << S(whiteKingSafety) << "   "
                  << " -- " << "   |   "
                  << std::setw(4) << S(blackKingSafety) << "   "
                  << " -- " << "   |   "
                  << std::setw(4) << S(whiteKingSafety) - S(blackKingSafety) << "   "
                  << " -- " << std::endl;
        std::cerr << "    Pieces        |   "
                  << std::setw(4) << S(decEvalMg(whitePieceScore)) << "   "
                  << std::setw(4) << S(decEvalEg(whitePieceScore)) << "   |   "
                  << std::setw(4) << S(decEvalMg(blackPieceScore)) << "   "
                  << std::setw(4) << S(decEvalEg(blackPieceScore)) << "   |   "
                  << std::setw(4) << S(decEvalMg(whitePieceScore)) - S(decEvalMg(blackPieceScore)) << "   "
                  << std::setw(4) << S(decEvalEg(whitePieceScore)) - S(decEvalEg(blackPieceScore)) << std::endl;
        std::cerr << "    Threats       |   "
                  << std::setw(4) << S(decEvalMg(whiteThreatScore)) << "   "
                  << std::setw(4) << S(decEvalEg(whiteThreatScore)) << "   |   "
                  << std::setw(4) << S(decEvalMg(blackThreatScore)) << "   "
                  << std::setw(4) << S(decEvalEg(blackThreatScore)) << "   |   "
                  << std::setw(4) << S(decEvalMg(whiteThreatScore)) - S(decEvalMg(blackThreatScore)) << "   "
                  << std::setw(4) << S(decEvalEg(whiteThreatScore)) - S(decEvalEg(blackThreatScore)) << std::endl;
        std::cerr << "    Pawns         |   "
                  << std::setw(4) << S(decEvalMg(whitePawnScore)) << "   "
                  << std::setw(4) << S(decEvalEg(whitePawnScore)) << "   |   "
                  << std::setw(4) << S(decEvalMg(blackPawnScore)) << "   "
                  << std::setw(4) << S(decEvalEg(blackPawnScore)) << "   |   "
                  << std::setw(4) << S(decEvalMg(whitePawnScore)) - S(decEvalMg(blackPawnScore)) << "   "
                  << std::setw(4) << S(decEvalEg(whitePawnScore)) - S(decEvalEg(blackPawnScore)) << std::endl;
        std::cerr << std::string(70, '-') << std::endl;
        std::cerr << std::string(54, ' ') << "|  "
                  << std::setw(4) << S(totalMg) << "   "
                  << std::setw(4) << S(totalEg) << std::endl;
        std::cerr << "Static evaluation: " << S(totalEval) << std::endl;
        std::cerr << std::endl;
    }

    // Scales the internal score representation into centipawns
    int S(int v) {
        return (int) (v * 100 / PIECE_VALUES[EG][PAWNS]);
    }
};

static EvalDebug evalDebugStats;


static int scaleMaterial = DEFAULT_EVAL_SCALE;
static int scaleKingSafety = DEFAULT_EVAL_SCALE;

void setMaterialScale(int s) {
    scaleMaterial = s;
}
void setKingSafetyScale(int s) {
    scaleKingSafety = s;
}


/*
 * Evaluates the current board position in hundredths of pawns. White is
 * positive and black is negative in traditional negamax format.
 */
template <bool debug>
int Eval::evaluate(Board &b) {
    // Copy necessary values from Board
    for (int color = WHITE; color <= BLACK; color++) {
        for (int pieceID = PAWNS; pieceID <= KINGS; pieceID++)
            pieces[color][pieceID] = b.getPieces(color, pieceID);
    }
    allPieces[WHITE] = b.getAllPieces(WHITE);
    allPieces[BLACK] = b.getAllPieces(BLACK);
    playerToMove = b.getPlayerToMove();

    // Precompute some values
    int pieceCounts[2][6];
    for (int color = 0; color < 2; color++) {
        for (int pieceID = 0; pieceID < 6; pieceID++)
            pieceCounts[color][pieceID] = count(pieces[color][pieceID]);
    }

    // Material
    int whiteMaterial = 0, blackMaterial = 0;
    for (int pieceID = PAWNS; pieceID <= QUEENS; pieceID++)
        whiteMaterial += PIECE_VALUES[MG][pieceID] * pieceCounts[WHITE][pieceID];
    for (int pieceID = PAWNS; pieceID <= QUEENS; pieceID++)
        blackMaterial += PIECE_VALUES[MG][pieceID] * pieceCounts[BLACK][pieceID];

    int whiteEGFactorMat = 0, blackEGFactorMat = 0;
    for (int pieceID = PAWNS; pieceID <= QUEENS; pieceID++)
        whiteEGFactorMat += EG_FACTOR_PIECE_VALS[pieceID] * pieceCounts[WHITE][pieceID];
    for (int pieceID = PAWNS; pieceID <= QUEENS; pieceID++)
        blackEGFactorMat += EG_FACTOR_PIECE_VALS[pieceID] * pieceCounts[BLACK][pieceID];

    // Compute endgame factor which is between 0 and EG_FACTOR_RES, inclusive
    int egFactor = EG_FACTOR_RES - (whiteEGFactorMat + blackEGFactorMat - EG_FACTOR_ALPHA) * EG_FACTOR_RES / EG_FACTOR_BETA;
    egFactor = std::max(0, std::min(EG_FACTOR_RES, egFactor));


    // Check for special endgames
    if (egFactor == EG_FACTOR_RES) {
        int endgameScore = checkEndgameCases();
        if (endgameScore != -INFTY)
            return endgameScore;
    }


    // Precompute more values
    PieceMoveList pmlWhite = b.getPieceMoveList(WHITE);
    PieceMoveList pmlBlack = b.getPieceMoveList(BLACK);

    ei.clear();

    // Get the overall attack maps
    ei.attackMaps[WHITE][PAWNS] = b.getWPawnCaptures(pieces[WHITE][PAWNS]);
    for (unsigned int i = 0; i < pmlWhite.size(); i++)
        ei.attackMaps[WHITE][pmlWhite.get(i).pieceID] |= pmlWhite.get(i).legal;
    ei.attackMaps[BLACK][PAWNS] = b.getBPawnCaptures(pieces[BLACK][PAWNS]);
    for (unsigned int i = 0; i < pmlBlack.size(); i++)
        ei.attackMaps[BLACK][pmlBlack.get(i).pieceID] |= pmlBlack.get(i).legal;
    for (int color = WHITE; color <= BLACK; color++)
        for (int pieceID = KNIGHTS; pieceID <= QUEENS; pieceID++)
            ei.fullAttackMaps[color] |= ei.attackMaps[color][pieceID];

    ei.rammedPawns[WHITE] = pieces[WHITE][PAWNS] & (pieces[BLACK][PAWNS] >> 8);
    ei.rammedPawns[BLACK] = pieces[BLACK][PAWNS] & (pieces[WHITE][PAWNS] << 8);


    //---------------------------Material terms---------------------------------
    int valueMg = 0;
    int valueEg = 0;

    // Bishop pair bonus
    if ((pieces[WHITE][BISHOPS] & LIGHT) && (pieces[WHITE][BISHOPS] & DARK)) {
        whiteMaterial += BISHOP_PAIR_VALUE;
        valueEg += BISHOP_PAIR_VALUE;
    }
    if ((pieces[BLACK][BISHOPS] & LIGHT) && (pieces[BLACK][BISHOPS] & DARK)) {
        blackMaterial += BISHOP_PAIR_VALUE;
        valueEg -= BISHOP_PAIR_VALUE;
    }

    // MG and EG material
    valueMg += whiteMaterial;
    valueMg -= blackMaterial;
    for (int pieceID = PAWNS; pieceID <= QUEENS; pieceID++)
        valueEg += PIECE_VALUES[EG][pieceID] * pieceCounts[WHITE][pieceID];
    for (int pieceID = PAWNS; pieceID <= QUEENS; pieceID++)
        valueEg -= PIECE_VALUES[EG][pieceID] * pieceCounts[BLACK][pieceID];

    // Tempo bonus
    valueMg += (playerToMove == WHITE) ? TEMPO_VALUE : -TEMPO_VALUE;

    valueMg = valueMg * scaleMaterial / DEFAULT_EVAL_SCALE;
    valueEg = valueEg * scaleMaterial / DEFAULT_EVAL_SCALE;

    if (debug) {
        evalDebugStats.totalMaterialMg = valueMg;
        evalDebugStats.totalMaterialEg = valueEg;
    }


    int imbalanceValue[2] = {0, 0};
    // Material imbalance evaluation
    if (pieceCounts[WHITE][KNIGHTS] == 2) {
        imbalanceValue[MG] += KNIGHT_PAIR_PENALTY;
        imbalanceValue[EG] += KNIGHT_PAIR_PENALTY;
    }
    if (pieceCounts[BLACK][KNIGHTS] == 2) {
        imbalanceValue[MG] -= KNIGHT_PAIR_PENALTY;
        imbalanceValue[EG] -= KNIGHT_PAIR_PENALTY;
    }

    if (pieceCounts[WHITE][ROOKS] == 2) {
        imbalanceValue[MG] += ROOK_PAIR_PENALTY;
        imbalanceValue[EG] += ROOK_PAIR_PENALTY;
    }
    if (pieceCounts[BLACK][ROOKS] == 2) {
        imbalanceValue[MG] -= ROOK_PAIR_PENALTY;
        imbalanceValue[EG] -= ROOK_PAIR_PENALTY;
    }

    // Own-opp imbalance terms
    // Gain OWN_OPP_IMBALANCE[][ownID][oppID] centipawns for each ownID piece
    // you have and each oppID piece the opponent has
    for (int ownID = KNIGHTS; ownID <= QUEENS; ownID++) {
        for (int oppID = PAWNS; oppID < ownID; oppID++) {
            imbalanceValue[MG] += OWN_OPP_IMBALANCE[MG][ownID][oppID] * pieceCounts[WHITE][ownID] * pieceCounts[BLACK][oppID];
            imbalanceValue[EG] += OWN_OPP_IMBALANCE[EG][ownID][oppID] * pieceCounts[WHITE][ownID] * pieceCounts[BLACK][oppID];
            imbalanceValue[MG] -= OWN_OPP_IMBALANCE[MG][ownID][oppID] * pieceCounts[BLACK][ownID] * pieceCounts[WHITE][oppID];
            imbalanceValue[EG] -= OWN_OPP_IMBALANCE[EG][ownID][oppID] * pieceCounts[BLACK][ownID] * pieceCounts[WHITE][oppID];
        }
    }

    valueMg += imbalanceValue[MG] * scaleMaterial / DEFAULT_EVAL_SCALE;
    valueEg += imbalanceValue[EG] * scaleMaterial / DEFAULT_EVAL_SCALE;

    if (debug) {
        evalDebugStats.totalImbalanceMg = imbalanceValue[MG];
        evalDebugStats.totalImbalanceEg = imbalanceValue[EG];
    }

    // Increase knight value in closed positions
    int numRammedPawns = count(ei.rammedPawns[WHITE]);
    valueMg += KNIGHT_CLOSED_BONUS[MG] * pieceCounts[WHITE][KNIGHTS] * numRammedPawns * numRammedPawns / 4;
    valueEg += KNIGHT_CLOSED_BONUS[EG] * pieceCounts[WHITE][KNIGHTS] * numRammedPawns * numRammedPawns / 4;
    valueMg -= KNIGHT_CLOSED_BONUS[MG] * pieceCounts[BLACK][KNIGHTS] * numRammedPawns * numRammedPawns / 4;
    valueEg -= KNIGHT_CLOSED_BONUS[EG] * pieceCounts[BLACK][KNIGHTS] * numRammedPawns * numRammedPawns / 4;


    //----------------------------Positional terms------------------------------
    // Pawn piece square tables
    Score psqtScores[2] = {EVAL_ZERO, EVAL_ZERO};
    for (int color = WHITE; color <= BLACK; color++) {
        uint64_t bitboard = pieces[color][PAWNS];
        while (bitboard) {
            int sq = bitScanForward(bitboard);
            bitboard &= bitboard - 1;
            psqtScores[color] += PSQT[color][PAWNS][sq];
        }
    }
    // Queen piece square tables
    for (int color = WHITE; color <= BLACK; color++) {
        uint64_t bitboard = pieces[color][QUEENS];
        while (bitboard) {
            int sq = bitScanForward(bitboard);
            bitboard &= bitboard - 1;
            psqtScores[color] += PSQT[color][QUEENS][sq];
        }
    }

    //--------------------------------Mobility----------------------------------
    int whiteMobilityMg, whiteMobilityEg;
    int blackMobilityMg, blackMobilityEg;
    getMobility<WHITE>(pmlWhite, pmlBlack, whiteMobilityMg, whiteMobilityEg);
    getMobility<BLACK>(pmlBlack, pmlWhite, blackMobilityMg, blackMobilityEg);
    valueMg += whiteMobilityMg - blackMobilityMg;
    valueEg += whiteMobilityEg - blackMobilityEg;

    if (debug) {
        evalDebugStats.whiteMobilityMg = whiteMobilityMg;
        evalDebugStats.whiteMobilityEg = whiteMobilityEg;
        evalDebugStats.blackMobilityMg = blackMobilityMg;
        evalDebugStats.blackMobilityEg = blackMobilityEg;
    }


    //------------------------------King Safety---------------------------------
    int kingSq[2] = {bitScanForward(pieces[WHITE][KINGS]),
                     bitScanForward(pieces[BLACK][KINGS])};
    uint64_t kingNeighborhood[2] = {b.getKingSquares(kingSq[WHITE]),
                                    b.getKingSquares(kingSq[BLACK])};

    // Calculate king piece square table scores
    psqtScores[WHITE] += PSQT[WHITE][KINGS][kingSq[WHITE]];
    psqtScores[BLACK] += PSQT[BLACK][KINGS][kingSq[BLACK]];

    int ksValue[2] = {0, 0};

    // All king safety terms are midgame only, so don't calculate them in the endgame
    if (egFactor < EG_FACTOR_RES) {
        for (int color = WHITE; color <= BLACK; color++) {
            // Pawn shield and storm values: king file and the two adjacent files
            int kingFile = kingSq[color] & 7;
            kingFile = std::min(6, std::max(1, kingFile));
            for (int i = kingFile-1; i <= kingFile+1; i++) {
                int f = std::min(i, 7-i);

                uint64_t pawnShield = pieces[color][PAWNS] & FILES[i];
                if (pawnShield) {
                    int pawnSq = (color == WHITE) ? bitScanForward(pawnShield)
                                                  : bitScanReverse(pawnShield);
                    int r = relativeRank(color, pawnSq >> 3);

                    ksValue[color] += PAWN_SHIELD_VALUE[f][r];
                }
                // Semi-open file: no pawn shield
                else
                    ksValue[color] += PAWN_SHIELD_VALUE[f][0];

                uint64_t pawnStorm = pieces[color^1][PAWNS] & FILES[i];
                if (pawnStorm) {
                    int pawnSq = (color == WHITE) ? bitScanForward(pawnStorm)
                                                  : bitScanReverse(pawnStorm);
                    int r = relativeRank(color, pawnSq >> 3);
                    int stopSq = pawnSq + ((color == WHITE) ? -8 : 8);

                    ksValue[color] -= PAWN_STORM_VALUE[
                        (pieces[color][PAWNS] & FILES[i]) == 0             ? 0 :
                        (pieces[color][PAWNS] & INDEX_TO_BIT[stopSq]) != 0 ? 1 : 2][f][r];
                }
                // Semi-open file: no pawn for attacker
                else
                    ksValue[color] -= PAWN_STORM_VALUE[0][f][0];
            }

            // King pressure
            uint64_t KING_ZONE;
            if (kingFile < 3) KING_ZONE = FILE_A | FILE_B | FILE_C | FILE_D;
            else if (kingFile < 5) KING_ZONE = FILE_C | FILE_D | FILE_E | FILE_F;
            else KING_ZONE = FILE_E | FILE_F | FILE_G | FILE_H;
            if (color == WHITE)
                KING_ZONE &= RANK_1 | RANK_2 | RANK_3 | RANK_4 | RANK_5;
            else
                KING_ZONE &= RANK_4 | RANK_5 | RANK_6 | RANK_7 | RANK_8;

            ksValue[color] -= KING_PRESSURE * count(ei.fullAttackMaps[color^1] & KING_ZONE);
        }

        // Piece attacks
        ksValue[BLACK] -= getKingSafety<WHITE>(b, pmlWhite, kingNeighborhood[BLACK], ksValue[BLACK]);
        ksValue[WHITE] -= getKingSafety<BLACK>(b, pmlBlack, kingNeighborhood[WHITE], ksValue[WHITE]);

        // Castling rights
        ksValue[WHITE] += CASTLING_RIGHTS_VALUE[count(b.getCastlingRights() & WHITECASTLE)];
        ksValue[BLACK] += CASTLING_RIGHTS_VALUE[count(b.getCastlingRights() & BLACKCASTLE)];
    }

    ksValue[WHITE] = ksValue[WHITE] * scaleKingSafety / DEFAULT_EVAL_SCALE;
    ksValue[BLACK] = ksValue[BLACK] * scaleKingSafety / DEFAULT_EVAL_SCALE;

    valueMg += ksValue[WHITE] - ksValue[BLACK];

    if (debug) {
        evalDebugStats.whiteKingSafety = ksValue[WHITE];
        evalDebugStats.blackKingSafety = ksValue[BLACK];
    }


    // Get all squares attackable by pawns in the future
    // Used for outposts and backwards pawns
    uint64_t wPawnFrontSpan = pieces[WHITE][PAWNS] << 8;
    uint64_t bPawnFrontSpan = pieces[BLACK][PAWNS] >> 8;
    for (int i = 0; i < 5; i++) {
        wPawnFrontSpan |= wPawnFrontSpan << 8;
        bPawnFrontSpan |= bPawnFrontSpan >> 8;
    }
    uint64_t pawnStopAtt[2];
    pawnStopAtt[WHITE] = ((wPawnFrontSpan >> 1) & NOTH) | ((wPawnFrontSpan << 1) & NOTA);
    pawnStopAtt[BLACK] = ((bPawnFrontSpan >> 1) & NOTH) | ((bPawnFrontSpan << 1) & NOTA);


    Score pieceEvalScore[2] = {EVAL_ZERO, EVAL_ZERO};

    //------------------------------Minor Pieces--------------------------------
    // Bishops tend to be worse if too many pawns are on squares of their color
    if (pieces[WHITE][BISHOPS] & LIGHT) {
        pieceEvalScore[WHITE] += BISHOP_PAWN_COLOR_PENALTY * count(pieces[WHITE][PAWNS] & LIGHT);
        pieceEvalScore[WHITE] += BISHOP_RAMMED_PAWN_COLOR_PENALTY * count(ei.rammedPawns[WHITE] & LIGHT);
    }
    if (pieces[WHITE][BISHOPS] & DARK) {
        pieceEvalScore[WHITE] += BISHOP_PAWN_COLOR_PENALTY * count(pieces[WHITE][PAWNS] & DARK);
        pieceEvalScore[WHITE] += BISHOP_RAMMED_PAWN_COLOR_PENALTY * count(ei.rammedPawns[WHITE] & DARK);
    }
    if (pieces[BLACK][BISHOPS] & LIGHT) {
        pieceEvalScore[BLACK] += BISHOP_PAWN_COLOR_PENALTY * count(pieces[BLACK][PAWNS] & LIGHT);
        pieceEvalScore[BLACK] += BISHOP_RAMMED_PAWN_COLOR_PENALTY * count(ei.rammedPawns[BLACK] & LIGHT);
    }
    if (pieces[BLACK][BISHOPS] & DARK) {
        pieceEvalScore[BLACK] += BISHOP_PAWN_COLOR_PENALTY * count(pieces[BLACK][PAWNS] & DARK);
        pieceEvalScore[BLACK] += BISHOP_RAMMED_PAWN_COLOR_PENALTY * count(ei.rammedPawns[BLACK] & DARK);
    }

    // Shielded minors: minors behind own pawns
    pieceEvalScore[WHITE] += SHIELDED_MINOR_BONUS * count((pieces[WHITE][PAWNS] >> 8)
                                                        & (pieces[WHITE][KNIGHTS] | pieces[WHITE][BISHOPS])
                                                        & (RANK_2 | RANK_3 | RANK_4));
    pieceEvalScore[BLACK] += SHIELDED_MINOR_BONUS * count((pieces[BLACK][PAWNS] << 8)
                                                        & (pieces[BLACK][KNIGHTS] | pieces[BLACK][BISHOPS])
                                                        & (RANK_7 | RANK_6 | RANK_5));

    // Outposts
    const uint64_t OUTPOST_SQS[2] = {((FILE_C | FILE_D | FILE_E | FILE_F) & (RANK_4 | RANK_5 | RANK_6))
                                   | ((FILE_B | FILE_G) &  (RANK_5 | RANK_6)),
                                     ((FILE_C | FILE_D | FILE_E | FILE_F) & (RANK_5 | RANK_4 | RANK_3))
                                   | ((FILE_B | FILE_G) &  (RANK_4 | RANK_3))};

    for (int color = WHITE; color <= BLACK; color++) {
        //-----------------------------------Knights--------------------------------
        uint64_t knightsTemp = pieces[color][KNIGHTS];
        while (knightsTemp) {
            int knightSq = bitScanForward(knightsTemp);
            knightsTemp &= knightsTemp - 1;
            uint64_t bit = INDEX_TO_BIT[knightSq];

            psqtScores[color] += PSQT[color][KNIGHTS][knightSq];

            // Outposts
            if (bit & ~pawnStopAtt[color^1] & OUTPOST_SQS[color]) {
                pieceEvalScore[color] += KNIGHT_OUTPOST_BONUS;
                // Defended by pawn
                if (bit & ei.attackMaps[color][PAWNS])
                    pieceEvalScore[color] += KNIGHT_OUTPOST_PAWN_DEF_BONUS;
            }
        }

        //-----------------------------------Bishops--------------------------------
        uint64_t bishopsTemp = pieces[color][BISHOPS];
        while (bishopsTemp) {
            int bishopSq = bitScanForward(bishopsTemp);
            bishopsTemp &= bishopsTemp - 1;
            uint64_t bit = INDEX_TO_BIT[bishopSq];

            psqtScores[color] += PSQT[color][BISHOPS][bishopSq];

            if (bit & ~pawnStopAtt[color^1] & OUTPOST_SQS[color]) {
                pieceEvalScore[color] += BISHOP_OUTPOST_BONUS;
                if (bit & ei.attackMaps[color][PAWNS])
                    pieceEvalScore[color] += BISHOP_OUTPOST_PAWN_DEF_BONUS;
            }
        }

        //-------------------------------Rooks--------------------------------------
        uint64_t rooksTemp = pieces[color][ROOKS];
        while (rooksTemp) {
            int rookSq = bitScanForward(rooksTemp);
            rooksTemp &= rooksTemp - 1;
            int file = rookSq & 7;
            int rank = rookSq >> 3;

            psqtScores[color] += PSQT[color][ROOKS][rookSq];

            // Bonus for having rooks on open or semiopen files
            if (!(FILES[file] & (pieces[color][PAWNS] | pieces[color^1][PAWNS])))
                pieceEvalScore[color] += ROOK_OPEN_FILE_BONUS;
            else if (!(FILES[file] & pieces[color][PAWNS]))
                pieceEvalScore[color] += ROOK_SEMIOPEN_FILE_BONUS;
            // Bonus for having rooks on same ranks as enemy pawns
            if (relativeRank(color, rank) >= 4)
                pieceEvalScore[color] += ROOK_PAWN_RANK_THREAT * count(RANKS[rank] & pieces[color^1][PAWNS]);
        }
    }


    valueMg += decEvalMg(pieceEvalScore[WHITE]) - decEvalMg(pieceEvalScore[BLACK]);
    valueEg += decEvalEg(pieceEvalScore[WHITE]) - decEvalEg(pieceEvalScore[BLACK]);

    if (debug) {
        evalDebugStats.whitePieceScore = pieceEvalScore[WHITE];
        evalDebugStats.blackPieceScore = pieceEvalScore[BLACK];
    }

    valueMg += decEvalMg(psqtScores[WHITE]) - decEvalMg(psqtScores[BLACK]);
    valueEg += decEvalEg(psqtScores[WHITE]) - decEvalEg(psqtScores[BLACK]);

    if (debug) {
        evalDebugStats.whitePsqtScore = psqtScores[WHITE];
        evalDebugStats.blackPsqtScore = psqtScores[BLACK];
    }


    //-------------------------------Threats------------------------------------
    Score threatScore[2] = {EVAL_ZERO, EVAL_ZERO};
    // Pawns attacked by opposing pieces and not defended by own pawns
    if (uint64_t upawns = pieces[WHITE][PAWNS] & ei.fullAttackMaps[BLACK] & ~ei.attackMaps[WHITE][PAWNS]) {
        threatScore[WHITE] += UNDEFENDED_PAWN * count(upawns);
    }
    if (uint64_t upawns = pieces[BLACK][PAWNS] & ei.fullAttackMaps[WHITE] & ~ei.attackMaps[BLACK][PAWNS]) {
        threatScore[BLACK] += UNDEFENDED_PAWN * count(upawns);
    }
    // Minors attacked by opposing pieces and not defended by own pawns
    if (uint64_t minors = (pieces[WHITE][KNIGHTS] | pieces[WHITE][BISHOPS]) & ei.fullAttackMaps[BLACK] & ~ei.attackMaps[WHITE][PAWNS]) {
        threatScore[WHITE] += UNDEFENDED_MINOR * count(minors);
    }
    if (uint64_t minors = (pieces[BLACK][KNIGHTS] | pieces[BLACK][BISHOPS]) & ei.fullAttackMaps[WHITE] & ~ei.attackMaps[BLACK][PAWNS]) {
        threatScore[BLACK] += UNDEFENDED_MINOR * count(minors);
    }
    // Rooks attacked by opposing minors
    if (uint64_t rooks = pieces[WHITE][ROOKS] & (ei.attackMaps[BLACK][KNIGHTS] | ei.attackMaps[BLACK][BISHOPS])) {
        threatScore[WHITE] += MINOR_ROOK_THREAT * count(rooks);
    }
    if (uint64_t rooks = pieces[BLACK][ROOKS] & (ei.attackMaps[WHITE][KNIGHTS] | ei.attackMaps[WHITE][BISHOPS])) {
        threatScore[BLACK] += MINOR_ROOK_THREAT * count(rooks);
    }
    // Queens attacked by opposing minors
    if (uint64_t queens = pieces[WHITE][QUEENS] & (ei.attackMaps[BLACK][KNIGHTS] | ei.attackMaps[BLACK][BISHOPS])) {
        threatScore[WHITE] += MINOR_QUEEN_THREAT * count(queens);
    }
    if (uint64_t queens = pieces[BLACK][QUEENS] & (ei.attackMaps[WHITE][KNIGHTS] | ei.attackMaps[WHITE][BISHOPS])) {
        threatScore[BLACK] += MINOR_QUEEN_THREAT * count(queens);
    }
    // Queens attacked by opposing rooks
    if (uint64_t queens = pieces[WHITE][QUEENS] & ei.attackMaps[BLACK][ROOKS]) {
        threatScore[WHITE] += ROOK_QUEEN_THREAT * count(queens);
    }
    if (uint64_t queens = pieces[BLACK][QUEENS] & ei.attackMaps[WHITE][ROOKS]) {
        threatScore[BLACK] += ROOK_QUEEN_THREAT * count(queens);
    }
    // Pieces attacked by opposing pawns
    if (uint64_t threatened = (pieces[WHITE][KNIGHTS] | pieces[WHITE][BISHOPS]
                             | pieces[WHITE][ROOKS]   | pieces[WHITE][QUEENS]) & ei.attackMaps[BLACK][PAWNS]) {
        threatScore[WHITE] += PAWN_PIECE_THREAT * count(threatened);
    }
    if (uint64_t threatened = (pieces[BLACK][KNIGHTS] | pieces[BLACK][BISHOPS]
                             | pieces[BLACK][ROOKS]   | pieces[BLACK][QUEENS]) & ei.attackMaps[WHITE][PAWNS]) {
        threatScore[BLACK] += PAWN_PIECE_THREAT * count(threatened);
    }

    // Loose pawns: pawns in opponent's half of the board with no defenders
    const uint64_t WHITE_HALF = RANK_1 | RANK_2 | RANK_3 | RANK_4;
    const uint64_t BLACK_HALF = RANK_5 | RANK_6 | RANK_7 | RANK_8;
    if (uint64_t lpawns = pieces[WHITE][PAWNS] & BLACK_HALF & ~(ei.fullAttackMaps[WHITE] | ei.attackMaps[WHITE][PAWNS])) {
        threatScore[WHITE] += LOOSE_PAWN * count(lpawns);
    }
    if (uint64_t lpawns = pieces[BLACK][PAWNS] & WHITE_HALF & ~(ei.fullAttackMaps[BLACK] | ei.attackMaps[BLACK][PAWNS])) {
        threatScore[BLACK] += LOOSE_PAWN * count(lpawns);
    }

    // Loose minors
    if (uint64_t lminors = (pieces[WHITE][KNIGHTS] | pieces[WHITE][BISHOPS]) & BLACK_HALF & ~(ei.fullAttackMaps[WHITE] | ei.attackMaps[WHITE][PAWNS])) {
        threatScore[WHITE] += LOOSE_MINOR * count(lminors);
    }
    if (uint64_t lminors = (pieces[BLACK][KNIGHTS] | pieces[BLACK][BISHOPS]) & WHITE_HALF & ~(ei.fullAttackMaps[BLACK] | ei.attackMaps[BLACK][PAWNS])) {
        threatScore[BLACK] += LOOSE_MINOR * count(lminors);
    }

    valueMg += decEvalMg(threatScore[WHITE]) - decEvalMg(threatScore[BLACK]);
    valueEg += decEvalEg(threatScore[WHITE]) - decEvalEg(threatScore[BLACK]);

    if (debug) {
        evalDebugStats.whiteThreatScore = threatScore[WHITE];
        evalDebugStats.blackThreatScore = threatScore[BLACK];
    }


    //----------------------------Pawn structure--------------------------------
    Score whitePawnScore = EVAL_ZERO, blackPawnScore = EVAL_ZERO;
    // Passed pawns
    uint64_t wPassedBlocker = pieces[BLACK][PAWNS] >> 8;
    uint64_t bPassedBlocker = pieces[WHITE][PAWNS] << 8;
    // If opposing pawns are on the same or an adjacent file on a pawn's front
    // span, then the pawn is not passed
    wPassedBlocker |= ((wPassedBlocker >> 1) & NOTH) | ((wPassedBlocker << 1) & NOTA);
    bPassedBlocker |= ((bPassedBlocker >> 1) & NOTH) | ((bPassedBlocker << 1) & NOTA);
    // Include own pawns as blockers to prevent doubled pawns from both being
    // scored as passers
    wPassedBlocker |= (pieces[WHITE][PAWNS] >> 8);
    bPassedBlocker |= (pieces[BLACK][PAWNS] << 8);
    // Find opposing pawn front spans
    for(int i = 0; i < 4; i++) {
        wPassedBlocker |= (wPassedBlocker >> 8);
        bPassedBlocker |= (bPassedBlocker << 8);
    }
    // Passers are pawns outside the opposing pawn front span
    uint64_t wPassedPawns = pieces[WHITE][PAWNS] & ~wPassedBlocker;
    uint64_t bPassedPawns = pieces[BLACK][PAWNS] & ~bPassedBlocker;

    uint64_t wPasserTemp = wPassedPawns;
    while (wPasserTemp) {
        int passerSq = bitScanForward(wPasserTemp);
        wPasserTemp &= wPasserTemp - 1;
        int file = passerSq & 7;
        int rank = passerSq >> 3;
        whitePawnScore += PASSER_BONUS[rank];
        whitePawnScore += PASSER_FILE_BONUS[file];

        // Non-linear bonus based on rank
        int rFactor = (rank-1) * (rank-2) / 2;
        if (rFactor) {
            // Only give bonuses based on push path if the pawn isn't blockaded
            if (!(INDEX_TO_BIT[passerSq+8] & allPieces[BLACK])) {
                uint64_t pathToQueen = INDEX_TO_BIT[passerSq];
                pathToQueen |= pathToQueen << 8;
                pathToQueen |= pathToQueen << 16;
                pathToQueen |= pathToQueen << 32;

                // Consider x-ray attacks of rooks and queens behind passers
                uint64_t rookBehind = INDEX_TO_BIT[passerSq];
                for (int i = 0; i < 5; i++)
                    rookBehind |= (rookBehind >> 8) & ~(allPieces[WHITE] | allPieces[BLACK]);
                rookBehind >>= 8;
                uint64_t wBlock = allPieces[BLACK] | ei.fullAttackMaps[BLACK];
                uint64_t wDefend = ei.fullAttackMaps[WHITE] | ei.attackMaps[WHITE][PAWNS];
                if (rookBehind & (pieces[WHITE][ROOKS] | pieces[WHITE][QUEENS]))
                    wDefend |= pathToQueen;
                else if (rookBehind & (pieces[BLACK][ROOKS] | pieces[BLACK][QUEENS]))
                    wBlock |= pathToQueen;

                // Path to queen is completely undefended by opponent
                if (!(pathToQueen & wBlock))
                    whitePawnScore += rFactor * FREE_PROMOTION_BONUS;
                // Stop square is undefended by opponent
                else if (!(INDEX_TO_BIT[passerSq+8] & wBlock))
                    whitePawnScore += rFactor * FREE_STOP_BONUS;
                // Path to queen is completely defended by own pieces
                if ((pathToQueen & wDefend) == pathToQueen)
                    whitePawnScore += rFactor * FULLY_DEFENDED_PASSER_BONUS;
                // Stop square is defended by own pieces
                else if (INDEX_TO_BIT[passerSq+8] & wDefend)
                    whitePawnScore += rFactor * DEFENDED_PASSER_BONUS;
            }

            // Bonuses and penalties for king distance
            whitePawnScore -= OWN_KING_DIST * getManhattanDistance(passerSq+8, kingSq[WHITE]) * rFactor;
            whitePawnScore += OPP_KING_DIST * getManhattanDistance(passerSq+8, kingSq[BLACK]) * rFactor;
        }
    }
    uint64_t bPasserTemp = bPassedPawns;
    while (bPasserTemp) {
        int passerSq = bitScanForward(bPasserTemp);
        bPasserTemp &= bPasserTemp - 1;
        int file = passerSq & 7;
        int rank = 7 - (passerSq >> 3);
        blackPawnScore += PASSER_BONUS[rank];
        blackPawnScore += PASSER_FILE_BONUS[file];

        int rFactor = (rank-1) * (rank-2) / 2;
        if (rFactor) {
            if (!(INDEX_TO_BIT[passerSq-8] & allPieces[WHITE])) {
                uint64_t pathToQueen = INDEX_TO_BIT[passerSq];
                pathToQueen |= pathToQueen >> 8;
                pathToQueen |= pathToQueen >> 16;
                pathToQueen |= pathToQueen >> 32;

                uint64_t rookBehind = INDEX_TO_BIT[passerSq];
                for (int i = 0; i < 5; i++)
                    rookBehind |= (rookBehind << 8) & ~(allPieces[WHITE] | allPieces[BLACK]);
                rookBehind <<= 8;
                uint64_t bBlock = allPieces[WHITE] | ei.fullAttackMaps[WHITE];
                uint64_t bDefend = ei.fullAttackMaps[BLACK] | ei.attackMaps[BLACK][PAWNS];
                if (rookBehind & (pieces[BLACK][ROOKS] | pieces[BLACK][QUEENS]))
                    bDefend |= pathToQueen;
                else if (rookBehind & (pieces[WHITE][ROOKS] | pieces[WHITE][QUEENS]))
                    bBlock |= pathToQueen;

                if (!(pathToQueen & bBlock))
                    blackPawnScore += rFactor * FREE_PROMOTION_BONUS;
                else if (!(INDEX_TO_BIT[passerSq-8] & bBlock))
                    blackPawnScore += rFactor * FREE_STOP_BONUS;
                if ((pathToQueen & bDefend) == pathToQueen)
                    blackPawnScore += rFactor * FULLY_DEFENDED_PASSER_BONUS;
                else if (INDEX_TO_BIT[passerSq-8] & bDefend)
                    blackPawnScore += rFactor * DEFENDED_PASSER_BONUS;
            }

            blackPawnScore += OPP_KING_DIST * getManhattanDistance(passerSq-8, kingSq[WHITE]) * rFactor;
            blackPawnScore -= OWN_KING_DIST * getManhattanDistance(passerSq-8, kingSq[BLACK]) * rFactor;
        }
    }

    // Doubled pawns
    whitePawnScore += DOUBLED_PENALTY * count(pieces[WHITE][PAWNS] & (pieces[WHITE][PAWNS] << 8));
    blackPawnScore += DOUBLED_PENALTY * count(pieces[BLACK][PAWNS] & (pieces[BLACK][PAWNS] >> 8));

    // Isolated pawns
    // Count the pawns on each file
    int wPawnCtByFile[8];
    int bPawnCtByFile[8];
    for (int i = 0; i < 8; i++) {
        wPawnCtByFile[i] = count(pieces[WHITE][PAWNS] & FILES[i]);
        bPawnCtByFile[i] = count(pieces[BLACK][PAWNS] & FILES[i]);
    }
    // Fill a bitmap of which files have pawns
    uint64_t wIsolated = 0, bIsolated = 0;
    for (int i = 7; i >= 0; i--) {
        wIsolated |= (bool) (wPawnCtByFile[i]);
        bIsolated |= (bool) (bPawnCtByFile[i]);
        wIsolated <<= 1;
        bIsolated <<= 1;
    }
    wIsolated >>= 1;
    bIsolated >>= 1;
    // If there are pawns on either adjacent file, we remove this pawn
    wIsolated &= ~((wIsolated >> 1) | (wIsolated << 1));
    bIsolated &= ~((bIsolated >> 1) | (bIsolated << 1));

    uint64_t wIsolatedBB = wIsolated;
    wIsolatedBB |= wIsolatedBB << 8;
    wIsolatedBB |= wIsolatedBB << 16;
    wIsolatedBB |= wIsolatedBB << 32;
    uint64_t bIsolatedBB = bIsolated;
    bIsolatedBB |= bIsolatedBB << 8;
    bIsolatedBB |= bIsolatedBB << 16;
    bIsolatedBB |= bIsolatedBB << 32;

    // Score isolated pawns
    for (int f = 0; f < 8; f++) {
        if (wIsolated & INDEX_TO_BIT[f]) {
            whitePawnScore += ISOLATED_PENALTY * wPawnCtByFile[f];
            if (!(FILES[f] & pieces[BLACK][PAWNS]))
                whitePawnScore += ISOLATED_SEMIOPEN_PENALTY * wPawnCtByFile[f];
        }
        if (bIsolated & INDEX_TO_BIT[f]) {
            blackPawnScore += ISOLATED_PENALTY * bPawnCtByFile[f];
            if (!(FILES[f] & pieces[WHITE][PAWNS]))
                blackPawnScore += ISOLATED_SEMIOPEN_PENALTY * bPawnCtByFile[f];
        }
    }

    // Backward pawns
    uint64_t wBadStopSqs = ~pawnStopAtt[WHITE] & ei.attackMaps[BLACK][PAWNS];
    uint64_t bBadStopSqs = ~pawnStopAtt[BLACK] & ei.attackMaps[WHITE][PAWNS];
    for (int i = 0; i < 6; i++) {
        wBadStopSqs |= wBadStopSqs >> 8;
        bBadStopSqs |= bBadStopSqs << 8;
    }

    uint64_t wBackwards = wBadStopSqs & pieces[WHITE][PAWNS] & ~wIsolatedBB & ~ei.attackMaps[BLACK][PAWNS];
    uint64_t bBackwards = bBadStopSqs & pieces[BLACK][PAWNS] & ~bIsolatedBB & ~ei.attackMaps[WHITE][PAWNS];
    whitePawnScore += BACKWARD_PENALTY * count(wBackwards);
    blackPawnScore += BACKWARD_PENALTY * count(bBackwards);

    // Semi-open files with backwards pawns
    uint64_t wBackwardsTemp = wBackwards;
    while (wBackwardsTemp) {
        int pawnSq = bitScanForward(wBackwardsTemp);
        wBackwardsTemp &= wBackwardsTemp - 1;
        int f = pawnSq & 7;
        if (!(FILES[f] & pieces[BLACK][PAWNS])) {
            whitePawnScore += BACKWARD_SEMIOPEN_PENALTY;
        }
    }
    uint64_t bBackwardsTemp = bBackwards;
    while (bBackwardsTemp) {
        int pawnSq = bitScanForward(bBackwardsTemp);
        bBackwardsTemp &= bBackwardsTemp - 1;
        int f = pawnSq & 7;
        if (!(FILES[f] & pieces[WHITE][PAWNS])) {
            blackPawnScore += BACKWARD_SEMIOPEN_PENALTY;
        }
    }

    // Undefended pawns
    uint64_t wUndefendedPawns = pieces[WHITE][PAWNS] & ~ei.attackMaps[WHITE][PAWNS] & ~wBackwards & ~wIsolatedBB;
    uint64_t bUndefendedPawns = pieces[BLACK][PAWNS] & ~ei.attackMaps[BLACK][PAWNS] & ~bBackwards & ~bIsolatedBB;
    whitePawnScore += UNDEFENDED_PAWN_PENALTY * count(wUndefendedPawns);
    blackPawnScore += UNDEFENDED_PAWN_PENALTY * count(bUndefendedPawns);

    // Pawn phalanxes
    uint64_t wPawnPhalanx = (pieces[WHITE][PAWNS] & (pieces[WHITE][PAWNS] << 1) & NOTA)
                          | (pieces[WHITE][PAWNS] & (pieces[WHITE][PAWNS] >> 1) & NOTH);
    uint64_t bPawnPhalanx = (pieces[BLACK][PAWNS] & (pieces[BLACK][PAWNS] << 1) & NOTA)
                          | (pieces[BLACK][PAWNS] & (pieces[BLACK][PAWNS] >> 1) & NOTH);
    wPawnPhalanx &= RANK_3 | RANK_4 | RANK_5 | RANK_6 | RANK_7;
    bPawnPhalanx &= RANK_2 | RANK_3 | RANK_4 | RANK_5 | RANK_6;
    wPawnPhalanx &= ~(pieces[BLACK][PAWNS] >> 8);
    bPawnPhalanx &= ~(pieces[WHITE][PAWNS] << 8);
    while (wPawnPhalanx) {
        int pawnSq = bitScanForward(wPawnPhalanx);
        wPawnPhalanx &= wPawnPhalanx - 1;
        int r = pawnSq >> 3;
        whitePawnScore += PAWN_PHALANX_RANK_BONUS * (r-2);
    }
    while (bPawnPhalanx) {
        int pawnSq = bitScanForward(bPawnPhalanx);
        bPawnPhalanx &= bPawnPhalanx - 1;
        int r = 7 - (pawnSq >> 3);
        blackPawnScore += PAWN_PHALANX_RANK_BONUS * (r-2);
    }

    // Other connected pawns
    uint64_t wConnected = pieces[WHITE][PAWNS] & ei.attackMaps[WHITE][PAWNS];
    uint64_t bConnected = pieces[BLACK][PAWNS] & ei.attackMaps[BLACK][PAWNS];
    while (wConnected) {
        int pawnSq = bitScanForward(wConnected);
        wConnected &= wConnected - 1;
        int r = pawnSq >> 3;
        whitePawnScore += PAWN_CONNECTED_RANK_BONUS * (r-2);
        int f = pawnSq & 7;
        if (!(FILES[f] & pieces[BLACK][PAWNS]))
            whitePawnScore += PAWN_CONNECTED_RANK_BONUS * (r-2);
    }
    while (bConnected) {
        int pawnSq = bitScanForward(bConnected);
        bConnected &= bConnected - 1;
        int r = 7 - (pawnSq >> 3);
        blackPawnScore += PAWN_CONNECTED_RANK_BONUS * (r-2);
        int f = pawnSq & 7;
        if (!(FILES[f] & pieces[WHITE][PAWNS]))
            blackPawnScore += PAWN_CONNECTED_RANK_BONUS * (r-2);
    }
    
    valueMg += decEvalMg(whitePawnScore) - decEvalMg(blackPawnScore);
    valueEg += decEvalEg(whitePawnScore) - decEvalEg(blackPawnScore);

    if (debug) {
        evalDebugStats.whitePawnScore = whitePawnScore;
        evalDebugStats.blackPawnScore = blackPawnScore;
    }


    // King-pawn tropism
    int kingPawnTropism = 0;
    if (egFactor > 0) {
        uint64_t pawnBits = pieces[WHITE][PAWNS] | pieces[BLACK][PAWNS];
        int pawnWeight = 0;

        int wTropismTotal = 0, bTropismTotal = 0;
        while (pawnBits) {
            int pawnSq = bitScanForward(pawnBits);
            pawnBits &= pawnBits - 1;

            wTropismTotal += getManhattanDistance(pawnSq, kingSq[WHITE]);
            bTropismTotal += getManhattanDistance(pawnSq, kingSq[BLACK]);
            pawnWeight++;
        }

        if (pawnWeight)
            kingPawnTropism = (bTropismTotal - wTropismTotal) / pawnWeight;

        valueEg += KING_TROPISM_VALUE * kingPawnTropism;
    }


    if (debug) {
        evalDebugStats.totalMg = valueMg;
        evalDebugStats.totalEg = valueEg;
    }

    int totalEval = (valueMg * (EG_FACTOR_RES - egFactor) + valueEg * egFactor) / EG_FACTOR_RES;

    // Scale factors
    int scaleFactor = MAX_SCALE_FACTOR;
    // Opposite colored bishops
    if (egFactor > 3 * EG_FACTOR_RES / 4) {
        if (pieceCounts[WHITE][BISHOPS] == 1
         && pieceCounts[BLACK][BISHOPS] == 1
         && (((pieces[WHITE][BISHOPS] & LIGHT) && (pieces[BLACK][BISHOPS] & DARK))
          || ((pieces[WHITE][BISHOPS] & DARK) && (pieces[BLACK][BISHOPS] & LIGHT)))) {
            if ((b.getNonPawnMaterial(WHITE) == pieces[WHITE][BISHOPS])
             && (b.getNonPawnMaterial(BLACK) == pieces[BLACK][BISHOPS]))
                scaleFactor = OPPOSITE_BISHOP_SCALING[0];
            else
                scaleFactor = OPPOSITE_BISHOP_SCALING[1];
        }
    }
    // Reduce eval for lack of pawns
    if (whiteMaterial - blackMaterial > 0
     && whiteMaterial - blackMaterial <= PIECE_VALUES[MG][KNIGHTS]
     && pieceCounts[WHITE][PAWNS] <= 1) {
        if (pieceCounts[WHITE][PAWNS] == 0) {
            if (whiteMaterial < PIECE_VALUES[MG][BISHOPS] + 50)
                scaleFactor = PAWNLESS_SCALING[0];
            else if (blackMaterial <= PIECE_VALUES[MG][BISHOPS])
                scaleFactor = PAWNLESS_SCALING[1];
            else
                scaleFactor = PAWNLESS_SCALING[2];
        }
        else {
            scaleFactor = PAWNLESS_SCALING[3];
        }
    }
    if (blackMaterial - whiteMaterial > 0
     && blackMaterial - whiteMaterial <= PIECE_VALUES[MG][KNIGHTS]
     && pieceCounts[BLACK][PAWNS] <= 1) {
        if (pieceCounts[BLACK][PAWNS] == 0) {
            if (blackMaterial < PIECE_VALUES[MG][BISHOPS] + 50)
                scaleFactor = PAWNLESS_SCALING[0];
            else if (whiteMaterial <= PIECE_VALUES[MG][BISHOPS])
                scaleFactor = PAWNLESS_SCALING[1];
            else
                scaleFactor = PAWNLESS_SCALING[2];
        }
        else {
            scaleFactor = PAWNLESS_SCALING[3];
        }
    }

    if (scaleFactor < MAX_SCALE_FACTOR)
        totalEval = totalEval * scaleFactor / MAX_SCALE_FACTOR;


    if (debug) {
        evalDebugStats.totalEval = totalEval;
        evalDebugStats.print();
    }

    return totalEval;
}

// Explicitly instantiate templates
template int Eval::evaluate<true>(Board &b);
template int Eval::evaluate<false>(Board &b);

// Scores the board for a player based on estimates of mobility. This function
// also calculates control of center.
template <int color>
void Eval::getMobility(PieceMoveList &pml, PieceMoveList &oppPml, int &valueMg, int &valueEg) {
    // Bitboard of center 4 squares: d4, e4, d5, e5
    const uint64_t CENTER_SQS = 0x0000001818000000;
    // Extended center: center, plus c4, f4, c5, f5, and d6/d3, e6/e3
    const uint64_t EXTENDED_CENTER_SQS = 0x0000183C3C180000;
    // Holds the mobility values and the final result
    int mgMobility = 0, egMobility = 0;
    // Holds the center control score
    int centerControl = 0;

    // Calculate center control only for pawns
    uint64_t pawnAttackMap = ei.attackMaps[color][PAWNS];
    centerControl += EXTENDED_CENTER_VAL * count(pawnAttackMap & EXTENDED_CENTER_SQS);
    centerControl += CENTER_BONUS * count(pawnAttackMap & CENTER_SQS);

    uint64_t oppPawnAttackMap = ei.attackMaps[color^1][PAWNS];

    // We count mobility for all squares other than ones occupied by own rammed
    // pawns, king, or attacked by opponent's pawns
    // Idea of using rammed pawns from Stockfish
    uint64_t openSqs = ~(ei.rammedPawns[color] | pieces[color][KINGS] | oppPawnAttackMap);

    // For a queen, we also exclude squares not controlled by an opponent's minor or rook
    uint64_t oppAttackMap = 0;
    for (int pieceID = KNIGHTS; pieceID <= ROOKS; pieceID++)
        oppAttackMap |= ei.attackMaps[color^1][pieceID];


    // Iterate over piece move information to extract all mobility-related scores
    for (unsigned int i = 0; i < pml.starts[QUEENS]; i++) {
        PieceMoveInfo pmi = pml.get(i);
        int pieceIndex = pmi.pieceID - 1;
        uint64_t legal = pmi.legal;

        mgMobility += mobilityScore[MG][pieceIndex][count(legal & openSqs)];
        egMobility += mobilityScore[EG][pieceIndex][count(legal & openSqs)];
        centerControl += EXTENDED_CENTER_VAL * count(legal & EXTENDED_CENTER_SQS & ~oppPawnAttackMap);
        centerControl += CENTER_BONUS * count(legal & CENTER_SQS & ~oppPawnAttackMap);
    }
    for (unsigned int i = pml.starts[QUEENS]; i < pml.size(); i++) {
        PieceMoveInfo pmi = pml.get(i);
        uint64_t legal = pmi.legal;

        mgMobility += mobilityScore[MG][QUEENS-1][count(legal & openSqs & ~oppAttackMap)];
        egMobility += mobilityScore[EG][QUEENS-1][count(legal & openSqs & ~oppAttackMap)];
        centerControl += EXTENDED_CENTER_VAL * count(legal & EXTENDED_CENTER_SQS & ~oppPawnAttackMap & ~oppAttackMap);
        centerControl += CENTER_BONUS * count(legal & CENTER_SQS & ~oppPawnAttackMap & ~oppAttackMap);
    }

    valueMg = mgMobility + centerControl;
    valueEg = egMobility;
}

// King safety, based on the number of opponent pieces near the king
// The lookup table approach is inspired by Ed Schroder's Rebel chess engine,
// and by Stockfish
template <int attackingColor>
int Eval::getKingSafety(Board &b, PieceMoveList &attackers, uint64_t kingSqs, int pawnScore) {
    // Precalculate a few things
    uint64_t kingNeighborhood = (attackingColor == WHITE) ? ((pieces[BLACK][KINGS] & RANK_8) ? (kingSqs | (kingSqs >> 8)) : kingSqs)
                                                          : ((pieces[WHITE][KINGS] & RANK_1) ? (kingSqs | (kingSqs << 8)) : kingSqs);
    uint64_t defendMap = ei.attackMaps[attackingColor^1][PAWNS] | ei.fullAttackMaps[attackingColor^1];
    // Analyze undefended squares directly adjacent to king
    uint64_t kingDefenseless = defendMap & kingSqs;
    kingDefenseless ^= kingSqs;


    // Holds the king safety score
    int kingSafetyPts = 0;
    // Count the number and value of pieces participating in the king attack
    int kingAttackPts = 0;
    int kingAttackPieces = count(ei.attackMaps[attackingColor][PAWNS] & kingNeighborhood);
    // Get check information
    uint64_t checkMaps[4];
    b.getCheckMaps(attackingColor^1, checkMaps);

    // Iterate over piece move information to extract all mobility-related scores
    for (unsigned int i = 0; i < attackers.size(); i++) {
        PieceMoveInfo pmi = attackers.get(i);
        int pieceIndex = pmi.pieceID - 1;
        // Get all potential legal moves
        uint64_t legal = pmi.legal;

        // Get king safety score
        if (legal & kingNeighborhood) {
            kingAttackPieces++;
            kingAttackPts += KING_THREAT_MULTIPLIER[pieceIndex];
            kingSafetyPts += KING_THREAT_SQUARE[pieceIndex] * count(legal & kingSqs);

            // Bonus for overloading on defenseless squares
            kingSafetyPts += KING_DEFENSELESS_SQUARE * count(legal & kingDefenseless);
        }

        // Add bonuses for safe checks
        if (legal & checkMaps[pieceIndex] & ~kingSqs & ~defendMap)
            kingSafetyPts += SAFE_CHECK_BONUS[pieceIndex];
    }

    // Give a decent bonus for each additional piece participating
    kingSafetyPts += kingAttackPieces * kingAttackPts;

    // Adjust based on pawn shield and pawn storms
    kingSafetyPts -= KS_PAWN_FACTOR * pawnScore / 32;


    // Convert king safety points into centipawns using a quadratic relationship
    kingSafetyPts = std::max(0, kingSafetyPts);
    return std::min(kingSafetyPts * kingSafetyPts / KS_ARRAY_FACTOR, 600);
}

// Check special endgame cases: where help mate is possible (detecting this
// is delegated to search), but forced mate is not, or where a simple
// forced mate is possible.
int Eval::checkEndgameCases() {
    int numWPieces = count(allPieces[WHITE]) - 1;
    int numBPieces = count(allPieces[BLACK]) - 1;
    int numPieces = numWPieces + numBPieces;

    // Rook or queen + anything else vs. lone king is a forced win
    if (numBPieces == 0 && (pieces[WHITE][ROOKS] || pieces[WHITE][QUEENS])) {
        return scoreSimpleKnownWin(WHITE);
    }
    if (numWPieces == 0 && (pieces[BLACK][ROOKS] || pieces[BLACK][QUEENS])) {
        return scoreSimpleKnownWin(BLACK);
    }

    // TODO detect when KPvK is drawn
    if (numPieces == 1) {
        if (pieces[WHITE][PAWNS]) {
            int wPawn = bitScanForward(pieces[WHITE][PAWNS]);
            int r = (wPawn >> 3);
            return 3 * PIECE_VALUES[EG][PAWNS] / 2 + 5 * (r - 1) * (r - 2);
        }
        if (pieces[BLACK][PAWNS]) {
            int bPawn = bitScanForward(pieces[BLACK][PAWNS]);
            int r = 7 - (bPawn >> 3);
            return -3 * PIECE_VALUES[EG][PAWNS] / 2 - 5 * (r - 1) * (r - 2);
        }
    }

    else if (numPieces == 2) {
        // If white has one piece, the other must be black's
        if (numWPieces == 1) {
            // If each side has one minor piece, then draw
            if ((pieces[WHITE][KNIGHTS] | pieces[WHITE][BISHOPS])
             && (pieces[BLACK][KNIGHTS] | pieces[BLACK][BISHOPS]))
                return 0;
            // If each side has a rook, then draw
            if (pieces[WHITE][ROOKS] && pieces[BLACK][ROOKS])
                return 0;
            // If each side has a queen, then draw
            if (pieces[WHITE][QUEENS] && pieces[BLACK][QUEENS])
                return 0;
        }
        // Otherwise, one side has both pieces
        else {
            // Pawn + anything is a win
            // TODO bishop can block losing king's path to queen square
            if (pieces[WHITE][PAWNS]) {
                int value = KNOWN_WIN / 2;
                int wKingSq = bitScanForward(pieces[WHITE][KINGS]);
                int bKingSq = bitScanForward(pieces[BLACK][KINGS]);
                int wPawnSq = bitScanForward(pieces[WHITE][PAWNS]);
                int wf = wPawnSq & 7;
                int wr = wPawnSq >> 3;
                if (pieces[WHITE][BISHOPS]
                 && ((wf == 0 && (pieces[WHITE][BISHOPS] & DARK))
                  || (wf == 7 && (pieces[WHITE][BISHOPS] & LIGHT)))) {
                    int wDist = std::max(7 - (wKingSq >> 3), std::abs((wKingSq & 7) - wf));
                    int bDist = std::max(7 - (bKingSq >> 3), std::abs((bKingSq & 7) - wf));
                    int wQueenDist = std::min(7-wr, 5) + 1;
                    if (playerToMove == BLACK)
                        bDist--;
                    if (bDist < std::min(wDist, wQueenDist))
                        return 0;
                }

                value += 8 * wr * wr;
                value += scoreCornerDistance(WHITE, wKingSq, bKingSq);
                return value;
            }
            if (pieces[BLACK][PAWNS]) {
                int value = -KNOWN_WIN / 2;
                int wKingSq = bitScanForward(pieces[WHITE][KINGS]);
                int bKingSq = bitScanForward(pieces[BLACK][KINGS]);
                int bPawnSq = bitScanForward(pieces[BLACK][PAWNS]);
                int bf = bPawnSq & 7;
                int br = bPawnSq >> 3;
                if (pieces[BLACK][BISHOPS]
                 && ((bf == 0 && (pieces[BLACK][BISHOPS] & LIGHT))
                  || (bf == 7 && (pieces[BLACK][BISHOPS] & DARK)))) {
                    int wDist = std::max((wKingSq >> 3), std::abs((wKingSq & 7) - bf));
                    int bDist = std::max((bKingSq >> 3), std::abs((bKingSq & 7) - bf));
                    int bQueenDist = std::min(br, 5) + 1;
                    if (playerToMove == WHITE)
                        wDist--;
                    if (wDist < std::min(bDist, bQueenDist))
                        return 0;
                }

                value -= 8 * br * br;
                value += scoreCornerDistance(WHITE, wKingSq, bKingSq);
                return value;
            }
            // Two knights is a draw
            if (count(pieces[WHITE][KNIGHTS]) == 2 || count(pieces[BLACK][KNIGHTS]) == 2)
                return 0;
            // Two bishops is a win
            if (count(pieces[WHITE][BISHOPS]) == 2)
                return scoreSimpleKnownWin(WHITE);
            if (count(pieces[BLACK][BISHOPS]) == 2)
                return scoreSimpleKnownWin(BLACK);

            // Mating with knight and bishop
            if (pieces[WHITE][KNIGHTS] && pieces[WHITE][BISHOPS]) {
                int value = KNOWN_WIN;
                int wKingSq = bitScanForward(pieces[WHITE][KINGS]);
                int bKingSq = bitScanForward(pieces[BLACK][KINGS]);
                value += scoreCornerDistance(WHITE, wKingSq, bKingSq);

                // Light squared corners are H1 (7) and A8 (56)
                if (pieces[WHITE][BISHOPS] & LIGHT)
                    value -= 20 * std::min(getManhattanDistance(bKingSq, 7), getManhattanDistance(bKingSq, 56));
                // Dark squared corners are A1 (0) and H8 (63)
                else
                    value -= 20 * std::min(getManhattanDistance(bKingSq, 0), getManhattanDistance(bKingSq, 63));
                return value;
            }
            if (pieces[BLACK][KNIGHTS] && pieces[BLACK][BISHOPS]) {
                int value = -KNOWN_WIN;
                int wKingSq = bitScanForward(pieces[WHITE][KINGS]);
                int bKingSq = bitScanForward(pieces[BLACK][KINGS]);
                value += scoreCornerDistance(BLACK, wKingSq, bKingSq);

                // Light squared corners are H1 (7) and A8 (56)
                if (pieces[BLACK][BISHOPS] & LIGHT)
                    value += 20 * std::min(getManhattanDistance(wKingSq, 7), getManhattanDistance(wKingSq, 56));
                // Dark squared corners are A1 (0) and H8 (63)
                else
                    value += 20 * std::min(getManhattanDistance(wKingSq, 0), getManhattanDistance(wKingSq, 63));
                return value;
            }
        }
    }

    // If not an endgame case, return -INFTY
    return -INFTY;
}

// A function for scoring the most basic mating cases, when it is only necessary
// to get the opposing king into a corner.
int Eval::scoreSimpleKnownWin(int winningColor) {
    int wKingSq = bitScanForward(pieces[WHITE][KINGS]);
    int bKingSq = bitScanForward(pieces[BLACK][KINGS]);
    int winScore = (winningColor == WHITE) ? KNOWN_WIN : -KNOWN_WIN;
    return winScore + scoreCornerDistance(winningColor, wKingSq, bKingSq);
}

// A function for scoring knight and bishop mates.
inline int Eval::scoreCornerDistance(int winningColor, int wKingSq, int bKingSq) {
    int wf = wKingSq & 7;
    int wr = wKingSq >> 3;
    int bf = bKingSq & 7;
    int br = bKingSq >> 3;
    int wDist = std::min(wf, 7-wf) + std::min(wr, 7-wr);
    int bDist = std::min(bf, 7-bf) + std::min(br, 7-br);
    return (winningColor == WHITE) ? wDist - 2*bDist : 2*wDist - bDist;
}

inline int Eval::getManhattanDistance(int sq1, int sq2) {
    return std::abs((sq1 >> 3) - (sq2 >> 3)) + std::abs((sq1 & 7) - (sq2 & 7));
}