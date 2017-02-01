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
const int EG_FACTOR_PIECE_VALS[5] = {67, 388, 388, 702, 1814};
const int EG_FACTOR_ALPHA = 2700;
const int EG_FACTOR_BETA = 5430;
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
  {100, 392, 428, 610, 1298},
  {143, 389, 446, 698, 1287}
};
const int KNOWN_WIN = PIECE_VALUES[EG][PAWNS] * 75;
const int TB_WIN = PIECE_VALUES[EG][PAWNS] * 125;

//------------------------------Piece tables--------------------------------
const int pieceSquareTable[2][6][32] = {
// Midgame
{
{ // Pawns
  0,  0,  0,  0,
 36, 28, 44, 60,
 22, 26, 37, 44,
  5, 13, 18, 30,
 -6,  1,  9, 16,
 -3, -1,  3,  7,
-10,  2, -2, -3,
  0,  0,  0,  0
},
{ // Knights
-116,-22, -8, -4,
-21,-12, 10, 17,
-13,  4, 13, 30,
  5,  6, 15, 20,
  0, 10, 12, 16,
-16,  5,  5, 15,
-20, -9, -2,  6,
-85,-29, -8, -4
},
{ // Bishops
-15,-10, -5,-10,
-10, -5, -5,  5,
  5,  0,  5,  0,
-10,  9,  2,  7,
  0,  5, -5,  0,
 -5, 15, 10,  5,
 -5, 15, 10,  5,
-20, -5,-10,  0
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
-15,-10, -5, -5,
 -5,-15,  0,  0,
 -5,  0,  0,  0,
 -5,  0,  0,  0,
 -5,  0,  0,  0,
 -5, -5,  0,  0,
-10, -5,  5,  0,
-15,-10, -5,  0
},
{ // Kings
-62,-47,-50,-53,
-49,-38,-40,-41,
-34,-29,-35,-35,
-23,-20,-30,-31,
-20,-15,-25,-25,
 -8, 12,-17,-15,
 42, 45, 11,  0,
 33, 51, 22, -8
}
},
// Endgame
{
{ // Pawns
  0,  0,  0,  0,
 20, 40, 50, 50,
 15, 20, 25, 25,
  4,  6,  8,  8,
-10, -7, -3, -3,
-20,-15, -5, -5,
-20,-15, -5, -5,
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
-15,-10, -5, -5,
-10, -5, -5,  5,
 -5, -5,  5, 10,
 -5,  5,  5, 10,
 -5,  5,  5, 10,
 -5,  5,  5, 10,
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
-20,-10,-10,-10,
 -5,  0,  0,  0,
 -5,  0,  0,  0,
 -5,  0,  0,  5,
 -5,  0,  0,  5,
 -5,  0,  0,  0,
-10, -5, -5, -5,
-20,-10,-10, -5
},
{ // Kings
-82,-14, -8, -3,
-12,  5, 14, 19,
  0, 25, 30, 34,
 -3, 20, 25, 29,
-12,  5, 13, 18,
-24, -2,  3,  8,
-25, -9, -5, -5,
-50,-30,-27,-24
}
}
};

//-------------------------Material eval constants------------------------------
const int BISHOP_PAIR_VALUE = 54;
const int TEMPO_VALUE = 12;

// Pawns are a target for the queen in the endgame
const int QUEEN_PAWN_PENALTY = -21;

//------------------------Positional eval constants-----------------------------
// Mobility tables, with zero padding for pieces that cannot move up to 27 squares
const int mobilityScore[2][4][28] = {
// Midgame
{
{ // Knights
-16,  9, 16, 20, 24, 28, 31, 33, 36,
  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{ // Bishops
-32,-10,  0,  7, 14, 20, 25, 30, 35, 39, 44, 48, 52, 56,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{ // Rooks
-41,-30,-23,-18,-13, -8, -3,  0,  4,  8, 11, 15, 19, 22, 26,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{ // Queens
-24,-17,-12, -8, -4, -1,  1,  5,  8, 11, 13, 16, 19, 22,
 24, 27, 30, 32, 35, 37, 39, 42, 44, 46, 49, 51, 53, 56}
},

// Endgame
{
{ // Knights
-46, -7,  2, 10, 15, 21, 25, 29, 33,
  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{ // Bishops
-64, -2,  8, 16, 22, 27, 31, 35, 38, 41, 44, 47, 49, 52,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{ // Rooks
-68, 11, 26, 36, 43, 50, 55, 60, 65, 68, 72, 76, 79, 82, 85,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{ // Queens
-65,-34,-24,-17,-11, -6, -2,  1,  5,  8, 11, 14, 17, 20,
 22, 25, 27, 29, 31, 34, 36, 38, 40, 41, 43, 45, 47, 49}
}
};

// Value of each square in the extended center in cp
const int EXTENDED_CENTER_VAL = 3;
// Additional bonus for squares in the center four squares in cp, in addition
// to EXTENDED_CENTER_VAL
const int CENTER_BONUS = 5;

// King safety
// The value of having 0, 1, and both castling rights
const int CASTLING_RIGHTS_VALUE[3] = {0, 33, 62};
// The value of a pawn shield per pawn. First rank value is used for the
// penalty when the pawn is missing.
const int PAWN_SHIELD_VALUE[4][8] = {
    {-12, 17, 24, 10, -2, -5, -8,  0}, // open h file, h2, h3, ...
    {-24, 37, 23,  0, -6,-12,-18,  0}, // g/b file
    {-14, 36,  7,  0, -6,-10,-12,  0}, // f/c file
    {-10, 14, 10,  1, -1, -4, -7,  0}  // d/e file
};
// Array for pawn storm values. Rank 1 of open is used for penalty
// when there is no opposing pawn
const int PAWN_STORM_VALUE[3][4][8] = {
// Open file
{
    {11,-25, 24, 12,  8,  0,  0,  0},
    {12,-10, 33, 10,  5,  0,  0,  0},
    { 6, 10, 36, 15,  8,  0,  0,  0},
    { 8, 18, 20, 10,  6,  0,  0,  0}
},
// Blocked pawn
{
    { 0,  0, 18,  3,  0,  0,  0,  0},
    { 0,  0, 40,  4,  1,  0,  0,  0},
    { 0,  0, 32,  4,  1,  0,  0,  0},
    { 0,  0, 22,  5,  3,  0,  0,  0}
},
// Non-blocked pawn
{
    { 0, 10, 24, 13,  4,  0,  0,  0},
    { 0, 10, 29, 15,  8,  0,  0,  0},
    { 0,  5, 29, 15,  6,  0,  0,  0},
    { 0,  4, 20, 15,  2,  0,  0,  0}
},
};

// Scale factor for pieces attacking opposing king
const double KS_ARRAY_FACTOR = 36.0;
const int KING_THREAT_MULTIPLIER[4] = {2, 0, 2, 5};
const int KING_THREAT_SQUARE[4] = {7, 7, 3, 7};
const int KING_DEFENSELESS_SQUARE = 11;
const int KS_PAWN_FACTOR = 9;

// Minor pieces
// A penalty for each own pawn that is on a square of the same color as your bishop
const Score BISHOP_PAWN_COLOR_PENALTY = E(-7, -5);
// A bonus for each opponent pawn on the board, given once for each knight
const Score KNIGHT_PAWN_BONUS = E(0, 8);
// A bonus for strong outpost knights
const Score KNIGHT_OUTPOST_BONUS1 = E(28, 15);
const Score KNIGHT_OUTPOST_BONUS2 = E(20, 11);
const Score KNIGHT_OUTPOST_PAWN_DEF_BONUS = E(17, 15);
// A smaller bonus for bishops
const Score BISHOP_OUTPOST_BONUS1 = E(18, 6);
const Score BISHOP_OUTPOST_BONUS2 = E(14, 4);
const Score BISHOP_OUTPOST_PAWN_DEF_BONUS = E(20, 10);
// A penalty for Nc3 blocking c2-c4 in closed openings
const Score KNIGHT_C3_CLOSED_PENALTY = E(-24, 0);

// Rooks
const Score ROOK_OPEN_FILE_BONUS = E(30, 16);
const Score ROOK_SEMIOPEN_FILE_BONUS = E(7, 0);

// Threats
const Score UNDEFENDED_PAWN = E(-7, -21);
const Score UNDEFENDED_MINOR = E(-14, -38);
const Score ATTACKED_MAJOR = E(-7, -4);
const Score PAWN_MINOR_THREAT = E(-58, -41);
const Score PAWN_MAJOR_THREAT = E(-52, -22);

const Score LOOSE_PAWN = E(-15, -6);
const Score LOOSE_MINOR = E(-8, -13);

// Pawn structure
// Passed pawns
const Score PASSER_BONUS[8] = {E(  0,   0), E(  2,   5), E(  3,  6), E(  6,  12),
                               E( 21,  21), E( 60,  60), E( 99, 99), E(  0,   0)};
const Score PASSER_FILE_BONUS[8] = {E( 10,  13), E( 8, 11), E(-9, -5), E(-12, -19),
                                    E(-12, -19), E(-9, -5), E( 8, 11), E( 10,  13)};
const Score FREE_PROMOTION_BONUS = E(0, 13);
const Score FREE_STOP_BONUS = E(5, 4);
const Score FULLY_DEFENDED_PASSER_BONUS = E(13, 13);
const Score DEFENDED_PASSER_BONUS = E(8, 8);
const Score OWN_KING_DIST = E(0, 2);
const Score OPP_KING_DIST = E(0, 4);

// Doubled pawns
const Score DOUBLED_PENALTY = E(-11, -16);
// Doubled pawns are worse the less pawns you have
const Score DOUBLED_PENALTY_SCALE[9] = {0, 0, 3, 2, 1, 1, 1, 1, 1};
// Isolated pawns
const Score ISOLATED_PENALTY = E(-8, -11);
const Score CENTRAL_ISOLATED_PENALTY = E(-3, -4);
// Isolated, doubled pawns
const Score ISOLATED_DOUBLED_PENALTY = E(-11, -16);
// Backward pawns
const Score BACKWARD_PENALTY = E(-11, 0);
const Score BACKWARD_SEMIOPEN_PENALTY = E(-10, -16);
// King-pawn tropism
const int KING_TROPISM_VALUE = 14;

#undef E

#endif