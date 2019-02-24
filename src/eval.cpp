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

#include <cstring>
#include <functional>
#include <iomanip>
#include <iostream>
#include "bbinit.h"
#include "board.h"
#include "common.h"
#include "eval.h"
#include "uci.h"

namespace {

constexpr uint64_t KING_ZONE_DEFENDER[2] = {HALF[WHITE] | RANK_5, RANK_4 | HALF[BLACK]};
constexpr uint64_t KING_ZONE_FLANK[8] = {
    QSIDE, QSIDE, QSIDE, CENTER_FILES, CENTER_FILES, KSIDE, KSIDE, KSIDE
};
constexpr uint64_t KING_DEFENSE_ZONE[8] = {
    QSIDE ^ FILE_D, QSIDE ^ FILE_D, QSIDE ^ FILE_D, FILE_D | FILE_E,
    FILE_D | FILE_E, KSIDE ^ FILE_E, KSIDE ^ FILE_E, KSIDE ^ FILE_E
};

Score PSQT[2][6][64];
Score MOBILITY[5][28];
char manhattanDistance[64][64], kingDistance[64][64];

struct EvalDebug {
    int totalEval;
    int totalMg, totalEg;
    int totalMaterialMg, totalMaterialEg;
    int totalImbalanceMg, totalImbalanceEg;
    Score whitePsqtScore, blackPsqtScore;
    Score whiteMobilityScore, blackMobilityScore;
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
        whiteMobilityScore = blackMobilityScore = EVAL_ZERO;
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
                  << std::setw(4) << S(decEvalMg(whiteMobilityScore)) << "   "
                  << std::setw(4) << S(decEvalEg(whiteMobilityScore)) << "   |   "
                  << std::setw(4) << S(decEvalMg(blackMobilityScore)) << "   "
                  << std::setw(4) << S(decEvalEg(blackMobilityScore)) << "   |   "
                  << std::setw(4) << S(decEvalMg(whiteMobilityScore)) - S(decEvalMg(blackMobilityScore)) << "   "
                  << std::setw(4) << S(decEvalEg(whiteMobilityScore)) - S(decEvalEg(blackMobilityScore)) << std::endl;
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

EvalDebug evalDebugStats;

} // namespace


void initEvalTables() {
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
    for (int pieceID = 0; pieceID < 5; pieceID++) {
        for (int sqs = 0; sqs < 28; sqs++)
            MOBILITY[pieceID][sqs] = E(mobilityTable[MG][pieceID][sqs], mobilityTable[EG][pieceID][sqs]);
    }
    #undef E
}

void initDistances() {
    for (int sq1 = 0; sq1 < 64; sq1++) {
        for (int sq2 = 0; sq2 < 64; sq2++) {
            int r = std::abs((sq1 >> 3) - (sq2 >> 3));
            int f = std::abs((sq1 & 7) - (sq2 & 7));
            manhattanDistance[sq1][sq2] = r + f;
            kingDistance[sq1][sq2] = std::min(5, std::max(r, f));
        }
    }
}

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
    int material[2][2] = {{0, 0}, {0, 0}};
    int egFactorMaterial = 0;
    // Copy necessary values from Board and precompute the number of each piece on the board as well as material totals
    for (int color = WHITE; color <= BLACK; color++) {
        for (int pieceID = PAWNS; pieceID <= KINGS; pieceID++) {
            pieces[color][pieceID] = b.getPieces(color, pieceID);
            pieceCounts[color][pieceID] = count(pieces[color][pieceID]);
            if (pieceID != KINGS) {
                material[MG][color] += PIECE_VALUES[MG][pieceID] * pieceCounts[color][pieceID];
                material[EG][color] += PIECE_VALUES[EG][pieceID] * pieceCounts[color][pieceID];
                egFactorMaterial += EG_FACTOR_PIECE_VALS[pieceID] * pieceCounts[color][pieceID];
            }
        }
    }
    allPieces[WHITE] = b.getAllPieces(WHITE);
    allPieces[BLACK] = b.getAllPieces(BLACK);
    playerToMove = b.getPlayerToMove();
    int kingSq[2] = {b.getKingSq(WHITE), b.getKingSq(BLACK)};

    // Compute endgame factor which is between 0 and EG_FACTOR_RES, inclusive
    int egFactor = EG_FACTOR_RES - (egFactorMaterial - EG_FACTOR_ALPHA) * EG_FACTOR_RES / EG_FACTOR_BETA;
    egFactor = std::max(0, std::min(EG_FACTOR_RES, egFactor));

    // Check for special endgames
    if (egFactor == EG_FACTOR_RES) {
        int endgameScore = checkEndgameCases();
        if (endgameScore != -INFTY)
            return endgameScore;
    }

    // Precompute eval info, such as attack maps
    PieceMoveList pmlWhite = b.getPieceMoveList(WHITE);
    PieceMoveList pmlBlack = b.getPieceMoveList(BLACK);

    ei.clear();

    // Get the overall attack maps
    ei.attackMaps[WHITE][PAWNS] = b.getWPawnCaptures(pieces[WHITE][PAWNS]);
    ei.attackMaps[BLACK][PAWNS] = b.getBPawnCaptures(pieces[BLACK][PAWNS]);
    for (unsigned int i = 0; i < pmlWhite.size(); i++) {
        uint64_t legal = pmlWhite.get(i).legal;
        ei.doubleAttackMaps[WHITE] |= legal & (ei.fullAttackMaps[WHITE] | ei.attackMaps[WHITE][PAWNS]);
        ei.attackMaps[WHITE][pmlWhite.get(i).pieceID] |= legal;
        ei.fullAttackMaps[WHITE] |= legal;
    }
    for (unsigned int i = 0; i < pmlBlack.size(); i++) {
        uint64_t legal = pmlBlack.get(i).legal;
        ei.doubleAttackMaps[BLACK] |= legal & (ei.fullAttackMaps[BLACK] | ei.attackMaps[BLACK][PAWNS]);
        ei.attackMaps[BLACK][pmlBlack.get(i).pieceID] |= legal;
        ei.fullAttackMaps[BLACK] |= legal;
    }

    ei.rammedPawns[WHITE] = pieces[WHITE][PAWNS] & (pieces[BLACK][PAWNS] >> 8);
    ei.rammedPawns[BLACK] = pieces[BLACK][PAWNS] & (pieces[WHITE][PAWNS] << 8);

    uint64_t openFiles = pieces[WHITE][PAWNS] | pieces[BLACK][PAWNS];
    openFiles |= openFiles >> 8;
    openFiles |= openFiles >> 16;
    openFiles |= openFiles >> 32;
    openFiles |= openFiles << 8;
    openFiles |= openFiles << 16;
    openFiles |= openFiles << 32;
    ei.openFiles = ~openFiles;


    //---------------------------Material terms---------------------------------
    // Midgame and endgame material
    int valueMg = material[MG][WHITE] - material[MG][BLACK];
    int valueEg = material[EG][WHITE] - material[EG][BLACK];

    // Bishop pair bonus
    if ((pieces[WHITE][BISHOPS] & LIGHT) && (pieces[WHITE][BISHOPS] & DARK)) {
        material[MG][WHITE] += BISHOP_PAIR_VALUE;
        valueMg += BISHOP_PAIR_VALUE;
        valueEg += BISHOP_PAIR_VALUE;
    }
    if ((pieces[BLACK][BISHOPS] & LIGHT) && (pieces[BLACK][BISHOPS] & DARK)) {
        material[MG][BLACK] += BISHOP_PAIR_VALUE;
        valueMg -= BISHOP_PAIR_VALUE;
        valueEg -= BISHOP_PAIR_VALUE;
    }

    // Tempo bonus
    valueMg += (playerToMove == WHITE) ? TEMPO_VALUE : -TEMPO_VALUE;

    valueMg = valueMg * scaleMaterial / DEFAULT_EVAL_SCALE;
    valueEg = valueEg * scaleMaterial / DEFAULT_EVAL_SCALE;

    if (debug) {
        evalDebugStats.totalMaterialMg = valueMg;
        evalDebugStats.totalMaterialEg = valueEg;
    }


    // Material imbalance evaluation
    int imbalanceValue[2] = {0, 0};

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


    //--------------------------------Space-------------------------------------
    uint64_t allPawns = pieces[WHITE][PAWNS] | pieces[BLACK][PAWNS];
    int openFileCount = count(ei.openFiles & 0xFF);
    // Space is more important with more pieces on the board
    int whiteSpaceWeight = std::max(0, count(allPieces[WHITE]) - openFileCount);
    int blackSpaceWeight = std::max(0, count(allPieces[BLACK]) - openFileCount);

    // Consider two definitions of space, valuing the center 4 files higher, and excluding squares occupied by a pawn,
    // attacked by an opposing pawn, or attacked by two opposing pieces and not doubly defended
    //  1. Up to 3 squares behind a friendly pawn
    //  2. Up to 3 squares in front of an enemy pawn
    uint64_t whiteSafeSpace = ~(ei.attackMaps[BLACK][PAWNS] | (ei.doubleAttackMaps[BLACK] & ~ei.doubleAttackMaps[WHITE]));
    uint64_t whiteSpace = pieces[WHITE][PAWNS] >> 8;
    whiteSpace |= whiteSpace >> 8;
    whiteSpace |= whiteSpace >> 16;
    whiteSpace &= whiteSafeSpace & ~allPawns;
    uint64_t whiteSpace2 = pieces[BLACK][PAWNS] >> 8;
    whiteSpace2 |= whiteSpace2 >> 8;
    whiteSpace2 |= whiteSpace2 >> 16;
    whiteSpace2 &= whiteSafeSpace & ~whiteSpace & ~allPawns;
    int whiteSpaceScore = (SPACE_BONUS[0][1] * count(whiteSpace & CENTER_FILES) + SPACE_BONUS[0][0] * count(whiteSpace & ~CENTER_FILES)
                         + SPACE_BONUS[1][1] * count(whiteSpace2 & CENTER_FILES) + SPACE_BONUS[1][0] * count(whiteSpace2 & ~CENTER_FILES))
                            * whiteSpaceWeight * whiteSpaceWeight / 512;
    valueMg += whiteSpaceScore;
    valueEg += whiteSpaceScore / 2;

    uint64_t blackSafeSpace = ~(ei.attackMaps[WHITE][PAWNS] | (ei.doubleAttackMaps[WHITE] & ~ei.doubleAttackMaps[BLACK]));
    uint64_t blackSpace = pieces[BLACK][PAWNS] << 8;
    blackSpace |= blackSpace << 8;
    blackSpace |= blackSpace << 16;
    blackSpace &= blackSafeSpace & ~allPawns;
    uint64_t blackSpace2 = pieces[WHITE][PAWNS] << 8;
    blackSpace2 |= blackSpace2 << 8;
    blackSpace2 |= blackSpace2 << 16;
    blackSpace2 &= blackSafeSpace & ~blackSpace & ~allPawns;
    int blackSpaceScore = (SPACE_BONUS[0][1] * count(blackSpace & CENTER_FILES) + SPACE_BONUS[0][0] * count(blackSpace & ~CENTER_FILES)
                         + SPACE_BONUS[1][1] * count(blackSpace2 & CENTER_FILES) + SPACE_BONUS[1][0] * count(blackSpace2 & ~CENTER_FILES))
                            * blackSpaceWeight * blackSpaceWeight / 512;
    valueMg -= blackSpaceScore;
    valueEg -= blackSpaceScore / 2;


    //------------------------------King Safety---------------------------------
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
            int kingRank = kingSq[color] >> 3;
            int fileRange = std::min(6, std::max(1, kingFile));
            for (int i = fileRange-1; i <= fileRange+1; i++) {
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
                        (pieces[color][PAWNS] & indexToBit(stopSq)) != 0 ? 1 : 2][f][r];

                    if (f == 0 && (kingFile == 0 || kingFile == 7)
                     && (r == 1 || r == 2) && relativeRank(color, kingRank) + 1 == r)
                        ksValue[color] -= PAWN_STORM_SHIELDING_KING;
                }
                // Semi-open file: no pawn for attacker
                else
                    ksValue[color] -= PAWN_STORM_VALUE[0][f][0];
            }
        }

        // Piece attacks
        ksValue[BLACK] -= getKingSafety<WHITE>(b, pmlWhite, kingNeighborhood[BLACK], ksValue[BLACK], kingSq[BLACK] & 7);
        ksValue[WHITE] -= getKingSafety<BLACK>(b, pmlBlack, kingNeighborhood[WHITE], ksValue[WHITE], kingSq[WHITE] & 7);

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


    //-------------------------Minor Pieces and Mobility------------------------
    Score pieceEvalScore[2] = {EVAL_ZERO, EVAL_ZERO};
    // Mobility indexed by mg/eg, color
    Score mobilityScore[2] = {EVAL_ZERO, EVAL_ZERO};

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
    constexpr uint64_t OUTPOST_SQS[2] = {(CENTER_FILES & (RANK_4 | RANK_5 | RANK_6))
                                       | ((FILE_B | FILE_G) &  (RANK_5 | RANK_6)),
                                         (CENTER_FILES & (RANK_5 | RANK_4 | RANK_3))
                                       | ((FILE_B | FILE_G) &  (RANK_4 | RANK_3))};

    uint64_t occ = allPieces[WHITE] | allPieces[BLACK];
    uint64_t pieceRammedPawns[2] = {pieces[WHITE][PAWNS] & (occ >> 8),
                                    pieces[BLACK][PAWNS] & (occ << 8)};

    for (int color = WHITE; color <= BLACK; color++) {
        PieceMoveList &pml = (color == WHITE) ? pmlWhite : pmlBlack;
        // We count mobility for all squares other than ones:
        //  - Occupied by own blocked pawns or king
        //  - Attacked by opponent's pawns
        //  - Doubly attacked by opponent's pieces and not defended by own piece
        // Idea of using blocked pawns from Stockfish
        uint64_t mobilitySafeSqs = ~(pieceRammedPawns[color] | pieces[color][KINGS] | ei.attackMaps[color^1][PAWNS]
                                   | (ei.doubleAttackMaps[color^1] & ~ei.doubleAttackMaps[color]));

        //--------------------------------Knights-----------------------------------
        for (unsigned int i = 0; i < pml.starts[BISHOPS]; i++) {
            int knightSq = pml.get(i).startSq;
            uint64_t bit = indexToBit(knightSq);
            uint64_t mobilityMap = pml.get(i).legal & mobilitySafeSqs;

            psqtScores[color] += PSQT[color][KNIGHTS][knightSq];
            mobilityScore[color] += MOBILITY[KNIGHTS-1][count(mobilityMap)]
                                 + EXTENDED_CENTER_VAL * count(mobilityMap & EXTENDED_CENTER_SQS)
                                 + CENTER_BONUS * count(mobilityMap & CENTER_SQS);

            // Outposts
            if (bit & ~pawnStopAtt[color^1] & OUTPOST_SQS[color]) {
                pieceEvalScore[color] += KNIGHT_OUTPOST_BONUS;
                // Defended by pawn
                if (bit & ei.attackMaps[color][PAWNS])
                    pieceEvalScore[color] += KNIGHT_OUTPOST_PAWN_DEF_BONUS;
            }
            else if (uint64_t potential = pml.get(i).legal & ~pawnStopAtt[color^1] & OUTPOST_SQS[color] & ~allPieces[color]) {
                pieceEvalScore[color] += KNIGHT_POTENTIAL_OUTPOST_BONUS;
                if (potential & ei.attackMaps[color][PAWNS])
                    pieceEvalScore[color] += KNIGHT_POTENTIAL_OUTPOST_PAWN_DEF_BONUS;
            }
        }

        //---------------------------------Bishops----------------------------------
        for (unsigned int i = pml.starts[BISHOPS]; i < pml.starts[ROOKS]; i++) {
            int bishopSq = pml.get(i).startSq;
            uint64_t bit = indexToBit(bishopSq);
            uint64_t mobilityMap = pml.get(i).legal & mobilitySafeSqs;

            psqtScores[color] += PSQT[color][BISHOPS][bishopSq];
            mobilityScore[color] += MOBILITY[BISHOPS-1][count(mobilityMap)]
                                 + EXTENDED_CENTER_VAL * count(mobilityMap & EXTENDED_CENTER_SQS)
                                 + CENTER_BONUS * count(mobilityMap & CENTER_SQS);

            if (bit & ~pawnStopAtt[color^1] & OUTPOST_SQS[color]) {
                pieceEvalScore[color] += BISHOP_OUTPOST_BONUS;
                if (bit & ei.attackMaps[color][PAWNS])
                    pieceEvalScore[color] += BISHOP_OUTPOST_PAWN_DEF_BONUS;
            }
            else if (uint64_t potential = pml.get(i).legal & ~pawnStopAtt[color^1] & OUTPOST_SQS[color] & ~allPieces[color]) {
                pieceEvalScore[color] += BISHOP_POTENTIAL_OUTPOST_BONUS;
                if (potential & ei.attackMaps[color][PAWNS])
                    pieceEvalScore[color] += BISHOP_POTENTIAL_OUTPOST_PAWN_DEF_BONUS;
            }

            // A bonus for fianchettoed bishops that are not blocked by pawns
            // We can easily tell if a bishop is on the long diagonal since it can see two center squares at once
            uint64_t fianchettoBishop = b.getBishopSquares(bishopSq, pieces[WHITE][PAWNS] | pieces[BLACK][PAWNS]) & CENTER_SQS;
            if (fianchettoBishop & (fianchettoBishop - 1))
                pieceEvalScore[color] += BISHOP_FIANCHETTO_BONUS;
        }

        //---------------------------------Rooks------------------------------------
        for (unsigned int i = pml.starts[ROOKS]; i < pml.starts[QUEENS]; i++) {
            int rookSq = pml.get(i).startSq;
            int file = rookSq & 7;
            int rank = rookSq >> 3;
            uint64_t mobilityMap = pml.get(i).legal & mobilitySafeSqs;

            psqtScores[color] += PSQT[color][ROOKS][rookSq];
            mobilityScore[color] += MOBILITY[ROOKS-1][count(mobilityMap)]
                                 + EXTENDED_CENTER_VAL * count(mobilityMap & EXTENDED_CENTER_SQS)
                                 + CENTER_BONUS * count(mobilityMap & CENTER_SQS);

            // Bonus for having rooks on open or semiopen files
            if (FILES[file] & ei.openFiles)
                pieceEvalScore[color] += ROOK_OPEN_FILE_BONUS;
            else if (!(FILES[file] & pieces[color][PAWNS]))
                pieceEvalScore[color] += ROOK_SEMIOPEN_FILE_BONUS;
            // Bonus for having rooks on same ranks as enemy pawns
            if (relativeRank(color, rank) >= 4)
                pieceEvalScore[color] += ROOK_PAWN_RANK_THREAT * count(RANKS[rank] & pieces[color^1][PAWNS]);
        }

        //---------------------------------Queens-----------------------------------
        // For queen mobility, we also exclude squares not controlled by an opponent's minor or rook
        uint64_t queenMobilitySafeSqs = ~(ei.attackMaps[color^1][KNIGHTS] | ei.attackMaps[color^1][BISHOPS] | ei.attackMaps[color^1][ROOKS]);
        for (unsigned int i = pml.starts[QUEENS]; i < pml.size(); i++) {
            int queenSq = pml.get(i).startSq;
            uint64_t mobilityMap = pml.get(i).legal & mobilitySafeSqs & queenMobilitySafeSqs;

            psqtScores[color] += PSQT[color][QUEENS][queenSq];
            mobilityScore[color] += MOBILITY[QUEENS-1][count(mobilityMap)];

            // Penalty if an enemy knight can safely threaten our queen on the next move
            if (ei.attackMaps[color^1][KNIGHTS] & b.getKnightSquares(queenSq) & ~ei.attackMaps[color][PAWNS]
              & ~(ei.doubleAttackMaps[color] & ~ei.doubleAttackMaps[color^1]))
                pieceEvalScore[color] += KNIGHT_QUEEN_POTENTIAL_THREAT;
        }

        // King mobility
        uint64_t kingMobilityMap = kingNeighborhood[color] & mobilitySafeSqs & ~ei.fullAttackMaps[color^1];
        mobilityScore[color] += MOBILITY[KINGS-1][count(kingMobilityMap)];
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

    valueMg += decEvalMg(mobilityScore[WHITE]) - decEvalMg(mobilityScore[BLACK]);
    valueEg += decEvalEg(mobilityScore[WHITE]) - decEvalEg(mobilityScore[BLACK]);

    if (debug) {
        evalDebugStats.whiteMobilityScore = mobilityScore[WHITE];
        evalDebugStats.blackMobilityScore = mobilityScore[BLACK];
    }


    //-------------------------------Threats------------------------------------
    Score threatScore[2] = {EVAL_ZERO, EVAL_ZERO};

    for (int color = WHITE; color <= BLACK; color++) {
        uint64_t weakSqs = ~ei.attackMaps[color][PAWNS] & (ei.doubleAttackMaps[color^1] | ~ei.doubleAttackMaps[color]);
        // Pawns attacked by opposing pieces and not defended by own pawns
        if (uint64_t upawns = pieces[color][PAWNS] & ei.fullAttackMaps[color^1] & weakSqs)
            threatScore[color] += UNDEFENDED_PAWN * count(upawns);
        // Minors attacked by opposing pieces and not defended by own pawns
        if (uint64_t minors = (pieces[color][KNIGHTS] | pieces[color][BISHOPS]) & ei.fullAttackMaps[color^1] & weakSqs)
            threatScore[color] += UNDEFENDED_MINOR * count(minors);
        // Rooks attacked by opposing minors
        if (uint64_t rooks = pieces[color][ROOKS] & (ei.attackMaps[color^1][KNIGHTS] | ei.attackMaps[color^1][BISHOPS]))
            threatScore[color] += MINOR_ROOK_THREAT * count(rooks);
        // Queens attacked by opposing minors
        if (uint64_t queens = pieces[color][QUEENS] & (ei.attackMaps[color^1][KNIGHTS] | ei.attackMaps[color^1][BISHOPS]))
            threatScore[color] += MINOR_QUEEN_THREAT * count(queens);
        // Queens attacked by opposing rooks
        if (uint64_t queens = pieces[color][QUEENS] & ei.attackMaps[color^1][ROOKS])
            threatScore[color] += ROOK_QUEEN_THREAT * count(queens);
        // Pieces attacked by opposing pawns
        if (uint64_t threatened = (pieces[color][KNIGHTS] | pieces[color][BISHOPS]
                                 | pieces[color][ROOKS]   | pieces[color][QUEENS]) & ei.attackMaps[color^1][PAWNS])
            threatScore[color] += PAWN_PIECE_THREAT * count(threatened);
        // Loose pawns: pawns in opponent's half of the board with no defenders
        if (uint64_t lpawns = pieces[color][PAWNS] & HALF[color^1] & ~(ei.fullAttackMaps[color] | ei.attackMaps[color][PAWNS]))
            threatScore[color] += LOOSE_PAWN * count(lpawns);
        // Loose minors
        if (uint64_t lminors = (pieces[color][KNIGHTS] | pieces[color][BISHOPS]) & HALF[color^1] & ~(ei.fullAttackMaps[color] | ei.attackMaps[color][PAWNS]))
            threatScore[color] += LOOSE_MINOR * count(lminors);
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
            if (!(indexToBit(passerSq+8) & allPieces[BLACK])) {
                uint64_t pathToQueen = indexToBit(passerSq);
                pathToQueen |= pathToQueen << 8;
                pathToQueen |= pathToQueen << 16;
                pathToQueen |= pathToQueen << 32;

                // Consider x-ray attacks of rooks and queens behind passers
                uint64_t rookBehind = indexToBit(passerSq);
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
                else if (!(indexToBit(passerSq+8) & wBlock))
                    whitePawnScore += rFactor * FREE_STOP_BONUS;
                // Path to queen is completely defended by own pieces
                if ((pathToQueen & wDefend) == pathToQueen)
                    whitePawnScore += rFactor * FULLY_DEFENDED_PASSER_BONUS;
                // Stop square is defended by own pieces
                else if (indexToBit(passerSq+8) & wDefend)
                    whitePawnScore += rFactor * DEFENDED_PASSER_BONUS;
            }

            // Bonuses and penalties for king distance
            whitePawnScore -= OWN_KING_DIST * (int) kingDistance[passerSq+8][kingSq[WHITE]] * rFactor;
            whitePawnScore += OPP_KING_DIST * (int) kingDistance[passerSq+8][kingSq[BLACK]] * rFactor;
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
            if (!(indexToBit(passerSq-8) & allPieces[WHITE])) {
                uint64_t pathToQueen = indexToBit(passerSq);
                pathToQueen |= pathToQueen >> 8;
                pathToQueen |= pathToQueen >> 16;
                pathToQueen |= pathToQueen >> 32;

                uint64_t rookBehind = indexToBit(passerSq);
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
                else if (!(indexToBit(passerSq-8) & bBlock))
                    blackPawnScore += rFactor * FREE_STOP_BONUS;
                if ((pathToQueen & bDefend) == pathToQueen)
                    blackPawnScore += rFactor * FULLY_DEFENDED_PASSER_BONUS;
                else if (indexToBit(passerSq-8) & bDefend)
                    blackPawnScore += rFactor * DEFENDED_PASSER_BONUS;
            }

            blackPawnScore += OPP_KING_DIST * (int) kingDistance[passerSq-8][kingSq[WHITE]] * rFactor;
            blackPawnScore -= OWN_KING_DIST * (int) kingDistance[passerSq-8][kingSq[BLACK]] * rFactor;
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
        if (wIsolated & indexToBit(f)) {
            whitePawnScore += ISOLATED_PENALTY * wPawnCtByFile[f];
            if (!(FILES[f] & pieces[BLACK][PAWNS]) && (pieces[BLACK][QUEENS] | pieces[BLACK][ROOKS]))
                whitePawnScore += ISOLATED_SEMIOPEN_PENALTY * wPawnCtByFile[f];
        }
        if (bIsolated & indexToBit(f)) {
            blackPawnScore += ISOLATED_PENALTY * bPawnCtByFile[f];
            if (!(FILES[f] & pieces[WHITE][PAWNS]) && (pieces[WHITE][QUEENS] | pieces[WHITE][ROOKS]))
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
        if (!(FILES[f] & pieces[BLACK][PAWNS]) && (pieces[BLACK][QUEENS] | pieces[BLACK][ROOKS])) {
            whitePawnScore += BACKWARD_SEMIOPEN_PENALTY;
        }
    }
    uint64_t bBackwardsTemp = bBackwards;
    while (bBackwardsTemp) {
        int pawnSq = bitScanForward(bBackwardsTemp);
        bBackwardsTemp &= bBackwardsTemp - 1;
        int f = pawnSq & 7;
        if (!(FILES[f] & pieces[WHITE][PAWNS]) && (pieces[WHITE][QUEENS] | pieces[WHITE][ROOKS])) {
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
    while (wPawnPhalanx) {
        int pawnSq = bitScanForward(wPawnPhalanx);
        wPawnPhalanx &= wPawnPhalanx - 1;
        int r = pawnSq >> 3;
        int bonus = PAWN_PHALANX_BONUS[r];
        whitePawnScore += bonus;
        int f = pawnSq & 7;
        if (!(FILES[f] & pieces[BLACK][PAWNS]))
            whitePawnScore += bonus;
    }
    while (bPawnPhalanx) {
        int pawnSq = bitScanForward(bPawnPhalanx);
        bPawnPhalanx &= bPawnPhalanx - 1;
        int r = 7 - (pawnSq >> 3);
        int bonus = PAWN_PHALANX_BONUS[r];
        blackPawnScore += bonus;
        int f = pawnSq & 7;
        if (!(FILES[f] & pieces[WHITE][PAWNS]))
            blackPawnScore += bonus;
    }

    // Other connected pawns
    uint64_t wConnected = pieces[WHITE][PAWNS] & ei.attackMaps[WHITE][PAWNS];
    uint64_t bConnected = pieces[BLACK][PAWNS] & ei.attackMaps[BLACK][PAWNS];
    while (wConnected) {
        int pawnSq = bitScanForward(wConnected);
        wConnected &= wConnected - 1;
        int r = pawnSq >> 3;
        int bonus = PAWN_CONNECTED_BONUS[r];
        whitePawnScore += bonus;
        int f = pawnSq & 7;
        if (!(FILES[f] & pieces[BLACK][PAWNS]))
            whitePawnScore += bonus;
    }
    while (bConnected) {
        int pawnSq = bitScanForward(bConnected);
        bConnected &= bConnected - 1;
        int r = 7 - (pawnSq >> 3);
        int bonus = PAWN_CONNECTED_BONUS[r];
        blackPawnScore += bonus;
        int f = pawnSq & 7;
        if (!(FILES[f] & pieces[WHITE][PAWNS]))
            blackPawnScore += bonus;
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

            wTropismTotal += (int)manhattanDistance[pawnSq][kingSq[WHITE]];
            bTropismTotal += (int)manhattanDistance[pawnSq][kingSq[BLACK]];
            pawnWeight++;
        }

        if (pawnWeight)
            kingPawnTropism = (bTropismTotal - wTropismTotal) / pawnWeight;

        valueEg += KING_TROPISM_VALUE * kingPawnTropism;
    }


    // Adjust endgame eval based on the probability of converting the advantage to a win
    if (egFactor > 0) {
        uint64_t wPawnAsymmetry = pieces[WHITE][PAWNS];
        wPawnAsymmetry |= wPawnAsymmetry >> 8;
        wPawnAsymmetry |= wPawnAsymmetry >> 16;
        wPawnAsymmetry |= wPawnAsymmetry >> 32;
        wPawnAsymmetry &= 0xFF;
        uint64_t bPawnAsymmetry = pieces[BLACK][PAWNS];
        bPawnAsymmetry |= bPawnAsymmetry >> 8;
        bPawnAsymmetry |= bPawnAsymmetry >> 16;
        bPawnAsymmetry |= bPawnAsymmetry >> 32;
        bPawnAsymmetry &= 0xFF;
        // Asymmetry: greater asymmetry means less locked position, more potential passers
        int pawnAsymmetry = count((wPawnAsymmetry & ~bPawnAsymmetry) | (~wPawnAsymmetry & bPawnAsymmetry));
        // King opposition distance: when kings are farther apart by file, there is a
        // lower chance of the defending king keeping the attacking king from penetrating
        int oppositionDistance = std::abs((kingSq[WHITE] & 7)  - (kingSq[BLACK] & 7))
                               - std::abs((kingSq[WHITE] >> 3) - (kingSq[BLACK] >> 3));

        int egWinAdjustment = PAWN_ASYMMETRY_BONUS * pawnAsymmetry
                            + PAWN_COUNT_BONUS * (pieceCounts[WHITE][PAWNS] + pieceCounts[BLACK][PAWNS])
                            + KING_OPPOSITION_DISTANCE_BONUS * oppositionDistance
                            + ENDGAME_BASE;
        // Cap the penalty at reducing to a score of 0
        if (valueEg > 0)
            valueEg = std::max(0, valueEg + egWinAdjustment);
        else if (valueEg < 0)
            valueEg = std::min(0, valueEg - egWinAdjustment);
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
    for (int color = WHITE; color <= BLACK; color++) {
        if (material[MG][color] - material[MG][color^1] > 0
         && material[MG][color] - material[MG][color^1] <= PIECE_VALUES[MG][KNIGHTS]
         && pieceCounts[color][PAWNS] <= 1
         && totalEval * (1 - 2 * color) > 0) {
            if (pieceCounts[color][PAWNS] == 0) {
                if (material[MG][color] < PIECE_VALUES[MG][BISHOPS] + 50)
                    scaleFactor = PAWNLESS_SCALING[0];
                else if (material[MG][color^1] <= PIECE_VALUES[MG][BISHOPS])
                    scaleFactor = PAWNLESS_SCALING[1];
                else
                    scaleFactor = PAWNLESS_SCALING[2];
            }
            else if (scaleFactor != OPPOSITE_BISHOP_SCALING[0])
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

// King safety, based on the number of opponent pieces near the king
// The lookup table approach is inspired by Ed Schroder's Rebel chess engine,
// and by Stockfish
template <int attackingColor>
int Eval::getKingSafety(Board &b, PieceMoveList &attackers, uint64_t kingSqs, int pawnScore, int kingFile) {
    // Precalculate a few things
    uint64_t kingNeighborhood = (attackingColor == WHITE) ? ((pieces[BLACK][KINGS] & RANK_8) ? (kingSqs | (kingSqs >> 8)) : kingSqs)
                                                          : ((pieces[WHITE][KINGS] & RANK_1) ? (kingSqs | (kingSqs << 8)) : kingSqs);
    if (kingFile == 7)
        kingNeighborhood |= kingNeighborhood >> 1;
    else if (kingFile == 0)
        kingNeighborhood |= kingNeighborhood << 1;

    uint64_t weakMap = ~ei.doubleAttackMaps[attackingColor^1]
                     & ((~ei.attackMaps[attackingColor^1][PAWNS] & ~ei.fullAttackMaps[attackingColor^1]) | ei.attackMaps[attackingColor^1][QUEENS]);
    // Analyze undefended squares directly adjacent to king
    uint64_t kingDefenseless = kingSqs & weakMap;

    // Holds the king safety score
    int kingSafetyPts = KS_BASE;
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
    }

    // Add bonuses for safe checks
    kingSafetyPts += SAFE_CHECK_BONUS[KNIGHTS-1] * !!(ei.attackMaps[attackingColor][KNIGHTS] & checkMaps[KNIGHTS-1] & ~kingSqs & weakMap);
    kingSafetyPts += SAFE_CHECK_BONUS[BISHOPS-1] * !!(ei.attackMaps[attackingColor][BISHOPS] & checkMaps[BISHOPS-1] & ~kingSqs & weakMap);
    kingSafetyPts += SAFE_CHECK_BONUS[ROOKS-1] * !!(ei.attackMaps[attackingColor][ROOKS] & checkMaps[ROOKS-1] & ~kingSqs & weakMap);
    kingSafetyPts += SAFE_CHECK_BONUS[QUEENS-1] * !!(ei.attackMaps[attackingColor][QUEENS] & checkMaps[QUEENS-1] & ~kingSqs & weakMap
                                                  & ~ei.attackMaps[attackingColor^1][QUEENS]);

    // Give a decent bonus for each additional piece participating
    kingSafetyPts += kingAttackPieces * kingAttackPts;

    // King pressure: a smaller bonus for attacker's pieces generally pointed at
    // the region of the opposing king
    const uint64_t kingZone = KING_ZONE_DEFENDER[attackingColor^1] & KING_ZONE_FLANK[kingFile];
    int kingPressure = KING_PRESSURE * (count(ei.fullAttackMaps[attackingColor] & kingZone)
                                      + count(ei.doubleAttackMaps[attackingColor] & ~ei.attackMaps[attackingColor^1][PAWNS] & kingZone));

    // Adjust based on pawn shield, storms, and king pressure
    kingSafetyPts += (-KS_PAWN_FACTOR * pawnScore + KS_KING_PRESSURE_FACTOR * kingPressure) / 32;

    // Bonus for the attacker if the defender has no defending minors
    const uint64_t kingDefenseZone = KING_DEFENSE_ZONE[kingFile] & HALF[attackingColor^1];
    kingSafetyPts += KS_NO_KNIGHT_DEFENDER
                   * (!(kingDefenseZone & ei.attackMaps[attackingColor^1][KNIGHTS])
                    + !(kingZone & ei.attackMaps[attackingColor^1][KNIGHTS]))
                   * pieceCounts[attackingColor^1][KNIGHTS];
    kingSafetyPts += KS_NO_BISHOP_DEFENDER
                   * (!(kingDefenseZone & ei.attackMaps[attackingColor^1][BISHOPS])
                    + !(kingZone & ei.attackMaps[attackingColor^1][BISHOPS]))
                   * pieceCounts[attackingColor^1][BISHOPS];

    // Look for central pawn structures that support slider king attacks
    constexpr uint64_t QSIDE_DIAG_REGION[2] = {FILE_F | ((FILE_E | FILE_D) & (RANK_3 | RANK_4 | RANK_5 | RANK_6)),
                                               FILE_F | FILE_E | FILE_D};
    constexpr uint64_t KSIDE_DIAG_REGION[2] = {FILE_C | ((FILE_D | FILE_E) & (RANK_3 | RANK_4 | RANK_5 | RANK_6)),
                                               FILE_C | FILE_D | FILE_E};
    auto attackerBishopFactor = [this](int color, uint64_t diagonalZone, uint64_t diagonal) -> int {
        int c = count(diagonal);
        return KS_BISHOP_PRESSURE * (c * (c+1) / 2 + !!(diagonalZone & ei.attackMaps[color][BISHOPS]) - 1);
    };
    auto defenderBishopFactor = [this](uint64_t diagonal) -> int {
        int c = count(diagonal);
        return KS_BISHOP_PRESSURE * (c * (c+1) / 2 - 1);
    };
    if (kingFile < 3) {
        if (attackingColor == WHITE) {
            if (uint64_t diagonal = ((pieces[WHITE][PAWNS] & QSIDE_DIAG_REGION[0]) << 7) & pieces[WHITE][PAWNS])
                kingSafetyPts += attackerBishopFactor(WHITE, kingDefenseZone, diagonal);
            if (uint64_t diagonal = ((pieces[BLACK][PAWNS] & KSIDE_DIAG_REGION[1]) >> 7) & pieces[BLACK][PAWNS])
                kingSafetyPts += defenderBishopFactor(diagonal);
        }
        else {
            if (uint64_t diagonal = ((pieces[BLACK][PAWNS] & QSIDE_DIAG_REGION[0]) >> 9) & pieces[BLACK][PAWNS])
                kingSafetyPts += attackerBishopFactor(BLACK, kingDefenseZone, diagonal);
            if (uint64_t diagonal = ((pieces[WHITE][PAWNS] & KSIDE_DIAG_REGION[1]) << 9) & pieces[WHITE][PAWNS])
                kingSafetyPts += defenderBishopFactor(diagonal);
        }
    }
    else if (kingFile > 4) {
        if (attackingColor == WHITE) {
            if (uint64_t diagonal = ((pieces[WHITE][PAWNS] & KSIDE_DIAG_REGION[0]) << 9) & pieces[WHITE][PAWNS])
                kingSafetyPts += attackerBishopFactor(WHITE, kingDefenseZone, diagonal);
            if (uint64_t diagonal = ((pieces[BLACK][PAWNS] & QSIDE_DIAG_REGION[1]) >> 9) & pieces[BLACK][PAWNS])
                kingSafetyPts += defenderBishopFactor(diagonal);
        }
        else {
            if (uint64_t diagonal = ((pieces[BLACK][PAWNS] & KSIDE_DIAG_REGION[0]) >> 7) & pieces[BLACK][PAWNS])
                kingSafetyPts += attackerBishopFactor(BLACK, kingDefenseZone, diagonal);
            if (uint64_t diagonal = ((pieces[WHITE][PAWNS] & QSIDE_DIAG_REGION[1]) << 7) & pieces[WHITE][PAWNS])
                kingSafetyPts += defenderBishopFactor(diagonal);
        }
    }

    // Reduce attack when attacker has no queen
    kingSafetyPts += KS_NO_QUEEN * (pieces[attackingColor][QUEENS] == 0);


    // Convert king safety points into centipawns using a quadratic relationship
    kingSafetyPts = std::max(0, kingSafetyPts);
    return std::min(kingSafetyPts * kingSafetyPts / KS_ARRAY_FACTOR, 600) + kingPressure;
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
                    value -= 20 * (int)std::min(manhattanDistance[bKingSq][7], manhattanDistance[bKingSq][56]);
                // Dark squared corners are A1 (0) and H8 (63)
                else
                    value -= 20 * (int)std::min(manhattanDistance[bKingSq][0], manhattanDistance[bKingSq][63]);
                return value;
            }
            if (pieces[BLACK][KNIGHTS] && pieces[BLACK][BISHOPS]) {
                int value = -KNOWN_WIN;
                int wKingSq = bitScanForward(pieces[WHITE][KINGS]);
                int bKingSq = bitScanForward(pieces[BLACK][KINGS]);
                value += scoreCornerDistance(BLACK, wKingSq, bKingSq);

                // Light squared corners are H1 (7) and A8 (56)
                if (pieces[BLACK][BISHOPS] & LIGHT)
                    value += 20 * (int)std::min(manhattanDistance[wKingSq][7], manhattanDistance[wKingSq][56]);
                // Dark squared corners are A1 (0) and H8 (63)
                else
                    value += 20 * (int)std::min(manhattanDistance[wKingSq][0], manhattanDistance[wKingSq][63]);
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
