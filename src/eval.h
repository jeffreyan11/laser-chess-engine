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
    int pieceCounts[2][6];
    int playerToMove;

    // Eval helpers
    template <int attackingColor>
    int getKingSafety(Board &b, PieceMoveList &attackers, uint64_t kingSqs, int pawnScore, int kingFile);
    int checkEndgameCases();
    int scoreSimpleKnownWin(int winningColor);
    int scoreCornerDistance(int winningColor, int wKingSq, int bKingSq);
};


constexpr int EG_FACTOR_PIECE_VALS[5] = {46, 372, 377, 682, 1565};
constexpr int EG_FACTOR_ALPHA = 2150;
constexpr int EG_FACTOR_BETA = 6320;
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
    {100, 408, 447, 695, 1351},
    {135, 398, 453, 745, 1447}
};
constexpr int KNOWN_WIN = PIECE_VALUES[EG][PAWNS] * 75;
constexpr int TB_WIN = PIECE_VALUES[EG][PAWNS] * 125;

//------------------------------Piece tables--------------------------------
constexpr int pieceSquareTable[2][6][32] = {
// Midgame
{
{ // Pawns
  0,  0,  0,  0,
 15,  9, 26, 39,
 13, 15, 26, 30,
 -2,  2,  6, 14,
-13, -7,  2,  9,
-11, -4,  0,  2,
 -7,  3, -1,  0,
  0,  0,  0,  0
},
{ // Knights
-122,-44,-37,-32,
-32,-19, -5, 10,
-10,  0, 12, 25,
 12,  7, 26, 30,
  1,  6, 20, 24,
-11,  5,  6, 16,
-17, -8, -2,  7,
-52,-16,-11, -6
},
{ // Bishops
-16,-20,-15,-15,
-20,-12,-10, -8,
  5,  0,  1,  2,
  0, 10,  5, 10,
  0,  4,  6,  9,
  1, 10,  9,  5,
  2, 14, 10,  5,
 -8,  0, -5, -1
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
-25,-21,-15, -5,
-16,-21, -7, -6,
 -8, -3,  0,  2,
 -5, -3, -3, -3,
 -3,  0, -3, -3,
 -6,  5, -1, -2,
-12,  1,  3,  2,
-16,-16,-10,  2
},
{ // Kings
-37,-32,-34,-45,
-34,-28,-32,-38,
-32,-24,-28,-30,
-31,-27,-30,-31,
-35,-20,-32,-32,
 -7, 15,-18,-23,
 34, 50, 10,-14,
 30, 62, 15, -9
}
},
// Endgame
{
{ // Pawns
  0,  0,  0,  0,
 24, 28, 30, 30,
 26, 26, 20, 20,
  8,  6,  2,  2,
 -5,  0, -2, -2,
-12, -3,  0,  0,
-12, -3,  2,  2,
  0,  0,  0,  0
},
{ // Knights
-65,-31,-22,-11,
-18,  0,  4,  8,
  0,  5, 13, 18,
  4, 11, 18, 25,
  0,  9, 16, 24,
 -7,  3,  7, 17,
-10,  0, -3,  6,
-30,-14, -8,  0
},
{ // Bishops
-12,-10, -7, -8
 -7, -4, -4, -2,
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
-10, 20, 28, 28,
  8, 32, 38, 40,
 -8, 19, 28, 30,
-16, 10, 20, 22,
-20, -2, 10, 14,
-26, -7,  4,  6,
-64,-36,-20,-17
}
}
};

//-------------------------Material eval constants------------------------------
constexpr int BISHOP_PAIR_VALUE = 60;
constexpr int TEMPO_VALUE = 18;

// Material imbalance terms
constexpr int OWN_OPP_IMBALANCE[2][5][5] = {
{
//       Opponent's
//    P   N   B   R   Q
    { 0},                   // Own pawns
    { 3,  0},               // Own knights
    { 2, -6,  0},           // Own bishops
    { 0, -5,-16,  0},       // Own rooks
    {-3,-20,-12,-26,  0}    // Own queens
},
{
    { 0},                   // Own pawns
    { 5,  0},               // Own knights
    { 3, -3,  0},           // Own bishops
    { 3,-11,-14,  0},       // Own rooks
    {24,  0,  8, 27,  0}    // Own queens
}
};

// Bonus for knight in closed positions
constexpr int KNIGHT_CLOSED_BONUS[2] = {2, 8};

//------------------------Positional eval constants-----------------------------
// SPACE_BONUS[0][0] = behind own pawn, not center files
// SPACE_BONUS[0][1] = behind own pawn, center files
// SPACE_BONUS[1][0] = in front of opp pawn, not center files
// SPACE_BONUS[1][1] = in front of opp pawn, center files
constexpr int SPACE_BONUS[2][2] = {{14, 35}, {2, 14}};

// Mobility tables
constexpr int mobilityTable[2][5][28] = {
// Midgame
{
{ // Knights
-60,-12, 13, 24, 32, 36, 41, 46, 51},
{ // Bishops
-51,-26, -3,  8, 19, 23, 26, 29, 31, 33, 39, 43, 49, 55},
{ // Rooks
-90,-47,-19, -5,  0,  5,  7, 12, 15, 18, 20, 22, 24, 26, 28},
{ // Queens
-100,-80,-60,-39,-30,-18,-11, -8, -5, -3, -1,  2,  5,  7,
 10, 12, 15, 17, 19, 21, 23, 25, 26, 27, 29, 30, 31, 32},
{ // Kings
-25, 15, 28, 18, 11,  2,  1, -5, -5}
},

// Endgame
{
{ // Knights
-93,-46, -8,  6, 20, 26, 30, 32, 34},
{ // Bishops
-95,-50,-18,  3, 11, 21, 26, 31, 35, 38, 42, 45, 47, 48},
{ // Rooks
-105,-65, -8, 22, 36, 48, 55, 61, 67, 72, 77, 81, 86, 90, 94},
{ // Queens
-108,-82,-66,-44,-26,-17,-11, -2,  4, 10, 15, 18, 20, 23,
 25, 27, 29, 31, 33, 35, 37, 39, 41, 43, 45, 47, 49, 51},
{ // Kings
-55,-19,  1, 18, 21, 11, 20, 16, -2}
}
};

// Value of each square in the extended center in cp
constexpr Score EXTENDED_CENTER_VAL = E(3, 0);
// Additional bonus for squares in the center four squares in cp, in addition
// to EXTENDED_CENTER_VAL
constexpr Score CENTER_BONUS = E(4, 0);

// King safety
// The value of having 0, 1, and both castling rights
constexpr int CASTLING_RIGHTS_VALUE[3] = {0, 24, 66};
// The value of a pawn shield per pawn. First rank value is used for the
// penalty when the pawn is missing.
constexpr int PAWN_SHIELD_VALUE[4][8] = {
    {-15, 23, 27, 12,  6,  7,  7,  0}, // open h file, h2, h3, ...
    {-20, 38, 24, -8, -6, -1,  2,  0}, // g/b file
    {-14, 39,  2, -6, -5, -3,  3,  0}, // f/c file
    { -5, 14, 13,  8, -5,-10, -5,  0}  // d/e file
};
// Array for pawn storm values. Rank 1 of open is used for penalty
// when there is no opposing pawn
constexpr int PAWN_STORM_VALUE[3][4][8] = {
// Open file
{
    {16,-24, 38, 21, 12,  0,  0,  0},
    {14,-23, 58, 16,  7,  0,  0,  0},
    { 6, 18, 54, 32, 18,  0,  0,  0},
    { 9,  0, 33, 19, 14,  0,  0,  0}
},
// Blocked pawn
{
    { 0,  0, 28,  1,  0,  0,  0,  0},
    { 0,  0, 65,  3,  0,  0,  0,  0},
    { 0,  0, 69,  4,  0,  0,  0,  0},
    { 0,  0, 51,  9,  2,  0,  0,  0}
},
// Non-blocked pawn
{
    { 0,  0, 28, 14,  3,  0,  0,  0},
    { 0,-12, 28, 17,  8,  0,  0,  0},
    { 0, -3, 37, 21,  6,  0,  0,  0},
    { 0, -3,  5, 22,  7,  0,  0,  0}
},
};
// Penalty when the enemy king can use a storming pawn as protection
constexpr int PAWN_STORM_SHIELDING_KING = -135;

// Scale factor for pieces attacking opposing king
constexpr int KS_ARRAY_FACTOR = 128;
constexpr int KING_THREAT_MULTIPLIER[4] = {8, 5, 7, 4};
constexpr int KING_THREAT_SQUARE[4] = {8, 10, 7, 9};
constexpr int KING_DEFENSELESS_SQUARE = 24;
constexpr int KS_PAWN_FACTOR = 10;
constexpr int KING_PRESSURE = 3;
constexpr int KS_KING_PRESSURE_FACTOR = 20;
constexpr int KS_NO_KNIGHT_DEFENDER = 12;
constexpr int KS_NO_BISHOP_DEFENDER = 12;
constexpr int KS_BISHOP_PRESSURE = 6;
constexpr int KS_NO_QUEEN = -44;
constexpr int KS_BASE = -16;
constexpr int SAFE_CHECK_BONUS[4] = {56, 25, 65, 53};

// Minor pieces
// A penalty for each own pawn that is on a square of the same color as your bishop
constexpr Score BISHOP_PAWN_COLOR_PENALTY = E(-2, -3);
constexpr Score BISHOP_RAMMED_PAWN_COLOR_PENALTY = E(-9, -9);
// Minors shielded by own pawn in front
constexpr Score SHIELDED_MINOR_BONUS = E(16, 0);
// A bonus for strong outpost knights
constexpr Score KNIGHT_OUTPOST_BONUS = E(31, 23);
constexpr Score KNIGHT_OUTPOST_PAWN_DEF_BONUS = E(26, 8);
constexpr Score KNIGHT_POTENTIAL_OUTPOST_BONUS = E(10, 15);
constexpr Score KNIGHT_POTENTIAL_OUTPOST_PAWN_DEF_BONUS = E(13, 12);
// A smaller bonus for bishops
constexpr Score BISHOP_OUTPOST_BONUS = E(27, 18);
constexpr Score BISHOP_OUTPOST_PAWN_DEF_BONUS = E(26, 14);
constexpr Score BISHOP_POTENTIAL_OUTPOST_BONUS = E(10, 13);
constexpr Score BISHOP_POTENTIAL_OUTPOST_PAWN_DEF_BONUS = E(15, 5);

// Rooks
constexpr Score ROOK_OPEN_FILE_BONUS = E(40, 12);
constexpr Score ROOK_SEMIOPEN_FILE_BONUS = E(21, 1);
constexpr Score ROOK_PAWN_RANK_THREAT = E(1, 11);

// Threats
constexpr Score UNDEFENDED_PAWN = E(-3, -15);
constexpr Score UNDEFENDED_MINOR = E(-27, -46);
constexpr Score PAWN_PIECE_THREAT = E(-85, -33);
constexpr Score MINOR_ROOK_THREAT = E(-81, -30);
constexpr Score MINOR_QUEEN_THREAT = E(-81, -43);
constexpr Score ROOK_QUEEN_THREAT = E(-88, -44);

constexpr Score LOOSE_PAWN = E(-13, -2);
constexpr Score LOOSE_MINOR = E(-16, -8);

// Pawn structure
// Passed pawns
constexpr Score PASSER_BONUS[8] = {E(  0,   0), E(  1,   6), E(  1, 7), E( 10,  17),
                                   E( 28,  23), E( 58,  54), E(110,121), E(  0,   0)};
constexpr Score PASSER_FILE_BONUS[8] = {E( 15, 17), E(  8, 11), E( -8,  1), E(-12, -7),
                                        E(-12, -7), E( -8,  1), E(  8, 11), E( 15, 17)};
constexpr Score FREE_PROMOTION_BONUS = E(8, 24);
constexpr Score FREE_STOP_BONUS = E(6, 11);
constexpr Score FULLY_DEFENDED_PASSER_BONUS = E(10, 14);
constexpr Score DEFENDED_PASSER_BONUS = E(9, 9);
constexpr Score OWN_KING_DIST = E(0, 3);
constexpr Score OPP_KING_DIST = E(0, 7);

// Doubled pawns
constexpr Score DOUBLED_PENALTY = E(-3, -18);
// Isolated pawns
constexpr Score ISOLATED_PENALTY = E(-18, -11);
constexpr Score ISOLATED_SEMIOPEN_PENALTY = E(-3, -9);
// Backward pawns
constexpr Score BACKWARD_PENALTY = E(-9, -8);
constexpr Score BACKWARD_SEMIOPEN_PENALTY = E(-17, -11);
// Undefended pawns that are not backwards or isolated
constexpr Score UNDEFENDED_PAWN_PENALTY = E(-6, -3);
// Pawn phalanxes
constexpr Score PAWN_PHALANX_BONUS[8] = {E( 0,  0), E( 7,  0), E( 3,  2), E(12,  7),
                                         E(26, 20), E(55, 43), E(70, 74), E( 0,  0)};
// Connected pawns
constexpr Score PAWN_CONNECTED_BONUS[8] = {E( 0,  0), E( 0,  0), E(12,  5), E(10,  4),
                                           E(14, 11), E(30, 32), E(67, 64), E( 0,  0)};
// King-pawn tropism
constexpr int KING_TROPISM_VALUE = 18;

// Endgame win probability adjustment
constexpr int PAWN_ASYMMETRY_BONUS = 3;
constexpr int PAWN_COUNT_BONUS = 5;
constexpr int KING_OPPOSITION_DISTANCE_BONUS = 2;
constexpr int ENDGAME_BASE = -38;

// Scale factors for drawish endgames
constexpr int MAX_SCALE_FACTOR = 32;
constexpr int OPPOSITE_BISHOP_SCALING[2] = {14, 28};
constexpr int PAWNLESS_SCALING[4] = {2, 5, 9, 24};


#undef E

#endif
