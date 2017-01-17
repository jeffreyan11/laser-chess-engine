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

const int SEE_PIECE_VALS[6] = {100, 400, 400, 600, 1150, MATE_SCORE/2};
const int EG_FACTOR_PIECE_VALS[5] = {68, 399, 390, 699, 1813};
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
  {100, 390, 427, 613, 1288},
  {142, 380, 432, 669, 1289}
};
const int KNOWN_WIN = PIECE_VALUES[EG][PAWNS] * 75;
const int TB_WIN = PIECE_VALUES[EG][PAWNS] * 125;

//------------------------------Piece tables--------------------------------
const int pieceSquareTable[2][6][32] = {
// Midgame
{
{ // Pawns
  0,  0,  0,  0,
 26, 28, 38, 50,
 22, 26, 30, 37,
  4,  7, 12, 29,
-10, -2,  6, 16,
 -4,  0, -1, 10,
 -9, -3, -3,  5,
  0,  0,  0,  0
},
{ // Knights
-99,-16,-10, -6,
-19,-14,  4, 15,
-12, 10, 12, 25,
  5,  8, 13, 21,
  0,  6,  8, 14,
-14,  1,  6, 11,
-20, -8, -5,  1,
-52,-23,-12, -8
},
{ // Bishops
-10, -5, -5, -5,
 -5, -5,  0,  0,
 -5,  0,  5,  0,
 -5, 10,  5,  5,
 -5,  5,  0,  0,
 -5, 10,  5,  0,
 -5, 10,  5,  0,
-10, -5, -5, -5
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
-10, -5, -5, -5,
 -5, -5,  0,  0,
 -5,  0,  0,  0,
 -5,  0,  0,  0,
 -5,  0,  0,  0,
 -5,  0,  0,  0,
 -5,  0,  0,  0,
-10, -5, -5, -5
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
 15, 20, 20, 20,
  0,  5,  3,  3,
-10, -7, -3, -3,
-18,-12, -5, -5,
-18,-12, -5, -5,
  0,  0,  0,  0
},
{ // Knights
-50,-10,  0,  3,
-10,  0,  5, 15,
  1,  8, 10, 19,
  7, 12, 18, 21,
  6, 11, 17, 21,
 -8, -3,  2, 10,
-11, -4,  0,  5,
-45,-24,-19,-15
},
{ // Bishops
-15,-10, -5, -5,
-10,  0,  0,  5,
 -5,  0,  5,  5,
 -5,  5,  5,  5,
 -5,  5,  5,  5,
 -5,  5,  5,  5,
 -5,  5,  0,  0,
-10, -5, -5, -5
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
-10, -5, -5, -5,
 -5,  0,  0,  0,
 -5,  0,  0,  0,
 -5,  0,  0,  0,
 -5,  0,  0,  0,
 -5,  0,  0,  0,
 -5,  0,  0,  0,
-10, -5, -5, -5
},
{ // Kings
-82,-19,-11, -5,
-27,  0, 11, 16,
 -3, 15, 28, 32,
 -8, 15, 25, 34,
-12,  5, 18, 23,
-24, -2,  3,  8,
-26,-11, -5, -5,
-60,-31,-27,-24
}
}
};

//-------------------------Material eval constants------------------------------
const int BISHOP_PAIR_VALUE = 55;
const int TEMPO_VALUE = 10;

// Pawns are a target for the queen in the endgame
const int QUEEN_PAWN_PENALTY = -19;

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
-32,-10,  0,  7, 14, 20, 26, 31, 36, 40, 45, 49, 53, 57,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{ // Rooks
-41,-27,-19,-14, -8, -4,  0,  3,  7, 11, 14, 17, 20, 24, 27,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{ // Queens
-24,-16,-11, -7, -3,  0,  3,  6,  9, 12, 15, 18, 20, 23,
 25, 28, 30, 33, 35, 37, 40, 42, 44, 46, 48, 50, 52, 55}
},

// Endgame
{
{ // Knights
-46,-23,-13, -5,  1,  8, 13, 19, 24,
  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{ // Bishops
-64,-14, -3,  4, 10, 15, 20, 24, 27, 30, 33, 36, 39, 42,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{ // Rooks
-68, -5,  8, 18, 26, 32, 38, 43, 47, 51, 55, 59, 62, 65, 69,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{ // Queens
-65,-46,-37,-30,-24,-19,-14,-10, -6, -2,  1,  5,  8, 11,
 15, 18, 21, 24, 27, 29, 32, 35, 37, 40, 42, 45, 47, 49}
}
};

// Value of each square in the extended center in cp
const int EXTENDED_CENTER_VAL = 3;
// Additional bonus for squares in the center four squares in cp, in addition
// to EXTENDED_CENTER_VAL
const int CENTER_BONUS = 5;

// King safety
// The value of having 0, 1, and both castling rights
const int CASTLING_RIGHTS_VALUE[3] = {0, 24, 52};
// The value of a pawn shield per pawn. First rank value is used for the
// penalty when the pawn is missing.
const int PAWN_SHIELD_VALUE[4][8] = {
    {-13, 15, 24, 10, -2, -6,-10,  0}, // open h file, h2, h3, ...
    {-24, 36, 21,  0, -4, -8,-18,  0}, // g/b file
    {-15, 36,  4, -2, -6,-10,-12,  0}, // f/c file
    {-10, 10,  5,  3,  1, -2, -5,  0}  // d/e file
};
// Array for pawn storm values. Rank 1 of open is used for penalty
// when there is no opposing pawn
const int PAWN_STORM_VALUE[3][4][8] = {
// Open file
{
    { 9,  2, 16, 10,  7,  0,  0,  0},
    {11,  5, 22, 12,  8,  0,  0,  0},
    { 3, 16, 30, 14,  9,  0,  0,  0},
    { 4, 18, 20, 10,  6,  0,  0,  0}
},
// Blocked pawn
{
    { 0,  0, 15,  3,  1,  0,  0,  0},
    { 0,  0, 23,  4,  1,  0,  0,  0},
    { 0,  0, 20,  4,  1,  0,  0,  0},
    { 0,  0, 25,  5,  2,  0,  0,  0}
},
// Non-blocked pawn
{
    { 0, 11, 18, 10,  5,  0,  0,  0},
    { 0, 11, 20, 11,  5,  0,  0,  0},
    { 0,  5, 20, 11,  5,  0,  0,  0},
    { 0,  5, 16, 10,  5,  0,  0,  0}
},
};

// Scale factor for pieces attacking opposing king
const double KS_ARRAY_FACTOR = 36.0;
const int KING_THREAT_MULTIPLIER[4] = {2, 1, 2, 6};
const int KING_THREAT_SQUARE[4] = {7, 4, 3, 6};
const int KING_DEFENSELESS_SQUARE = 12;

// Minor pieces
// A penalty for each own pawn that is on a square of the same color as your bishop
const Score BISHOP_PAWN_COLOR_PENALTY = E(-6, -6);
// A bonus for each opponent pawn on the board, given once for each knight
const Score KNIGHT_PAWN_BONUS = E(0, 7);
// A bonus for strong outpost knights
const Score KNIGHT_OUTPOST_BONUS1 = E(25, 11);
const Score KNIGHT_OUTPOST_BONUS2 = E(15, 2);
const Score KNIGHT_OUTPOST_PAWN_DEF_BONUS = E(14, 17);
// A smaller bonus for bishops
const Score BISHOP_OUTPOST_BONUS1 = E(11, 5);
const Score BISHOP_OUTPOST_BONUS2 = E(13, 2);
const Score BISHOP_OUTPOST_PAWN_DEF_BONUS = E(22, 9);
// A penalty for Nc3 blocking c2-c4 in closed openings
const Score KNIGHT_C3_CLOSED_PENALTY = E(-21, 0);

// Rooks
const Score ROOK_OPEN_FILE_BONUS = E(27, 12);
const Score ROOK_SEMIOPEN_FILE_BONUS = E(7, 5);

// Threats
const Score UNDEFENDED_PAWN = E(-8, -19);
const Score UNDEFENDED_MINOR = E(-19, -31);
const Score ATTACKED_MAJOR = E(-6, -6);

const Score LOOSE_PAWN = E(-11, -6);
const Score LOOSE_MINOR = E(-7, -7);

// Pawn structure
// Passed pawns
const Score PASSER_BONUS[8] = {E(  0,   0), E(  3,   6), E(  4,  8), E( 10,  15),
                               E( 19,  25), E( 55,  55), E( 94, 94), E(  0,   0)};
const Score PASSER_FILE_BONUS[8] = {E( 12,  12), E( 7, 12), E(-5, -3), E(-10, -14),
                                    E(-10, -14), E(-5, -3), E( 7, 12), E( 12,  12)};
const Score FREE_PROMOTION_BONUS = E(12, 12);
const Score FREE_STOP_BONUS = E(5, 5);
const Score FULLY_DEFENDED_PASSER_BONUS = E(8, 8);
const Score DEFENDED_PASSER_BONUS = E(6, 6);
const Score OWN_KING_DIST = E(0, 2);
const Score OPP_KING_DIST = E(0, 4);

// Doubled pawns
const Score DOUBLED_PENALTY = E(-10, -16);
// Doubled pawns are worse the less pawns you have
const Score DOUBLED_PENALTY_SCALE[9] = {0, 0, 3, 2, 1, 1, 1, 1, 1};
// Isolated pawns
const Score ISOLATED_PENALTY = E(-8, -11);
const Score CENTRAL_ISOLATED_PENALTY = E(-4, -5);
// Isolated, doubled pawns
const Score ISOLATED_DOUBLED_PENALTY = E(-8, -13);
// Backward pawns
const Score BACKWARD_PENALTY = E(-12, 0);
const Score BACKWARD_SEMIOPEN_PENALTY = E(-11, -11);
// King-pawn tropism
const int KING_TROPISM_VALUE = 13;

#undef E

#endif