/*
    Laser, a UCI chess engine written in C++11.
    Copyright 2015-2018 Jeffrey An and Michael An
    Copyright (c) 2011-2015 Ronald de Man

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

// The probing code currently expects a little-endian architecture (e.g. x86).

// Define DECOMP64 when compiling for a 64-bit platform.
// 32-bit is only supported for 5-piece tables, because tables are mmap()ed
// into memory.
// #ifdef IS_64BIT
#define DECOMP64
// #endif

#include "../bbinit.h"
#include "../board.h"
#include "../common.h"
#include "../eval.h"
#include "../search.h"
#include "../uci.h"

#include "tbprobe.h"
#include "tbcore.h"

extern uint64_t zobristTable[794];

// int TBlargest = 0;

#include "tbcore.c"

// Given a position with 6 or fewer pieces, produce a text string
// of the form KQPvKRP, where "KQP" represents the white pieces if
// mirror == 0 and the black pieces if mirror == 1.
// No need to make this very efficient.
static void prt_str(const Board &b, char *str, int mirror) {
    int color = (mirror == 0) ? WHITE : BLACK;
    for (int pt = KINGS; pt >= PAWNS; pt--)
        for (int i = count(b.getPieces(color, pt)); i > 0; i--)
            *str++ = pchr[5 - pt];
    *str++ = 'v';
    color = color^1;
    for (int pt = KINGS; pt >= PAWNS; pt--)
        for (int i = count(b.getPieces(color, pt)); i > 0; i--)
            *str++ = pchr[5 - pt];
    *str++ = 0;
}

// Given a position, produce a 64-bit material signature key.
// Again no need to make this very efficient.
static uint64 calc_key(const Board &b, int mirror) {
    uint64 key = 0;
    int color = (mirror == 0) ? WHITE : BLACK;

    for (int pt = PAWNS; pt <= KINGS; pt++)
        for (int i = count(b.getPieces(color, pt))-1; i >= 0; i--)
            key ^= zobristTable[64*pt+i];
    color = color^1;
    for (int pt = PAWNS; pt <= KINGS; pt++)
        for (int i = count(b.getPieces(color, pt))-1; i >= 0; i--)
            key ^= zobristTable[384+64*pt+i];

    return key;
}

// Produce a 64-bit material key corresponding to the material combination
// defined by pcs[16], where pcs[1], ..., pcs[6] is the number of white
// pawns, ..., kings and pcs[9], ..., pcs[14] is the number of black
// pawns, ..., kings.
// Again no need to be efficient here.
static uint64 calc_key_from_pcs(int *pcs, int mirror) {
    uint64 key = 0;
    int color = (mirror == 0) ? 0 : 8;

    for (int pt = PAWNS; pt <= KINGS; pt++)
        for (int i = 0; i < pcs[color + pt + 1]; i++)
            key ^= zobristTable[64*pt+i];
    color ^= 8;
    for (int pt = PAWNS; pt <= KINGS; pt++)
        for (int i = 0; i < pcs[color + pt + 1]; i++)
            key ^= zobristTable[384+64*pt+i];

    return key;
}

// probe_wdl_table and probe_dtz_table require similar adaptations.
static int probe_wdl_table(const Board &b, int *success) {
    struct TBEntry *ptr;
    struct TBHashEntry *ptr2;
    uint64 idx;
    int i;
    ubyte res;
    int p[TBPIECES];

    // Obtain the position's material signature key.
    uint64 key = calc_key(b, 0);

    // Test for KvK.
    if (key == (zobristTable[64*KINGS] ^ zobristTable[384+64*KINGS]))
        return 0;

    ptr2 = TB_hash[key >> (64 - TBHASHBITS)];
    for (i = 0; i < HSHMAX; i++)
        if (ptr2[i].key == key) break;
    if (i == HSHMAX) {
        *success = 0;
        return 0;
    }

    ptr = ptr2[i].ptr;
    if (!ptr->ready) {
        LOCK(TB_mutex);
        if (!ptr->ready) {
            char str[16];
            prt_str(b, str, ptr->key != key);
            if (!init_table_wdl(ptr, str)) {
                ptr2[i].key = 0ULL;
                *success = 0;
                UNLOCK(TB_mutex);
                return 0;
            }
            // Memory barrier to ensure ptr->ready = 1 is not reordered.
            __asm__ __volatile__ ("" ::: "memory");
            ptr->ready = 1;
        }
        UNLOCK(TB_mutex);
    }

    int bside, mirror, cmirror;
    if (!ptr->symmetric) {
        if (key != ptr->key) {
            cmirror = 8;
            mirror = 0x38;
            bside = (b.getPlayerToMove() == WHITE);
        } else {
            cmirror = mirror = 0;
            bside = !(b.getPlayerToMove() == WHITE);
        }
    } else {
        cmirror = b.getPlayerToMove() == WHITE ? 0 : 8;
        mirror = b.getPlayerToMove() == WHITE ? 0 : 0x38;
        bside = 0;
    }

    // p[i] is to contain the square 0-63 (A1-H8) for a piece of type
    // pc[i] ^ cmirror, where 1 = white pawn, ..., 14 = black king.
    // Pieces of the same type are guaranteed to be consecutive.
    if (!ptr->has_pawns) {
        struct TBEntry_piece *entry = (struct TBEntry_piece *)ptr;
        ubyte *pc = entry->pieces[bside];
        for (i = 0; i < entry->num;) {
            uint64_t bb = b.getPieces((int)((pc[i] ^ cmirror) >> 3),
                            (int)(pc[i] & 0x07) - 1);
            do {
                p[i++] = bitScanForward(bb);
                bb &= bb-1;
            } while (bb);
        }
        idx = encode_piece(entry, entry->norm[bside], p, entry->factor[bside]);
        res = decompress_pairs(entry->precomp[bside], idx);
    } else {
        struct TBEntry_pawn *entry = (struct TBEntry_pawn *)ptr;
        int k = entry->file[0].pieces[0][0] ^ cmirror;
        uint64_t bb = b.getPieces((int)(k >> 3), (int)(k & 0x07) - 1);
        i = 0;
        do {
            p[i++] = bitScanForward(bb) ^ mirror;
            bb &= bb-1;
        } while (bb);
        int f = pawn_file(entry, p);
        ubyte *pc = entry->file[f].pieces[bside];
        for (; i < entry->num;) {
            bb = b.getPieces((int)((pc[i] ^ cmirror) >> 3),
                        (int)(pc[i] & 0x07) - 1);
            do {
                p[i++] = bitScanForward(bb) ^ mirror;
                bb &= bb-1;
            } while (bb);
        }
        idx = encode_pawn(entry, entry->file[f].norm[bside], p, entry->file[f].factor[bside]);
        res = decompress_pairs(entry->file[f].precomp[bside], idx);
    }

    return ((int)res) - 2;
}

// The value of wdl MUST correspond to the WDL value of the position without
// en passant rights.
static int probe_dtz_table(const Board &b, int wdl, int *success) {
    struct TBEntry *ptr;
    uint64 idx;
    int i, res;
    int p[TBPIECES];

    // Obtain the position's material signature key.
    uint64 key = calc_key(b, 0);

    if (DTZ_table[0].key1 != key && DTZ_table[0].key2 != key) {
        for (i = 1; i < DTZ_ENTRIES; i++)
            if (DTZ_table[i].key1 == key || DTZ_table[i].key2 == key) break;
        if (i < DTZ_ENTRIES) {
            struct DTZTableEntry table_entry = DTZ_table[i];
            for (; i > 0; i--)
                DTZ_table[i] = DTZ_table[i - 1];
            DTZ_table[0] = table_entry;
        } else {
            struct TBHashEntry *ptr2 = TB_hash[key >> (64 - TBHASHBITS)];
            for (i = 0; i < HSHMAX; i++)
                if (ptr2[i].key == key) break;
            if (i == HSHMAX) {
                *success = 0;
                return 0;
            }
            ptr = ptr2[i].ptr;
            char str[16];
            int mirror = (ptr->key != key);
            prt_str(b, str, mirror);
            if (DTZ_table[DTZ_ENTRIES - 1].entry)
                free_dtz_entry(DTZ_table[DTZ_ENTRIES-1].entry);
            for (i = DTZ_ENTRIES - 1; i > 0; i--)
                DTZ_table[i] = DTZ_table[i - 1];
            load_dtz_table(str, calc_key(b, mirror), calc_key(b, !mirror));
        }
    }

    ptr = DTZ_table[0].entry;
    if (!ptr) {
        *success = 0;
        return 0;
    }

    int bside, mirror, cmirror;
    if (!ptr->symmetric) {
        if (key != ptr->key) {
            cmirror = 8;
            mirror = 0x38;
            bside = (b.getPlayerToMove() == WHITE);
        } else {
            cmirror = mirror = 0;
            bside = !(b.getPlayerToMove() == WHITE);
        }
    } else {
        cmirror = b.getPlayerToMove() == WHITE ? 0 : 8;
        mirror = b.getPlayerToMove() == WHITE ? 0 : 0x38;
        bside = 0;
    }

    if (!ptr->has_pawns) {
        struct DTZEntry_piece *entry = (struct DTZEntry_piece *)ptr;
        if ((entry->flags & 1) != bside && !entry->symmetric) {
            *success = -1;
            return 0;
        }
        ubyte *pc = entry->pieces;
        for (i = 0; i < entry->num;) {
            uint64_t bb = b.getPieces((int)((pc[i] ^ cmirror) >> 3),
                        (int)(pc[i] & 0x07) - 1);
            do {
                p[i++] = bitScanForward(bb);
                bb &= bb-1;
            } while (bb);
        }
        idx = encode_piece((struct TBEntry_piece *)entry, entry->norm, p, entry->factor);
        res = decompress_pairs(entry->precomp, idx);

        if (entry->flags & 2)
            res = entry->map[entry->map_idx[wdl_to_map[wdl + 2]] + res];

        if (!(entry->flags & pa_flags[wdl + 2]) || (wdl & 1))
            res *= 2;
    } else {
        struct DTZEntry_pawn *entry = (struct DTZEntry_pawn *)ptr;
        int k = entry->file[0].pieces[0] ^ cmirror;
        uint64_t bb = b.getPieces((int)(k >> 3), (int)(k & 0x07) - 1);
        i = 0;
        do {
            p[i++] = bitScanForward(bb) ^ mirror;
            bb &= bb-1;
        } while (bb);
        int f = pawn_file((struct TBEntry_pawn *)entry, p);
        if ((entry->flags[f] & 1) != bside) {
            *success = -1;
            return 0;
        }
        ubyte *pc = entry->file[f].pieces;
        for (; i < entry->num;) {
            bb = b.getPieces((int)((pc[i] ^ cmirror) >> 3),
                    (int)(pc[i] & 0x07) - 1);
            do {
                p[i++] = bitScanForward(bb) ^ mirror;
                bb &= bb-1;
            } while (bb);
        }
        idx = encode_pawn((struct TBEntry_pawn *)entry, entry->file[f].norm, p, entry->file[f].factor);
        res = decompress_pairs(entry->file[f].precomp, idx);

        if (entry->flags[f] & 2)
            res = entry->map[entry->map_idx[f][wdl_to_map[wdl + 2]] + res];

        if (!(entry->flags[f] & pa_flags[wdl + 2]) || (wdl & 1))
            res *= 2;
    }

    return res;
}

static int probe_ab(Board &b, int alpha, int beta, int *success) {
    int v;
    MoveList legalMoves;
    int color = b.getPlayerToMove();

    // Generate (at least) all legal captures including (under)promotions.
    // It is OK to generate more, as long as they are filtered out below.
    b.getPseudoLegalCaptures(legalMoves, color, true);

    for (unsigned int i = 0; i < legalMoves.size(); i++) {
        Move capture = legalMoves.get(i);
        Board copy = b.staticCopy();
        if (!isCapture(capture) || !copy.doPseudoLegalMove(capture, color))
            continue;

        v = -probe_ab(copy, -beta, -alpha, success);

        if (*success == 0) return 0;
        if (v > alpha) {
            if (v >= beta)
                return v;
            alpha = v;
        }
    }

    v = probe_wdl_table(b, success);

    return alpha >= v ? alpha : v;
}

// Probe the WDL table for a particular position.
//
// If *success != 0, the probe was successful.
//
// If *success == 2, the position has a winning capture, or the position
// is a cursed win and has a cursed winning capture, or the position
// has an ep capture as only best move.
// This is used in probe_dtz().
//
// The return value is from the point of view of the side to move:
// -2 : loss
// -1 : loss, but draw under 50-move rule
//  0 : draw
//  1 : win, but draw under 50-move rule
//  2 : win
int probe_wdl(const Board &b, int *success) {
    *success = 1;
    int color = b.getPlayerToMove();

    // Generate (at least) all legal en passant captures.
    MoveList legalMoves;

    // Generate (at least) all legal captures including (under)promotions.
    b.getPseudoLegalCaptures(legalMoves, color, true);

    int best_cap = -3, best_ep = -3;

    // We do capture resolution, letting best_cap keep track of the best
    // capture without ep rights and letting best_ep keep track of still
    // better ep captures if they exist.
    for (unsigned int i = 0; i < legalMoves.size(); i++) {
        Move capture = legalMoves.get(i);
        Board copy = b.staticCopy();
        if (!isCapture(capture) || !copy.doPseudoLegalMove(capture, color))
            continue;

        int v = -probe_ab(copy, -2, -best_cap, success);

        if (*success == 0) return 0;
        if (v > best_cap) {
            if (v == 2) {
                *success = 2;
                return 2;
            }
            if (!isEP(capture))
                best_cap = v;
            else if (v > best_ep)
                best_ep = v;
        }
    }

    int v = probe_wdl_table(b, success);
    if (*success == 0) return 0;

    // Now max(v, best_cap) is the WDL value of the position without ep rights.
    // If the position without ep rights is not stalemate or no ep captures
    // exist, then the value of the position is max(v, best_cap, best_ep).
    // If the position without ep rights is stalemate and best_ep > -3,
    // then the value of the position is best_ep (and we will have v == 0).

    if (best_ep > best_cap) {
        if (best_ep > v) { // ep capture (possibly cursed losing) is best.
            *success = 2;
            return best_ep;
        }
        best_cap = best_ep;
    }

    // Now max(v, best_cap) is the WDL value of the position unless
    // the position without ep rights is stalemate and best_ep > -3.

    if (best_cap >= v) {
        // No need to test for the stalemate case here: either there are
        // non-ep captures, or best_cap == best_ep >= v anyway.
        *success = 1 + (best_cap > 0);
        return best_cap;
    }

    // Now handle the stalemate case.
    if (best_ep > -3 && v == 0) {
        // Check for stalemate in the position with ep captures.
        unsigned int i = 0;
        for (; i < legalMoves.size(); i++) {
            Move move = legalMoves.get(i);
            if (isEP(move)) continue;
            Board copy = b.staticCopy();
            if (copy.doPseudoLegalMove(move, color)) break;
        }
        if (i == legalMoves.size() && !b.isInCheck(color)) {
            b.getPseudoLegalQuiets(legalMoves, color);
            for (; i < legalMoves.size(); i++) {
                Move move = legalMoves.get(i);
                Board copy = b.staticCopy();
                if (copy.doPseudoLegalMove(move, color))
                    break;
            }
        }
        if (i == legalMoves.size()) { // Stalemate detected.
            *success = 2;
            return best_ep;
        }
    }

    // Stalemate / en passant not an issue, so v is the correct value.

    return v;
}

static int wdl_to_dtz[] = {
    -1, -101, 0, 101, 1
};

// Probe the DTZ table for a particular position.
// If *success != 0, the probe was successful.
// The return value is from the point of view of the side to move:
//         n < -100 : loss, but draw under 50-move rule
// -100 <= n < -1   : loss in n ply (assuming 50-move counter == 0)
//         0        : draw
//     1 < n <= 100 : win in n ply (assuming 50-move counter == 0)
//   100 < n        : win, but draw under 50-move rule
//
// If the position is mate, -1 is returned instead of 0.
//
// The return value n can be off by 1: a return value -n can mean a loss
// in n+1 ply and a return value +n can mean a win in n+1 ply. This
// cannot happen for tables with positions exactly on the "edge" of
// the 50-move rule.
//
// This means that if dtz > 0 is returned, the position is certainly
// a win if dtz + 50-move-counter <= 99. Care must be taken that the engine
// picks moves that preserve dtz + 50-move-counter <= 99.
//
// If n = 100 immediately after a capture or pawn move, then the position
// is also certainly a win, and during the whole phase until the next
// capture or pawn move, the inequality to be preserved is
// dtz + 50-movecounter <= 100.
//
// In short, if a move is available resulting in dtz + 50-move-counter <= 99,
// then do not accept moves leading to dtz + 50-move-counter == 100.
//
int probe_dtz(const Board &b, int *success) {
    int wdl = probe_wdl(b, success);
    if (*success == 0) return 0;

    // If draw, then dtz = 0.
    if (wdl == 0) return 0;

    // Check for winning (cursed) capture or ep capture as only best move.
    if (*success == 2)
        return wdl_to_dtz[wdl + 2];

    int color = b.getPlayerToMove();
    MoveList legalMoves;

    // If winning, check for a winning pawn move.
    if (wdl > 0) {
        // Generate at least all legal non-capturing pawn moves
        // including non-capturing promotions.
        b.getPseudoLegalQuiets(legalMoves, color);

        for (unsigned int i = 0; i < legalMoves.size(); i++) {
            Move move = legalMoves.get(i);
            Board copy = b.staticCopy();
            if (b.getPieceOnSquare(color, getStartSq(move)) != PAWNS || isCapture(move)
                                || !copy.doPseudoLegalMove(move, color))
                continue;

            int v = -probe_wdl(copy, success);

            if (*success == 0) return 0;
            if (v == wdl)
                return wdl_to_dtz[wdl + 2];
        }
    }

    // If we are here, we know that the best move is not an ep capture.
    // In other words, the value of wdl corresponds to the WDL value of
    // the position without ep rights. It is therefore safe to probe the
    // DTZ table with the current value of wdl.

    int dtz = probe_dtz_table(b, wdl, success);
    if (*success >= 0)
        return wdl_to_dtz[wdl + 2] + ((wdl > 0) ? dtz : -dtz);

    // *success < 0 means we need to probe DTZ for the other side to move.
    int best;
    if (wdl > 0) {
        best = INT32_MAX;
        // If wdl > 0, we already generated all moves.
    } else {
        // If (cursed) loss, the worst case is a losing capture or pawn move
        // as the "best" move, leading to dtz of -1 or -101.
        // In case of mate, this will cause -1 to be returned.
        best = wdl_to_dtz[wdl + 2];
        b.getPseudoLegalQuiets(legalMoves, color);
    }

    for (unsigned int i = 0; i < legalMoves.size(); i++) {
        Move move = legalMoves.get(i);
        Board copy = b.staticCopy();
        // We can skip pawn moves and captures.
        // If wdl > 0, we already caught them. If wdl < 0, the initial value
        // of best already takes account of them.
        if (isCapture(move) || b.getPieceOnSquare(color, getStartSq(move)) == PAWNS
                            || !copy.doPseudoLegalMove(move, color))
            continue;

        int v = -probe_dtz(copy, success);

        if (*success == 0) return 0;
        if (wdl > 0) {
            if (v > 0 && v + 1 < best)
                best = v + 1;
        } else {
            if (v -1 < best)
                best = v - 1;
        }
    }
    return best;
}

// Check whether there has been at least one repetition of positions
// since the last capture or pawn move.
static int has_repeated() {
    TwoFoldStack *tfp = getTwoFoldStackPointer();
    if (tfp->length < 3)
        return false;

    uint64_t pos = tfp->keys[tfp->length-1];
    for (int i = tfp->length-1; i > 0; i--) {
        if (tfp->keys[i-1] == pos)
            return true;
    }
    return false;
}

static int wdl_to_value[5] = {
    -TB_WIN,
    -2,
    0,
    2,
    TB_WIN
};

// Use the DTZ tables to filter out moves that don't preserve the win or draw.
// If the position is lost, but DTZ is fairly high, only keep moves that
// maximise DTZ.
//
// A return value of 0 indicates that not all probes were successful and that
// no moves were filtered out.
int root_probe(const Board *b, MoveList &rootMoves, ScoreList &scores, int &TBScore) {
    int success;

    int dtz = probe_dtz(*b, &success);
    if (!success) return 0;

    int color = b->getPlayerToMove();

    // Probe each move.
    for (unsigned int i = 0; i < rootMoves.size(); i++) {
        Move move = rootMoves.get(i);
        Board copy = b->staticCopy();
        copy.doMove(move, color);
        int v = 0;
        if (copy.isInCheck(color^1) && dtz > 0) {
            if (copy.getAllLegalMoves(color^1).size() == 0)
                v = 1;
        }
        if (!v) {
            if (copy.getFiftyMoveCounter() != 0) {
                v = -probe_dtz(copy, &success);
                if (v > 0) v++;
                else if (v < 0) v--;
            } else {
                v = -probe_wdl(copy, &success);
                v = wdl_to_dtz[v + 2];
            }
        }
        // pos.undo_move(move);
        if (!success) return 0;
        scores.add(v);
    }

    // Obtain 50-move counter for the root position.
    int cnt50 = b->getFiftyMoveCounter();

    // Use 50-move counter to determine whether the root position is
    // won, lost or drawn.
    int wdl = 0;
    if (dtz > 0)
        wdl = (dtz + cnt50 <= 100) ? 2 : 1;
    else if (dtz < 0)
        wdl = (-dtz + cnt50 <= 100) ? -2 : -1;

    // Determine the score to report to the user.
    TBScore = wdl_to_value[wdl + 2];
    // If the position is winning or losing, but too few moves left, adjust the
    // score to show how close it is to winning or losing.
    // NOTE: int(PawnValueEg) is used as scaling factor in score_to_uci().
    if (wdl == 1 && dtz <= 100)
        TBScore = (int) (((200 - dtz - cnt50) * int(PIECE_VALUES[EG][PAWNS])) / 200);
    else if (wdl == -1 && dtz >= -100)
        TBScore = -(int) (((200 + dtz - cnt50) * int(PIECE_VALUES[EG][PAWNS])) / 200);

    // Now be a bit smart about filtering out moves.
    size_t j = 0;
    if (dtz > 0) { // winning (or 50-move rule draw)
        int best = 0xffff;
        for (unsigned int i = 0; i < rootMoves.size(); i++) {
            int v = scores.get(i);
            if (v > 0 && v < best)
                best = v;
        }
        int max = best;
        // If the current phase has not seen repetitions, then try all moves
        // that stay safely within the 50-move budget, if there are any.
        if (!has_repeated() && best + cnt50 <= 99)
            max = 99 - cnt50;
        for (unsigned int i = 0; i < rootMoves.size(); i++) {
            int v = scores.get(i);
            if (v > 0 && v <= max) {
                rootMoves.swap(j++, i);
                scores.swap(j-1, i);
            }
        }
    } else if (dtz < 0) {
        int best = 0;
        for (unsigned int i = 0; i < rootMoves.size(); i++) {
            int v = scores.get(i);
            if (v < best)
                best = v;
        }
        // Try all moves, unless we approach or have a 50-move rule draw.
        if (-best * 2 + cnt50 < 100)
            return 1;
        for (unsigned int i = 0; i < rootMoves.size(); i++) {
            if (scores.get(i) == best) {
                rootMoves.swap(j++, i);
                scores.swap(j-1, i);
            }
        }
    } else { // drawing
        // Try all moves that preserve the draw.
        for (unsigned int i = 0; i < rootMoves.size(); i++) {
            if (scores.get(i) == 0) {
                rootMoves.swap(j++, i);
                scores.swap(j-1, i);
            }
        }
    }
    rootMoves.resize(j);

    return 1;
}

// Use the WDL tables to filter out moves that don't preserve the win or draw.
// This is a fallback for the case that some or all DTZ tables are missing.
//
// A return value of 0 indicates that not all probes were successful and that
// no moves were filtered out.
int root_probe_wdl(const Board *b, MoveList &rootMoves, ScoreList &scores, int &TBScore) {
    int success;

    int wdl = probe_wdl(*b, &success);
    if (!success) return false;
    TBScore = wdl_to_value[wdl + 2];

    int color = b->getPlayerToMove();

    int best = -2;

    // Probe each move.
    for (unsigned int i = 0; i < rootMoves.size(); i++) {
        Move move = rootMoves.get(i);
        Board copy = b->staticCopy();
        copy.doMove(move, color);

        int v = -probe_wdl(copy, &success);

        if (!success) return false;
        scores.add(v);
        if (v > best)
            best = v;
    }

    size_t j = 0;
    for (unsigned int i = 0; i < rootMoves.size(); i++) {
        if (scores.get(i) == best) {
            rootMoves.swap(j++, i);
            scores.swap(j-1, i);
        }
    }
    rootMoves.resize(j);

    return 1;
}
