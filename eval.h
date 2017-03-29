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
const int EG_FACTOR_PIECE_VALS[5] = {68, 384, 388, 699, 1821};
const int EG_FACTOR_ALPHA = 2720;
const int EG_FACTOR_BETA = 5450;
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
  {100, 389, 426, 605, 1301},
  {140, 387, 445, 694, 1287}
};
const int KNOWN_WIN = PIECE_VALUES[EG][PAWNS] * 75;
const int TB_WIN = PIECE_VALUES[EG][PAWNS] * 125;

//------------------------------Piece tables--------------------------------
const int pieceSquareTable[2][6][32] = {
// Midgame
{
{ // Pawns
  0,  0,  0,  0,
 32, 28, 54, 70,
 24, 26, 39, 49,
  5, 11, 20, 25,
 -4, -2,  8, 14,
 -5, -1,  4,  8,
 -9,  2,  0,  2,
  0,  0,  0,  0
},
{ // Knights
-116,-18, -5,  0,
-22,-10, 15, 19,
-17,  5, 15, 31,
  6,  7, 16, 20,
  1,  9, 12, 15,
-17,  6,  5, 15,
-20,-12, -4,  3,
-67,-27,-16, -4
},
{ // Bishops
-18,-15,-10,-10,
-15, -8, -7, -1,
  3,  4,  2,  0,
 -6, 11,  0,  5,
  5,  9, -5,  0,
  3, 15, 10,  5,
 -5, 15,  5,  5,
-15, -5, -7, -5
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
-25,-21,-13, -8,
 -7,-19, -7, -3,
  0,  0,  0,  0,
 -5, -5, -5, -5,
 -5, -5, -5, -5,
  0,  4, -4, -3,
-10, -5,  5,  1,
-10,-10, -5,  0
},
{ // Kings
-52,-47,-44,-46,
-44,-40,-40,-41,
-34,-29,-35,-35,
-28,-24,-30,-31,
-25,-18,-25,-25,
 -7, 15,-17,-15,
 42, 45, 17, 12,
 33, 52, 22, -8
}
},
// Endgame
{
{ // Pawns
  0,  0,  0,  0,
 29, 39, 67, 70,
 23, 26, 30, 30,
 13,  8,  8,  8,
-12, -5, -3, -3,
-22,-15, -5, -5,
-22,-15, -5, -5,
  0,  0,  0,  0
},
{ // Knights
-55,  0,  5,  6,
  0,  5, 10, 18,
  5, 10, 12, 20,
  7, 14, 18, 25,
  6, 13, 17, 25,
-10,  1,  7, 18,
-20, -8, -2, 10,
-35,-27,-17,-15
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
-99,-14, -8, -3,
 -5, 20, 24, 24,
  6, 36, 36, 37,
  0, 25, 25, 27,
-16,  5, 13, 15,
-22, -7,  0,  3,
-25,-13, -6, -6,
-46,-29,-24,-21
}
}
};

//-------------------------Material eval constants------------------------------
const int BISHOP_PAIR_VALUE = 52;
const int TEMPO_VALUE = 15;

// Material imbalance terms
const int KNIGHT_PAIR_PENALTY = 0;
const int ROOK_PAIR_PENALTY = -19;

const int OWN_OPP_IMBALANCE[2][5][5] = {
{
//       Opponent's
//    P   N   B   R   Q
    { 0},                   // Own pawns
    { 0,  0},               // Own knights
    {-1, -2,  0},           // Own bishops
    {-2,  1, -2,  0},       // Own rooks
    {-2,  7,  3,-21,  0}    // Own queens
},
{
    { 0},                   // Own pawns
    { 7,  0},               // Own knights
    { 2, -2,  0},           // Own bishops
    { 0, -1, -5,  0},       // Own rooks
    {22,  5, 10, 18,  0}    // Own queens
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
-32, -9,  0,  7, 13, 18, 23, 28, 32, 36, 39, 43, 46, 50,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{ // Rooks
-41,-27,-19,-14, -8, -4,  0,  3,  7, 11, 14, 17, 20, 24, 27,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{ // Queens
-24,-17,-13, -9, -5, -2,  0,  3,  6,  9, 12, 15, 17, 20,
 23, 25, 28, 30, 32, 35, 37, 39, 42, 44, 46, 48, 50, 53}
},

// Endgame
{
{ // Knights
-46,-11,  0,  8, 14, 20, 25, 29, 34,
  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{ // Bishops
-64,-16, -3,  5, 12, 19, 24, 29, 33, 37, 41, 45, 48, 52,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{ // Rooks
-68,  1, 17, 28, 37, 44, 50, 56, 61, 66, 70, 74, 78, 81, 85,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{ // Queens
-65,-46,-37,-31,-25,-20,-15,-11, -7, -3,  0,  3,  7, 10,
 13, 16, 19, 22, 25, 28, 30, 33, 35, 38, 40, 43, 45, 48}
}
};

// Value of each square in the extended center in cp
const int EXTENDED_CENTER_VAL = 3;
// Additional bonus for squares in the center four squares in cp, in addition
// to EXTENDED_CENTER_VAL
const int CENTER_BONUS = 4;

// King safety
// The value of having 0, 1, and both castling rights
const int CASTLING_RIGHTS_VALUE[3] = {0, 33, 63};
// The value of a pawn shield per pawn. First rank value is used for the
// penalty when the pawn is missing.
const int PAWN_SHIELD_VALUE[4][8] = {
    {-12, 16, 24,  9, -2, -5, -8,  0}, // open h file, h2, h3, ...
    {-23, 36, 24,  0, -6,-12,-18,  0}, // g/b file
    {-12, 38,  5,  0, -6,-10,-12,  0}, // f/c file
    {-10, 14,  8,  5,  2, -4, -7,  0}  // d/e file
};
// Array for pawn storm values. Rank 1 of open is used for penalty
// when there is no opposing pawn
const int PAWN_STORM_VALUE[3][4][8] = {
// Open file
{
    {11,-40, 22, 11,  8,  0,  0,  0},
    {12,-13, 30, 10,  5,  0,  0,  0},
    { 6, 10, 35, 14,  8,  0,  0,  0},
    { 8, 18, 20, 10,  6,  0,  0,  0}
},
// Blocked pawn
{
    { 0,  0, 24,  3,  0,  0,  0,  0},
    { 0,  0, 46,  4,  0,  0,  0,  0},
    { 0,  0, 35,  5,  0,  0,  0,  0},
    { 0,  0, 32,  5,  2,  0,  0,  0}
},
// Non-blocked pawn
{
    { 0,  0, 22, 10,  6,  0,  0,  0},
    { 0,  0, 20, 15,  8,  0,  0,  0},
    { 0,  0, 24, 12,  6,  0,  0,  0},
    { 0,  4, 15, 15,  2,  0,  0,  0}
},
};

// Scale factor for pieces attacking opposing king
const double KS_ARRAY_FACTOR = 36.0;
const int KING_THREAT_MULTIPLIER[4] = {2, 1, 2, 4};
const int KING_THREAT_SQUARE[4] = {7, 7, 3, 7};
const int KING_DEFENSELESS_SQUARE = 11;
const int KS_PAWN_FACTOR = 7;
const int SAFE_CHECK_BONUS[4] = {32, 8, 20, 20};

// Minor pieces
// A penalty for each own pawn that is on a square of the same color as your bishop
const Score BISHOP_PAWN_COLOR_PENALTY = E(-7, -2);
const Score BISHOP_RAMMED_PAWN_COLOR_PENALTY = E(-3, -14);
// A bonus for strong outpost knights
const Score KNIGHT_OUTPOST_BONUS1 = E(25, 12);
const Score KNIGHT_OUTPOST_BONUS2 = E(17, 5);
const Score KNIGHT_OUTPOST_PAWN_DEF_BONUS = E(13, 11);
// A smaller bonus for bishops
const Score BISHOP_OUTPOST_BONUS1 = E(18, 3);
const Score BISHOP_OUTPOST_BONUS2 = E(9, 0);
const Score BISHOP_OUTPOST_PAWN_DEF_BONUS = E(19, 6);
// A penalty for Nc3 blocking c2-c4 in closed openings
const Score KNIGHT_C3_CLOSED_PENALTY = E(-20, 0);

// Rooks
const Score ROOK_OPEN_FILE_BONUS = E(28, 13);
const Score ROOK_SEMIOPEN_FILE_BONUS = E(10, 0);

// Threats
const Score UNDEFENDED_PAWN = E(-6, -20);
const Score UNDEFENDED_MINOR = E(-14, -38);
const Score ATTACKED_MAJOR = E(-7, -1);
const Score PAWN_MINOR_THREAT = E(-53, -34);
const Score PAWN_MAJOR_THREAT = E(-53, -27);

const Score LOOSE_PAWN = E(-16, -7);
const Score LOOSE_MINOR = E(-9, -7);

// Pawn structure
// Passed pawns
const Score PASSER_BONUS[8] = {E(  0,   0), E(  1,   4), E(  1,  5), E(  4,  12),
                               E( 21,  24), E( 56,  60), E( 99, 99), E(  0,   0)};
const Score PASSER_FILE_BONUS[8] = {E( 11,  11), E(  6, 11), E(-12, -6), E(-13, -10),
                                    E(-13, -10), E(-12, -6), E(  6, 11), E( 11,  11)};
const Score FREE_PROMOTION_BONUS = E(3, 12);
const Score FREE_STOP_BONUS = E(3, 4);
const Score FULLY_DEFENDED_PASSER_BONUS = E(11, 14);
const Score DEFENDED_PASSER_BONUS = E(8, 10);
const Score OWN_KING_DIST = E(0, 2);
const Score OPP_KING_DIST = E(0, 4);

// Doubled pawns
const Score DOUBLED_PENALTY = E(-13, -17);
// Doubled pawns are worse the less pawns you have
const Score DOUBLED_PENALTY_SCALE[9] = {0, 0, 3, 2, 1, 1, 1, 1, 1};
// Isolated pawns
const Score ISOLATED_PENALTY = E(-8, -12);
const Score CENTRAL_ISOLATED_PENALTY = E(-3, -5);
// Isolated, doubled pawns
const Score ISOLATED_DOUBLED_PENALTY = E(-10, -21);
// Backward pawns
const Score BACKWARD_PENALTY = E(-11, 0);
const Score BACKWARD_SEMIOPEN_PENALTY = E(-8, -15);
// Undefended pawns that are not backwards or isolated
const Score UNDEFENDED_PAWN_PENALTY = E(-9, -6);
// Pawn phalanxes
const Score PAWN_PHALANX_BONUS = E(2, 0);
const Score PAWN_PHALANX_RANK_BONUS = E(14, 12);
// King-pawn tropism
const int KING_TROPISM_VALUE = 14;

// Scale factors for drawish endgames
const int MAX_SCALE_FACTOR = 32;
const int OPPOSITE_BISHOP_SCALING[2] = {16, 27};
const int PAWNLESS_SCALING[4] = {1, 3, 8, 21};

#undef E

#endif