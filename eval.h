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
  void getPseudoMobility(PieceMoveList &pml, PieceMoveList &oppPml, int &valueMg, int &valueEg);
  template <int attackingColor>
  int getKingSafety(Board &b, PieceMoveList &attackers, uint64_t kingSqs, int pawnScore);
  int checkEndgameCases();
  int scoreSimpleKnownWin(int winningColor);
  int scoreCornerDistance(int winningColor, int wKingSq, int bKingSq);
  int getManhattanDistance(int sq1, int sq2);
};


const int SEE_PIECE_VALS[6] = {100, 400, 400, 600, 1150, MATE_SCORE/2};
const int EG_FACTOR_PIECE_VALS[5] = {68, 383, 388, 697, 1818};
const int EG_FACTOR_ALPHA = 2730;
const int EG_FACTOR_BETA = 5510;
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
  {100, 388, 434, 646, 1337},
  {140, 391, 443, 705, 1368}
};
const int KNOWN_WIN = PIECE_VALUES[EG][PAWNS] * 75;
const int TB_WIN = PIECE_VALUES[EG][PAWNS] * 125;

//------------------------------Piece tables--------------------------------
const int pieceSquareTable[2][6][32] = {
// Midgame
{
{ // Pawns
  0,  0,  0,  0,
 30, 38, 54, 64,
 19, 34, 52, 55,
  6, 12, 16, 28,
 -3, -2,  8, 10,
 -1,  5,  6,  8,
 -1,  8,  3,  0,
  0,  0,  0,  0
},
{ // Knights
-110,-36,-28,-19,
-29, -9, 12, 17,
-16,  4, 18, 25,
  9, 10, 21, 25,
  0, 10, 16, 19,
-15,  3,  5, 15,
-22, -9, -4,  6,
-68,-25,-14, -7
},
{ // Bishops
-18,-15,-10,-10,
-15, -8, -6,  2,
  3,  4,  3,  2,
  0, 11,  5,  5,
  3,  9,  4, 10,
  0, 12,  8,  5,
 -2, 14,  8,  5,
-15, -5, -5, -2
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
 -3, -3, -3, -6,
 -3, -3, -3, -6,
 -5,  4, -4, -3,
 -8, -2,  5,  5,
-13, -8, -5,  5
},
{ // Kings
-42,-37,-39,-41,
-36,-30,-35,-36,
-29,-24,-30,-30,
-28,-24,-30,-31,
-25,-10,-25,-25,
 -4, 21,-12,-15,
 37, 42, 10,  2,
 29, 53, 20,  0
}
},
// Endgame
{
{ // Pawns
  0,  0,  0,  0,
 28, 38, 52, 61,
 27, 29, 30, 30,
 10,  8,  8,  8,
-12,-10, -5, -5,
-18,-12, -5, -5,
-18,-12, -5, -5,
  0,  0,  0,  0
},
{ // Knights
-57,-15,-12, -8,
  0,  7, 11, 17,
  2, 10, 16, 20,
  4, 14, 18, 25,
  4, 14, 17, 21,
 -6,  3,  7, 20,
-21, -4, -2,  5,
-44,-24,-18,-12
},
{ // Bishops
-11, -5, -5, -5
 -5,  0,  2,  3,
 -2,  2,  5,  4,
  0,  3,  3,  4,
 -3,  2,  2,  2,
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
-14, -8, -4, -2,
 -4,  6,  8, 11,
  0, 10, 10, 16,
  2, 12, 11, 18,
  1, 10, 11, 16,
 -1,  2,  4,  6,
-14,-11, -8, -8,
-23,-20,-19,-11
},
{ // Kings
-97,-20,-14,-10,
-10, 20, 24, 24,
 12, 32, 34, 36,
  0, 19, 24, 26,
-12, 10, 16, 18,
-20,  0,  8, 11,
-24, -6,  0,  3,
-55,-26,-20,-16
}
}
};

//-------------------------Material eval constants------------------------------
const int BISHOP_PAIR_VALUE = 55;
const int TEMPO_VALUE = 16;

// Material imbalance terms
const int KNIGHT_PAIR_PENALTY = 0;
const int ROOK_PAIR_PENALTY = -6;

const int OWN_OPP_IMBALANCE[2][5][5] = {
{
//       Opponent's
//    P   N   B   R   Q
    { 0},                   // Own pawns
    { 3,  0},               // Own knights
    { 1, -5,  0},           // Own bishops
    { 0, -5,-11,  0},       // Own rooks
    { 0, -9, -5,-16,  0}    // Own queens
},
{
    { 0},                   // Own pawns
    { 6,  0},               // Own knights
    { 1, -1,  0},           // Own bishops
    { 3, -5,-11,  0},       // Own rooks
    {19, -3,  0, 16,  0}    // Own queens
}
};

// Bonus for knight in closed positions
const int KNIGHT_CLOSED_BONUS[2] = {4, 4};

//------------------------Positional eval constants-----------------------------
// Mobility tables
const int mobilityScore[2][4][28] = {
// Midgame
{
{ // Knights
-24, -2, 12, 25, 31, 35, 39, 42, 44},
{ // Bishops
-40,-20, -6,  5, 14, 21, 24, 27, 30, 33, 37, 43, 49, 56},
{ // Rooks
-48,-31,-10, -5,  1,  4,  7, 13, 15, 18, 20, 22, 26, 28, 29},
{ // Queens
-36,-24,-18,-13, -7, -4, -1,  1,  4,  7,  9, 12, 15, 17,
 20, 22, 25, 27, 30, 32, 34, 37, 39, 41, 44, 46, 48, 50}
},

// Endgame
{
{ // Knights
-52,-16,  0, 10, 18, 26, 31, 33, 34},
{ // Bishops
-70,-28, -8,  6, 14, 21, 26, 31, 36, 40, 44, 47, 50, 52},
{ // Rooks
-68,-17, 12, 28, 41, 48, 55, 61, 66, 71, 75, 79, 83, 87, 91},
{ // Queens
-75,-43,-25,-15, -9, -4,  2,  6,  9, 12, 15, 18, 20, 23,
 25, 27, 29, 31, 33, 35, 37, 39, 41, 42, 44, 45, 46, 47}
}
};

// Value of each square in the extended center in cp
const int EXTENDED_CENTER_VAL = 3;
// Additional bonus for squares in the center four squares in cp, in addition
// to EXTENDED_CENTER_VAL
const int CENTER_BONUS = 3;

// King safety
// The value of having 0, 1, and both castling rights
const int CASTLING_RIGHTS_VALUE[3] = {0, 25, 61};
// The value of a pawn shield per pawn. First rank value is used for the
// penalty when the pawn is missing.
const int PAWN_SHIELD_VALUE[4][8] = {
    {-11, 20, 24, 11,  8,  5,-11,  0}, // open h file, h2, h3, ...
    {-20, 39, 24, -1, -2, -5,-17,  0}, // g/b file
    {-12, 38,  4, -3, -4, -5, -7,  0}, // f/c file
    { -8, 15,  8,  6, -1, -6, -8,  0}  // d/e file
};
// Array for pawn storm values. Rank 1 of open is used for penalty
// when there is no opposing pawn
const int PAWN_STORM_VALUE[3][4][8] = {
// Open file
{
    {11,-45, 12, 13,  9,  0,  0,  0},
    {14,-15, 38, 16,  7,  0,  0,  0},
    { 6,  8, 44, 16,  9,  0,  0,  0},
    { 6,  8, 34, 18,  9,  0,  0,  0}
},
// Blocked pawn
{
    { 0,  0, 28,  3,  0,  0,  0,  0},
    { 0,  0, 55,  5,  1,  0,  0,  0},
    { 0,  0, 56,  8,  0,  0,  0,  0},
    { 0,  0, 56, 10,  2,  0,  0,  0}
},
// Non-blocked pawn
{
    { 0, -6, 27, 12,  2,  0,  0,  0},
    { 0,  5, 30, 12,  5,  0,  0,  0},
    { 0,  5, 36, 13,  5,  0,  0,  0},
    { 0,  4, 25, 20,  7,  0,  0,  0}
},
};

// Scale factor for pieces attacking opposing king
const int KS_ARRAY_FACTOR = 128;
const int KING_THREAT_MULTIPLIER[4] = {7, 5, 5, 6};
const int KING_THREAT_SQUARE[4] = {9, 12, 9, 14};
const int KING_DEFENSELESS_SQUARE = 22;
const int KS_PAWN_FACTOR = 10;
const int SAFE_CHECK_BONUS[4] = {78, 27, 47, 51};

// Minor pieces
// A penalty for each own pawn that is on a square of the same color as your bishop
const Score BISHOP_PAWN_COLOR_PENALTY = E(-3, -1);
const Score BISHOP_RAMMED_PAWN_COLOR_PENALTY = E(-3, -9);
// Minors shielded by own pawn in front
const Score SHIELDED_MINOR_BONUS = E(14, 0);
// A bonus for strong outpost knights
const Score KNIGHT_OUTPOST_BONUS1 = E(25, 13);
const Score KNIGHT_OUTPOST_BONUS2 = E(15, 10);
const Score KNIGHT_OUTPOST_PAWN_DEF_BONUS = E(14, 9);
// A smaller bonus for bishops
const Score BISHOP_OUTPOST_BONUS1 = E(13, 6);
const Score BISHOP_OUTPOST_BONUS2 = E(9, 4);
const Score BISHOP_OUTPOST_PAWN_DEF_BONUS = E(14, 5);

// Rooks
const Score ROOK_OPEN_FILE_BONUS = E(28, 12);
const Score ROOK_SEMIOPEN_FILE_BONUS = E(13, 1);
const Score ROOK_PAWN_RANK_THREAT = E(3, 7);

// Threats
const Score UNDEFENDED_PAWN = E(-6, -19);
const Score UNDEFENDED_MINOR = E(-22, -45);
const Score PAWN_PIECE_THREAT = E(-67, -42);
const Score MINOR_ROOK_THREAT = E(-50, -30);
const Score MINOR_QUEEN_THREAT = E(-50, -20);
const Score ROOK_QUEEN_THREAT = E(-54, -15);

const Score LOOSE_PAWN = E(-15, -10);
const Score LOOSE_MINOR = E(-11, -9);

// Pawn structure
// Passed pawns
const Score PASSER_BONUS[8] = {E(  0,   0), E(  4,   8), E(  5,  8), E(  7,  14),
                               E( 22,  26), E( 56,  64), E( 97, 97), E(  0,   0)};
const Score PASSER_FILE_BONUS[8] = {E( 14, 14), E(  6, 10), E( -8, -2), E(-12, -8),
                                    E(-12, -8), E( -8, -2), E(  6, 10), E( 14, 14)};
const Score FREE_PROMOTION_BONUS = E(11, 15);
const Score FREE_STOP_BONUS = E(5, 6);
const Score FULLY_DEFENDED_PASSER_BONUS = E(8, 8);
const Score DEFENDED_PASSER_BONUS = E(6, 6);
const Score OWN_KING_DIST = E(0, 2);
const Score OPP_KING_DIST = E(0, 5);

// Doubled pawns
const Score DOUBLED_PENALTY = E(-8, -21);
// Isolated pawns
const Score ISOLATED_PENALTY = E(-22, -13);
const Score ISOLATED_SEMIOPEN_PENALTY = E(-6, -6);
// Backward pawns
const Score BACKWARD_PENALTY = E(-19, -10);
const Score BACKWARD_SEMIOPEN_PENALTY = E(-15, -9);
// Undefended pawns that are not backwards or isolated
const Score UNDEFENDED_PAWN_PENALTY = E(-6, -5);
// Pawn phalanxes
const Score PAWN_PHALANX_RANK_BONUS = E(14, 14);
// Connected pawns
const Score PAWN_CONNECTED_RANK_BONUS = E(5, 3);
// King-pawn tropism
const int KING_TROPISM_VALUE = 18;

// Scale factors for drawish endgames
const int MAX_SCALE_FACTOR = 32;
const int OPPOSITE_BISHOP_SCALING[2] = {15, 30};
const int PAWNLESS_SCALING[4] = {3, 4, 7, 25};


#undef E

#endif