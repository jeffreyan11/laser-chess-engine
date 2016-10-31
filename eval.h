/*
    Laser, a UCI chess engine written in C++11.
    Copyright 2015-2016 Jeffrey An and Michael An

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

const int SEE_PIECE_VALS[6] = {100, 400, 400, 600, 1150, MATE_SCORE};
const int EG_FACTOR_PIECE_VALS[5] = {75, 409, 391, 702, 1807};
const int EG_FACTOR_ALPHA = 2705;
const int EG_FACTOR_BETA = 5411;
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
  {100, 391, 426, 620, 1274},
  {131, 382, 422, 656, 1276}
};
const int KNOWN_WIN = PIECE_VALUES[EG][PAWNS] * 100;

//------------------------------Piece tables--------------------------------
const int pieceSquareTable[2][6][32] = {
// Midgame
{
{ // Pawns
  0,  0,  0,  0,
 20, 30, 30, 35,
 10, 20, 25, 25,
  0,  5,  5, 20,
 -8, -7,  8, 15,
 -6,  0,  0, 12,
 -7,  0,-10,  5,
  0,  0,  0,  0
},
{ // Knights
-69,-24, -2, -1,
-22, -8, 10, 10,
-18, 16,  0, 25,
  0, 10, 15, 20,
 -5,  5,  5, 15,
-15,  5,  5, 10,
-25,-15,  0,  5,
-47,-24,-15,-15
},
{ // Bishops
-10, -5, -5, -5,
 -5,  0,  0,  0,
 -5,  0,  5,  0,
 -5,  5,  5,  0,
 -5,  5,  5,  0,
 -5,  5,  5,  0,
 -5,  5,  0,  0,
-10, -5, -5, -5
},
{ // Rooks
 -5,  0,  0,  0,
 10, 10, 10, 10,
 -5,  0,  0,  0,
 -5,  0,  0,  0,
 -5,  0,  0,  0,
 -5,  0,  0,  0,
 -5,  0,  0,  0,
 -5,  0,  0,  0
},
{ // Queens
 -5,  0,  0,  0,
  0,  0,  0,  0,
  0,  0,  0,  0,
  0,  0,  0,  0,
  0,  0,  0,  0,
  0,  0,  0,  0,
  0,  0,  0,  0,
 -5,  0,  0,  0
},
{ // Kings
-49,-52,-50,-49,
-49,-50,-46,-39,
-29,-24,-25,-33,
-12,-16,-20,-23,
 -7,-10,-15,-20,
 -3,  5,-10,-15,
 22, 33,  3, -5,
 27, 48, 24,  0
}
},
// Endgame
{
{ // Pawns
  0,  0,  0,  0,
 30, 35, 40, 40,
 15, 20, 20, 20,
 -5,  0,  5,  5,
-10, -5,  0,  0,
-15,-10, -5, -5,
-15,-10, -5, -5,
  0,  0,  0,  0
},
{ // Knights
-60,-18, -5,  0,
-15,-10,  0,  5,
-10,  0, 10, 15,
  0, 10, 15, 15,
  0, 10, 15, 15,
-15, -5,  0, 10,
-20,-12,-10, -5,
-40,-21,-10, -5
},
{ // Bishops
-10, -5, -5, -5,
 -5,  0,  0,  0,
 -5,  0,  5,  0,
 -5,  5,  5,  0,
 -5,  5,  5,  0,
 -5,  5,  5,  0,
 -5,  5,  0,  0,
-10, -5, -5, -5
},
{ // Rooks
 -5,  0,  0,  0,
 10, 10, 10, 10,
 -5,  0,  0,  0,
 -5,  0,  0,  0,
 -5,  0,  0,  0,
 -5,  0,  0,  0,
 -5,  0,  0,  0,
 -5,  0,  0,  0
},
{ // Queens
 -5,  0,  0,  0,
  0,  0,  0,  0,
  0,  0,  0,  0,
  0,  0,  0,  0,
  0,  0,  0,  0,
  0,  0,  0,  0,
  0,  0,  0,  0,
 -5,  0,  0,  0
},
{ // Kings
-60,-39,-34,-33,
-41,-24, -4,  6,
-25,  0, 12, 21,
-15,  4, 24, 31,
-13, -1, 15, 31,
-22, -7,  7,  7,
-25,-15,  0,  0,
-45,-25,-23,-22
}
}
};

//-------------------------Material eval constants------------------------------
const int BISHOP_PAIR_VALUE = 56;
const int TEMPO_VALUE = 12;

// Pawn value scaling: incur a penalty for having no pawns since the
// ability to promote is gone
const int PAWN_SCALING_MG[9] = {30, 13,  0,  0,  0, 0, 0, 0, 0};
const int PAWN_SCALING_EG[9] = {40, 32, 25, 18, 12, 0, 0, 0, 0};
// Pawns are a target for the queen in the endgame
const int QUEEN_PAWN_PENALTY = -12;

//------------------------Positional eval constants-----------------------------
// Mobility tables, with zero padding for pieces that cannot move up to 27 squares
const int mobilityScore[2][4][28] = {
// Midgame
{
{ // Knights
-16, -7,  0,  6, 12, 17, 22, 26, 29,
  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{ // Bishops
-32,-19, -8,  2, 11, 19, 26, 32, 38, 43, 48, 52, 56, 60,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{ // Rooks
-41,-29,-19,-11, -4,  2,  8, 13, 17, 20, 22, 23, 24, 25, 26,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{ // Queens
-24,-13, -8, -4,  0,  4,  8, 11, 14, 17, 20, 22, 24, 26,
 28, 30, 32, 33, 34, 35, 36, 37, 38, 38, 39, 39, 40, 40}
},

// Endgame
{
{ // Knights
-46,-32,-20, -8,  4, 12, 18, 23, 26,
  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{ // Bishops
-64,-40,-22, -8,  0,  6, 11, 16, 20, 24, 27, 29, 31, 32,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{ // Rooks
-68,-40,-20, -6,  5, 13, 20, 26, 31, 36, 40, 44, 47, 49, 50,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{ // Queens
-65,-44,-28,-15, -5,  2,  6, 10, 13, 16, 19, 22, 24, 26,
 28, 30, 32, 33, 34, 35, 36, 37, 38, 38, 39, 39, 40, 40}
}
};

// Value of each square in the extended center in cp
const int EXTENDED_CENTER_VAL = 1;
// Additional bonus for squares in the center four squares in cp, in addition
// to EXTENDED_CENTER_VAL
const int CENTER_BONUS = 6;

// King safety
// The value of having 0, 1, and both castling rights
const int CASTLING_RIGHTS_VALUE[3] = {0, 30, 52};
// The value of a pawn shield per pawn. First rank value is used for the
// penalty when the pawn is missing.
const int PAWN_SHIELD_VALUE[4][8] = {
    { -8, 20, 16, 12,  7,  6,  5,  0}, // open h file, h2, h3, ...
    {-20, 25, 17,  4, -4, -7,-10,  0}, // g/b file
    {-14, 30, 10,  4,  2, -2, -6,  0}, // f/c file
    {-12,  6,  1, -2, -4, -7,-10,  0}  // d/e file
};
// Array for pawn storm values. Rank 1 of open is used for penalty
// when there is no opposing pawn
const int PAWN_STORM_VALUE[3][4][8] = {
// Open file
{
    { 4, 15, 21, 12,  7,  0,  0,  0},
    { 5, 15, 21, 12,  7,  0,  0,  0},
    { 4, 15, 21, 12,  7,  0,  0,  0},
    { 4, 12, 13,  7,  4,  0,  0,  0}
},
// Blocked pawn
{
    { 0,  0, 15,  3,  1,  0,  0,  0},
    { 0,  0, 23,  6,  2,  0,  0,  0},
    { 0,  0, 11,  5,  2,  0,  0,  0},
    { 0,  0, 30,  6,  3,  0,  0,  0}
},
// Non-blocked pawn
{
    { 0, 15, 18,  8,  5,  0,  0,  0},
    { 0, 15, 20,  8,  5,  0,  0,  0},
    { 0, 15, 20, 10,  5,  0,  0,  0},
    { 0, 12, 14,  6,  4,  0,  0,  0}
},
};

// Scale factor for pieces attacking opposing king
const double KS_ARRAY_FACTOR = 9.0;
const int KING_THREAT_MULTIPLIER[4] = {3, 4, 4, 6};
const int KING_DEFENSELESS_SQUARE = 5;

// Minor pieces
// A penalty for each own pawn that is on a square of the same color as your bishop
const Score BISHOP_PAWN_COLOR_PENALTY = E(-4, -4);
// A bonus for each opponent pawn on the board, given once for each knight
const Score KNIGHT_PAWN_BONUS = E(1, 2);
// A bonus for strong outpost knights
const Score KNIGHT_OUTPOST_BONUS = E(30, 10);
const Score KNIGHT_OUTPOST_PAWN_DEF_BONUS = E(13, 6);
// A smaller bonus for bishops
const Score BISHOP_OUTPOST_BONUS = E(12, 5);
const Score BISHOP_OUTPOST_PAWN_DEF_BONUS = E(5, 4);
// A penalty for Nc3 blocking c2-c4 in closed openings
const Score KNIGHT_C3_CLOSED_PENALTY = E(-15, 0);

// Rooks
const Score ROOK_OPEN_FILE_BONUS = E(16, 16);

// Pawn structure
// Passed pawns
const Score PASSER_BONUS[8] = {E(  0,   0), E(  3,   8), E(  4, 10), E( 10,  15),
                               E( 25,  30), E( 55,  61), E( 86, 92), E(  0,   0)};
const Score PASSER_FILE_BONUS[8] = {E(10,  8), E( 5,  4), E( 2,  1), E( 0,  0),
                                    E( 0,  0), E( 2,  1), E( 5,  4), E(10,  8)};
const Score FREE_PROMOTION_BONUS = E(12, 12);
const Score FREE_STOP_BONUS = E(5, 5);
const Score FULLY_DEFENDED_PASSER_BONUS = E(6, 6);
const Score DEFENDED_PASSER_BONUS = E(4, 4);
const Score OWN_KING_DIST = E(0, 2);
const Score OPP_KING_DIST = E(0, 4);

// Doubled pawns
const Score DOUBLED_PENALTY[7] = {E(   0,    0), E(   0,    0), E( -10,  -15), E( -24,  -46),
                                  E( -80, -120), E(-150, -230), E(-250, -350)};
// Doubled pawns are worse the less pawns you have
const Score DOUBLED_PENALTY_SCALE[9] = {0, 0, 3, 2, 1, 1, 1, 1, 1};
// Isolated pawns
const Score ISOLATED_PENALTY = E(-10, -13);
const Score CENTRAL_ISOLATED_PENALTY = E(-3, -3);
// Isolated, doubled pawns
const Score ISOLATED_DOUBLED_PENALTY = E(-8, -12);
// Backward pawns
const Score BACKWARD_PENALTY = E(-12, -6);
const Score BACKWARD_SEMIOPEN_PENALTY = E(-10, -7);
// const Score BACKWARD_SEMIOPEN_ROOK_BONUS = E(4, 4);

#undef E

#endif