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

#ifndef __EVAL_H__
#define __EVAL_H__

#include <cstring>
#include "board.h"
#include "common.h"

class Board;

void initEvalTables();
void initDistances();
void setMaterialScale(int s);
void setKingSafetyScale(int s);

struct EvalInfo {
    uint64_t attackMaps[2][5];
    uint64_t fullAttackMaps[2];
    uint64_t doubleAttackMaps[2];
    uint64_t rammedPawns[2];
    uint64_t openFiles;

    void clear() {
        std::memset(this, 0, sizeof(EvalInfo));
    }
};

class Eval {
public:
    template <bool debug = false> int evaluate(Board &b);

private:
    EvalInfo ei;
    uint64_t pieces[2][6];
    uint64_t allPieces[2];
    int pieceCounts[2][6];
    int playerToMove;

    // Eval helpers
    template <int attackingColor>
    int getKingSafety(Board &b, PieceMoveList &attackers, uint64_t kingSqs, int pawnScore, int kingFile);
    int checkEndgameCases();
    int scoreSimpleKnownWin(int winningColor);
    int scoreCornerDistance(int winningColor, int wKingSq, int bKingSq);
};


constexpr int EG_FACTOR_PIECE_VALS[5] = {33, 371, 364, 685, 1562};
constexpr int EG_FACTOR_ALPHA = 2010;
constexpr int EG_FACTOR_BETA = 6410;
constexpr int EG_FACTOR_RES = 1000;

// Eval scores are packed into an unsigned 32-bit integer during calculations
// (the SWAR technique)
typedef uint32_t Score;

// Encodes 16-bit midgame and endgame evaluation scores into a single int
#define E(mg, eg) ((Score) ((int32_t) (((uint32_t) eg) << 16) + ((int32_t) mg)))

// Retrieves the final evaluation score to return from the packed eval value
inline int decEvalMg(Score encodedValue) {
    return (int) (encodedValue & 0xFFFF) - 0x8000;
}

inline int decEvalEg(Score encodedValue) {
    return (int) (encodedValue >> 16) - 0x8000;
}

// Since we can only work with unsigned numbers due to carryover / twos-complement
// negative number issues, we make 2^15 the 0 point for each of the two 16-bit
// halves of Score
constexpr Score EVAL_ZERO = 0x80008000;

// Array indexing constants
constexpr int MG = 0;
constexpr int EG = 1;

// Material constants
constexpr int PIECE_VALUES[2][5] = {
    {100, 405, 452, 720, 1379},
    {138, 410, 461, 766, 1497}
};
constexpr int KNOWN_WIN = PIECE_VALUES[EG][PAWNS] * 75;
constexpr int TB_WIN = PIECE_VALUES[EG][PAWNS] * 125;

//------------------------------Piece tables--------------------------------
constexpr int pieceSquareTable[2][6][32] = {
// Midgame
{
{ // Pawns
  0,  0,  0,  0,
 22,  7, 45, 55,
  5, 13, 38, 44,
 -2,  0,  0, 10,
-14, -5,  0, 14,
 -9, -1,  0,  0,
 -3, 10,  4,  7,
  0,  0,  0,  0
},
{ // Knights
-129,-42,-41,-32,
-16, -8, 10, 24,
 -1,  9, 18, 28,
 28, 15, 31, 32,
 10, 12, 18, 23,
-11,  9,  6, 16,
 -8,-11,  0,  6,
-62, -9,-13, -7
},
{ // Bishops
-29,-33,-23,-26,
-34,-39,-15,-20,
 15, -2, -6,  5,
  6, 20,  6, 19,
 21, 10,  8, 22,
 14, 18,  0, 11,
 15,  7, 15,  9,
 -7, 14, -8,  1
},
{ // Rooks
 -5,  0,  0,  0,
  5, 10, 10, 10,
 -5,  0,  0,  0,
 -5,  0,  0,  0,
 -5,  0,  0,  0,
 -5,  0,  0,  0,
 -5,  0,  0,  0,
 -5,  0,  0,  0
},
{ // Queens
-19, -1,  0,  0,
-12,-31, -7, -8,
 -2,  4,  5,  5,
  0, -7, -7,-19,
 -1,  1, -6,-19,
  3, 15, -3,  2,
 -1, 10, 11,  6,
-10,-16,-13,  4
},
{ // Kings
-37,-32,-34,-45,
-34,-28,-32,-38,
-32,-24,-28,-30,
-31,-27,-30,-31,
-32,-20,-34,-32,
 -4, 20,-13,-23,
 43, 57, 11,-19,
 31, 63, 19,-21
}
},
// Endgame
{
{ // Pawns
  0,  0,  0,  0,
 34, 34, 30, 28,
 30, 30, 24, 20,
 11, 11,  6,  2,
 -5, -5, -4, -4,
-12, -5,  0,  2,
-12, -5,  4,  6,
  0,  0,  0,  0
},
{ // Knights
-64,-21, -8, -3,
-10,  4,  7, 15,
  4,  6, 12, 18,
 14,  9, 20, 27,
  4, 12, 13, 20,
 -5, -5,  4, 14,
 -9,  3, -3,  4,
-24, -1, -4,  0
},
{ // Bishops
-11,  0,  0, -5
 -6,  0, -2,  4,
  3, -4,  1,  1,
  3,  2,  2,  4,
 -3,  2,  2,  4,
 -4,  3,  5,  5,
 -4, -4, -2, -2,
-10, -5,  1,  1
},
{ // Rooks
  0,  0,  0,  0,
  0,  0,  0,  0,
  0,  0,  0,  0,
  0,  0,  0,  0,
  0,  0,  0,  0,
  0,  0,  0,  0,
  0,  0,  0,  0,
  0,  0,  0,  0
},
{ // Queens
 -9,  5, 10,  7,
  3, 12, 13, 18,
  3, 16, 19, 35,
  7, 22, 25, 32,
  7, 20, 23, 32,
-12,  8, 12, 12,
-22,-17,-11, -6,
-30,-20,-18,-20
},
{ // Kings
-79,-16,-11, -9,
-20, 32, 40, 40,
 12, 44, 46, 49,
 -6, 33, 38, 42,
-17, 17, 28, 34,
-24, -1, 16, 21,
-39,-13,  2,  7,
-74,-45,-28,-25
}
}
};

//-------------------------Material eval constants------------------------------
constexpr int BISHOP_PAIR_VALUE = 55;
constexpr int TEMPO_VALUE = 25;

// Material imbalance terms
constexpr int OWN_OPP_IMBALANCE[2][5][5] = {
{
//       Opponent's
//    P   N   B   R   Q
    { 0},                   // Own pawns
    { 5,  0},               // Own knights
    { 2, -2,  0},           // Own bishops
    { 1, -4,-17,  0},       // Own rooks
    {-4,-20,-20, -19,  0}    // Own queens
},
{
    { 0},                   // Own pawns
    { 6,  0},               // Own knights
    { 6,  0,  0},           // Own bishops
    { 2,-19,-17,  0},       // Own rooks
    {27, 14, 24, 24,  0}    // Own queens
}
};

// Bonus for knight in closed positions
constexpr int KNIGHT_CLOSED_BONUS[2] = {2, 8};

//------------------------Positional eval constants-----------------------------
// SPACE_BONUS[0][0] = behind own pawn, not center files
// SPACE_BONUS[0][1] = behind own pawn, center files
// SPACE_BONUS[1][0] = in front of opp pawn, not center files
// SPACE_BONUS[1][1] = in front of opp pawn, center files
constexpr int SPACE_BONUS[2][2] = {{19, 46}, {1, 18}};

// Mobility tables
constexpr int mobilityTable[2][5][28] = {
// Midgame
{
{ // Knights
-54, -3, 14, 29, 36, 40, 44, 48, 52},
{ // Bishops
-50,-15,  9, 16, 22, 25, 26, 27, 25, 30, 40, 51, 55, 65},
{ // Rooks
-95,-60,-15, -4,  1,  6, 10, 15, 19, 24, 28, 30, 32, 36, 39},
{ // Queens
-102,-88,-63,-39,-26,-16,-11, -8, -5, -3, -1,  2,  5,  7,
 10, 12, 15, 17, 19, 21, 23, 25, 26, 27, 29, 30, 31, 32},
{ // Kings
-13, 22, 27, 20, 12,  5, -1, -7, -9}
},

// Endgame
{
{ // Knights
-98,-49, -4, 12, 21, 28, 32, 34, 36},
{ // Bishops
-98,-42,-15,  5, 14, 23, 28, 30, 35, 35, 36, 39, 42, 47},
{ // Rooks
-108,-64, -4, 28, 41, 50, 56, 62, 65, 71, 75, 81, 85, 90, 96},
{ // Queens
-104,-80,-64,-43,-24,-19,-10, -2,  4, 10, 15, 18, 20, 23,
 25, 27, 29, 31, 33, 35, 37, 39, 41, 43, 45, 47, 49, 51},
{ // Kings
-59,-15,  3, 15, 19, 11, 16, 14, -1}
}
};

// Value of each square in the extended center in cp
constexpr Score EXTENDED_CENTER_VAL = E(2, 0);
// Additional bonus for squares in the center four squares in cp, in addition
// to EXTENDED_CENTER_VAL
constexpr Score CENTER_BONUS = E(4, 0);

// King safety
// The value of having 0, 1, and both castling rights
constexpr int CASTLING_RIGHTS_VALUE[3] = {0, 33, 76};
// The value of a pawn shield per pawn. First rank value is used for the
// penalty when the pawn is missing.
constexpr int PAWN_SHIELD_VALUE[4][8] = {
    {-16, 23, 26, 12, 10,  8, 15,  0}, // open h file, h2, h3, ...
    {-22, 38, 23, -8, -6, -1,  5,  0}, // g/b file
    {-13, 41,  4, -7, -6, -5,  3,  0}, // f/c file
    { -2, 16, 12,  8, -6,-12, -2,  0}  // d/e file
};
// Array for pawn storm values. Rank 1 of open is used for penalty
// when there is no opposing pawn
constexpr int PAWN_STORM_VALUE[3][4][8] = {
// Open file
{
    {22,-37, 27, 16, 13,  0,  0,  0},
    {16,-37, 48, 13,  6,  0,  0,  0},
    { 7, 17, 52, 27, 18,  0,  0,  0},
    {11,-13, 31, 17, 15,  0,  0,  0}
},
// Blocked pawn
{
    { 0,  0, 28,-10, -4,  0,  0,  0},
    { 0,  0, 60, -5, -2,  0,  0,  0},
    { 0,  0, 66, -1, -2,  0,  0,  0},
    { 0,  0, 52, 14,  1,  0,  0,  0}
},
// Non-blocked pawn
{
    { 0,  3, 21, 12,  1,  0,  0,  0},
    { 0,-13, 26, 14, 10,  0,  0,  0},
    { 0, -4, 36, 27,  9,  0,  0,  0},
    { 0, -3,  5, 25,  5,  0,  0,  0}
},
};
// Penalty when the enemy king can use a storming pawn as protection
constexpr int PAWN_STORM_SHIELDING_KING = -148;

// Scale factor for pieces attacking opposing king
constexpr int KS_ARRAY_FACTOR = 128;
constexpr int KING_THREAT_MULTIPLIER[4] = {8, 5, 7, 3};
constexpr int KING_THREAT_SQUARE[4] = {8, 10, 7, 9};
constexpr int KING_DEFENSELESS_SQUARE = 24;
constexpr int KS_PAWN_FACTOR = 10;
constexpr int KING_PRESSURE = 3;
constexpr int KS_KING_PRESSURE_FACTOR = 27;
constexpr int KS_NO_KNIGHT_DEFENDER = 15;
constexpr int KS_NO_BISHOP_DEFENDER = 15;
constexpr int KS_BISHOP_PRESSURE = 8;
constexpr int KS_NO_QUEEN = -40;
constexpr int KS_BASE = -18;
constexpr int SAFE_CHECK_BONUS[4] = {54, 23, 68, 51};

// Minor pieces
// A penalty for each own pawn that is on a square of the same color as your bishop
constexpr Score BISHOP_PAWN_COLOR_PENALTY = E(-3, -8);
constexpr Score BISHOP_RAMMED_PAWN_COLOR_PENALTY = E(-5, -5);
// Minors shielded by own pawn in front
constexpr Score SHIELDED_MINOR_BONUS = E(16, 0);
// A bonus for strong outpost knights
constexpr Score KNIGHT_OUTPOST_BONUS = E(30, 25);
constexpr Score KNIGHT_OUTPOST_PAWN_DEF_BONUS = E(24, 10);
constexpr Score KNIGHT_POTENTIAL_OUTPOST_BONUS = E(10, 15);
constexpr Score KNIGHT_POTENTIAL_OUTPOST_PAWN_DEF_BONUS = E(12, 15);
// A smaller bonus for bishops
constexpr Score BISHOP_OUTPOST_BONUS = E(23, 22);
constexpr Score BISHOP_OUTPOST_PAWN_DEF_BONUS = E(34, 10);
constexpr Score BISHOP_POTENTIAL_OUTPOST_BONUS = E(6, 9);
constexpr Score BISHOP_POTENTIAL_OUTPOST_PAWN_DEF_BONUS = E(16, 9);
// A bonus for fianchettoed bishops that are not blocked by pawns
constexpr Score BISHOP_FIANCHETTO_BONUS = E(31, 0);

// Rooks
constexpr Score ROOK_OPEN_FILE_BONUS = E(36, 13);
constexpr Score ROOK_SEMIOPEN_FILE_BONUS = E(21, 4);
constexpr Score ROOK_PAWN_RANK_THREAT = E(4, 15);

// Queens
constexpr Score KNIGHT_QUEEN_POTENTIAL_THREAT = E(-15, -5);

// Threats
constexpr Score UNDEFENDED_PAWN = E(-3, -12);
constexpr Score UNDEFENDED_MINOR = E(-27, -41);
constexpr Score PAWN_PIECE_THREAT = E(-79, -28);
constexpr Score MINOR_ROOK_THREAT = E(-80, -23);
constexpr Score MINOR_QUEEN_THREAT = E(-78, -36);
constexpr Score ROOK_QUEEN_THREAT = E(-81, -33);

constexpr Score LOOSE_PAWN = E(-16, -4);
constexpr Score LOOSE_MINOR = E(-16, -6);

// Pawn structure
// Passed pawns
constexpr Score PASSER_BONUS[8] = {E(  0,   0), E( -4,   3), E( -4,  3), E(  7,  17),
                                   E( 29,  24), E( 58,  56), E(119,128), E(  0,   0)};
constexpr Score PASSER_FILE_BONUS[8] = {E( 25, 20), E( 11, 15), E(-14,  5), E(-14, -7),
                                        E(-14, -7), E(-14,  5), E( 11, 15), E( 25, 20)};
constexpr Score FREE_PROMOTION_BONUS = E(6, 21);
constexpr Score FREE_STOP_BONUS = E(5, 9);
constexpr Score FULLY_DEFENDED_PASSER_BONUS = E(8, 13);
constexpr Score DEFENDED_PASSER_BONUS = E(10, 9);
constexpr Score OWN_KING_DIST = E(2, 3);
constexpr Score OPP_KING_DIST = E(2, 6);

// Doubled pawns
constexpr Score DOUBLED_PENALTY = E(0, -18);
// Isolated pawns
constexpr Score ISOLATED_PENALTY = E(-16, -9);
constexpr Score ISOLATED_SEMIOPEN_PENALTY = E(-3, -10);
// Backward pawns
constexpr Score BACKWARD_PENALTY = E(-8, -6);
constexpr Score BACKWARD_SEMIOPEN_PENALTY = E(-18, -9);
// Undefended pawns that are not backwards or isolated
constexpr Score UNDEFENDED_PAWN_PENALTY = E(-3, 0);
// Pawn phalanxes
constexpr Score PAWN_PHALANX_BONUS[8] = {E( 0,  0), E( 5,  1), E( 4,  2), E(11,  8),
                                         E(31, 19), E(62, 46), E(82, 79), E( 0,  0)};
// Connected pawns
constexpr Score PAWN_CONNECTED_BONUS[8] = {E( 0,  0), E( 0,  0), E(15,  8), E(10, 10),
                                           E(19, 11), E(36, 31), E(72, 62), E( 0,  0)};
// King-pawn tropism
constexpr int KING_TROPISM_VALUE = 18;

// Endgame win probability adjustment
constexpr int PAWN_ASYMMETRY_BONUS = 5;
constexpr int PAWN_COUNT_BONUS = 8;
constexpr int KING_OPPOSITION_DISTANCE_BONUS = 2;
constexpr int ENDGAME_BASE = -54;

// Scale factors for drawish endgames
constexpr int MAX_SCALE_FACTOR = 32;
constexpr int OPPOSITE_BISHOP_SCALING[2] = {12, 29};
constexpr int PAWNLESS_SCALING[4] = {1, 3, 7, 22};


#undef E

#endif
