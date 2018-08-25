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

#include <cassert>
#include <cstdint>

#include "bbinit.h"
#include "common.h"

#ifdef USE_PEXT
#include <immintrin.h> // for _pext_u64()
#endif

uint64_t KnightAttacks[64];
uint64_t BishopAttacks[0x1480];
uint64_t RookAttacks[0x19000];
uint64_t KingAttacks[64];

uint64_t BishopMask[64], RookMask[64];
unsigned BishopShift[64], RookShift[64];
uint64_t *BishopAttacksPtr[64], *RookAttacksPtr[64];

constexpr uint64_t RookMagic[64] = {
    0xA180022080400230ull, 0x0040100040022000ull, 0x0080088020001002ull, 0x0080080280841000ull,
    0x4200042010460008ull, 0x04800A0003040080ull, 0x0400110082041008ull, 0x008000A041000880ull,
    0x10138001A080C010ull, 0x0000804008200480ull, 0x00010011012000C0ull, 0x0022004128102200ull,
    0x000200081201200Cull, 0x202A001048460004ull, 0x0081000100420004ull, 0x4000800380004500ull,
    0x0000208002904001ull, 0x0090004040026008ull, 0x0208808010002001ull, 0x2002020020704940ull,
    0x8048010008110005ull, 0x6820808004002200ull, 0x0A80040008023011ull, 0x00B1460000811044ull,
    0x4204400080008EA0ull, 0xB002400180200184ull, 0x2020200080100380ull, 0x0010080080100080ull,
    0x2204080080800400ull, 0x0000A40080360080ull, 0x02040604002810B1ull, 0x008C218600004104ull,
    0x8180004000402000ull, 0x488C402000401001ull, 0x4018A00080801004ull, 0x1230002105001008ull,
    0x8904800800800400ull, 0x0042000C42003810ull, 0x008408110400B012ull, 0x0018086182000401ull,
    0x2240088020C28000ull, 0x001001201040C004ull, 0x0A02008010420020ull, 0x0010003009010060ull,
    0x0004008008008014ull, 0x0080020004008080ull, 0x0282020001008080ull, 0x50000181204A0004ull,
    0x48FFFE99FECFAA00ull, 0x48FFFE99FECFAA00ull, 0x497FFFADFF9C2E00ull, 0x613FFFDDFFCE9200ull,
    0xFFFFFFE9FFE7CE00ull, 0xFFFFFFF5FFF3E600ull, 0x0010301802830400ull, 0x510FFFF5F63C96A0ull,
    0xEBFFFFB9FF9FC526ull, 0x61FFFEDDFEEDAEAEull, 0x53BFFFEDFFDEB1A2ull, 0x127FFFB9FFDFB5F6ull,
    0x411FFFDDFFDBF4D6ull, 0x0801000804000603ull, 0x0003FFEF27EEBE74ull, 0x7645FFFECBFEA79Eull
};

constexpr uint64_t BishopMagic[64] = {
    0xFFEDF9FD7CFCFFFFull, 0xFC0962854A77F576ull, 0x5822022042000000ull, 0x2CA804A100200020ull,
    0x0204042200000900ull, 0x2002121024000002ull, 0xFC0A66C64A7EF576ull, 0x7FFDFDFCBD79FFFFull,
    0xFC0846A64A34FFF6ull, 0xFC087A874A3CF7F6ull, 0x1001080204002100ull, 0x1810080489021800ull,
    0x0062040420010A00ull, 0x5028043004300020ull, 0xFC0864AE59B4FF76ull, 0x3C0860AF4B35FF76ull,
    0x73C01AF56CF4CFFBull, 0x41A01CFAD64AAFFCull, 0x040C0422080A0598ull, 0x4228020082004050ull,
    0x0200800400E00100ull, 0x020B001230021040ull, 0x7C0C028F5B34FF76ull, 0xFC0A028E5AB4DF76ull,
    0x0020208050A42180ull, 0x001004804B280200ull, 0x2048020024040010ull, 0x0102C04004010200ull,
    0x020408204C002010ull, 0x02411100020080C1ull, 0x102A008084042100ull, 0x0941030000A09846ull,
    0x0244100800400200ull, 0x4000901010080696ull, 0x0000280404180020ull, 0x0800042008240100ull,
    0x0220008400088020ull, 0x04020182000904C9ull, 0x0023010400020600ull, 0x0041040020110302ull,
    0xDCEFD9B54BFCC09Full, 0xF95FFA765AFD602Bull, 0x1401210240484800ull, 0x0022244208010080ull,
    0x1105040104000210ull, 0x2040088800C40081ull, 0x43FF9A5CF4CA0C01ull, 0x4BFFCD8E7C587601ull,
    0xFC0FF2865334F576ull, 0xFC0BF6CE5924F576ull, 0x80000B0401040402ull, 0x0020004821880A00ull,
    0x8200002022440100ull, 0x0009431801010068ull, 0xC3FFB7DC36CA8C89ull, 0xC3FF8A54F4CA2C89ull,
    0xFFFFFCFCFD79EDFFull, 0xFC0863FCCB147576ull, 0x040C000022013020ull, 0x2000104000420600ull,
    0x0400000260142410ull, 0x0800633408100500ull, 0xFC087E8E4BB2F736ull, 0x43FF9E4EF4CA2C89ull
};

static int fileOf(int sq) {
    assert(0 <= sq && sq < 64);
    return sq % 8;
}

static int rankOf(int sq) {
    assert(0 <= sq && sq < 64);
    return sq / 8;
}

static int makeSquare(int rank, int file) {
    assert(0 <= rank && rank < 8);
    assert(0 <= file && file < 8);
    return rank * 8 + file;
}

static int validCoordinate(int rank, int file) {
    return 0 <= rank && rank < 8
        && 0 <= file && file < 8;
}

static void setSquare(uint64_t *bb, int rank, int file){
    if (validCoordinate(rank, file))
        *bb |= 1ull << makeSquare(rank, file);
}

static int sliderIndex(uint64_t occupied, uint64_t mask, uint64_t magic, unsigned shift) {
#ifdef USE_PEXT
    (void)magic, (void)shift;  // Silence compiler warnings
    return _pext_u64(occupied, mask);
#else
    return ((occupied & mask) * magic) >> shift;
#endif
}

static uint64_t sliderAttacks(int sq, uint64_t occupied, const int delta[4][2]) {

    int rank, file, dr, df;
    uint64_t result = 0ull;

    for (int i = 0; i < 4; i++) {

        dr = delta[i][0], df = delta[i][1];

        for (rank = rankOf(sq) + dr, file = fileOf(sq) + df;
             validCoordinate(rank, file);
             rank += dr, file += df) {

            result ^= 1ull << makeSquare(rank, file);

            if (occupied & (1ull << makeSquare(rank, file)))
                break;
        }
    }

    return result;
}

static void initSliderAttacks(int sq, uint64_t mask[64], const uint64_t magic[64],
    unsigned shift[64], uint64_t *attacksPtr[64], const int delta[4][2]) {

    uint64_t edges = ((RANK_1 | RANK_8) & ~RANKS[rankOf(sq)])
                   | ((FILE_A | FILE_H) & ~FILES[fileOf(sq)]);

    uint64_t occupied = 0ull;

    mask[sq] = sliderAttacks(sq, 0, delta) & ~edges;

    shift[sq] = 64 - count(mask[sq]);

    if (sq != 63) attacksPtr[sq + 1] = attacksPtr[sq] + (1 << count(mask[sq]));

    do {
        int index = sliderIndex(occupied, mask[sq], magic[sq], shift[sq]);
        attacksPtr[sq][index] = sliderAttacks(sq, occupied, delta);
        occupied = (occupied - mask[sq]) & mask[sq];
    } while (occupied);
}

void initAttacks() {

    const int KnightDelta[8][2] = {{-2,-1}, {-2, 1}, {-1,-2}, {-1, 2},{ 1,-2}, { 1, 2}, { 2,-1}, { 2, 1}};
    const int KingDelta[8][2]   = {{-1,-1}, {-1, 0}, {-1, 1}, { 0,-1},{ 0, 1}, { 1,-1}, { 1, 0}, { 1, 1}};
    const int BishopDelta[4][2] = {{-1,-1}, {-1, 1}, { 1,-1}, { 1, 1}};
    const int RookDelta[4][2]   = {{-1, 0}, { 0,-1}, { 0, 1}, { 1, 0}};

    // A1 for the start of the tables
    BishopAttacksPtr[0] = BishopAttacks;
    RookAttacksPtr[0] = RookAttacks;

    // Init attack tables for Knights & Kings
    for (int sq = 0; sq < 64; sq++) {
        for (int dir = 0; dir < 8; dir++) {
            setSquare(&KnightAttacks[sq], rankOf(sq) + KnightDelta[dir][0], fileOf(sq) + KnightDelta[dir][1]);
            setSquare(  &KingAttacks[sq], rankOf(sq) +   KingDelta[dir][0], fileOf(sq) +   KingDelta[dir][1]);
        }
    }
    // Init attack tables for Bishops & Rooks (+ Queens)
    for (int sq = 0; sq < 64; sq++) {
        initSliderAttacks(sq, BishopMask, BishopMagic, BishopShift, BishopAttacksPtr, BishopDelta);
        initSliderAttacks(sq,   RookMask,   RookMagic,   RookShift,   RookAttacksPtr,   RookDelta);
    }
}

uint64_t knightAttacks(int sq) {
    assert(0 <= sq && sq < 64);
    return KnightAttacks[sq];
}

uint64_t bishopAttacks(int sq, uint64_t occupied) {
    assert(0 <= sq && sq < 64);
    int index = sliderIndex(occupied, BishopMask[sq], BishopMagic[sq], BishopShift[sq]);
    return BishopAttacksPtr[sq][index];
}

uint64_t rookAttacks(int sq, uint64_t occupied) {
    assert(0 <= sq && sq < 64);
    int index = sliderIndex(occupied, RookMask[sq], RookMagic[sq], RookShift[sq]);
    return RookAttacksPtr[sq][index];
}

uint64_t queenAttacks(int sq, uint64_t occupied) {
    assert(0 <= sq && sq < 64);
    return bishopAttacks(sq, occupied) | rookAttacks(sq, occupied);
}

uint64_t kingAttacks(int sq) {
    assert(0 <= sq && sq < 64);
    return KingAttacks[sq];
}