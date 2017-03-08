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

#ifndef __EVAL_H__
#define __EVAL_H__

#include "common.h"

/*
 * This file stores evaluation constants and encoding.
 */

const int SEE_PIECE_VALS[6] = {100, 400, 400, 600, 1150, MATE_SCORE/2};
const int EG_FACTOR_PIECE_VALS[5] = {68, 388, 388, 699, 1820};
const int EG_FACTOR_ALPHA = 2700;
const int EG_FACTOR_BETA = 5420;
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
  {100, 390, 425, 608, 1299},
  {142, 389, 446, 696, 1287}
};
const int KNOWN_WIN = PIECE_VALUES[EG][PAWNS] * 75;
const int TB_WIN = PIECE_VALUES[EG][PAWNS] * 125;

//------------------------------Piece tables--------------------------------
const int pieceSquareTable[2][6][32] = {
// Midgame
{
{ // Pawns
  0,  0,  0,  0,
 30, 28, 47, 64,
 21, 25, 36, 49,
  5, 13, 20, 28,
 -6,  1,  8, 17,
 -2,  1,  2,  6,
 -9,  0, -3,  0,
  0,  0,  0,  0
},
{ // Knights
-116,-22, -8, -4,
-24,-12, 10, 17,
-17,  4, 14, 28,
  5,  6, 15, 20,
  4,  9, 12, 15,
-19,  6,  5, 15,
-23, -9, -4,  3,
-85,-27,-13, -4
},
{ // Bishops
-15, -9, -8, -8,
-10, -5, -5,  5,
  2,  0,  2,  0,
 -6,  9,  0,  5,
  2,  7, -5,  0,
  0, 15, 10,  5,
  0, 15, 10,  5,
-20, -5, -7,  0
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
-20,-15,-12,-10,
 -6,-18, -6, -3,
 -5,  0,  0,  0,
 -5,  0,  0,  0,
 -5,  0,  0,  0,
 -5, -1,  1,  0,
-10, -5,  5,  1,
-15,-10, -5,  0
},
{ // Kings
-52,-47,-44,-46,
-44,-40,-40,-41,
-34,-29,-35,-35,
-28,-24,-30,-31,
-25,-18,-25,-25,
 -7, 12,-17,-15,
 42, 45, 14,  3,
 33, 51, 22, -8
}
},
// Endgame
{
{ // Pawns
  0,  0,  0,  0,
 20, 42, 57, 58,
 18, 26, 30, 30,
  4,  6,  8,  8,
-12, -5, -3, -3,
-22,-15, -5, -5,
-22,-15, -5, -5,
  0,  0,  0,  0
},
{ // Knights
-50, -5,  3,  6,
 -5,  5, 10, 18,
  5, 10, 12, 20,
  7, 14, 18, 25,
  6, 13, 17, 25,
-10,  1,  7, 18,
-20, -8, -2, 10,
-40,-27,-17,-15
},
{ // Bishops
-15,-10, -5, -5
-10, -5, -5,  8,
 -2, -2,  8,  7,
 -5,  5,  5,  7,
 -5,  2,  5,  5,
 -5,  2,  5,  8,
 -8,  2,  0,  3,
-10, -8, -8, -6
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
-20, -8, -8, -8,
 -5,  1,  2,  1,
 -5,  2,  2,  7,
 -5,  7,  6,  5,
 -5,  5,  3,  6,
 -5,  0,  0,  1,
-11, -8, -5, -5,
-19,-13,-13, -8
},
{ // Kings
-82,-14, -8, -3,
-15, 15, 19, 24,
  6, 30, 30, 34,
 -3, 25, 25, 29,
-16,  5, 13, 18,
-24, -7,  0,  8,
-25,-12, -6, -6,
-50,-30,-27,-24
}
}
};

//-------------------------Material eval constants------------------------------
const int BISHOP_PAIR_VALUE = 52;
const int TEMPO_VALUE = 14;

// Material imbalance terms
const int KNIGHT_PAIR_PENALTY = -2;
const int ROOK_PAIR_PENALTY = -13;

const int OWN_OPP_IMBALANCE[2][5][5] = {
{
//       Opponent's
//    P   N   B   R   Q
    { 0},                   // Own pawns
    { 0,  0},               // Own knights
    {-1,  0,  0},           // Own bishops
    {-2,  1,  0,  0},       // Own rooks
    { 0,  7,  3,-19,  0}    // Own queens
},
{
    { 0},                   // Own pawns
    { 8,  0},               // Own knights
    { 2,  0,  0},           // Own bishops
    { 0, -1, -4,  0},       // Own rooks
    {24,  1, 11, 18,  0}    // Own queens
}
};

//------------------------Positional eval constants-----------------------------
// Mobility tables, with zero padding for pieces that cannot move up to 27 squares
const int mobilityScore[2][4][28] = {
// Midgame
{
{ // Knights
-16,  6, 13, 19, 23, 27, 30, 33, 36,
  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{ // Bishops
-32,-10,  0,  7, 14, 20, 25, 30, 35, 39, 44, 48, 52, 56,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{ // Rooks
-41,-30,-23,-17,-12, -7, -3,  0,  4,  8, 12, 16, 20, 23, 27,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{ // Queens
-24,-17,-12, -8, -4, -1,  1,  5,  8, 11, 13, 16, 19, 22,
 24, 27, 30, 32, 35, 37, 39, 42, 44, 46, 49, 51, 53, 56}
},

// Endgame
{
{ // Knights
-46,-11,  0,  8, 14, 20, 25, 29, 34,
  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{ // Bishops
-64, -2,  8, 16, 22, 27, 31, 35, 38, 41, 44, 47, 49, 52,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{ // Rooks
-68, 11, 26, 36, 43, 50, 55, 60, 65, 68, 72, 76, 79, 82, 85,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{ // Queens
-65,-29,-19,-12, -7, -2,  1,  5,  8, 11, 14, 17, 20, 22,
 24, 26, 29, 31, 33, 34, 36, 38, 40, 41, 43, 44, 46, 48}
}
};

// Value of each square in the extended center in cp
const int EXTENDED_CENTER_VAL = 3;
// Additional bonus for squares in the center four squares in cp, in addition
// to EXTENDED_CENTER_VAL
const int CENTER_BONUS = 4;

// King safety
// The value of having 0, 1, and both castling rights
const int CASTLING_RIGHTS_VALUE[3] = {0, 34, 61};
// The value of a pawn shield per pawn. First rank value is used for the
// penalty when the pawn is missing.
const int PAWN_SHIELD_VALUE[4][8] = {
    {-12, 18, 24,  8, -2, -5, -8,  0}, // open h file, h2, h3, ...
    {-24, 37, 25,  0, -6,-12,-18,  0}, // g/b file
    {-13, 38,  5,  0, -6,-10,-12,  0}, // f/c file
    {-10, 14, 10,  5, -1, -4, -7,  0}  // d/e file
};
// Array for pawn storm values. Rank 1 of open is used for penalty
// when there is no opposing pawn
const int PAWN_STORM_VALUE[3][4][8] = {
// Open file
{
    {10,-30, 24, 12,  8,  0,  0,  0},
    {12,-13, 33, 10,  5,  0,  0,  0},
    { 7, 10, 36, 15,  8,  0,  0,  0},
    { 8, 18, 20, 10,  6,  0,  0,  0}
},
// Blocked pawn
{
    { 0,  0, 18,  3,  0,  0,  0,  0},
    { 0,  0, 45,  4,  1,  0,  0,  0},
    { 0,  0, 35,  5,  1,  0,  0,  0},
    { 0,  0, 32,  5,  2,  0,  0,  0}
},
// Non-blocked pawn
{
    { 0, 10, 22, 10,  4,  0,  0,  0},
    { 0, 10, 25, 15,  8,  0,  0,  0},
    { 0,  5, 29, 15,  6,  0,  0,  0},
    { 0,  4, 15, 15,  2,  0,  0,  0}
},
};

// Scale factor for pieces attacking opposing king
const double KS_ARRAY_FACTOR = 36.0;
const int KING_THREAT_MULTIPLIER[4] = {2, 0, 2, 5};
const int KING_THREAT_SQUARE[4] = {7, 7, 3, 7};
const int KING_DEFENSELESS_SQUARE = 11;
const int KS_PAWN_FACTOR = 8;
const int SAFE_CHECK_BONUS[4] = {23, 8, 17, 17};

// Minor pieces
// A penalty for each own pawn that is on a square of the same color as your bishop
const Score BISHOP_PAWN_COLOR_PENALTY = E(-7, -6);
// A bonus for strong outpost knights
const Score KNIGHT_OUTPOST_BONUS1 = E(27, 13);
const Score KNIGHT_OUTPOST_BONUS2 = E(19, 7);
const Score KNIGHT_OUTPOST_PAWN_DEF_BONUS = E(14, 9);
// A smaller bonus for bishops
const Score BISHOP_OUTPOST_BONUS1 = E(18, 6);
const Score BISHOP_OUTPOST_BONUS2 = E(12, 3);
const Score BISHOP_OUTPOST_PAWN_DEF_BONUS = E(18, 6);
// A penalty for Nc3 blocking c2-c4 in closed openings
const Score KNIGHT_C3_CLOSED_PENALTY = E(-24, 0);

// Rooks
const Score ROOK_OPEN_FILE_BONUS = E(30, 13);
const Score ROOK_SEMIOPEN_FILE_BONUS = E(9, 0);

// Threats
const Score UNDEFENDED_PAWN = E(-8, -20);
const Score UNDEFENDED_MINOR = E(-14, -35);
const Score ATTACKED_MAJOR = E(-7, -3);
const Score PAWN_MINOR_THREAT = E(-54, -35);
const Score PAWN_MAJOR_THREAT = E(-53, -22);

const Score LOOSE_PAWN = E(-14, -8);
const Score LOOSE_MINOR = E(-8, -9);

// Pawn structure
// Passed pawns
const Score PASSER_BONUS[8] = {E(  0,   0), E(  1,   4), E(  1,  5), E(  4,  12),
                               E( 21,  24), E( 56,  60), E( 99, 99), E(  0,   0)};
const Score PASSER_FILE_BONUS[8] = {E(  9,  11), E(  6, 11), E(-10, -5), E(-13, -16),
                                    E(-13, -16), E(-10, -5), E(  6, 11), E(  9,  11)};
const Score FREE_PROMOTION_BONUS = E(0, 12);
const Score FREE_STOP_BONUS = E(3, 4);
const Score FULLY_DEFENDED_PASSER_BONUS = E(10, 15);
const Score DEFENDED_PASSER_BONUS = E(8, 11);
const Score OWN_KING_DIST = E(0, 2);
const Score OPP_KING_DIST = E(0, 4);

// Doubled pawns
const Score DOUBLED_PENALTY = E(-12, -15);
// Doubled pawns are worse the less pawns you have
const Score DOUBLED_PENALTY_SCALE[9] = {0, 0, 3, 2, 1, 1, 1, 1, 1};
// Isolated pawns
const Score ISOLATED_PENALTY = E(-9, -12);
const Score CENTRAL_ISOLATED_PENALTY = E(-3, -5);
// Isolated, doubled pawns
const Score ISOLATED_DOUBLED_PENALTY = E(-13, -20);
// Backward pawns
const Score BACKWARD_PENALTY = E(-11, 0);
const Score BACKWARD_SEMIOPEN_PENALTY = E(-8, -16);
// Undefended pawns that are not backwards or isolated
const Score UNDEFENDED_PAWN_PENALTY = E(-8, -5);
// Pawn phalanxes
const Score PAWN_PHALANX_BONUS = E(1, -1);
const Score PAWN_PHALANX_RANK_BONUS = E(11, 9);
// King-pawn tropism
const int KING_TROPISM_VALUE = 14;

#undef E

#endif