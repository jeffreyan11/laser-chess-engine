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

void initPSQT();
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
  template <int color>
  void getMobility(PieceMoveList &pml, PieceMoveList &oppPml, int &valueMg, int &valueEg);
  template <int attackingColor>
  int getKingSafety(Board &b, PieceMoveList &attackers, uint64_t kingSqs, int pawnScore, int kingFile);
  int checkEndgameCases();
  int scoreSimpleKnownWin(int winningColor);
  int scoreCornerDistance(int winningColor, int wKingSq, int bKingSq);
  int getManhattanDistance(int sq1, int sq2);
  int getKingDistance(int sq1, int sq2);
};


const int SEE_PIECE_VALS[6] = {100, 400, 400, 600, 1150, MATE_SCORE/2};
const int EG_FACTOR_PIECE_VALS[5] = {51, 380, 384, 674, 1559};
const int EG_FACTOR_ALPHA = 2300;
const int EG_FACTOR_BETA = 6370;
const int EG_FACTOR_RES = 1000;

// Eval scores are packed into an unsigned 32-bit integer during calculations
// (the SWAR technique)
typedef uint32_t Score;

// Encodes 16-bit midgame and endgame evaluation scores into a single int
#define E(mg, eg) ((Score) ((((int32_t) eg) << 16) + ((int32_t) mg)))

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
const Score EVAL_ZERO = 0x80008000;

// Array indexing constants
const int MG = 0;
const int EG = 1;

// Material constants
const int PIECE_VALUES[2][5] = {
  {100, 404, 442, 680, 1365},
  {138, 399, 455, 738, 1418}
};
const int KNOWN_WIN = PIECE_VALUES[EG][PAWNS] * 75;
const int TB_WIN = PIECE_VALUES[EG][PAWNS] * 125;

//------------------------------Piece tables--------------------------------
const int pieceSquareTable[2][6][32] = {
// Midgame
{
{ // Pawns
  0,  0,  0,  0,
  3,  9, 16, 26,
  9,  9, 16, 24,
  0,  1,  6, 14,
-10,-10,  4, 10,
-10, -5,  0,  4,
 -7,  0, -3, -2,
  0,  0,  0,  0
},
{ // Knights
-110,-41,-30,-27,
-26,-19, -6,  8,
-12,  1, 16, 25,
  5,  7, 20, 26,
  0,  7, 16, 22,
-15,  5,  6, 12,
-17, -8, -2,  7,
-53,-16,-13, -4
},
{ // Bishops
-20,-15,-10,-10,
-15, -8, -4, -2,
  3,  4,  3,  2,
  2, 10,  5,  5,
  3,  4,  4,  9,
  0, 10,  7,  5,
 -2, 12,  7,  5,
-10, -5, -5, -2
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
-29,-21,-12, -8,
-14,-18, -7, -4,
 -5,  0,  0,  2,
 -5, -3,  0,  0,
 -3, -3,  0,  0,
-11,  4, -1, -2,
-14,  0,  2,  2,
-18,-14, -8,  0
},
{ // Kings
-37,-32,-34,-45,
-34,-28,-32,-38,
-32,-24,-28,-30,
-31,-27,-30,-31,
-28,-16,-28,-28,
 -4, 21,-12,-16,
 35, 42,  8,-12,
 32, 55, 18, -2
}
},
// Endgame
{
{ // Pawns
  0,  0,  0,  0,
 10, 17, 23, 31,
 13, 15, 17, 19,
  0,  0,  2,  2,
 -7, -3,  0,  0,
-10, -3,  0,  0,
-10, -3,  0,  0,
  0,  0,  0,  0
},
{ // Knights
-69,-26,-18,-12,
-17,  0,  4,  9,
 -4,  5, 13, 18,
  4,  9, 18, 25,
  2,  9, 16, 24,
 -8,  3,  7, 17,
-12, -2, -3,  6,
-35,-16,-10, -4
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
-20, -9, -4, -2,
 -8,  5,  8, 13,
  0, 10, 15, 20,
  2, 13, 18, 22,
  2, 13, 16, 20,
 -4,  4,  6,  8,
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
const int BISHOP_PAIR_VALUE = 58;
const int TEMPO_VALUE = 16;

// Material imbalance terms
const int OWN_OPP_IMBALANCE[2][5][5] = {
{
//       Opponent's
//    P   N   B   R   Q
    { 0},                   // Own pawns
    { 3,  0},               // Own knights
    { 2, -6,  0},           // Own bishops
    { 0, -8,-14,  0},       // Own rooks
    {-2,-10, -5,-18,  0}    // Own queens
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
const int KNIGHT_CLOSED_BONUS[2] = {3, 6};

//------------------------Positional eval constants-----------------------------
// SPACE_BONUS[0][0] = behind own pawn, not center files
// SPACE_BONUS[0][1] = behind own pawn, center files
// SPACE_BONUS[1][0] = in front of opp pawn, not center files
// SPACE_BONUS[1][1] = in front of opp pawn, center files
const int SPACE_BONUS[2][2] = {{10, 24}, {2, 9}};

// Mobility tables
const int mobilityScore[2][4][28] = {
// Midgame
{
{ // Knights
-40, -7, 10, 25, 31, 36, 40, 44, 48},
{ // Bishops
-42,-21, -4,  7, 16, 21, 25, 29, 32, 35, 39, 44, 48, 52},
{ // Rooks
-69,-46,-16, -4,  1,  4,  7, 12, 15, 18, 20, 22, 24, 26, 28},
{ // Queens
-66,-51,-36,-24,-12, -7, -3,  1,  4,  7,  9, 12, 15, 17,
 20, 22, 25, 27, 30, 32, 34, 37, 39, 41, 43, 45, 47, 49}
},

// Endgame
{
{ // Knights
-72,-34, -3, 10, 18, 26, 31, 33, 35},
{ // Bishops
-90,-41,-14,  5, 14, 21, 26, 31, 36, 40, 43, 46, 49, 51},
{ // Rooks
-84,-43, -7, 21, 38, 48, 55, 61, 66, 71, 75, 79, 83, 87, 90},
{ // Queens
-97,-64,-47,-33,-21,-11, -4,  3,  8, 12, 15, 18, 20, 23,
 25, 27, 29, 31, 33, 35, 37, 39, 41, 43, 45, 47, 49, 51}
}
};

// Value of each square in the extended center in cp
const int EXTENDED_CENTER_VAL = 3;
// Additional bonus for squares in the center four squares in cp, in addition
// to EXTENDED_CENTER_VAL
const int CENTER_BONUS = 5;

// King safety
// The value of having 0, 1, and both castling rights
const int CASTLING_RIGHTS_VALUE[3] = {0, 25, 63};
// The value of a pawn shield per pawn. First rank value is used for the
// penalty when the pawn is missing.
const int PAWN_SHIELD_VALUE[4][8] = {
    {-11, 23, 27, 10,  5,  5,-11,  0}, // open h file, h2, h3, ...
    {-18, 38, 24, -4, -5, -5, -8,  0}, // g/b file
    {-13, 38,  3, -5, -5, -5, -5,  0}, // f/c file
    { -5, 14, 10,  8, -4,-10,-10,  0}  // d/e file
};
// Array for pawn storm values. Rank 1 of open is used for penalty
// when there is no opposing pawn
const int PAWN_STORM_VALUE[3][4][8] = {
// Open file
{
    {12, -3, 42, 18, 12,  0,  0,  0},
    {13,-12, 44, 18,  7,  0,  0,  0},
    { 7, 20, 53, 22, 14,  0,  0,  0},
    { 7,  6, 40, 18, 12,  0,  0,  0}
},
// Blocked pawn
{
    { 0,  0, 30,  2,  0,  0,  0,  0},
    { 0,  0, 63,  4,  1,  0,  0,  0},
    { 0,  0, 64,  6,  0,  0,  0,  0},
    { 0,  0, 56,  9,  2,  0,  0,  0}
},
// Non-blocked pawn
{
    { 0,  9, 31, 11,  3,  0,  0,  0},
    { 0,  6, 30, 10,  5,  0,  0,  0},
    { 0,  6, 36, 17,  5,  0,  0,  0},
    { 0,  3, 14, 20,  8,  0,  0,  0}
},
};
// Penalty when the enemy king can use a storming pawn as protection
const int PAWN_STORM_SHIELDING_KING = -105;

// Scale factor for pieces attacking opposing king
const int KS_ARRAY_FACTOR = 128;
const int KING_THREAT_MULTIPLIER[4] = {8, 4, 7, 6};
const int KING_THREAT_SQUARE[4] = {7, 12, 9, 11};
const int KING_DEFENSELESS_SQUARE = 23;
const int KS_PAWN_FACTOR = 11;
const int KING_PRESSURE = 3;
const int KS_KING_PRESSURE_FACTOR = 9;
const int KS_NO_QUEEN = -60;
const int KS_BASE = 0;
const int SAFE_CHECK_BONUS[4] = {74, 22, 54, 50};

// Minor pieces
// A penalty for each own pawn that is on a square of the same color as your bishop
const Score BISHOP_PAWN_COLOR_PENALTY = E(-4, -3);
const Score BISHOP_RAMMED_PAWN_COLOR_PENALTY = E(-5, -8);
// Minors shielded by own pawn in front
const Score SHIELDED_MINOR_BONUS = E(15, 0);
// A bonus for strong outpost knights
const Score KNIGHT_OUTPOST_BONUS = E(30, 20);
const Score KNIGHT_OUTPOST_PAWN_DEF_BONUS = E(21, 9);
const Score KNIGHT_POTENTIAL_OUTPOST_BONUS = E(10, 11);
const Score KNIGHT_POTENTIAL_OUTPOST_PAWN_DEF_BONUS = E(11, 13);
// A smaller bonus for bishops
const Score BISHOP_OUTPOST_BONUS = E(21, 14);
const Score BISHOP_OUTPOST_PAWN_DEF_BONUS = E(26, 12);
const Score BISHOP_POTENTIAL_OUTPOST_BONUS = E(8, 8);
const Score BISHOP_POTENTIAL_OUTPOST_PAWN_DEF_BONUS = E(14, 8);

// Rooks
const Score ROOK_OPEN_FILE_BONUS = E(36, 15);
const Score ROOK_SEMIOPEN_FILE_BONUS = E(22, 2);
const Score ROOK_PAWN_RANK_THREAT = E(2, 10);

// Threats
const Score UNDEFENDED_PAWN = E(-1, -17);
const Score UNDEFENDED_MINOR = E(-18, -50);
const Score PAWN_PIECE_THREAT = E(-77, -42);
const Score MINOR_ROOK_THREAT = E(-68, -39);
const Score MINOR_QUEEN_THREAT = E(-71, -36);
const Score ROOK_QUEEN_THREAT = E(-78, -35);

const Score LOOSE_PAWN = E(-10, 0);
const Score LOOSE_MINOR = E(-17, -12);

// Pawn structure
// Passed pawns
const Score PASSER_BONUS[8] = {E(  0,   0), E(  3,  11), E(  3, 13), E(  7,  16),
                               E( 25,  23), E( 53,  53), E(106,108), E(  0,   0)};
const Score PASSER_FILE_BONUS[8] = {E( 11, 13), E(  5,  6), E( -8, -2), E(-11, -7),
                                    E(-11, -7), E( -8, -2), E(  5,  6), E( 11, 13)};
const Score FREE_PROMOTION_BONUS = E(12, 20);
const Score FREE_STOP_BONUS = E(6, 8);
const Score FULLY_DEFENDED_PASSER_BONUS = E(8, 12);
const Score DEFENDED_PASSER_BONUS = E(8, 8);
const Score OWN_KING_DIST = E(0, 3);
const Score OPP_KING_DIST = E(0, 7);

// Doubled pawns
const Score DOUBLED_PENALTY = E(-6, -20);
// Isolated pawns
const Score ISOLATED_PENALTY = E(-20, -14);
const Score ISOLATED_SEMIOPEN_PENALTY = E(-5, -6);
// Backward pawns
const Score BACKWARD_PENALTY = E(-13, -10);
const Score BACKWARD_SEMIOPEN_PENALTY = E(-15, -11);
// Undefended pawns that are not backwards or isolated
const Score UNDEFENDED_PAWN_PENALTY = E(-8, -5);
// Pawn phalanxes
const Score PAWN_PHALANX_BONUS[8] = {E(0, 0), E(6, -1), E(4, 1), E(10, 7),
                                     E(28, 22), E(57, 41), E(82, 75), E(0, 0)};
// Connected pawns
const Score PAWN_CONNECTED_BONUS[8] = {E(0, 0), E(0, 0), E(9, 3), E(6, 2),
                                       E(14, 10), E(31, 33), E(60, 55), E(0, 0)};
// King-pawn tropism
const int KING_TROPISM_VALUE = 18;

// Scale factors for drawish endgames
const int MAX_SCALE_FACTOR = 32;
const int OPPOSITE_BISHOP_SCALING[2] = {14, 28};
const int PAWNLESS_SCALING[4] = {3, 4, 7, 24};


#undef E

#endif