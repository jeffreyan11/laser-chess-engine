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

#include "attacks.h"
#include "bbinit.h"

// Dumb7fill methods, only used to initialize magic bitboard tables
uint64_t fillRayRight(uint64_t rayPieces, uint64_t empty, int shift);
uint64_t fillRayLeft(uint64_t rayPieces, uint64_t empty, int shift);

// Lookup table for all squares in a line between the from and to squares
uint64_t inBetweenSqs[64][64];

// Initializes the 64x64 table, indexed by from and to square, of all
// squares in a line between from and to
void initInBetweenTable() {
    for (int sq1 = 0; sq1 < 64; sq1++) {
        for (int sq2 = 0; sq2 < 64; sq2++) {
            // Check horizontal/vertical lines
            uint64_t imaginaryRook = rookAttacks(sq1, indexToBit(sq2));
            // If the two squares are on a line
            if (imaginaryRook & indexToBit(sq2)) {
                uint64_t imaginaryRook2 = rookAttacks(sq2, indexToBit(sq1));
                // Set the bitboard of squares in between
                inBetweenSqs[sq1][sq2] = imaginaryRook & imaginaryRook2;
            }
            else {
                // Check diagonal lines
                uint64_t imaginaryBishop = bishopAttacks(sq1, indexToBit(sq2));
                if (imaginaryBishop & indexToBit(sq2)) {
                    uint64_t imaginaryBishop2 = bishopAttacks(sq2, indexToBit(sq1));
                    inBetweenSqs[sq1][sq2] = imaginaryBishop & imaginaryBishop2;
                }
                // If the squares are not on a line, the bitboard is empty
                else
                    inBetweenSqs[sq1][sq2] = 0;
            }
        }
    }
}


// Dumb7Fill
uint64_t fillRayRight(uint64_t rayPieces, uint64_t empty, int shift) {
    uint64_t flood = rayPieces;
    // To prevent overflow across the sides of the board on east/west fills
    uint64_t borderMask = 0xFFFFFFFFFFFFFFFF;
    if (shift == 1 || shift == 9)
        borderMask = NOTH;
    else if (shift == 7)
        borderMask = NOTA;
    empty &= borderMask;
    flood |= rayPieces = (rayPieces >> shift) & empty;
    flood |= rayPieces = (rayPieces >> shift) & empty;
    flood |= rayPieces = (rayPieces >> shift) & empty;
    flood |= rayPieces = (rayPieces >> shift) & empty;
    flood |= rayPieces = (rayPieces >> shift) & empty;
    flood |=         (rayPieces >> shift) & empty;
    return           (flood >> shift) & borderMask;
}

uint64_t fillRayLeft(uint64_t rayPieces, uint64_t empty, int shift) {
    uint64_t flood = rayPieces;
    // To prevent overflow across the sides of the board on east/west fills
    uint64_t borderMask = 0xFFFFFFFFFFFFFFFF;
    if (shift == 1 || shift == 9)
        borderMask = NOTA;
    else if (shift == 7)
        borderMask = NOTH;
    empty &= borderMask;
    flood |= rayPieces = (rayPieces << shift) & empty;
    flood |= rayPieces = (rayPieces << shift) & empty;
    flood |= rayPieces = (rayPieces << shift) & empty;
    flood |= rayPieces = (rayPieces << shift) & empty;
    flood |= rayPieces = (rayPieces << shift) & empty;
    flood |=         (rayPieces << shift) & empty;
    return           (flood << shift) & borderMask;
}