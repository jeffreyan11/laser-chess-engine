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

// Encodes 16-bit midgame and endgame evaluation scores into a single int
#define encEval(mg, eg) ((eg << 16) | mg)

inline int decEval(int encodedValue, int egFactor) {
    return ((encodedValue & 0xFFFF) * (EG_FACTOR_RES - egFactor)
             + (encodedValue >> 16) * egFactor) / EG_FACTOR_RES;
}

//-------------------------Material eval constants------------------------------
const int BISHOP_PAIR_VALUE = 55;
const int TEMPO_VALUE = 10;

//------------------------Positional eval constants-----------------------------
// King safety
const int CASTLING_RIGHTS_VALUE[3] = {0, 23, 40};
const int PAWN_SHIELD_VALUE = 12;
const int P_PAWN_SHIELD_BONUS = 9;
const int SEMIOPEN_OWN_PENALTY = 7;
const int SEMIOPEN_OPP_PENALTY = 4;
const int OPEN_PENALTY = 3;

// Minor pieces
const int BISHOP_PAWN_COLOR_PENALTY = 2;
const int KNIGHT_PAWN_BONUS = 1;

// Pawn structure
// Passed pawns
const int PASSER_BONUS_MG[8] = {0, 10, 10, 15, 20, 30, 55, 0};
const int PASSER_BONUS_EG[8] = {0, 30, 35, 45, 65, 95, 140, 0};
// Doubled pawns
const int DOUBLED_PENALTY_MG[7] = {0, 0, 12, 36, 72, 120, 180};
const int DOUBLED_PENALTY_EG[7] = {0, 0, 17, 51, 102, 170, 255};
// Doubled pawns are worse the less pawns you have
const int DOUBLED_PENALTY_SCALE[9] = {0, 0, 3, 2, 1, 1, 1, 1, 1};
// Isolated pawns
const int ISOLATED_PENALTY_MG = 14;
const int ISOLATED_PENALTY_EG = 16;
// Isolated, doubled pawns
const int ISOLATED_DOUBLED_PENALTY = 11;

#endif