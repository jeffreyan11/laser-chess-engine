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
const int KNOWN_WIN = PAWN_VALUE_EG * 100;

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

//------------------------------Piece tables--------------------------------
const int pieceSquareTable[2][6][32] = {
// Midgame
{
{ // Pawns
  0,  0,  0,  0,
 20, 25, 30, 35,
  5, 10, 15, 21,
  0,  5, 10, 17,
 -6, -3,  5, 15,
 -6,  0,  0,  8,
 -6,  0,  0,  0,
  0,  0,  0,  0
},
{ // Knights
-35,-15,-10, -5,
-15, -5,  0,  0,
 -5,  5, 10, 15,
-10,  5,  5, 10,
-10,  0,  5, 10,
-15,  5,  5,  5,
-15, -5,  0,  5,
-30,-15,-10, -5
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
-60,-60,-60,-60,
-50,-50,-50,-50,
-40,-40,-40,-40,
-30,-30,-30,-30,
-10,-10,-15,-20,
  0,  5, -5,-10,
 20, 25, 10, -5,
 25, 32, 24,  5
}
},
// Endgame
{
{ // Pawns
  0,  0,  0,  0,
 30, 35, 40, 40,
 10, 15, 20, 20,
 -5,  0,  5,  5,
-10, -5,  0,  0,
-15,-10, -5, -5,
-15,-10, -5, -5,
  0,  0,  0,  0
},
{ // Knights
-20,-10,-10, -5,
-10, -5,  0,  0,
-10,  0,  5,  5,
 -5,  5,  5, 10,
 -5,  5,  5, 10,
-10,  0,  5,  5,
-10, -5,  0,  0,
-20,-10,-10, -5
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
-30,-25,-20,-17,
-25,-15,  0,  5,
-20,  0, 12, 15,
-17,  5, 15, 20,
-17,  5, 15, 20,
-20,  0, 12, 15,
-25,-15,  0,  5,
-30,-25,-20,-17
}
}
};

//-------------------------Material eval constants------------------------------
const int BISHOP_PAIR_VALUE = 58;
const int TEMPO_VALUE = 10;

//------------------------Positional eval constants-----------------------------
// Mobility tables, with zero padding for pieces that cannot move up to 27 squares
const int mobilityScore[2][4][28] = {
// Midgame
{
{ // Knights
-28,-15, -5,  3, 10, 15, 20, 24, 28,
  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{ // Bishops
-45,-30,-18, -8,  0,  7, 14, 20, 25, 30, 34, 38, 42, 45,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{ // Rooks
-40,-28,-20,-13, -7, -2,  2,  6, 10, 13, 16, 19, 22, 24, 26,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{ // Queens
-45,-31,-22,-16,-11, -7, -4, -1,  2,  4,  6,  8, 10, 12,
 14, 16, 18, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30}
},

// Endgame
{
{ // Knights
-28,-15, -5,  3, 10, 15, 20, 24, 28,
  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{ // Bishops
-45,-30,-18, -8,  0,  7, 14, 20, 25, 30, 34, 38, 42, 45,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{ // Rooks
-50,-28,-12,  0,  9, 16, 22, 27, 32, 36, 39, 42, 44, 45, 46,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{ // Queens
-45,-31,-22,-16,-11, -7, -4, -1,  2,  4,  6,  8, 10, 12,
 14, 16, 18, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30}
}
};

// King safety
// The value of having 0, 1, and both castling rights
const int CASTLING_RIGHTS_VALUE[3] = {0, 24, 40};
// The value of a pawn shield per pawn. First rank value is used for the
// penalty when the pawn is missing.
const int PAWN_SHIELD_VALUE[4][8] = {
    { -8, 20, 14,  3,  0, -1, -2,  0},
    {-10, 20, 12,  1, -1, -2, -3,  0},
    { -9, 20, 10,  1,  0, -1, -3,  0},
    { -6,  8,  5,  1,  0, -1, -2,  0}
};
// Array for pawn storm values. Rank 1 of open is used for penalty
// when there is no opposing pawn
const int PAWN_STORM_VALUE[3][4][8] = {
// Open file
{
    { 4, 15, 21, 11,  6,  0,  0,  0},
    { 5, 15, 21, 11,  6,  0,  0,  0},
    { 4, 15, 21, 11,  6,  0,  0,  0},
    { 4, 12, 11,  5,  4,  0,  0,  0}
},
// Blocked pawn
{
    { 0,  0,  6,  3,  1,  0,  0,  0},
    { 0,  0,  9,  5,  2,  0,  0,  0},
    { 0,  0,  9,  5,  2,  0,  0,  0},
    { 0,  0,  6,  4,  3,  0,  0,  0}
},
// Non-blocked pawn
{
    { 0, 15, 18,  8,  5,  0,  0,  0},
    { 0, 15, 18,  8,  5,  0,  0,  0},
    { 0, 15, 18,  8,  5,  0,  0,  0},
    { 0, 12,  9,  5,  4,  0,  0,  0}
},
};
// An additional penalty for a fully open file next to the king
// const int OPEN_PENALTY = 3;

// Minor pieces
// A penalty for each own pawn that is on a square of the same color as your bishop
const Score BISHOP_PAWN_COLOR_PENALTY = E(-2, -2);
// A bonus for each opponent pawn on the board, given once for each knight
const Score KNIGHT_PAWN_BONUS = E(1, 1);
// A bonus for strong outpost knights
const Score KNIGHT_OUTPOST_BONUS = E(22, 6);
const Score KNIGHT_OUTPOST_PAWN_DEF_BONUS = E(11, 4);
// A smaller bonus for bishops
const Score BISHOP_OUTPOST_BONUS = E(6, 2);
const Score BISHOP_OUTPOST_PAWN_DEF_BONUS = E(3, 1);
// A penalty for Nc3 blocking c2-c4 in closed openings
const Score KNIGHT_C3_CLOSED_PENALTY = E(-15, 0);

// Rooks
const Score ROOK_OPEN_FILE_BONUS = E(10, 10);

// Pawn structure
// Passed pawns
const Score PASSER_BONUS[8] = {E(  0,   0), E(  5,  12), E(  5,  14), E( 10,  25),
                               E( 20,  45), E( 45,  70), E( 80, 105), E(  0,   0)};
const Score PASSER_FILE_BONUS[8] = {E(10,  8), E( 5,  4), E( 2,  1), E( 0,  0),
                                    E( 0,  0), E( 2,  1), E( 5,  4), E(10,  8)};
const Score BLOCKADED_PASSER_PENALTY = E(-2, -2);
const Score FREE_PROMOTION_BONUS = E(4, 5);
const Score FULLY_DEFENDED_PASSER_BONUS = E(3, 3);
const Score DEFENDED_PASSER_BONUS = E(2, 2);
const Score OWN_KING_DIST = E(0, 2);
const Score OPP_KING_DIST = E(0, 4);

// Doubled pawns
const Score DOUBLED_PENALTY[7] = {E(   0,    0), E(   0,    0), E( -11,  -16), E( -33,  -48),
                                  E( -80, -120), E(-150, -230), E(-250, -350)};
// Doubled pawns are worse the less pawns you have
const Score DOUBLED_PENALTY_SCALE[9] = {0, 0, 3, 2, 1, 1, 1, 1, 1};
// Isolated pawns
const Score ISOLATED_PENALTY = E(-11, -13);
const Score CENTRAL_ISOLATED_PENALTY = E(-4, -4);
// Isolated, doubled pawns
const Score ISOLATED_DOUBLED_PENALTY = E(-8, -8);
// Backward pawns
const Score BACKWARD_PENALTY = E(-12, -13);

#undef E

#endif