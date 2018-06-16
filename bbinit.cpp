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

#include "bbinit.h"


/**
 * @brief Our implementation of a xorshift generator as discovered by George
 * Marsaglia.
 * This specific implementation is not fully pseudorandom, but attempts to
 * create good magic number candidates by artifically increasing the number
 * of high bits.
 */
static uint64_t mseed = 0, mstate = 0;
uint64_t magicRNG() {
    // Use "y" to achieve a larger number of high bits
    uint64_t y = ((mstate << 57) | (mseed << 57)) >> 1;
    mstate ^= mseed >> 17;
    mstate ^= mstate << 3;

    uint64_t temp = mseed;
    mseed = mstate;
    mstate = temp;

    // But not too high, or they will overflow out once multiplied by the mask
    return (y | (mseed ^ mstate)) >> 1;
}

// Shift amounts for Dumb7fill
constexpr int NORTH_SOUTH_FILL = 8;
constexpr int EAST_WEST_FILL = 1;
constexpr int NE_SW_FILL = 9;
constexpr int NW_SE_FILL = 7;

// Dumb7fill methods, only used to initialize magic bitboard tables
uint64_t fillRayRight(uint64_t rayPieces, uint64_t empty, int shift);
uint64_t fillRayLeft(uint64_t rayPieces, uint64_t empty, int shift);

// Masks the relevant rook or bishop occupancy bits for magic bitboards
static uint64_t ROOK_MASK[64];
static uint64_t BISHOP_MASK[64];
// The full attack table containing all attack sets of bishops and rooks
uint64_t *attackTable;
// The magic values for bishops, one for each square
MagicInfo magicBishops[64];
// The magic values for rooks, one for each square
MagicInfo magicRooks[64];

// Lookup table for all squares in a line between the from and to squares
uint64_t inBetweenSqs[64][64];

uint64_t indexToMask64(int index, int nBits, uint64_t mask);
uint64_t ratt(int sq, uint64_t block);
uint64_t batt(int sq, uint64_t block);
int magicMap(uint64_t masked, uint64_t magic, int nBits);
uint64_t findMagic(int sq, int m, bool isBishop);


// Initializes the 64x64 table, indexed by from and to square, of all
// squares in a line between from and to
void initInBetweenTable() {
    for (int sq1 = 0; sq1 < 64; sq1++) {
        for (int sq2 = 0; sq2 < 64; sq2++) {
            // Check horizontal/vertical lines
            uint64_t imaginaryRook = ratt(sq1, indexToBit(sq2));
            // If the two squares are on a line
            if (imaginaryRook & indexToBit(sq2)) {
                uint64_t imaginaryRook2 = ratt(sq2, indexToBit(sq1));
                // Set the bitboard of squares in between
                inBetweenSqs[sq1][sq2] = imaginaryRook & imaginaryRook2;
            }
            else {
                // Check diagonal lines
                uint64_t imaginaryBishop = batt(sq1, indexToBit(sq2));
                if (imaginaryBishop & indexToBit(sq2)) {
                    uint64_t imaginaryBishop2 = batt(sq2, indexToBit(sq1));
                    inBetweenSqs[sq1][sq2] = imaginaryBishop & imaginaryBishop2;
                }
                // If the squares are not on a line, the bitboard is empty
                else
                    inBetweenSqs[sq1][sq2] = 0;
            }
        }
    }
}


/**
 * @brief Initializes the tables and values necessary for magic bitboards.
 * We use the "fancy" approach.
 * https://chessprogramming.wikispaces.com/Magic+Bitboards
 */
void initMagicTables(uint64_t seed) {
    // An arbitrarily chosen random number generator and seed
    // The constant seed allows this process to be deterministic for optimization
    // and debugging.
    mstate = 74036198046ULL;
    mseed = seed;
    // Initialize the rook and bishop masks
    for (int i = 0; i < 64; i++) {
        // The relevant bits are everything except the edges
        // However, we don't want to remove the edge that we are on
        uint64_t relevantBits = ((~FILES[0] & ~FILES[7]) | FILES[i&7])
                              & ((~RANKS[0] & ~RANKS[7]) | RANKS[i>>3]);
        // The masks are rook and bishop attacks on an empty board
        ROOK_MASK[i] = ratt(i, 0) & relevantBits;
        BISHOP_MASK[i] = batt(i, 0) & relevantBits;
    }
    // The attack table has 107648 entries, found by summing the 2^(# relevant bits)
    // for all squares of both bishops and rooks
    attackTable = new uint64_t[107648];
    // Keeps track of the start location of attack set arrays
    int runningPtrLoc = 0;
    // Initialize bishop magic values
    for (int i = 0; i < 64; i++) {
        uint64_t *tableStart = attackTable;
        magicBishops[i].table = tableStart + runningPtrLoc;
        magicBishops[i].mask = BISHOP_MASK[i];
        magicBishops[i].magic = findMagic(i, NUM_BISHOP_BITS[i], true);
        magicBishops[i].shift = 64 - NUM_BISHOP_BITS[i];
        // We need 2^n array slots for a mask of n bits
        runningPtrLoc += 1 << NUM_BISHOP_BITS[i];
    }
    // Initialize rook magic values
    for (int i = 0; i < 64; i++) {
        uint64_t *tableStart = attackTable;
        magicRooks[i].table = tableStart + runningPtrLoc;
        magicRooks[i].mask = ROOK_MASK[i];
        magicRooks[i].magic = findMagic(i, NUM_ROOK_BITS[i], false);
        magicRooks[i].shift = 64 - NUM_ROOK_BITS[i];
        runningPtrLoc += 1 << NUM_ROOK_BITS[i];
    }
    // Set up the actual attack table, bishops first
    for (int sq = 0; sq < 64; sq++) {
        int nBits = NUM_BISHOP_BITS[sq];
        uint64_t mask = BISHOP_MASK[sq];
        // For each possible mask result
        for (int i = 0; i < (1 << nBits); i++) {
            // Find the pointer of where to store the attack sets
            uint64_t *attTableLoc = magicBishops[sq].table;
            // Find the actual masked bits from the mask index
            uint64_t occ = indexToMask64(i, nBits, mask);
            // Get the attack set for this masked occupancy
            uint64_t attSet = batt(sq, occ);
            // Do the mapping to get the location in the attack table where we
            // store the attack set
            int magicIndex = magicMap(occ, magicBishops[sq].magic, nBits);
            attTableLoc[magicIndex] = attSet;
        }
    }
    // Then rooks
    for (int sq = 0; sq < 64; sq++) {
        int nBits = NUM_ROOK_BITS[sq];
        uint64_t mask = ROOK_MASK[sq];
        for (int i = 0; i < (1 << nBits); i++) {
            uint64_t *attTableLoc = magicRooks[sq].table;
            uint64_t occ = indexToMask64(i, nBits, mask);
            uint64_t attSet = ratt(sq, occ);
            int magicIndex = magicMap(occ, magicRooks[sq].magic, nBits);
            attTableLoc[magicIndex] = attSet;
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


//------------------------------------------------------------------------------
//-----------------------------MAGIC BITBOARDS----------------------------------
//------------------------------------------------------------------------------
// This code is adapted from Tord Romstad's approach to finding magics,
// available online at https://chessprogramming.wikispaces.com/Looking+for+Magics

// Maps an index from 0 from 2^nBits - 1 into one of the
// 2^nBits possible masks
uint64_t indexToMask64(int index, int nBits, uint64_t mask) {
    uint64_t result = 0;
    // For each bit in the mask
    for (int i = 0; i < nBits; i++) {
        int j = bitScanForward(mask);
        mask &= mask - 1;
        // If this bit should be set...
        if (index & indexToBit(i))
            // Set it at the same position in the result
            result |= indexToBit(j);
    }
    return result;
}

// Gets rook attacks using Dumb7Fill methods
uint64_t ratt(int sq, uint64_t block) {
    return fillRayRight(indexToBit(sq), ~block, NORTH_SOUTH_FILL) // south
         | fillRayLeft(indexToBit(sq), ~block, NORTH_SOUTH_FILL)  // north
         | fillRayLeft(indexToBit(sq), ~block, EAST_WEST_FILL)    // east
         | fillRayRight(indexToBit(sq), ~block, EAST_WEST_FILL);  // west
}

// Gets bishop attacks using Dumb7Fill methods
uint64_t batt(int sq, uint64_t block) {
    return fillRayLeft(indexToBit(sq), ~block, NE_SW_FILL)   // northeast
         | fillRayLeft(indexToBit(sq), ~block, NW_SE_FILL)   // northwest
         | fillRayRight(indexToBit(sq), ~block, NE_SW_FILL)  // southwest
         | fillRayRight(indexToBit(sq), ~block, NW_SE_FILL); // southeast
}

// Maps a mask using a candidate magic into an index nBits long
inline int magicMap(uint64_t masked, uint64_t magic, int nBits) {
    return (int) ((masked * magic) >> (64 - nBits));
}

/**
 * @brief Finds a magic number for the given square using trial and error.
 * @param sq The square to find the magic for.
 * @param iBits The length of the desired index, in bits
 * @param isBishop True for bishop magics, false for rook magics
 * @param magicRNG A random number generator to get magic candidates
 */
uint64_t findMagic(int sq, int iBits, bool isBishop) {
    uint64_t mask, maskedBits[4096], attSet[4096], used[4096], magic;
    bool failed;

    mask = isBishop ? BISHOP_MASK[sq] : ROOK_MASK[sq];
    int nBits = count(mask);
    // For each possible masked occupancy, get the attack set corresponding to
    // that square and occupancy
    for (int i = 0; i < (1 << nBits); i++) {
        maskedBits[i] = indexToMask64(i, nBits, mask);
        attSet[i] = isBishop ? batt(sq, maskedBits[i]) : ratt(sq, maskedBits[i]);
    }
    // Try 100 mill iterations before giving up
    for (int k = 0; k < 100000000; k++) {
        // Get a random magic candidate
        // We make this random 64-bit integer sparse by &-ing 3 random numbers
        // Sparse numbers are beneficial to keep the multiplied bits from
        // bleeding together and becoming garbage
        magic = magicRNG() & magicRNG() & magicRNG();
        // We want a large number of high bits to get a higher success rate,
        // since mask * magic is shifted by 64 - n bits, leaving n bits at the
        // end. Thus, anything but the top 12 bits (for rooks in the corners) or
        // less (for bishops) is garbage. Having a large number of high bits
        // when multiplying by the full mask gives a better spread of values
        // with different partial masks.
        if (count((mask * magic) & 0xFFF0000000000000ULL) < 10)
            continue;

        // Clear the used table
        for (int i = 0; i < 4096; i++)
            used[i] = 0;
        // Calculate the packed bits for every possible mask using this magic
        // and see if any fail
        failed = false;
        for (int i = 0; !failed && i < (1 << nBits); i++) {
            int mappedIndex = magicMap(maskedBits[i], magic, iBits);
            // No collision, mark the index as used for the given attack set
            if (!used[mappedIndex])
                used[mappedIndex] = attSet[i];
            // Otherwise, check for a constructive collsion, where a different
            // occupancy has the same attack set.
            // If the collision is not constructive, then we failed.
            else if (used[mappedIndex] != attSet[i])
                failed = true;
        }
        // If there were no collisions in all 2^nBits mappings, we have found
        // a valid magic
        if (!failed)
            return magic;
    }

    // Otherwise we failed :(
    // (this should never happen)
    return 0;
}
