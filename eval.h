/*
    Laser, a UCI chess engine written in C++11.
    Copyright 2015-2018 Jeffrey An and Michael An

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

#include <cstring>
#include "board.h"
#include "common.h"

class Board;

void initPSQT();
void setMaterialScale(int s);
void setKingSafetyScale(int s);

struct EvalInfo {
    uint64_t attackMaps[2][5];
    uint64_t fullAttackMaps[2];
    uint64_t rammedPawns[2];
    uint64_t openFiles;

    void clear() {
        std::memset(this, 0, sizeof(EvalInfo));
    }
};

class Eval {
public:
  template <bool debug = false> int evaluate(Board &b);

private:
  EvalInfo ei;
  uint64_t pieces[2][6];
  uint64_t allPieces[2];
  int playerToMove;

  // Eval helpers
  template <int color>
  void getMobility(PieceMoveList &pml, PieceMoveList &oppPml, int &valueMg, int &valueEg);
  template <int attackingColor>
  int getKingSafety(Board &b, PieceMoveList &attackers, uint64_t kingSqs, int pawnScore, int kingFile);
  int checkEndgameCases();
  int scoreSimpleKnownWin(int winningColor);
  int scoreCornerDistance(int winningColor, int wKingSq, int bKingSq);
  int getManhattanDistance(int sq1, int sq2);
  int getKingDistance(int sq1, int sq2);
};


const int SEE_PIECE_VALS[6] = {100, 400, 400, 600, 1150, MATE_SCORE/2};
const int EG_FACTOR_PIECE_VALS[5] = {50, 381, 385, 674, 1559};
const int EG_FACTOR_ALPHA = 2310;
const int EG_FACTOR_BETA = 6380;
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
  {100, 397, 441, 668, 1356},
  {137, 397, 451, 726, 1403}
};
const int KNOWN_WIN = PIECE_VALUES[EG][PAWNS] * 75;
const int TB_WIN = PIECE_VALUES[EG][PAWNS] * 125;

//------------------------------Piece tables--------------------------------
const int pieceSquareTable[2][6][32] = {
// Midgame
{
{ // Pawns
  0,  0,  0,  0,
  1,  9, 16, 25,
  7,  9, 16, 23,
  0,  1,  6, 16,
-10,-10,  5, 11,
-10, -5,  0,  4,
 -7, -1, -3, -2,
  0,  0,  0,  0
},
{ // Knights
-110,-38,-30,-24,
-24,-13,  0,  6,
-12,  1, 16, 25,
  5,  7, 20, 26,
  0,  7, 16, 22,
-15,  2,  5, 12,
-19,-10, -6,  7,
-57,-18,-13, -8
},
{ // Bishops
-20,-15,-10,-10,
-15, -8, -4, -2,
  3,  4,  3,  2,
  2, 10,  5,  5,
  3,  4,  4,  9,
  0, 10,  7,  5,
 -2, 12,  7,  5,
-10, -5, -5, -2
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
-29,-21,-12, -8,
-11,-18, -7, -4,
 -3,  0,  0,  2,
 -3, -3,  0,  0,
 -3, -3,  0,  0,
 -7,  4, -1, -2,
-11,  0,  2,  2,
-16,-11, -7,  0
},
{ // Kings
-42,-34,-39,-42,
-34,-28,-32,-36,
-29,-24,-28,-30,
-31,-27,-30,-31,
-28,-13,-28,-28,
 -4, 21,-12,-16,
 35, 42, 10, -3,
 32, 53, 20,  0
}
},
// Endgame
{
{ // Pawns
  0,  0,  0,  0,
 16, 21, 25, 32,
 11, 13, 15, 17,
 -2,  0,  2,  2,
 -7, -3,  0,  0,
 -7, -3,  0,  0,
 -7, -3,  0,  0,
  0,  0,  0,  0
},
{ // Knights
-61,-26,-18,-12,
-12,  0,  4, 10,
 -4,  5, 13, 18,
  4,  9, 18, 25,
  2,  9, 16, 21,
 -8,  3,  7, 19,
-17, -4, -2,  7,
-34,-17,-12, -5
},
{ // Bishops
-12, -7, -5, -5
 -4,  0,  2,  3,
 -2,  2,  5,  4,
  1,  3,  3,  4,
 -3,  2,  2,  2,
 -5, -1,  5,  5,
 -8, -4, -2, -1,
-13,-10, -7, -4
},
{ // Rooks
  0,  0,  0,  0,
  0,  0,  0,  0,
  0,  0,  0,  0,
  0,  0,  0,  0,
  0,  0,  0,  0,
  0,  0,  0,  0,
  0,  0,  0,  0,
  0,  0,  0,  0
},
{ // Queens
-17, -9, -4, -2,
 -6,  6,  8, 11,
  0, 10, 10, 16,
  2, 12, 14, 20,
  1, 10, 14, 19,
 -1,  4,  6,  8,
-14,-11, -8, -8,
-23,-20,-19,-11
},
{ // Kings
-81,-20,-14,-10,
-10, 20, 24, 24,
 10, 32, 34, 36,
 -3, 19, 24, 26,
-14, 10, 16, 18,
-20,  0,  9, 12,
-24, -6,  0,  3,
-57,-26,-20,-18
}
}
};

//-------------------------Material eval constants------------------------------
const int BISHOP_PAIR_VALUE = 58;
const int TEMPO_VALUE = 16;

// Material imbalance terms
const int OWN_OPP_IMBALANCE[2][5][5] = {
{
//       Opponent's
//    P   N   B   R   Q
    { 0},                   // Own pawns
    { 2,  0},               // Own knights
    { 1, -5,  0},           // Own bishops
    {-1, -6,-13,  0},       // Own rooks
    {-1,-10, -5,-18,  0}    // Own queens
},
{
    { 0},                   // Own pawns
    { 5,  0},               // Own knights
    { 1, -2,  0},           // Own bishops
    { 3, -9,-15,  0},       // Own rooks
    {21, -3,  0, 16,  0}    // Own queens
}
};

// Bonus for knight in closed positions
const int KNIGHT_CLOSED_BONUS[2] = {3, 5};

//------------------------Positional eval constants-----------------------------
// SPACE_BONUS[0][0] = behind own pawn, not center files
// SPACE_BONUS[0][1] = behind own pawn, center files
// SPACE_BONUS[1][0] = in front of opp pawn, not center files
// SPACE_BONUS[1][1] = in front of opp pawn, center files
const int SPACE_BONUS[2][2] = {{10, 21}, {2, 7}};

// Mobility tables
const int mobilityScore[2][4][28] = {
// Midgame
{
{ // Knights
-39, -7, 10, 24, 31, 35, 39, 43, 46},
{ // Bishops
-45,-21, -6,  5, 14, 21, 25, 29, 32, 35, 39, 44, 48, 52},
{ // Rooks
-63,-40,-15, -5,  1,  4,  7, 12, 15, 18, 20, 22, 26, 28, 29},
{ // Queens
-54,-39,-27,-18,-11, -6, -2,  1,  4,  7,  9, 12, 15, 17,
 20, 22, 25, 27, 30, 32, 34, 37, 39, 41, 43, 45, 47, 49}
},

// Endgame
{
{ // Knights
-67,-24, -2, 10, 18, 26, 31, 33, 34},
{ // Bishops
-83,-35,-12,  5, 14, 21, 26, 31, 36, 40, 43, 46, 48, 50},
{ // Rooks
-77,-32, -2, 23, 38, 48, 55, 61, 66, 71, 75, 79, 83, 87, 90},
{ // Queens
-86,-55,-38,-25,-15, -7, -1,  4,  8, 12, 15, 18, 20, 23,
 25, 27, 29, 31, 33, 35, 37, 39, 41, 43, 45, 47, 49, 51}
}
};

// Value of each square in the extended center in cp
const int EXTENDED_CENTER_VAL = 3;
// Additional bonus for squares in the center four squares in cp, in addition
// to EXTENDED_CENTER_VAL
const int CENTER_BONUS = 4;

// King safety
// The value of having 0, 1, and both castling rights
const int CASTLING_RIGHTS_VALUE[3] = {0, 24, 62};
// The value of a pawn shield per pawn. First rank value is used for the
// penalty when the pawn is missing.
const int PAWN_SHIELD_VALUE[4][8] = {
    {-12, 22, 26, 11,  7,  5,-12,  0}, // open h file, h2, h3, ...
    {-18, 37, 23, -2, -3, -5,-12,  0}, // g/b file
    {-13, 38,  5, -3, -4, -5, -7,  0}, // f/c file
    { -7, 16, 10,  7, -2, -6, -8,  0}  // d/e file
};
// Array for pawn storm values. Rank 1 of open is used for penalty
// when there is no opposing pawn
const int PAWN_STORM_VALUE[3][4][8] = {
// Open file
{
    {12,-51, 15, 15, 10,  0,  0,  0},
    {13,-12, 42, 16,  7,  0,  0,  0},
    { 7, 17, 50, 20, 12,  0,  0,  0},
    { 7,  6, 40, 18, 10,  0,  0,  0}
},
// Blocked pawn
{
    { 0,  0, 32,  2,  0,  0,  0,  0},
    { 0,  0, 65,  4,  1,  0,  0,  0},
    { 0,  0, 65,  6,  0,  0,  0,  0},
    { 0,  0, 56,  8,  2,  0,  0,  0}
},
// Non-blocked pawn
{
    { 0,  3, 27, 13,  3,  0,  0,  0},
    { 0,  6, 30, 10,  3,  0,  0,  0},
    { 0,  6, 36, 19,  5,  0,  0,  0},
    { 0,  3, 18, 20,  8,  0,  0,  0}
},
};

// Scale factor for pieces attacking opposing king
const int KS_ARRAY_FACTOR = 128;
const int KING_THREAT_MULTIPLIER[4] = {8, 4, 6, 6};
const int KING_THREAT_SQUARE[4] = {7, 12, 10, 13};
const int KING_DEFENSELESS_SQUARE = 22;
const int KS_PAWN_FACTOR = 11;
const int KING_PRESSURE = 3;
const int KS_KING_PRESSURE_FACTOR = 9;
const int SAFE_CHECK_BONUS[4] = {77, 24, 49, 51};

// Minor pieces
// A penalty for each own pawn that is on a square of the same color as your bishop
const Score BISHOP_PAWN_COLOR_PENALTY = E(-4, -3);
const Score BISHOP_RAMMED_PAWN_COLOR_PENALTY = E(-5, -9);
// Minors shielded by own pawn in front
const Score SHIELDED_MINOR_BONUS = E(15, 0);
// A bonus for strong outpost knights
const Score KNIGHT_OUTPOST_BONUS = E(32, 17);
const Score KNIGHT_OUTPOST_PAWN_DEF_BONUS = E(20, 11);
const Score KNIGHT_POTENTIAL_OUTPOST_BONUS = E(10, 10);
const Score KNIGHT_POTENTIAL_OUTPOST_PAWN_DEF_BONUS = E(10, 10);
// A smaller bonus for bishops
const Score BISHOP_OUTPOST_BONUS = E(20, 12);
const Score BISHOP_OUTPOST_PAWN_DEF_BONUS = E(23, 10);
const Score BISHOP_POTENTIAL_OUTPOST_BONUS = E(8, 6);
const Score BISHOP_POTENTIAL_OUTPOST_PAWN_DEF_BONUS = E(12, 9);

// Rooks
const Score ROOK_OPEN_FILE_BONUS = E(37, 16);
const Score ROOK_SEMIOPEN_FILE_BONUS = E(20, 2);
const Score ROOK_PAWN_RANK_THREAT = E(2, 9);

// Threats
const Score UNDEFENDED_PAWN = E(-1, -18);
const Score UNDEFENDED_MINOR = E(-18, -48);
const Score PAWN_PIECE_THREAT = E(-74, -44);
const Score MINOR_ROOK_THREAT = E(-67, -39);
const Score MINOR_QUEEN_THREAT = E(-71, -30);
const Score ROOK_QUEEN_THREAT = E(-73, -29);

const Score LOOSE_PAWN = E(-9, -1);
const Score LOOSE_MINOR = E(-14, -13);

// Pawn structure
// Passed pawns
const Score PASSER_BONUS[8] = {E(  0,   0), E(  3,  11), E(  3, 13), E(  7,  16),
                               E( 23,  23), E( 56,  56), E(108,108), E(  0,   0)};
const Score PASSER_FILE_BONUS[8] = {E( 13, 11), E(  3,  6), E( -8, -2), E(-11, -7),
                                    E(-11, -7), E( -8, -2), E(  3,  6), E( 11, 13)};
const Score FREE_PROMOTION_BONUS = E(14, 19);
const Score FREE_STOP_BONUS = E(6, 8);
const Score FULLY_DEFENDED_PASSER_BONUS = E(9, 11);
const Score DEFENDED_PASSER_BONUS = E(7, 8);
const Score OWN_KING_DIST = E(0, 3);
const Score OPP_KING_DIST = E(0, 7);

// Doubled pawns
const Score DOUBLED_PENALTY = E(-7, -20);
// Isolated pawns
const Score ISOLATED_PENALTY = E(-20, -14);
const Score ISOLATED_SEMIOPEN_PENALTY = E(-6, -6);
// Backward pawns
const Score BACKWARD_PENALTY = E(-14, -10);
const Score BACKWARD_SEMIOPEN_PENALTY = E(-15, -11);
// Undefended pawns that are not backwards or isolated
const Score UNDEFENDED_PAWN_PENALTY = E(-8, -5);
// Pawn phalanxes
const Score PAWN_PHALANX_BONUS[8] = {E(0, 0), E(6, -1), E(4, 2), E(11, 6),
                                     E(28, 20), E(55, 40), E(79, 72), E(0, 0)};
// Connected pawns
const Score PAWN_CONNECTED_BONUS[8] = {E(0, 0), E(0, 0), E(9, 3), E(7, 2),
                                       E(16, 9), E(33, 33), E(55, 55), E(0, 0)};
// King-pawn tropism
const int KING_TROPISM_VALUE = 18;

// Scale factors for drawish endgames
const int MAX_SCALE_FACTOR = 32;
const int OPPOSITE_BISHOP_SCALING[2] = {14, 28};
const int PAWNLESS_SCALING[4] = {3, 4, 7, 24};


#undef E

#endif