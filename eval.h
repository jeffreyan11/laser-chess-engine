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
const int EG_FACTOR_PIECE_VALS[5] = {73, 402, 388, 703, 1811};
const int EG_FACTOR_ALPHA = 2710;
const int EG_FACTOR_BETA = 5413;
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
  {100, 392, 429, 615, 1276},
  {131, 375, 428, 661, 1282}
};
const int KNOWN_WIN = PIECE_VALUES[EG][PAWNS] * 100;

//------------------------------Piece tables--------------------------------
const int pieceSquareTable[2][6][32] = {
// Midgame
{
{ // Pawns
  0,  0,  0,  0,
 32, 31, 36, 46,
 16, 27, 29, 31,
 -3,  7, 12, 18,
-11, -9,  6, 11,
 -3,  0,  0,  8,
-11, -2, -6,  2,
  0,  0,  0,  0
},
{ // Knights
-69,-20, -4, -3,
-22,-15, 10, 10,
-12, 16, 15, 25,
  0, 10, 15, 20,
 -5,  5, 10, 15,
-17,  2,  5, 10,
-21,-15, -5,  3,
-40,-20,-15,-15
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
-62,-52,-50,-48,
-44,-43,-40,-36,
-29,-24,-30,-30,
-18,-20,-25,-26,
 -8,-10,-20,-20,
 -3,  0,-12,-15,
 27, 38,  3, -5,
 32, 48, 26,  0
}
},
// Endgame
{
{ // Pawns
  0,  0,  0,  0,
 30, 40, 40, 40,
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
 -5,  5, 10, 15,
  0, 10, 15, 15,
  0, 10, 15, 15,
-15, -5,  0, 10,
-20,-12,-10, -5,
-35,-21,-15,-10
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
-73,-28,-21,-20,
-28,-11,  9, 19,
-12, 10, 18, 25,
 -8, 17, 20, 26,
-11,  5, 18, 19,
-24, -6,  8, 13,
-27,-11, -5, -1,
-56,-32,-27,-18
}
}
};

//-------------------------Material eval constants------------------------------
const int BISHOP_PAIR_VALUE = 51;
const int TEMPO_VALUE = 8;

// Pawns are a target for the queen in the endgame
const int QUEEN_PAWN_PENALTY = -15;

//------------------------Positional eval constants-----------------------------
// Mobility tables, with zero padding for pieces that cannot move up to 27 squares
const int mobilityScore[2][4][28] = {
// Midgame
{
{ // Knights
-16, -1,  6, 11, 15, 19, 22, 26, 29,
  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{ // Bishops
-32, -7,  4, 12, 19, 25, 31, 36, 40, 45, 49, 53, 57, 61,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{ // Rooks
-41,-32,-25,-19,-14, -9, -4,  1,  5,  9, 14, 18, 22, 26, 30,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{ // Queens
-24,-17,-12, -9, -5, -2,  0,  3,  5,  8, 10, 12, 14, 17,
 19, 21, 22, 24, 26, 28, 30, 32, 33, 35, 37, 38, 40, 42}
},

// Endgame
{
{ // Knights
-46,-29,-18,-10, -3,  4, 11, 17, 22,
  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{ // Bishops
-64,-18, -7,  1,  6, 11, 16, 19, 23, 26, 29, 32, 34, 37,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{ // Rooks
-68,-27,-14, -4,  4, 10, 16, 22, 27, 31, 36, 40, 43, 47, 51,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{ // Queens
-65,-41,-33,-26,-21,-16,-12, -8, -4, -1,  2,  5,  8, 11,
 13, 16, 18, 21, 23, 25, 27, 29, 31, 33, 35, 37, 39, 41}
}
};

// Value of each square in the extended center in cp
const int EXTENDED_CENTER_VAL = 2;
// Additional bonus for squares in the center four squares in cp, in addition
// to EXTENDED_CENTER_VAL
const int CENTER_BONUS = 5;

// King safety
// The value of having 0, 1, and both castling rights
const int CASTLING_RIGHTS_VALUE[3] = {0, 22, 52};
// The value of a pawn shield per pawn. First rank value is used for the
// penalty when the pawn is missing.
const int PAWN_SHIELD_VALUE[4][8] = {
    {-10, 20, 18, 12,  4,  3,  2,  0}, // open h file, h2, h3, ...
    {-24, 29, 23,  4, -4, -7,-10,  0}, // g/b file
    {-16, 34,  6,  1, -4, -6, -8,  0}, // f/c file
    {-10,  4,  1, -1, -3, -5, -7,  0}  // d/e file
};
// Array for pawn storm values. Rank 1 of open is used for penalty
// when there is no opposing pawn
const int PAWN_STORM_VALUE[3][4][8] = {
// Open file
{
    {10,  2, 16, 10,  7,  0,  0,  0},
    {11,  5, 22, 12,  8,  0,  0,  0},
    { 1, 16, 30, 16,  9,  0,  0,  0},
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
const int KING_THREAT_MULTIPLIER[4] = {6, 6, 8, 12};
const int KING_DEFENSELESS_SQUARE = 10;

// Minor pieces
// A penalty for each own pawn that is on a square of the same color as your bishop
const Score BISHOP_PAWN_COLOR_PENALTY = E(-5, -6);
// A bonus for each opponent pawn on the board, given once for each knight
const Score KNIGHT_PAWN_BONUS = E(2, 6);
// A bonus for strong outpost knights
const Score KNIGHT_OUTPOST_BONUS = E(31, 0);
const Score KNIGHT_OUTPOST_PAWN_DEF_BONUS = E(15, 18);
// A smaller bonus for bishops
const Score BISHOP_OUTPOST_BONUS = E(15, 0);
const Score BISHOP_OUTPOST_PAWN_DEF_BONUS = E(16, 17);
// A penalty for Nc3 blocking c2-c4 in closed openings
const Score KNIGHT_C3_CLOSED_PENALTY = E(-15, 0);

// Rooks
const Score ROOK_OPEN_FILE_BONUS = E(20, 12);
const Score ROOK_SEMIOPEN_FILE_BONUS = E(8, 4);

// Pawn structure
// Passed pawns
const Score PASSER_BONUS[8] = {E(  0,   0), E(  3,   6), E(  4,  8), E( 10,  15),
                               E( 19,  25), E( 55,  55), E( 94, 94), E(  0,   0)};
const Score PASSER_FILE_BONUS[8] = {E(10,  8), E( 5,  4), E( 2,  1), E( 0,  0),
                                    E( 0,  0), E( 2,  1), E( 5,  4), E(10,  8)};
const Score FREE_PROMOTION_BONUS = E(12, 12);
const Score FREE_STOP_BONUS = E(5, 5);
const Score FULLY_DEFENDED_PASSER_BONUS = E(8, 8);
const Score DEFENDED_PASSER_BONUS = E(6, 6);
const Score OWN_KING_DIST = E(0, 2);
const Score OPP_KING_DIST = E(0, 4);

// Doubled pawns
const Score DOUBLED_PENALTY[7] = {E(   0,    0), E(   0,    0), E( -10,  -15), E( -24,  -46),
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
// const Score BACKWARD_SEMIOPEN_ROOK_BONUS = E(4, 4);

#undef E

#endif