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
const int EG_FACTOR_PIECE_VALS[5] = {68, 382, 387, 700, 1821};
const int EG_FACTOR_ALPHA = 2720;
const int EG_FACTOR_BETA = 5490;
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
  {100, 390, 426, 604, 1306},
  {142, 393, 449, 703, 1294}
};
const int KNOWN_WIN = PIECE_VALUES[EG][PAWNS] * 75;
const int TB_WIN = PIECE_VALUES[EG][PAWNS] * 125;

//------------------------------Piece tables--------------------------------
const int pieceSquareTable[2][6][32] = {
// Midgame
{
{ // Pawns
  0,  0,  0,  0,
 36, 25, 55, 69,
 18, 29, 40, 47,
  3, 11, 16, 29,
 -5, -2,  8, 14,
 -6, -2,  4,  8,
 -9,  2,  0,  2,
  0,  0,  0,  0
},
{ // Knights
-116,-20, -5, -2,
-22,-10, 18, 19,
-16,  7, 14, 25,
 10, 12, 18, 20,
  3, 12, 12, 15,
-16,  7,  4, 15,
-20,-14, -4,  6,
-68,-21,-12, -1
},
{ // Bishops
-18,-15,-10,-10,
-15, -8, -6,  2,
  3,  4,  3, -1,
  0, 11,  0,  5,
  8,  9, -5, 10,
  3, 15, 10,  5,
  0, 15, 10,  5,
-15, -5, -7,  0
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
-34,-26,-17,-11,
-11,-24, -7, -4,
 -3,  0,  0,  2,
 -3, -3, -3, -8,
 -3, -3, -3, -8,
 -5,  4, -4, -3,
 -8, -6,  5,  5,
 -8, -8, -5,  5
},
{ // Kings
-47,-42,-39,-41,
-39,-35,-35,-36,
-29,-24,-30,-30,
-28,-24,-30,-31,
-25, -9,-25,-25,
 -1, 20,-12,-15,
 41, 45, 17, 16,
 30, 48, 20,-12
}
},
// Endgame
{
{ // Pawns
  0,  0,  0,  0,
 29, 37, 58, 58,
 26, 28, 30, 30,
 10,  8,  8,  8,
-12,-10, -5, -5,
-18,-12, -5, -5,
-18,-12, -5, -5,
  0,  0,  0,  0
},
{ // Knights
-55,  4,  9, 12,
  1, 10, 16, 20,
 10, 15, 16, 20,
 10, 14, 18, 25,
  6, 13, 17, 21,
-10,  3,  7, 20,
-22, -4, -2,  5,
-35,-24,-18,-12
},
{ // Bishops
-10, -5, -5, -5
 -5, -1,  5,  6,
 -2,  2,  8,  4,
  0,  5,  5,  7,
 -3,  4,  5,  5,
 -5, -1,  5,  5,
-10, -4, -2, -1,
-13,-10, -7, -4
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
-16,-11, -8, -4,
 -4,  4,  4,  8,
 -2,  7,  7, 12,
 -1, 12, 11, 13,
 -2,  7,  7,  7,
 -1,  0,  1,  2,
-14,-11, -8, -8,
-23,-20,-19,-11
},
{ // Kings
-111,-20, -14, -10,
-10, 20, 24, 24,
  7, 32, 34, 36,
  0, 19, 24, 26,
-16,  2, 11, 12,
-22, -7, -2,  3,
-24,-13, -8, -8,
-46,-24,-19,-16
}
}
};

//-------------------------Material eval constants------------------------------
const int BISHOP_PAIR_VALUE = 50;
const int TEMPO_VALUE = 15;

// Material imbalance terms
const int KNIGHT_PAIR_PENALTY = 0;
const int ROOK_PAIR_PENALTY = -18;

const int OWN_OPP_IMBALANCE[2][5][5] = {
{
//       Opponent's
//    P   N   B   R   Q
    { 0},                   // Own pawns
    { 0,  0},               // Own knights
    {-1, -2,  0},           // Own bishops
    {-2,  0, -2,  0},       // Own rooks
    {-3,  8,  2,-26,  0}    // Own queens
},
{
    { 0},                   // Own pawns
    { 6,  0},               // Own knights
    { 1,  0,  0},           // Own bishops
    { 3, -2, -9,  0},       // Own rooks
    {19,  6, 15, 21,  0}    // Own queens
}
};

// Bonus for knight in closed positions
const int KNIGHT_CLOSED_BONUS[2] = {1, 2};

//------------------------Positional eval constants-----------------------------
// Mobility tables, with zero padding for pieces that cannot move up to 27 squares
const int mobilityScore[2][4][28] = {
// Midgame
{
{ // Knights
-16,  6, 13, 18, 22, 26, 29, 32, 35,
  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{ // Bishops
-32,-11, -2,  4, 10, 16, 21, 26, 30, 34, 38, 42, 46, 50,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{ // Rooks
-41,-25,-17,-11, -6, -2,  1,  5,  8, 12, 15, 18, 21, 24, 27,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{ // Queens
-24,-18,-14,-11, -7, -4, -1,  1,  3,  6,  9, 12, 14, 17,
 19, 22, 24, 27, 29, 31, 34, 36, 38, 41, 43, 45, 47, 49}
},

// Endgame
{
{ // Knights
-46, -5,  5, 13, 19, 25, 29, 34, 38,
  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{ // Bishops
-64,-15, -2,  6, 14, 20, 26, 31, 35, 39, 43, 47, 50, 54,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
{ // Rooks
-68,  2, 19, 30, 39, 46, 52, 58, 63, 68, 73, 77, 80, 84, 88,
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
const int CENTER_BONUS = 3;

// King safety
// The value of having 0, 1, and both castling rights
const int CASTLING_RIGHTS_VALUE[3] = {0, 33, 64};
// The value of a pawn shield per pawn. First rank value is used for the
// penalty when the pawn is missing.
const int PAWN_SHIELD_VALUE[4][8] = {
    {-12, 17, 23,  8,  4,  0, -8,  0}, // open h file, h2, h3, ...
    {-23, 37, 26,  0, -6,-10,-15,  0}, // g/b file
    {-12, 38,  5,  1, -2, -4, -8,  0}, // f/c file
    {-10, 14,  9,  6,  0, -3,-10,  0}  // d/e file
};
// Array for pawn storm values. Rank 1 of open is used for penalty
// when there is no opposing pawn
const int PAWN_STORM_VALUE[3][4][8] = {
// Open file
{
    {11,-48, 18, 11,  7,  0,  0,  0},
    {12,-15, 30, 14,  5,  0,  0,  0},
    { 6,  5, 35, 16,  8,  0,  0,  0},
    { 6, 17, 26,  9,  6,  0,  0,  0}
},
// Blocked pawn
{
    { 0,  0, 24,  3,  0,  0,  0,  0},
    { 0,  0, 56,  4,  0,  0,  0,  0},
    { 0,  0, 36, 10,  0,  0,  0,  0},
    { 0,  0, 38,  9,  2,  0,  0,  0}
},
// Non-blocked pawn
{
    { 0,  0, 20, 10,  5,  0,  0,  0},
    { 0,  0, 22, 15,  8,  0,  0,  0},
    { 0,  0, 27, 12,  5,  0,  0,  0},
    { 0,  4, 10, 15,  2,  0,  0,  0}
},
};

// Scale factor for pieces attacking opposing king
const double KS_ARRAY_FACTOR = 36.0;
const int KING_THREAT_MULTIPLIER[4] = {2, 1, 2, 4};
const int KING_THREAT_SQUARE[4] = {6, 6, 3, 7};
const int KING_DEFENSELESS_SQUARE = 11;
const int KS_PAWN_FACTOR = 6;
const int SAFE_CHECK_BONUS[4] = {31, 8, 19, 22};

// Minor pieces
// A penalty for each own pawn that is on a square of the same color as your bishop
const Score BISHOP_PAWN_COLOR_PENALTY = E(-6, -1);
const Score BISHOP_RAMMED_PAWN_COLOR_PENALTY = E(-1, -13);
// Minors shielded by own pawn in front
const Score SHIELDED_MINOR_BONUS = E(10, 0);
// A bonus for strong outpost knights
const Score KNIGHT_OUTPOST_BONUS1 = E(25, 14);
const Score KNIGHT_OUTPOST_BONUS2 = E(14, 8);
const Score KNIGHT_OUTPOST_PAWN_DEF_BONUS = E(15, 10);
// A smaller bonus for bishops
const Score BISHOP_OUTPOST_BONUS1 = E(17, 6);
const Score BISHOP_OUTPOST_BONUS2 = E(12, 2);
const Score BISHOP_OUTPOST_PAWN_DEF_BONUS = E(17, 9);

// Rooks
const Score ROOK_OPEN_FILE_BONUS = E(24, 10);
const Score ROOK_SEMIOPEN_FILE_BONUS = E(8, 0);
const Score ROOK_PAWN_RANK_THREAT = E(6, 9);

// Threats
const Score UNDEFENDED_PAWN = E(-7, -20);
const Score UNDEFENDED_MINOR = E(-17, -35);
const Score ATTACKED_MAJOR = E(-4, -1);
const Score PAWN_MINOR_THREAT = E(-49, -38);
const Score PAWN_MAJOR_THREAT = E(-50, -26);

const Score LOOSE_PAWN = E(-18, -11);
const Score LOOSE_MINOR = E(-10, -3);

// Pawn structure
// Passed pawns
const Score PASSER_BONUS[8] = {E(  0,   0), E(  1,   4), E(  1,  5), E(  4,  12),
                               E( 24,  30), E( 57,  59), E( 99, 99), E(  0,   0)};
const Score PASSER_FILE_BONUS[8] = {E(  8,   8), E(  6, 17), E(-10, -3), E(-18, -13),
                                    E(-18, -13), E(-10, -3), E(  6, 17), E(  8,   8)};
const Score FREE_PROMOTION_BONUS = E(3, 12);
const Score FREE_STOP_BONUS = E(3, 4);
const Score FULLY_DEFENDED_PASSER_BONUS = E(9, 13);
const Score DEFENDED_PASSER_BONUS = E(7, 9);
const Score OWN_KING_DIST = E(0, 2);
const Score OPP_KING_DIST = E(0, 4);

// Doubled pawns
const Score DOUBLED_PENALTY = E(-13, -14);
// Doubled pawns are worse the less pawns you have
const Score DOUBLED_PENALTY_SCALE[9] = {0, 0, 3, 2, 1, 1, 1, 1, 1};
// Isolated pawns
const Score ISOLATED_PENALTY = E(-9, -11);
const Score CENTRAL_ISOLATED_PENALTY = E(-6, -8);
// Isolated, doubled pawns
const Score ISOLATED_DOUBLED_PENALTY = E(-10, -22);
// Backward pawns
const Score BACKWARD_PENALTY = E(-11, 0);
const Score BACKWARD_SEMIOPEN_PENALTY = E(-11, -17);
// Undefended pawns that are not backwards or isolated
const Score UNDEFENDED_PAWN_PENALTY = E(-10, -5);
// Pawn phalanxes
const Score PAWN_PHALANX_BONUS = E(2, 1);
const Score PAWN_PHALANX_RANK_BONUS = E(13, 12);
// King-pawn tropism
const int KING_TROPISM_VALUE = 17;

// Scale factors for drawish endgames
const int MAX_SCALE_FACTOR = 32;
const int OPPOSITE_BISHOP_SCALING[2] = {14, 28};
const int PAWNLESS_SCALING[4] = {2, 3, 6, 18};


#undef E

#endif