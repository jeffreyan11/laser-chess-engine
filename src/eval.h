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

void initEvalTables();
void initDistances();
void setMaterialScale(int s);
void setKingSafetyScale(int s);

struct EvalInfo {
    uint64_t attackMaps[2][5];
    uint64_t fullAttackMaps[2];
    uint64_t doubleAttackMaps[2];
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
    template <int attackingColor>
    int getKingSafety(Board &b, PieceMoveList &attackers, uint64_t kingSqs, int pawnScore, int kingFile);
    int checkEndgameCases();
    int scoreSimpleKnownWin(int winningColor);
    int scoreCornerDistance(int winningColor, int wKingSq, int bKingSq);
};


constexpr int EG_FACTOR_PIECE_VALS[5] = {49, 378, 380, 677, 1560};
constexpr int EG_FACTOR_ALPHA = 2230;
constexpr int EG_FACTOR_BETA = 6340;
constexpr int EG_FACTOR_RES = 1000;

// Eval scores are packed into an unsigned 32-bit integer during calculations
// (the SWAR technique)
typedef uint32_t Score;

// Encodes 16-bit midgame and endgame evaluation scores into a single int
#define E(mg, eg) ((Score) ((int32_t) (((uint32_t) eg) << 16) + ((int32_t) mg)))

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
constexpr Score EVAL_ZERO = 0x80008000;

// Array indexing constants
constexpr int MG = 0;
constexpr int EG = 1;

// Material constants
constexpr int PIECE_VALUES[2][5] = {
    {100, 397, 438, 683, 1345},
    {139, 394, 450, 742, 1429}
};
constexpr int KNOWN_WIN = PIECE_VALUES[EG][PAWNS] * 75;
constexpr int TB_WIN = PIECE_VALUES[EG][PAWNS] * 125;

//------------------------------Piece tables--------------------------------
constexpr int pieceSquareTable[2][6][32] = {
// Midgame
{
{ // Pawns
  0,  0,  0,  0,
 12,  9, 18, 30,
 10, 12, 20, 24,
 -2,  2,  6, 14,
-13, -8,  2,  8,
-11, -4,  0,  2,
 -8,  2, -1,  0,
  0,  0,  0,  0
},
{ // Knights
-116,-41,-34,-30,
-32,-22, -7,  8,
-10,  2, 14, 25,
  8,  7, 26, 30,
  1,  6, 20, 24,
-11,  5,  6, 16,
-17, -8, -2,  7,
-50,-16,-11, -4
},
{ // Bishops
-16,-20,-15,-15,
-20,-12,-10, -8,
  5,  4,  3,  2,
  2, 10,  5, 10,
  3,  4,  6,  9,
  0, 10,  9,  5,
  0, 12,  9,  5,
 -8, -3, -5,  0
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
-29,-21,-15,-10,
-16,-21, -7, -6,
 -8, -3,  0,  2,
 -5, -3, -3, -3,
 -3, -3, -3, -3,
 -6,  5, -1, -2,
-14,  1,  3,  2,
-19,-16,-10,  2
},
{ // Kings
-37,-32,-34,-45,
-34,-28,-32,-38,
-32,-24,-28,-30,
-31,-27,-30,-31,
-32,-20,-32,-32,
 -6, 18,-14,-16,
 34, 44, 10,-12,
 30, 58, 15, -4
}
},
// Endgame
{
{ // Pawns
  0,  0,  0,  0,
 16, 25, 30, 32,
 22, 22, 20, 20,
  6,  4,  2,  2,
 -6,  0, -2, -2,
-12, -3,  0,  0,
-12, -3,  2,  2,
  0,  0,  0,  0
},
{ // Knights
-70,-35,-25,-17,
-20,  0,  4,  8,
 -4,  5, 13, 18,
  4,  9, 18, 25,
 -2,  9, 16, 24,
 -8,  3,  7, 17,
-12,  0, -3,  6,
-32,-14, -8,  0
},
{ // Bishops
-12,-10, -7, -8
 -7, -1, -4, -2,
 -2,  2,  3,  0,
  2,  2,  0,  4,
 -3,  0,  2,  2,
 -5, -1,  5,  2,
 -8, -4, -2, -2,
-13,-12, -4,  0
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
-18, -9, -1, -1,
 -9,  5, 10, 16,
 -2, 13, 18, 22,
  0, 16, 20, 26,
  0, 16, 20, 24,
 -4,  4,  8, 10,
-19,-14,-12, -8,
-26,-23,-23,-18
},
{ // Kings
-68,-18,-14, -7,
 -6, 20, 28, 28,
  8, 32, 38, 40,
 -5, 19, 28, 30,
-14, 10, 20, 22,
-20, -2, 10, 14,
-26, -7,  4,  6,
-64,-36,-20,-17
}
}
};

//-------------------------Material eval constants------------------------------
constexpr int BISHOP_PAIR_VALUE = 58;
constexpr int TEMPO_VALUE = 18;

// Material imbalance terms
constexpr int OWN_OPP_IMBALANCE[2][5][5] = {
{
//       Opponent's
//    P   N   B   R   Q
    { 0},                   // Own pawns
    { 3,  0},               // Own knights
    { 2, -6,  0},           // Own bishops
    { 0, -7,-16,  0},       // Own rooks
    {-3,-12,-10,-20,  0}    // Own queens
},
{
    { 0},                   // Own pawns
    { 5,  0},               // Own knights
    { 3, -3,  0},           // Own bishops
    { 3,-11,-14,  0},       // Own rooks
    {24, -3,  3, 24,  0}    // Own queens
}
};

// Bonus for knight in closed positions
constexpr int KNIGHT_CLOSED_BONUS[2] = {4, 8};

//------------------------Positional eval constants-----------------------------
// SPACE_BONUS[0][0] = behind own pawn, not center files
// SPACE_BONUS[0][1] = behind own pawn, center files
// SPACE_BONUS[1][0] = in front of opp pawn, not center files
// SPACE_BONUS[1][1] = in front of opp pawn, center files
constexpr int SPACE_BONUS[2][2] = {{11, 30}, {0, 11}};

// Mobility tables
constexpr int mobilityTable[2][5][28] = {
// Midgame
{
{ // Knights
-59, -7, 11, 24, 32, 36, 41, 46, 51},
{ // Bishops
-47,-26, -3,  8, 19, 23, 26, 29, 31, 33, 39, 43, 47, 50},
{ // Rooks
-90,-47,-19, -5,  0,  5,  7, 12, 15, 18, 20, 22, 24, 26, 28},
{ // Queens
-100,-80,-60,-39,-30,-18,-11, -8, -5, -3, -1,  2,  5,  7,
 10, 12, 15, 17, 19, 21, 23, 25, 26, 27, 29, 30, 31, 32},
{ // Kings
-29,  9, 23, 16, 12,  6,  8, -1, -2}
},

// Endgame
{
{ // Knights
-93,-46, -5,  9, 20, 26, 30, 32, 34},
{ // Bishops
-99,-52,-18,  3, 10, 21, 26, 31, 35, 38, 42, 45, 47, 48},
{ // Rooks
-105,-65, -8, 22, 36, 48, 55, 61, 67, 72, 77, 81, 86, 90, 94},
{ // Queens
-108,-82,-66,-44,-26,-17,-11, -2,  4, 10, 15, 18, 20, 23,
 25, 27, 29, 31, 33, 35, 37, 39, 41, 43, 45, 47, 49, 51},
{ // Kings
-49,-22, -6, 18, 21, 11, 24, 19, -4}
}
};

// Value of each square in the extended center in cp
constexpr Score EXTENDED_CENTER_VAL = E(2, 0);
// Additional bonus for squares in the center four squares in cp, in addition
// to EXTENDED_CENTER_VAL
constexpr Score CENTER_BONUS = E(5, 0);

// King safety
// The value of having 0, 1, and both castling rights
constexpr int CASTLING_RIGHTS_VALUE[3] = {0, 24, 64};
// The value of a pawn shield per pawn. First rank value is used for the
// penalty when the pawn is missing.
constexpr int PAWN_SHIELD_VALUE[4][8] = {
    {-12, 23, 27, 12,  5,  5, -5,  0}, // open h file, h2, h3, ...
    {-18, 37, 23, -8, -4,  0,  2,  0}, // g/b file
    {-13, 38,  3, -5, -5, -3,  3,  0}, // f/c file
    { -4, 13, 10,  8, -4,-10, -5,  0}  // d/e file
};
// Array for pawn storm values. Rank 1 of open is used for penalty
// when there is no opposing pawn
constexpr int PAWN_STORM_VALUE[3][4][8] = {
// Open file
{
    {12,-15, 38, 16, 11,  0,  0,  0},
    {13,-17, 46, 15,  7,  0,  0,  0},
    { 6, 15, 54, 26, 14,  0,  0,  0},
    { 8,  7, 36, 14,  8,  0,  0,  0}
},
// Blocked pawn
{
    { 0,  0, 28,  1,  0,  0,  0,  0},
    { 0,  0, 65,  3,  0,  0,  0,  0},
    { 0,  0, 64,  4,  0,  0,  0,  0},
    { 0,  0, 54,  6,  1,  0,  0,  0}
},
// Non-blocked pawn
{
    { 0,  9, 34, 10,  3,  0,  0,  0},
    { 0, -6, 31, 10,  5,  0,  0,  0},
    { 0,  0, 39, 16,  6,  0,  0,  0},
    { 0, -6,  0, 19,  4,  0,  0,  0}
},
};
// Penalty when the enemy king can use a storming pawn as protection
constexpr int PAWN_STORM_SHIELDING_KING = -126;

// Scale factor for pieces attacking opposing king
constexpr int KS_ARRAY_FACTOR = 128;
constexpr int KING_THREAT_MULTIPLIER[4] = {8, 5, 7, 4};
constexpr int KING_THREAT_SQUARE[4] = {8, 11, 9, 10};
constexpr int KING_DEFENSELESS_SQUARE = 22;
constexpr int KS_PAWN_FACTOR = 11;
constexpr int KING_PRESSURE = 3;
constexpr int KS_KING_PRESSURE_FACTOR = 12;
constexpr int KS_NO_QUEEN = -57;
constexpr int KS_BASE = -3;
constexpr int SAFE_CHECK_BONUS[4] = {62, 22, 60, 53};

// Minor pieces
// A penalty for each own pawn that is on a square of the same color as your bishop
constexpr Score BISHOP_PAWN_COLOR_PENALTY = E(-3, -4);
constexpr Score BISHOP_RAMMED_PAWN_COLOR_PENALTY = E(-6, -8);
// Minors shielded by own pawn in front
constexpr Score SHIELDED_MINOR_BONUS = E(14, 0);
// A bonus for strong outpost knights
constexpr Score KNIGHT_OUTPOST_BONUS = E(29, 22);
constexpr Score KNIGHT_OUTPOST_PAWN_DEF_BONUS = E(24, 8);
constexpr Score KNIGHT_POTENTIAL_OUTPOST_BONUS = E(10, 13);
constexpr Score KNIGHT_POTENTIAL_OUTPOST_PAWN_DEF_BONUS = E(14, 17);
// A smaller bonus for bishops
constexpr Score BISHOP_OUTPOST_BONUS = E(25, 17);
constexpr Score BISHOP_OUTPOST_PAWN_DEF_BONUS = E(30, 13);
constexpr Score BISHOP_POTENTIAL_OUTPOST_BONUS = E(10, 12);
constexpr Score BISHOP_POTENTIAL_OUTPOST_PAWN_DEF_BONUS = E(15, 5);

// Rooks
constexpr Score ROOK_OPEN_FILE_BONUS = E(41, 13);
constexpr Score ROOK_SEMIOPEN_FILE_BONUS = E(24, 1);
constexpr Score ROOK_PAWN_RANK_THREAT = E(1, 12);

// Threats
constexpr Score UNDEFENDED_PAWN = E(-3, -16);
constexpr Score UNDEFENDED_MINOR = E(-24, -50);
constexpr Score PAWN_PIECE_THREAT = E(-78, -38);
constexpr Score MINOR_ROOK_THREAT = E(-73, -32);
constexpr Score MINOR_QUEEN_THREAT = E(-73, -43);
constexpr Score ROOK_QUEEN_THREAT = E(-85, -43);

constexpr Score LOOSE_PAWN = E(-11, 0);
constexpr Score LOOSE_MINOR = E(-16, -9);

// Pawn structure
// Passed pawns
constexpr Score PASSER_BONUS[8] = {E(  0,   0), E(  1,   9), E(  2, 11), E(  6,  18),
                                   E( 26,  23), E( 51,  54), E(108,114), E(  0,   0)};
constexpr Score PASSER_FILE_BONUS[8] = {E( 12, 15), E(  5,  8), E( -7, -1), E(-12, -8),
                                        E(-12, -8), E( -7, -1), E(  5,  8), E( 12, 15)};
constexpr Score FREE_PROMOTION_BONUS = E(9, 23);
constexpr Score FREE_STOP_BONUS = E(7, 10);
constexpr Score FULLY_DEFENDED_PASSER_BONUS = E(8, 14);
constexpr Score DEFENDED_PASSER_BONUS = E(8, 9);
constexpr Score OWN_KING_DIST = E(0, 3);
constexpr Score OPP_KING_DIST = E(0, 7);

// Doubled pawns
constexpr Score DOUBLED_PENALTY = E(-3, -18);
// Isolated pawns
constexpr Score ISOLATED_PENALTY = E(-18, -11);
constexpr Score ISOLATED_SEMIOPEN_PENALTY = E(-3, -9);
// Backward pawns
constexpr Score BACKWARD_PENALTY = E(-12, -9);
constexpr Score BACKWARD_SEMIOPEN_PENALTY = E(-13, -10);
// Undefended pawns that are not backwards or isolated
constexpr Score UNDEFENDED_PAWN_PENALTY = E(-7, -2);
// Pawn phalanxes
constexpr Score PAWN_PHALANX_BONUS[8] = {E( 0,  0), E( 7, -1), E( 5,  2), E(11,  7),
                                         E(25, 20), E(54, 43), E(80, 84), E( 0,  0)};
// Connected pawns
constexpr Score PAWN_CONNECTED_BONUS[8] = {E( 0,  0), E( 0,  0), E(12,  5), E( 9,  4),
                                           E(13, 12), E(32, 33), E(68, 66), E( 0,  0)};
// King-pawn tropism
constexpr int KING_TROPISM_VALUE = 18;

// Endgame win probability adjustment
constexpr int PAWN_ASYMMETRY_BONUS = 2;
constexpr int PAWN_COUNT_BONUS = 3;
constexpr int KING_OPPOSITION_DISTANCE_BONUS = 2;
constexpr int ENDGAME_BASE = -30;

// Scale factors for drawish endgames
constexpr int MAX_SCALE_FACTOR = 32;
constexpr int OPPOSITE_BISHOP_SCALING[2] = {14, 28};
constexpr int PAWNLESS_SCALING[4] = {3, 5, 9, 24};


#undef E

#endif
