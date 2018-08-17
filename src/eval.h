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
  int playerToMove;

  // Eval helpers
  template <int attackingColor>
  int getKingSafety(Board &b, PieceMoveList &attackers, uint64_t kingSqs, int pawnScore, int kingFile);
  int checkEndgameCases();
  int scoreSimpleKnownWin(int winningColor);
  int scoreCornerDistance(int winningColor, int wKingSq, int bKingSq);
};


constexpr int SEE_PIECE_VALS[6] = {100, 400, 400, 600, 1150, MATE_SCORE/2};
constexpr int EG_FACTOR_PIECE_VALS[5] = {49, 379, 384, 677, 1559};
constexpr int EG_FACTOR_ALPHA = 2270;
constexpr int EG_FACTOR_BETA = 6370;
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
  {100, 395, 438, 677, 1339},
  {136, 392, 450, 736, 1408}
};
constexpr int KNOWN_WIN = PIECE_VALUES[EG][PAWNS] * 75;
constexpr int TB_WIN = PIECE_VALUES[EG][PAWNS] * 125;

//------------------------------Piece tables--------------------------------
constexpr int pieceSquareTable[2][6][32] = {
// Midgame
{
{ // Pawns
  0,  0,  0,  0,
  1, 12, 16, 28,
 10,  8, 14, 24,
 -1,  1,  4, 14,
-12,-12,  1,  8,
-12, -5, -1,  2,
 -8,  1, -3, -4,
  0,  0,  0,  0
},
{ // Knights
-116,-41,-34,-30,
-32,-22, -7,  8,
-15,  1, 16, 25,
  5,  7, 20, 26,
  0,  6, 18, 24,
-15,  5,  6, 14,
-17, -8, -2,  7,
-50,-16,-13, -6
},
{ // Bishops
-20,-18,-15,-15,
-18,-10, -8, -6,
  5,  4,  3,  2,
  2, 10,  5,  5,
  3,  4,  4,  9,
  0, 10,  7,  5,
  0, 12,  7,  5,
 -8, -3, -5,  0
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
-29,-21,-15,-10,
-16,-21, -7, -6,
 -8,  0,  0,  2,
 -5, -3,  0,  0,
 -3, -3,  0,  0,
-10,  4, -1, -2,
-14,  0,  2,  2,
-19,-16,-10,  0
},
{ // Kings
-37,-32,-34,-45,
-34,-28,-32,-38,
-32,-24,-28,-30,
-31,-27,-30,-31,
-32,-20,-32,-32,
 -6, 21,-14,-16,
 35, 42, 10,-12,
 32, 56, 18, -2
}
},
// Endgame
{
{ // Pawns
  0,  0,  0,  0,
  6, 18, 24, 30,
 18, 18, 18, 20,
  4,  2,  2,  2,
 -6,  0,  0,  0,
-12, -3,  0,  0,
-12, -3,  0,  0,
  0,  0,  0,  0
},
{ // Knights
-70,-35,-25,-17,
-20,  0,  4,  8,
 -4,  5, 13, 18,
  4,  9, 18, 25,
 -2,  9, 16, 24,
 -8,  3,  7, 17,
-12,  0, -3,  6,
-32,-14, -8,  0
},
{ // Bishops
-12,-10, -7, -8
 -7, -1, -4, -2,
 -2,  2,  3,  0,
  2,  2,  0,  4,
 -3,  0,  2,  2,
 -5, -1,  5,  2,
 -8, -4, -2, -2,
-13,-12, -4,  0
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
-21, -9, -4, -3,
-12,  5, 10, 16,
  0, 13, 18, 22,
  2, 16, 20, 26,
  2, 16, 20, 24,
 -4,  4,  8, 10,
-19,-14,-12, -8,
-26,-23,-23,-18
},
{ // Kings
-74,-18,-14, -7,
-10, 20, 24, 24,
 10, 32, 34, 36,
 -3, 19, 24, 26,
-14, 10, 16, 18,
-20,  0,  9, 12,
-24, -6,  4,  6,
-57,-27,-20,-17
}
}
};

//-------------------------Material eval constants------------------------------
constexpr int BISHOP_PAIR_VALUE = 56;
constexpr int TEMPO_VALUE = 16;

// Material imbalance terms
constexpr int OWN_OPP_IMBALANCE[2][5][5] = {
{
//       Opponent's
//    P   N   B   R   Q
    { 0},                   // Own pawns
    { 3,  0},               // Own knights
    { 2, -6,  0},           // Own bishops
    { 0, -9,-15,  0},       // Own rooks
    {-3,-10, -5,-20,  0}    // Own queens
},
{
    { 0},                   // Own pawns
    { 5,  0},               // Own knights
    { 2, -2,  0},           // Own bishops
    { 4,-10,-14,  0},       // Own rooks
    {24, -3,  3, 20,  0}    // Own queens
}
};

// Bonus for knight in closed positions
constexpr int KNIGHT_CLOSED_BONUS[2] = {4, 8};

//------------------------Positional eval constants-----------------------------
// SPACE_BONUS[0][0] = behind own pawn, not center files
// SPACE_BONUS[0][1] = behind own pawn, center files
// SPACE_BONUS[1][0] = in front of opp pawn, not center files
// SPACE_BONUS[1][1] = in front of opp pawn, center files
constexpr int SPACE_BONUS[2][2] = {{10, 28}, {1, 11}};

// Mobility tables
constexpr int mobilityTable[2][5][28] = {
// Midgame
{
{ // Knights
-49, -7, 11, 24, 31, 36, 41, 46, 50},
{ // Bishops
-42,-22, -3,  8, 16, 21, 25, 29, 32, 35, 39, 44, 49, 53},
{ // Rooks
-81,-58,-17, -5,  1,  4,  7, 12, 15, 18, 20, 22, 24, 26, 28},
{ // Queens
-78,-60,-40,-24,-14, -8, -3,  1,  4,  7,  9, 12, 15, 17,
 20, 22, 25, 27, 30, 32, 34, 37, 39, 41, 43, 45, 47, 49},
{ // Kings
-29,  3, 20, 16, 14,  6, 12,  0, -6}
},

// Endgame
{
{ // Knights
-82,-43, -7, 10, 18, 26, 31, 33, 35},
{ // Bishops
-92,-45,-17,  5, 14, 21, 26, 31, 36, 40, 43, 46, 49, 51},
{ // Rooks
-96,-55,-11, 20, 38, 48, 55, 61, 67, 72, 77, 81, 86, 90, 94},
{ // Queens
-105,-76,-58,-37,-24,-15, -7,  0,  6, 11, 15, 18, 20, 23,
 25, 27, 29, 31, 33, 35, 37, 39, 41, 43, 45, 47, 49, 51},
{ // Kings
-43,-19, -6, 16, 20, 14, 20, 16, -3}
}
};

// Value of each square in the extended center in cp
constexpr Score EXTENDED_CENTER_VAL = E(3, 0);
// Additional bonus for squares in the center four squares in cp, in addition
// to EXTENDED_CENTER_VAL
constexpr Score CENTER_BONUS = E(5, 0);

// King safety
// The value of having 0, 1, and both castling rights
constexpr int CASTLING_RIGHTS_VALUE[3] = {0, 23, 64};
// The value of a pawn shield per pawn. First rank value is used for the
// penalty when the pawn is missing.
constexpr int PAWN_SHIELD_VALUE[4][8] = {
    {-11, 23, 27, 12,  5,  5,-11,  0}, // open h file, h2, h3, ...
    {-18, 37, 24, -6, -5, -5,  0,  0}, // g/b file
    {-13, 38,  3, -5, -5, -5,  0,  0}, // f/c file
    { -4, 14, 11,  8, -4,-10,-10,  0}  // d/e file
};
// Array for pawn storm values. Rank 1 of open is used for penalty
// when there is no opposing pawn
constexpr int PAWN_STORM_VALUE[3][4][8] = {
// Open file
{
    {12, -6, 41, 18, 12,  0,  0,  0},
    {13,-13, 44, 17,  7,  0,  0,  0},
    { 7, 24, 54, 24, 16,  0,  0,  0},
    { 7,  7, 40, 16, 12,  0,  0,  0}
},
// Blocked pawn
{
    { 0,  0, 30,  2,  0,  0,  0,  0},
    { 0,  0, 65,  4,  1,  0,  0,  0},
    { 0,  0, 64,  6,  0,  0,  0,  0},
    { 0,  0, 54,  9,  2,  0,  0,  0}
},
// Non-blocked pawn
{
    { 0, 12, 28, 11,  3,  0,  0,  0},
    { 0,  0, 27, 13,  5,  0,  0,  0},
    { 0,  9, 36, 18,  5,  0,  0,  0},
    { 0,  0, 10, 20,  5,  0,  0,  0}
},
};
// Penalty when the enemy king can use a storming pawn as protection
constexpr int PAWN_STORM_SHIELDING_KING = -114;

// Scale factor for pieces attacking opposing king
constexpr int KS_ARRAY_FACTOR = 128;
constexpr int KING_THREAT_MULTIPLIER[4] = {8, 4, 7, 6};
constexpr int KING_THREAT_SQUARE[4] = {7, 12, 9, 11};
constexpr int KING_DEFENSELESS_SQUARE = 23;
constexpr int KS_PAWN_FACTOR = 11;
constexpr int KING_PRESSURE = 3;
constexpr int KS_KING_PRESSURE_FACTOR = 9;
constexpr int KS_NO_QUEEN = -60;
constexpr int KS_BASE = 0;
constexpr int SAFE_CHECK_BONUS[4] = {74, 22, 54, 50};

// Minor pieces
// A penalty for each own pawn that is on a square of the same color as your bishop
constexpr Score BISHOP_PAWN_COLOR_PENALTY = E(-4, -4);
constexpr Score BISHOP_RAMMED_PAWN_COLOR_PENALTY = E(-5, -9);
// Minors shielded by own pawn in front
constexpr Score SHIELDED_MINOR_BONUS = E(14, 0);
// A bonus for strong outpost knights
constexpr Score KNIGHT_OUTPOST_BONUS = E(30, 22);
constexpr Score KNIGHT_OUTPOST_PAWN_DEF_BONUS = E(20, 8);
constexpr Score KNIGHT_POTENTIAL_OUTPOST_BONUS = E(9, 13);
constexpr Score KNIGHT_POTENTIAL_OUTPOST_PAWN_DEF_BONUS = E(13, 16);
// A smaller bonus for bishops
constexpr Score BISHOP_OUTPOST_BONUS = E(23, 15);
constexpr Score BISHOP_OUTPOST_PAWN_DEF_BONUS = E(28, 12);
constexpr Score BISHOP_POTENTIAL_OUTPOST_BONUS = E(10, 12);
constexpr Score BISHOP_POTENTIAL_OUTPOST_PAWN_DEF_BONUS = E(12, 6);

// Rooks
constexpr Score ROOK_OPEN_FILE_BONUS = E(40, 15);
constexpr Score ROOK_SEMIOPEN_FILE_BONUS = E(24, 3);
constexpr Score ROOK_PAWN_RANK_THREAT = E(2, 10);

// Threats
constexpr Score UNDEFENDED_PAWN = E(-3, -18);
constexpr Score UNDEFENDED_MINOR = E(-22, -53);
constexpr Score PAWN_PIECE_THREAT = E(-81, -42);
constexpr Score MINOR_ROOK_THREAT = E(-73, -35);
constexpr Score MINOR_QUEEN_THREAT = E(-71, -39);
constexpr Score ROOK_QUEEN_THREAT = E(-82, -41);

constexpr Score LOOSE_PAWN = E(-10, -1);
constexpr Score LOOSE_MINOR = E(-18, -11);

// Pawn structure
// Passed pawns
constexpr Score PASSER_BONUS[8] = {E(  0,   0), E(  3,  12), E(  3, 14), E(  7,  16),
                               E( 23,  21), E( 51,  51), E(106,108), E(  0,   0)};
constexpr Score PASSER_FILE_BONUS[8] = {E( 13, 11), E(  5,  5), E( -8, -3), E(-10, -8),
                                    E(-10, -8), E( -8, -3), E(  5,  5), E( 13, 11)};
constexpr Score FREE_PROMOTION_BONUS = E(11, 21);
constexpr Score FREE_STOP_BONUS = E(6, 8);
constexpr Score FULLY_DEFENDED_PASSER_BONUS = E(8, 13);
constexpr Score DEFENDED_PASSER_BONUS = E(8, 8);
constexpr Score OWN_KING_DIST = E(0, 3);
constexpr Score OPP_KING_DIST = E(0, 7);

// Doubled pawns
constexpr Score DOUBLED_PENALTY = E(-5, -18);
// Isolated pawns
constexpr Score ISOLATED_PENALTY = E(-19, -13);
constexpr Score ISOLATED_SEMIOPEN_PENALTY = E(-4, -9);
// Backward pawns
constexpr Score BACKWARD_PENALTY = E(-12, -9);
constexpr Score BACKWARD_SEMIOPEN_PENALTY = E(-15, -13);
// Undefended pawns that are not backwards or isolated
constexpr Score UNDEFENDED_PAWN_PENALTY = E(-8, -4);
// Pawn phalanxes
constexpr Score PAWN_PHALANX_BONUS[8] = {E(0, 0), E(6, -1), E(3, 1), E(10, 7),
                                     E(26, 21), E(54, 38), E(85, 74), E(0, 0)};
// Connected pawns
constexpr Score PAWN_CONNECTED_BONUS[8] = {E(0, 0), E(0, 0), E(10, 4), E(7, 3),
                                       E(12, 12), E(33, 36), E(66, 58), E(0, 0)};
// King-pawn tropism
constexpr int KING_TROPISM_VALUE = 18;

// Scale factors for drawish endgames
constexpr int MAX_SCALE_FACTOR = 32;
constexpr int OPPOSITE_BISHOP_SCALING[2] = {14, 28};
constexpr int PAWNLESS_SCALING[4] = {3, 4, 7, 24};


#undef E

#endif