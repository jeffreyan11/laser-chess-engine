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


constexpr int EG_FACTOR_PIECE_VALS[5] = {37, 369, 372, 683, 1561};
constexpr int EG_FACTOR_ALPHA = 2080;
constexpr int EG_FACTOR_BETA = 6360;
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
    {100, 404, 448, 711, 1368},
    {128, 406, 452, 745, 1458}
};
constexpr int KNOWN_WIN = PIECE_VALUES[EG][PAWNS] * 75;
constexpr int TB_WIN = PIECE_VALUES[EG][PAWNS] * 125;

//------------------------------Piece tables--------------------------------
constexpr int pieceSquareTable[2][6][32] = {
// Midgame
{
{ // Pawns
  0,  0,  0,  0,
 16,  6, 40, 48,
 10, 15, 36, 42,
 -4,  2,  2, 10,
-13, -6,  0, 10,
-11, -2,  0,  2,
 -5,  6,  1,  3,
  0,  0,  0,  0
},
{ // Knights
-127,-42,-39,-32,
-20, -8,  2, 18,
 -5,  6, 15, 25,
 21, 11, 28, 32,
  6,  9, 18, 24,
-11,  9,  6, 14,
-17, -8, -3,  7,
-57,-16,-13,-10
},
{ // Bishops
-21,-27,-20,-20,
-30,-24, -6,-15,
  6,  2, -4,  4,
  1, 14,  5, 15,
  9,  7,  6, 15,
  4, 14,  5, 10,
  5,  6, 10,  5,
 -8,  5, -8, -2
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
-19,-10, -5, -5,
-12,-21, -7, -6,
 -5, -3,  0,  7,
 -5, -3, -3, -9,
 -1,  1, -3, -9,
  1, 10, -1, -2,
-10,  3,  8,  6,
-10,-18,-14,  2
},
{ // Kings
-37,-32,-34,-45,
-34,-28,-32,-38,
-32,-24,-28,-30,
-31,-27,-30,-31,
-35,-20,-32,-32,
 -4, 20,-14,-23,
 40, 54, 11,-16,
 31, 61, 22,-18
}
},
// Endgame
{
{ // Pawns
  0,  0,  0,  0,
 24, 28, 30, 30,
 26, 26, 20, 20,
 10, 10,  2,  2,
 -5,  0, -2, -2,
-12, -3,  0,  0,
-12, -3,  2,  2,
  0,  0,  0,  0
},
{ // Knights
-65,-31,-22,-11,
 -6,  0,  4, 12,
  0,  4, 12, 18,
 10, 11, 18, 25,
  3, 13, 16, 20,
 -7,  0,  4, 15,
 -8,  3, -3,  5,
-30,-13, -4,  0
},
{ // Bishops
 -6, -3, -7, -8
-10, -4, -2,  4,
  3,  0,  0,  0,
  3,  0,  2,  4,
 -3,  0,  2,  2,
 -4, -1,  5,  2,
  0, -4, -2, -2,
-13, -7,  4,  0
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
 -9,  0,  5,  5,
 -3,  5,  9, 12,
  0, 13, 18, 25,
 12, 20, 25, 30,
  8, 18, 23, 26,
 -6,  0,  8, 10,
-19,-17,-12, -8,
-26,-23,-21,-18
},
{ // Kings
-76,-15, -8, -7,
-20, 24, 34, 35,
 10, 37, 42, 44,
 -6, 30, 34, 37,
-18, 15, 22, 28,
-24, -3, 11, 16,
-32,-11,  0,  3,
-70,-39,-24,-21
}
}
};

//-------------------------Material eval constants------------------------------
constexpr int BISHOP_PAIR_VALUE = 58;
constexpr int TEMPO_VALUE = 22;

// Material imbalance terms
constexpr int OWN_OPP_IMBALANCE[2][5][5] = {
{
//       Opponent's
//    P   N   B   R   Q
    { 0},                   // Own pawns
    { 4,  0},               // Own knights
    { 1, -5,  0},           // Own bishops
    { 0, -1,-18,  0},       // Own rooks
    {-1,-18,-14, -6,  0}    // Own queens
},
{
    { 0},                   // Own pawns
    { 6,  0},               // Own knights
    { 4,  0,  0},           // Own bishops
    { 0,-14,-14,  0},       // Own rooks
    {24,  7, 15,  9,  0}    // Own queens
}
};

// Bonus for knight in closed positions
constexpr int KNIGHT_CLOSED_BONUS[2] = {2, 7};

//------------------------Positional eval constants-----------------------------
// SPACE_BONUS[0][0] = behind own pawn, not center files
// SPACE_BONUS[0][1] = behind own pawn, center files
// SPACE_BONUS[1][0] = in front of opp pawn, not center files
// SPACE_BONUS[1][1] = in front of opp pawn, center files
constexpr int SPACE_BONUS[2][2] = {{16, 39}, {1, 14}};

// Mobility tables
constexpr int mobilityTable[2][5][28] = {
// Midgame
{
{ // Knights
-56, -6, 12, 24, 34, 36, 40, 44, 49},
{ // Bishops
-45,-16,  6, 12, 21, 23, 24, 26, 28, 31, 37, 45, 52, 62},
{ // Rooks
-91,-56,-18, -4,  0,  5,  8, 14, 17, 25, 30, 31, 32, 32, 36},
{ // Queens
-101,-85,-64,-38,-28,-16,-11, -8, -5, -3, -1,  2,  5,  7,
 10, 12, 15, 17, 19, 21, 23, 25, 26, 27, 29, 30, 31, 32},
{ // Kings
-16, 21, 31, 17, 11,  5,  0, -5, -5}
},

// Endgame
{
{ // Knights
-100,-49, -2, 12, 19, 24, 31, 32, 34},
{ // Bishops
-93,-48,-16,  5, 14, 22, 26, 30, 34, 35, 36, 40, 45, 47},
{ // Rooks
-110,-64, -4, 28, 40, 49, 56, 61, 66, 71, 77, 81, 84, 90, 92},
{ // Queens
-104,-80,-64,-43,-24,-19,-10, -2,  4, 10, 15, 18, 20, 23,
 25, 27, 29, 31, 33, 35, 37, 39, 41, 43, 45, 47, 49, 51},
{ // Kings
-58,-21,  3, 16, 18, 12, 21, 16,  2}
}
};

// Value of each square in the extended center in cp
constexpr Score EXTENDED_CENTER_VAL = E(2, 0);
// Additional bonus for squares in the center four squares in cp, in addition
// to EXTENDED_CENTER_VAL
constexpr Score CENTER_BONUS = E(5, 0);

// King safety
// The value of having 0, 1, and both castling rights
constexpr int CASTLING_RIGHTS_VALUE[3] = {0, 31, 73};
// The value of a pawn shield per pawn. First rank value is used for the
// penalty when the pawn is missing.
constexpr int PAWN_SHIELD_VALUE[4][8] = {
    {-15, 23, 27, 12,  6,  7, 10,  0}, // open h file, h2, h3, ...
    {-22, 38, 23, -8, -6, -1,  2,  0}, // g/b file
    {-14, 39,  2, -6, -1,  0,  0,  0}, // f/c file
    { -4, 13, 12,  7, -4, -9, -5,  0}  // d/e file
};
// Array for pawn storm values. Rank 1 of open is used for penalty
// when there is no opposing pawn
constexpr int PAWN_STORM_VALUE[3][4][8] = {
// Open file
{
    {23,-31, 31, 18, 11,  0,  0,  0},
    {19,-31, 50, 15,  5,  0,  0,  0},
    { 7, 20, 49, 27, 18,  0,  0,  0},
    { 9,-11, 28, 17, 15,  0,  0,  0}
},
// Blocked pawn
{
    { 0,  0, 27, -5, -1,  0,  0,  0},
    { 0,  0, 60, -2, -1,  0,  0,  0},
    { 0,  0, 66,  0, -2,  0,  0,  0},
    { 0,  0, 49, 11,  0,  0,  0,  0}
},
// Non-blocked pawn
{
    { 0, -6, 23, 17,  1,  0,  0,  0},
    { 0,-13, 29, 14,  9,  0,  0,  0},
    { 0, -4, 36, 24, 10,  0,  0,  0},
    { 0, -7,  5, 22,  7,  0,  0,  0}
},
};
// Penalty when the enemy king can use a storming pawn as protection
constexpr int PAWN_STORM_SHIELDING_KING = -140;

// Scale factor for pieces attacking opposing king
constexpr int KS_ARRAY_FACTOR = 128;
constexpr int KING_THREAT_MULTIPLIER[4] = {8, 5, 7, 3};
constexpr int KING_THREAT_SQUARE[4] = {8, 10, 7, 10};
constexpr int KING_DEFENSELESS_SQUARE = 24;
constexpr int KS_PAWN_FACTOR = 10;
constexpr int KING_PRESSURE = 3;
constexpr int KS_KING_PRESSURE_FACTOR = 24;
constexpr int KS_NO_KNIGHT_DEFENDER = 15;
constexpr int KS_NO_BISHOP_DEFENDER = 15;
constexpr int KS_BISHOP_PRESSURE = 8;
constexpr int KS_NO_QUEEN = -44;
constexpr int KS_BASE = -18;
constexpr int SAFE_CHECK_BONUS[4] = {56, 25, 65, 53};

// Minor pieces
// A penalty for each own pawn that is on a square of the same color as your bishop
constexpr Score BISHOP_PAWN_COLOR_PENALTY = E(-5, -5);
constexpr Score BISHOP_RAMMED_PAWN_COLOR_PENALTY = E(-4, -8);
// Minors shielded by own pawn in front
constexpr Score SHIELDED_MINOR_BONUS = E(14, 0);
// A bonus for strong outpost knights
constexpr Score KNIGHT_OUTPOST_BONUS = E(32, 26);
constexpr Score KNIGHT_OUTPOST_PAWN_DEF_BONUS = E(21, 11);
constexpr Score KNIGHT_POTENTIAL_OUTPOST_BONUS = E(9, 18);
constexpr Score KNIGHT_POTENTIAL_OUTPOST_PAWN_DEF_BONUS = E(13, 14);
// A smaller bonus for bishops
constexpr Score BISHOP_OUTPOST_BONUS = E(25, 20);
constexpr Score BISHOP_OUTPOST_PAWN_DEF_BONUS = E(29, 13);
constexpr Score BISHOP_POTENTIAL_OUTPOST_BONUS = E(7, 13);
constexpr Score BISHOP_POTENTIAL_OUTPOST_PAWN_DEF_BONUS = E(16, 6);
// A bonus for fianchettoed bishops that are not blocked by pawns
constexpr Score BISHOP_FIANCHETTO_BONUS = E(26, 0);

// Rooks
constexpr Score ROOK_OPEN_FILE_BONUS = E(37, 12);
constexpr Score ROOK_SEMIOPEN_FILE_BONUS = E(21, 1);
constexpr Score ROOK_PAWN_RANK_THREAT = E(7, 11);

// Threats
constexpr Score UNDEFENDED_PAWN = E(-3, -15);
constexpr Score UNDEFENDED_MINOR = E(-25, -41);
constexpr Score PAWN_PIECE_THREAT = E(-76, -32);
constexpr Score MINOR_ROOK_THREAT = E(-74, -26);
constexpr Score MINOR_QUEEN_THREAT = E(-69, -36);
constexpr Score ROOK_QUEEN_THREAT = E(-78, -41);

constexpr Score LOOSE_PAWN = E(-14, -4);
constexpr Score LOOSE_MINOR = E(-12, -5);

// Pawn structure
// Passed pawns
constexpr Score PASSER_BONUS[8] = {E(  0,   0), E( -2,   2), E( -2,  4), E(  6,  17),
                                   E( 30,  28), E( 56,  54), E(111,124), E(  0,   0)};
constexpr Score PASSER_FILE_BONUS[8] = {E( 22, 19), E( 13, 10), E(-11,  2), E(-10, -5),
                                        E(-10, -5), E(-11,  2), E( 13, 10), E( 22, 19)};
constexpr Score FREE_PROMOTION_BONUS = E(6, 22);
constexpr Score FREE_STOP_BONUS = E(5, 10);
constexpr Score FULLY_DEFENDED_PASSER_BONUS = E(9, 13);
constexpr Score DEFENDED_PASSER_BONUS = E(10, 8);
constexpr Score OWN_KING_DIST = E(2, 3);
constexpr Score OPP_KING_DIST = E(1, 7);

// Doubled pawns
constexpr Score DOUBLED_PENALTY = E(-3, -20);
// Isolated pawns
constexpr Score ISOLATED_PENALTY = E(-19, -9);
constexpr Score ISOLATED_SEMIOPEN_PENALTY = E(-3, -11);
// Backward pawns
constexpr Score BACKWARD_PENALTY = E(-10, -7);
constexpr Score BACKWARD_SEMIOPEN_PENALTY = E(-16, -9);
// Undefended pawns that are not backwards or isolated
constexpr Score UNDEFENDED_PAWN_PENALTY = E(-5, -1);
// Pawn phalanxes
constexpr Score PAWN_PHALANX_BONUS[8] = {E( 0,  0), E( 6,  0), E( 4,  4), E(13,  9),
                                         E(29, 19), E(63, 46), E(80, 74), E( 0,  0)};
// Connected pawns
constexpr Score PAWN_CONNECTED_BONUS[8] = {E( 0,  0), E( 0,  0), E(14,  6), E( 9,  6),
                                           E(17, 10), E(36, 31), E(69, 62), E( 0,  0)};
// King-pawn tropism
constexpr int KING_TROPISM_VALUE = 16;

// Endgame win probability adjustment
constexpr int PAWN_ASYMMETRY_BONUS = 5;
constexpr int PAWN_COUNT_BONUS = 6;
constexpr int KING_OPPOSITION_DISTANCE_BONUS = 2;
constexpr int ENDGAME_BASE = -42;

// Scale factors for drawish endgames
constexpr int MAX_SCALE_FACTOR = 32;
constexpr int OPPOSITE_BISHOP_SCALING[2] = {13, 29};
constexpr int PAWNLESS_SCALING[4] = {1, 4, 8, 23};


#undef E

#endif
