/*
    Laser, a UCI chess engine written in C++11.
    Copyright 2015 Jeffrey An and Michael An

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

typedef uint32_t Score;

// Encodes 16-bit midgame and endgame evaluation scores into a single int
#define encEval(mg, eg) ((Score) ((eg << 16) | mg))

int decEval(Score encodedValue, int egFactor) {
    int valueMg = (int) (encodedValue & 0xFFFF) - 0x8000;
    int valueEg = (int) (encodedValue >> 16) - 0x8000;
    return (valueMg * (EG_FACTOR_RES - egFactor) + valueEg * egFactor) / EG_FACTOR_RES;
}

const Score EVAL_ZERO = 0x80008000;

//-------------------------Material eval constants------------------------------
const int BISHOP_PAIR_VALUE = 55;
const int TEMPO_VALUE = 10;

//------------------------Positional eval constants-----------------------------
// King safety
const Score CASTLING_RIGHTS_VALUE[3] = {0, 23, 40};
const Score PAWN_SHIELD_VALUE = 12;
const Score P_PAWN_SHIELD_BONUS = 9;
const Score SEMIOPEN_OWN_PENALTY = 7;
const Score SEMIOPEN_OPP_PENALTY = 4;
const Score OPEN_PENALTY = 3;

// Minor pieces
const Score BISHOP_PAWN_COLOR_PENALTY = 2;
const Score KNIGHT_PAWN_BONUS = 1;

// Pawn structure
// Passed pawns
const Score PASSER_BONUS_MG[8] = {0, 10, 10, 15, 20, 30, 55, 0};
const Score PASSER_BONUS_EG[8] = {0, 30, 35, 45, 65, 95, 140, 0};
// Doubled pawns
const Score DOUBLED_PENALTY_MG[7] = {0, 0, 12, 36, 72, 120, 180};
const Score DOUBLED_PENALTY_EG[7] = {0, 0, 17, 51, 102, 170, 255};
// Doubled pawns are worse the less pawns you have
const Score DOUBLED_PENALTY_SCALE[9] = {0, 0, 3, 2, 1, 1, 1, 1, 1};
// Isolated pawns
const Score ISOLATED_PENALTY_MG = 14;
const Score ISOLATED_PENALTY_EG = 16;
// Isolated, doubled pawns
const Score ISOLATED_DOUBLED_PENALTY = 11;

#endif