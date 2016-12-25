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
const int EG_FACTOR_PIECE_VALS[5] = {73, 400, 388, 702, 1814};
const int EG_FACTOR_ALPHA = 2720;
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
  {100, 392, 430, 615, 1279},
  {137, 375, 424, 660, 1284}
};
const int KNOWN_WIN = PIECE_VALUES[EG][PAWNS] * 100;
const int TB_WIN = PIECE_VALUES[EG][PAWNS] * 125;

//------------------------------Piece tables--------------------------------
const int pieceSquareTable[2][6][32] = {
// Midgame
{
{ // Pawns
  0,  0,  0,  0,
 32, 31, 36, 46,
 18, 24, 26, 31,
 -3,  7, 12, 20,
-11, -9,  6, 11,
 -3,  0,  0,  8,
-11, -2, -6,  2,
  0,  0,  0,  0
},
{ // Knights
-82,-20,-14, -8,
-26,-14,  5,  8,
-12, 16, 14, 25,
  5, 10, 15, 20,
 -5,  5, 10, 15,
-14,  2,  5, 10,
-21,-10, -5,  1,
-52,-27,-20,-15
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
-62,-52,-50,-53,
-49,-43,-40,-41,
-34,-24,-35,-35,
-23,-20,-30,-31,
-15,-10,-25,-25,
 -8,  4,-17,-20,
 37, 40, 11, -5,
 30, 49, 24,  0
}
},
// Endgame
{
{ // Pawns
  0,  0,  0,  0,
 25, 40, 40, 40,
 10, 20, 20, 20,
 -5,  5,  5,  5,
-10,  0,  0,  0,
-15,-10, -5, -5,
-15,-10, -5, -5,
  0,  0,  0,  0
},
{ // Knights
-50,-16, -5,  0,
-15,-10,  0,  5,
 -5, 10, 10, 15,
  0, 15, 15, 15,
  0, 15, 15, 15,
-15, -5,  0, 10,
-20,-12,-10, -5,
-45,-21,-15,-10
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
-73,-28,-16,-10,
-28, -4, 11, 16,
-12, 12, 21, 27,
 -8, 17, 25, 31,
-11,  8, 18, 19,
-24, -6,  8, 13,
-27,-11, -5, -1,
-56,-32,-27,-18
}
}
};

//-------------------------Material eval constants------------------------------
const int BISHOP_PAIR_VALUE = 55;
const int TEMPO_VALUE = 8;

// Pawns are a target for the queen in the endgame
const int QUEEN_PAWN_PENALTY = -15;

//------------------------Positional eval constants-----------------------------
// Mobility tables, with zero padding for pieces that cannot move up to 27 squares
const int mobilityScore[2][4][28] = {
// Midgame
{
{ // Knights
-16,  2, 10, 15, 20, 25, 29, 32, 36,
  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{ // Bishops
-32,-10,  0,  9, 16, 22, 28, 33, 38, 43, 47, 51, 56, 60,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{ // Rooks
-41,-29,-22,-17,-11, -7, -2,  2,  6, 10, 13, 17, 20, 23, 27,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{ // Queens
-24,-17,-12, -8, -4, -1,  3,  6,  9, 12, 14, 17, 20, 22,
 25, 27, 30, 32, 34, 36, 39, 41, 43, 45, 47, 49, 51, 54}
},

// Endgame
{
{ // Knights
-46,-31,-21,-12, -5,  2,  9, 15, 22,
  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{ // Bishops
-64,-18, -7,  1,  6, 11, 16, 19, 23, 26, 29, 32, 34, 37,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{ // Rooks
-68,-18, -4,  6, 14, 21, 27, 32, 37, 41, 45, 49, 53, 56, 60,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{ // Queens
-65,-50,-41,-35,-29,-24,-19,-14,-10, -6, -2,  2,  5,  9,
 12, 15, 19, 22, 25, 28, 31, 33, 36, 39, 42, 44, 47, 50}
}
};

// Value of each square in the extended center in cp
const int EXTENDED_CENTER_VAL = 2;
// Additional bonus for squares in the center four squares in cp, in addition
// to EXTENDED_CENTER_VAL
const int CENTER_BONUS = 5;

// King safety
// The value of having 0, 1, and both castling rights
const int CASTLING_RIGHTS_VALUE[3] = {0, 24, 52};
// The value of a pawn shield per pawn. First rank value is used for the
// penalty when the pawn is missing.
const int PAWN_SHIELD_VALUE[4][8] = {
    {-11, 18, 23, 12,  2, -4, -8,  0}, // open h file, h2, h3, ...
    {-24, 34, 23,  3, -7,-12,-18,  0}, // g/b file
    {-14, 35,  5, -2, -6, -8,-10,  0}, // f/c file
    {-10,  6,  2, -1, -3, -5, -7,  0}  // d/e file
};
// Array for pawn storm values. Rank 1 of open is used for penalty
// when there is no opposing pawn
const int PAWN_STORM_VALUE[3][4][8] = {
// Open file
{
    { 8,  2, 16, 10,  7,  0,  0,  0},
    {11,  5, 22, 12,  8,  0,  0,  0},
    { 4, 16, 30, 16,  9,  0,  0,  0},
    { 4, 18, 20, 10,  6,  0,  0,  0}
},
// Blocked pawn
{
    { 0,  0, 15,  3,  1,  0,  0,  0},
    { 0,  0, 23,  4,  1,  0,  0,  0},
    { 0,  0, 20,  4,  1,  0,  0,  0},
    { 0,  0, 30,  7,  2,  0,  0,  0}
},
// Non-blocked pawn
{
    { 0, 11, 18,  8,  5,  0,  0,  0},
    { 0, 11, 20,  8,  5,  0,  0,  0},
    { 0,  5, 20, 10,  5,  0,  0,  0},
    { 0,  3, 14,  7,  4,  0,  0,  0}
},
};

// Scale factor for pieces attacking opposing king
const double KS_ARRAY_FACTOR = 36.0;
const int KING_THREAT_MULTIPLIER[4] = {6, 5, 6, 11};
const int KING_DEFENSELESS_SQUARE = 10;

// Minor pieces
// A penalty for each own pawn that is on a square of the same color as your bishop
const Score BISHOP_PAWN_COLOR_PENALTY = E(-6, -7);
// A bonus for each opponent pawn on the board, given once for each knight
const Score KNIGHT_PAWN_BONUS = E(1, 6);
// A bonus for strong outpost knights
const Score KNIGHT_OUTPOST_BONUS = E(27, 0);
const Score KNIGHT_OUTPOST_PAWN_DEF_BONUS = E(24, 24);
// A smaller bonus for bishops
const Score BISHOP_OUTPOST_BONUS = E(15, 0);
const Score BISHOP_OUTPOST_PAWN_DEF_BONUS = E(26, 26);
// A penalty for Nc3 blocking c2-c4 in closed openings
const Score KNIGHT_C3_CLOSED_PENALTY = E(-24, 0);

// Rooks
const Score ROOK_OPEN_FILE_BONUS = E(26, 10);
const Score ROOK_SEMIOPEN_FILE_BONUS = E(8, 8);

// Pawn structure
// Passed pawns
const Score PASSER_BONUS[8] = {E(  0,   0), E(  3,   6), E(  4,  8), E( 10,  15),
                               E( 19,  25), E( 55,  55), E( 94, 94), E(  0,   0)};
const Score PASSER_FILE_BONUS[8] = {E(12,  9), E( 6,  8), E( 2,  1), E(-6, -9),
                                    E(-6, -9), E( 2,  1), E( 6,  8), E(12,  9)};
const Score FREE_PROMOTION_BONUS = E(12, 12);
const Score FREE_STOP_BONUS = E(5, 5);
const Score FULLY_DEFENDED_PASSER_BONUS = E(8, 8);
const Score DEFENDED_PASSER_BONUS = E(6, 6);
const Score OWN_KING_DIST = E(0, 2);
const Score OPP_KING_DIST = E(0, 4);

// Doubled pawns
const Score DOUBLED_PENALTY[7] = {E(   0,    0), E(   0,    0), E( -10,  -15), E( -24,  -41),
                                  E( -80, -120), E(-150, -230), E(-250, -350)};
// Doubled pawns are worse the less pawns you have
const Score DOUBLED_PENALTY_SCALE[9] = {0, 0, 3, 2, 1, 1, 1, 1, 1};
// Isolated pawns
const Score ISOLATED_PENALTY = E(-9, -12);
const Score CENTRAL_ISOLATED_PENALTY = E(-4, -5);
// Isolated, doubled pawns
const Score ISOLATED_DOUBLED_PENALTY = E(-8, -12);
// Backward pawns
const Score BACKWARD_PENALTY = E(-12, -2);
const Score BACKWARD_SEMIOPEN_PENALTY = E(-12, -10);

#undef E

#endif