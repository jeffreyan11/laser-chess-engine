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

const int SEE_PIECE_VALS[6] = {100, 400, 400, 600, 1150, MATE_SCORE};
const int KNOWN_WIN = PAWN_VALUE_EG * 100;

// Eval scores are packed into an unsigned 32-bit integer during calculations
// (the SWAR technique)
typedef uint32_t Score;

// Encodes 16-bit midgame and endgame evaluation scores into a single int
#define encEval(mg, eg) ((Score) ((eg << 16) | mg))

// Retrieves the final evaluation score to return from the packed eval value
int decEval(Score encodedValue, int egFactor) {
    int valueMg = (int) (encodedValue & 0xFFFF) - 0x8000;
    int valueEg = (int) (encodedValue >> 16) - 0x8000;
    return (valueMg * (EG_FACTOR_RES - egFactor) + valueEg * egFactor) / EG_FACTOR_RES;
}

// Since we can only work with unsigned numbers due to carryover / twos-complement
// negative number issues, we make 2^15 the 0 point for each of the two 16-bit
// halves of Score
const Score EVAL_ZERO = 0x80008000;

//-------------------------Material eval constants------------------------------
const int BISHOP_PAIR_VALUE = 55;
const int TEMPO_VALUE = 10;

//------------------------Positional eval constants-----------------------------
// King safety
// The value of having 0, 1, and both castling rights
const Score CASTLING_RIGHTS_VALUE[3] = {encEval(0, 0), encEval(23, 0), encEval(40, 0)};
// The value of a pawn shield per pawn
const Score PAWN_SHIELD_VALUE = encEval(12, 0);
// The additional bonus for each pawn in the primary pawn shield, the 3 squares
// directly in front of the king
const Score P_PAWN_SHIELD_BONUS = encEval(9, 0);
// The penalty for a semi-open file next to the king where your own pawn is missing
const Score SEMIOPEN_OWN_PENALTY = encEval(7, 0);
// The penalty for a semi-open file next to the king where your opponent's pawn is missing
const Score SEMIOPEN_OPP_PENALTY = encEval(4, 0);
// An additional penalty for a fully open file next to the king
const Score OPEN_PENALTY = encEval(3, 0);

// Minor pieces
// A penalty for each own pawn that is on a square of the same color as your bishop
const Score BISHOP_PAWN_COLOR_PENALTY = encEval(2, 2);
// A bonus for each opponent pawn on the board, given once for each knight
const Score KNIGHT_PAWN_BONUS = encEval(1, 1);

// Rooks
const Score ROOK_OPEN_FILE_BONUS = encEval(8, 8);

// Pawn structure
// Passed pawns
const Score PASSER_BONUS[8] = {encEval(0, 0), encEval(10, 30), encEval(10, 35), encEval(15, 45),
                               encEval(20, 65), encEval(30, 95), encEval(55, 140), encEval(0, 0)};
const Score BLOCKADED_PASSER_PENALTY = encEval(8, 20);

// Doubled pawns
const Score DOUBLED_PENALTY[7] = {encEval(0, 0), encEval(0, 0), encEval(12, 17), encEval(36, 51),
                                  encEval(72, 102), encEval(120, 170), encEval(180, 255)};
// Doubled pawns are worse the less pawns you have
const Score DOUBLED_PENALTY_SCALE[9] = {0, 0, 3, 2, 1, 1, 1, 1, 1};
// Isolated pawns
const Score ISOLATED_PENALTY = encEval(14, 16);
// Isolated, doubled pawns
const Score ISOLATED_DOUBLED_PENALTY = encEval(11, 11);
// Backward pawns
const Score BACKWARD_PENALTY = encEval(11, 12);

#endif