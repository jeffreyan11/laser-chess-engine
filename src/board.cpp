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

#include <algorithm>
#include <cstring>
#include <random>
#include <string>

#include "attacks.h"
#include "board.h"
#include "bbinit.h"
#include "eval.h"
#include "uci.h"

constexpr uint64_t WHITE_KSIDE_PASSTHROUGH_SQS = indexToBit(5) | indexToBit(6);
constexpr uint64_t WHITE_QSIDE_PASSTHROUGH_SQS = indexToBit(1) | indexToBit(2) | indexToBit(3);
constexpr uint64_t BLACK_KSIDE_PASSTHROUGH_SQS = indexToBit(61) | indexToBit(62);
constexpr uint64_t BLACK_QSIDE_PASSTHROUGH_SQS = indexToBit(57) | indexToBit(58) | indexToBit(59);

// Zobrist hashing table and the start position key, both initialized at startup
uint64_t zobristTable[794];
static uint64_t startPosZobristKey = 0;

void initZobristTable() {
    std::mt19937_64 rng (61280152908);
    for (int i = 0; i < 794; i++)
        zobristTable[i] = rng();

    Board b;
    int *mailbox = b.getMailbox();
    b.initZobristKey(mailbox);
    startPosZobristKey = b.getZobristKey();
    delete[] mailbox;
}

// Precalculated bitboard tables
extern uint64_t inBetweenSqs[64][64];


//------------------------------------------------------------------------------
//--------------------------------Constructors----------------------------------
//------------------------------------------------------------------------------
// Create a board object initialized to the start position.
Board::Board() {
    allPieces[WHITE] = 0x000000000000FFFF;
    allPieces[BLACK] = 0xFFFF000000000000;
    pieces[WHITE][PAWNS] = 0x000000000000FF00; // white pawns
    pieces[WHITE][KNIGHTS] = 0x0000000000000042; // white knights
    pieces[WHITE][BISHOPS] = 0x0000000000000024; // white bishops
    pieces[WHITE][ROOKS] = 0x0000000000000081; // white rooks
    pieces[WHITE][QUEENS] = 0x0000000000000008; // white queens
    pieces[WHITE][KINGS] = 0x0000000000000010; // white kings
    pieces[BLACK][PAWNS] = 0x00FF000000000000; // black pawns
    pieces[BLACK][KNIGHTS] = 0x4200000000000000; // black knights
    pieces[BLACK][BISHOPS] = 0x2400000000000000; // black bishops
    pieces[BLACK][ROOKS] = 0x8100000000000000; // black rooks
    pieces[BLACK][QUEENS] = 0x0800000000000000; // black queens
    pieces[BLACK][KINGS] = 0x1000000000000000; // black kings

    zobristKey = startPosZobristKey;
    epCaptureFile = NO_EP_POSSIBLE;
    playerToMove = WHITE;
    moveNumber = 1;
    castlingRights = WHITECASTLE | BLACKCASTLE;
    fiftyMoveCounter = 0;
}

// Create a board object from a mailbox of the current board state.
Board::Board(int *mailboxBoard, bool _whiteCanKCastle, bool _blackCanKCastle,
        bool _whiteCanQCastle, bool _blackCanQCastle,  uint16_t _epCaptureFile,
        int _fiftyMoveCounter, int _moveNumber, int _playerToMove) {
    // Initialize bitboards
    for (int i = 0; i < 12; i++) {
        pieces[i/6][i%6] = 0;
    }
    for (int i = 0; i < 64; i++) {
        if (0 <= mailboxBoard[i] && mailboxBoard[i] <= 11) {
            pieces[mailboxBoard[i]/6][mailboxBoard[i]%6] |= indexToBit(i);
        }
    }
    allPieces[WHITE] = 0;
    for (int i = 0; i < 6; i++)
        allPieces[WHITE] |= pieces[WHITE][i];
    allPieces[BLACK] = 0;
    for (int i = 0; i < 6; i++)
        allPieces[BLACK] |= pieces[BLACK][i];

    epCaptureFile = _epCaptureFile;
    playerToMove = _playerToMove;
    moveNumber = _moveNumber;
    castlingRights = 0;
    if (_whiteCanKCastle)
        castlingRights |= WHITEKSIDE;
    if (_whiteCanQCastle)
        castlingRights |= WHITEQSIDE;
    if (_blackCanKCastle)
        castlingRights |= BLACKKSIDE;
    if (_blackCanQCastle)
        castlingRights |= BLACKQSIDE;
    fiftyMoveCounter = _fiftyMoveCounter;
    initZobristKey(mailboxBoard);
}

Board::~Board() {}

Board Board::staticCopy() {
    Board b;
    std::memcpy(static_cast<void*>(&b), this, sizeof(Board));
    return b;
}


//------------------------------------------------------------------------------
//---------------------------------Do Move--------------------------------------
//------------------------------------------------------------------------------
/**
 * @brief Updates the board and Zobrist keys with Move m.
 */
void Board::doMove(Move m, int color) {
    int startSq = getStartSq(m);
    int endSq = getEndSq(m);
    int pieceID = getPieceOnSquare(color, startSq);

    // Update flag based elements of Zobrist key
    zobristKey ^= zobristTable[769 + castlingRights];
    zobristKey ^= zobristTable[785 + epCaptureFile];


    if (isPromotion(m)) {
        int promotionType = getPromotion(m);
        if (isCapture(m)) {
            int captureType = getPieceOnSquare(color^1, endSq);
            pieces[color][PAWNS] &= ~indexToBit(startSq);
            pieces[color][promotionType] |= indexToBit(endSq);
            pieces[color^1][captureType] &= ~indexToBit(endSq);

            allPieces[color] &= ~indexToBit(startSq);
            allPieces[color] |= indexToBit(endSq);
            allPieces[color^1] &= ~indexToBit(endSq);

            zobristKey ^= zobristTable[384*color + startSq];
            zobristKey ^= zobristTable[384*color + 64*promotionType + endSq];
            zobristKey ^= zobristTable[384*(color^1) + 64*captureType + endSq];
        }
        else {
            pieces[color][PAWNS] &= ~indexToBit(startSq);
            pieces[color][promotionType] |= indexToBit(endSq);

            allPieces[color] &= ~indexToBit(startSq);
            allPieces[color] |= indexToBit(endSq);

            zobristKey ^= zobristTable[384*color + startSq];
            zobristKey ^= zobristTable[384*color + 64*promotionType + endSq];
        }
        epCaptureFile = NO_EP_POSSIBLE;
        fiftyMoveCounter = 0;
    } // end promotion
    else if (isCapture(m)) {
        if (isEP(m)) {
            pieces[color][PAWNS] &= ~indexToBit(startSq);
            pieces[color][PAWNS] |= indexToBit(endSq);
            uint64_t epCaptureSq = indexToBit(epVictimSquare(color^1, epCaptureFile));
            pieces[color^1][PAWNS] &= ~epCaptureSq;

            allPieces[color] &= ~indexToBit(startSq);
            allPieces[color] |= indexToBit(endSq);
            allPieces[color^1] &= ~epCaptureSq;

            int capSq = epVictimSquare(color^1, epCaptureFile);
            zobristKey ^= zobristTable[384*color + startSq];
            zobristKey ^= zobristTable[384*color + endSq];
            zobristKey ^= zobristTable[384*(color^1) + capSq];
        }
        else {
            int captureType = getPieceOnSquare(color^1, endSq);
            pieces[color][pieceID] &= ~indexToBit(startSq);
            pieces[color][pieceID] |= indexToBit(endSq);
            pieces[color^1][captureType] &= ~indexToBit(endSq);

            allPieces[color] &= ~indexToBit(startSq);
            allPieces[color] |= indexToBit(endSq);
            allPieces[color^1] &= ~indexToBit(endSq);

            zobristKey ^= zobristTable[384*color + 64*pieceID + startSq];
            zobristKey ^= zobristTable[384*color + 64*pieceID + endSq];
            zobristKey ^= zobristTable[384*(color^1) + 64*captureType + endSq];
        }
        epCaptureFile = NO_EP_POSSIBLE;
        fiftyMoveCounter = 0;
    } // end capture
    else { // Quiet moves
        if (isCastle(m)) {
            if (endSq == 6) { // white kside
                pieces[WHITE][KINGS] &= ~indexToBit(4);
                pieces[WHITE][KINGS] |= indexToBit(6);
                pieces[WHITE][ROOKS] &= ~indexToBit(7);
                pieces[WHITE][ROOKS] |= indexToBit(5);

                allPieces[WHITE] &= ~indexToBit(4);
                allPieces[WHITE] |= indexToBit(6);
                allPieces[WHITE] &= ~indexToBit(7);
                allPieces[WHITE] |= indexToBit(5);

                zobristKey ^= zobristTable[64*KINGS+4];
                zobristKey ^= zobristTable[64*KINGS+6];
                zobristKey ^= zobristTable[64*ROOKS+7];
                zobristKey ^= zobristTable[64*ROOKS+5];
            }
            else if (endSq == 2) { // white qside
                pieces[WHITE][KINGS] &= ~indexToBit(4);
                pieces[WHITE][KINGS] |= indexToBit(2);
                pieces[WHITE][ROOKS] &= ~indexToBit(0);
                pieces[WHITE][ROOKS] |= indexToBit(3);

                allPieces[WHITE] &= ~indexToBit(4);
                allPieces[WHITE] |= indexToBit(2);
                allPieces[WHITE] &= ~indexToBit(0);
                allPieces[WHITE] |= indexToBit(3);

                zobristKey ^= zobristTable[64*KINGS+4];
                zobristKey ^= zobristTable[64*KINGS+2];
                zobristKey ^= zobristTable[64*ROOKS+0];
                zobristKey ^= zobristTable[64*ROOKS+3];
            }
            else if (endSq == 62) { // black kside
                pieces[BLACK][KINGS] &= ~indexToBit(60);
                pieces[BLACK][KINGS] |= indexToBit(62);
                pieces[BLACK][ROOKS] &= ~indexToBit(63);
                pieces[BLACK][ROOKS] |= indexToBit(61);

                allPieces[BLACK] &= ~indexToBit(60);
                allPieces[BLACK] |= indexToBit(62);
                allPieces[BLACK] &= ~indexToBit(63);
                allPieces[BLACK] |= indexToBit(61);

                zobristKey ^= zobristTable[384+64*KINGS+60];
                zobristKey ^= zobristTable[384+64*KINGS+62];
                zobristKey ^= zobristTable[384+64*ROOKS+63];
                zobristKey ^= zobristTable[384+64*ROOKS+61];
            }
            else { // black qside
                pieces[BLACK][KINGS] &= ~indexToBit(60);
                pieces[BLACK][KINGS] |= indexToBit(58);
                pieces[BLACK][ROOKS] &= ~indexToBit(56);
                pieces[BLACK][ROOKS] |= indexToBit(59);

                allPieces[BLACK] &= ~indexToBit(60);
                allPieces[BLACK] |= indexToBit(58);
                allPieces[BLACK] &= ~indexToBit(56);
                allPieces[BLACK] |= indexToBit(59);

                zobristKey ^= zobristTable[384+64*KINGS+60];
                zobristKey ^= zobristTable[384+64*KINGS+58];
                zobristKey ^= zobristTable[384+64*ROOKS+56];
                zobristKey ^= zobristTable[384+64*ROOKS+59];
            }
            epCaptureFile = NO_EP_POSSIBLE;
            fiftyMoveCounter++;
        } // end castling
        else { // other quiet moves
            pieces[color][pieceID] &= ~indexToBit(startSq);
            pieces[color][pieceID] |= indexToBit(endSq);

            allPieces[color] &= ~indexToBit(startSq);
            allPieces[color] |= indexToBit(endSq);

            zobristKey ^= zobristTable[384*color + 64*pieceID + startSq];
            zobristKey ^= zobristTable[384*color + 64*pieceID + endSq];

            // check for en passant
            if (pieceID == PAWNS) {
                if (getFlags(m) == MOVE_DOUBLE_PAWN)
                    epCaptureFile = startSq & 7;
                else
                    epCaptureFile = NO_EP_POSSIBLE;

                fiftyMoveCounter = 0;
            }
            else {
                epCaptureFile = NO_EP_POSSIBLE;
                fiftyMoveCounter++;
            }
        } // end other quiet moves
    } // end normal move

    // change castling flags
    if (pieceID == KINGS) {
        if (color == WHITE)
            castlingRights &= ~WHITECASTLE;
        else
            castlingRights &= ~BLACKCASTLE;
    }
    // Castling rights change because of the rook only when the rook moves or
    // is captured
    else if (isCapture(m) || pieceID == ROOKS) {
        // No sense in removing the rights if they're already gone
        if (castlingRights & WHITECASTLE) {
            if ((pieces[WHITE][ROOKS] & 0x80) == 0)
                castlingRights &= ~WHITEKSIDE;
            if ((pieces[WHITE][ROOKS] & 1) == 0)
                castlingRights &= ~WHITEQSIDE;
        }
        if (castlingRights & BLACKCASTLE) {
            uint64_t blackR = pieces[BLACK][ROOKS] >> 56;
            if ((blackR & 0x80) == 0)
                castlingRights &= ~BLACKKSIDE;
            if ((blackR & 0x1) == 0)
                castlingRights &= ~BLACKQSIDE;
        }
    } // end castling flags

    zobristKey ^= zobristTable[769 + castlingRights];
    zobristKey ^= zobristTable[785 + epCaptureFile];

    if (color == BLACK)
        moveNumber++;
    playerToMove = color^1;
    zobristKey ^= zobristTable[768];
}

bool Board::doPseudoLegalMove(Move m, int color) {
    doMove(m, color);
    // Pseudo-legal moves require a check for legality
    return !(isInCheck(color));
}

// Do a hash move, which requires a few more checks in case of a Type-1 error.
bool Board::doHashMove(Move m, int color) {
    int pieceID = getPieceOnSquare(color, getStartSq(m));
    // Check that the start square is not empty
    if (pieceID == -1)
        return false;

    // Check that the end square has correct occupancy
    uint64_t otherPieces = allPieces[color^1];
    uint64_t endSingle = indexToBit(getEndSq(m));
    bool captureRoutes = (isCapture(m) && (otherPieces & endSingle))
                      || (isCapture(m) && pieceID == PAWNS && (~otherPieces & endSingle));
    uint64_t empty = ~getOccupancy();
    if (!(captureRoutes || (!isCapture(m) && (empty & endSingle))))
        return false;
    // Check that the king is not captured
    if (isCapture(m) && ((endSingle & pieces[WHITE][KINGS]) || (endSingle & pieces[BLACK][KINGS])))
        return false;

    return doPseudoLegalMove(m, color);
}

// Handle null moves for null move pruning by switching the player to move.
void Board::doNullMove() {
    playerToMove = playerToMove ^ 1;
    zobristKey ^= zobristTable[768];
    zobristKey ^= zobristTable[785 + epCaptureFile];
    epCaptureFile = NO_EP_POSSIBLE;
    zobristKey ^= zobristTable[785 + epCaptureFile];
}

void Board::undoNullMove(uint16_t _epCaptureFile) {
    playerToMove = playerToMove ^ 1;
    zobristKey ^= zobristTable[768];
    zobristKey ^= zobristTable[785 + epCaptureFile];
    epCaptureFile = _epCaptureFile;
    zobristKey ^= zobristTable[785 + epCaptureFile];
}


//------------------------------------------------------------------------------
//-------------------------------Move Generation--------------------------------
//------------------------------------------------------------------------------
/*
 * @brief Returns a list of structs containing the piece type, start square,
 * and a bitboard of potential legal moves for each knight, bishop, rook, and queen.
 * Used for mobility evaluation of pieces.
 */
PieceMoveList Board::getPieceMoveList(int color) {
    PieceMoveList pml;

    uint64_t knights = pieces[color][KNIGHTS];
    while (knights) {
        int stSq = bitScanForward(knights);
        knights &= knights-1;
        uint64_t nSq = getKnightSquares(stSq);

        pml.add(PieceMoveInfo(KNIGHTS, stSq, nSq));
    }

    pml.starts[BISHOPS] = pml.size();
    uint64_t occ = allPieces[color^1] | pieces[color][PAWNS] | pieces[color][KNIGHTS] | pieces[color][KINGS];
    uint64_t bishops = pieces[color][BISHOPS];
    while (bishops) {
        int stSq = bitScanForward(bishops);
        bishops &= bishops-1;
        uint64_t bSq = getBishopSquares(stSq, occ | pieces[color][ROOKS]);

        pml.add(PieceMoveInfo(BISHOPS, stSq, bSq));
    }

    pml.starts[ROOKS] = pml.size();
    uint64_t rooks = pieces[color][ROOKS];
    while (rooks) {
        int stSq = bitScanForward(rooks);
        rooks &= rooks-1;
        uint64_t rSq = getRookSquares(stSq, occ | pieces[color][BISHOPS]);

        pml.add(PieceMoveInfo(ROOKS, stSq, rSq));
    }

    pml.starts[QUEENS] = pml.size();
    uint64_t queens = pieces[color][QUEENS];
    while (queens) {
        int stSq = bitScanForward(queens);
        queens &= queens-1;
        uint64_t qSq = getQueenSquares(stSq, occ);

        pml.add(PieceMoveInfo(QUEENS, stSq, qSq));
    }

    return pml;
}

// Get all legal moves and captures
MoveList Board::getAllLegalMoves(int color) {
    MoveList moves;
    getAllPseudoLegalMoves(moves, color);

    for (unsigned int i = 0; i < moves.size(); i++) {
        Board b = staticCopy();
        b.doMove(moves.get(i), color);

        if (b.isInCheck(color)) {
            moves.remove(i);
            i--;
        }
    }

    return moves;
}

//------------------------------Pseudo-legal Moves------------------------------
/* Pseudo-legal moves disregard whether the player's king is left in check
 * The pseudo-legal move and capture generators all follow a similar scheme:
 * Bitscan to obtain the square number for each piece (a1 is 0, a2 is 1, h8 is 63).
 * Get the legal moves as a bitboard, then bitscan this to get the destination
 * square and store as a Move object.
 */
void Board::getAllPseudoLegalMoves(MoveList &legalMoves, int color) {
    getPseudoLegalCaptures(legalMoves, color, true);
    getPseudoLegalQuiets(legalMoves, color);
}

/*
 * Quiet moves are generated in the following order:
 * Castling
 * Knight moves
 * Bishop moves
 * Rook moves
 * Queen moves
 * Pawn moves
 * King moves
 */
void Board::getPseudoLegalQuiets(MoveList &quiets, int color) {
    addCastlesToList(quiets, color);

    addPieceMovesToList<MOVEGEN_QUIETS>(quiets, color);

    addPawnMovesToList(quiets, color);

    int stsqK = bitScanForward(pieces[color][KINGS]);
    uint64_t kingSqs = getKingSquares(stsqK);
    addMovesToList<MOVEGEN_QUIETS>(quiets, stsqK, kingSqs);
}

/*
 * Do not include promotions for quiescence search, include promotions in normal search.
 * Captures are generated in the following order:
 * King captures
 * Pawn captures
 * Knight captures
 * Bishop captures
 * Rook captures
 * Queen captures
 */
void Board::getPseudoLegalCaptures(MoveList &captures, int color, bool includePromotions) {
    uint64_t otherPieces = allPieces[color^1];

    int kingStSq = bitScanForward(pieces[color][KINGS]);
    uint64_t kingSqs = getKingSquares(kingStSq);
    addMovesToList<MOVEGEN_CAPTURES>(captures, kingStSq, kingSqs, otherPieces);

    addPawnCapturesToList(captures, color, otherPieces, includePromotions);

    addPieceMovesToList<MOVEGEN_CAPTURES>(captures, color, otherPieces);
}

// Generates all queen promotions for quiescence search
void Board::getPseudoLegalPromotions(MoveList &moves, int color) {
    uint64_t otherPieces = allPieces[color^1];

    uint64_t pawns = pieces[color][PAWNS];
    uint64_t finalRank = (color == WHITE) ? RANK_8 : RANK_1;

    int leftDiff = (color == WHITE) ? -7 : 9;
    int rightDiff = (color == WHITE) ? -9 : 7;

    uint64_t legal = (color == WHITE) ? getWPawnLeftCaptures(pawns)
                                      : getBPawnLeftCaptures(pawns);
    legal &= otherPieces;
    uint64_t promotions = legal & finalRank;

    while (promotions) {
        int endSq = bitScanForward(promotions);
        promotions &= promotions-1;

        Move mq = encodeMove(endSq+leftDiff, endSq);
        mq = setCapture(mq, true);
        mq = setFlags(mq, MOVE_PROMO_Q);
        moves.add(mq);
    }

    legal = (color == WHITE) ? getWPawnRightCaptures(pawns)
                             : getBPawnRightCaptures(pawns);
    legal &= otherPieces;
    promotions = legal & finalRank;

    while (promotions) {
        int endSq = bitScanForward(promotions);
        promotions &= promotions-1;

        Move mq = encodeMove(endSq+rightDiff, endSq);
        mq = setCapture(mq, true);
        mq = setFlags(mq, MOVE_PROMO_Q);
        moves.add(mq);
    }

    int sqDiff = (color == WHITE) ? -8 : 8;

    legal = (color == WHITE) ? getWPawnSingleMoves(pawns)
                             : getBPawnSingleMoves(pawns);
    promotions = legal & finalRank;

    while (promotions) {
        int endSq = bitScanForward(promotions);
        promotions &= promotions - 1;
        int stSq = endSq + sqDiff;

        Move mq = encodeMove(stSq, endSq);
        mq = setFlags(mq, MOVE_PROMO_Q);
        moves.add(mq);
    }
}

/*
 * Get all pseudo-legal non-capture checks for a position. Used in quiescence search.
 * This function can be optimized compared to a normal getLegalMoves() because
 * for each piece, we can intersect the legal moves of the piece with the attack
 * map of this piece to the opposing king square to determine direct checks.
 * Discovered checks have to be handled separately.
 *
 * For simplicity, promotions and en passant are left out of this function.
 */
void Board::getPseudoLegalChecks(MoveList &checks, int color) {
    int kingSq = bitScanForward(pieces[color^1][KINGS]);
    // Square parity for knight and bishop moves
    uint64_t kingParity = (pieces[color^1][KINGS] & LIGHT) ? LIGHT : DARK;
    uint64_t potentialXRay = pieces[color][BISHOPS] | pieces[color][ROOKS] | pieces[color][QUEENS];

    // We can do pawns in parallel, since the start square of a pawn move is
    // determined by its end square.
    uint64_t pawns = pieces[color][PAWNS];

    // First, deal with discovered checks
    // TODO this is way too slow
    /*
    uint64_t tempPawns = pawns;
    while (tempPawns) {
        int stsq = bitScanForward(tempPawns);
        tempPawns &= tempPawns - 1;
        uint64_t xrays = getXRayPieceMap(color, kingSq, indexToBit(stsq));
        // If moving the pawn caused a new xray piece to attack the king
        if (!(xrays & invAttackMap)) {
            // Every legal move of this pawn is a legal check
            uint64_t legal = (color == WHITE) ? getWPawnSingleMoves(indexToBit(stsq)) | getWPawnDoubleMoves(indexToBit(stsq))
                                              : getBPawnSingleMoves(indexToBit(stsq)) | getBPawnDoubleMoves(indexToBit(stsq));
            while (legal) {
                int endsq = bitScanForward(legal);
                legal &= legal - 1;
                checks.add(encodeMove(stsq, endsq, PAWNS, false));
            }

            // Remove this pawn from future consideration
            pawns ^= indexToBit(stsq);
        }
    }
    */

    uint64_t pAttackMap = (color == WHITE)
            ? getBPawnCaptures(indexToBit(kingSq))
            : getWPawnCaptures(indexToBit(kingSq));
    uint64_t finalRank = (color == WHITE) ? RANK_8 : RANK_1;
    int sqDiff = (color == WHITE) ? -8 : 8;

    uint64_t pLegal = (color == WHITE) ? getWPawnSingleMoves(pawns)
                                       : getBPawnSingleMoves(pawns);
    // Remove promotions
    uint64_t promotions = pLegal & finalRank;
    pLegal ^= promotions;

    pLegal &= pAttackMap;
    while (pLegal) {
        int endsq = bitScanForward(pLegal);
        pLegal &= pLegal - 1;
        checks.add(encodeMove(endsq+sqDiff, endsq));
    }

    pLegal = (color == WHITE) ? getWPawnDoubleMoves(pawns)
                              : getBPawnDoubleMoves(pawns);
    pLegal &= pAttackMap;
    while (pLegal) {
        int endsq = bitScanForward(pLegal);
        pLegal &= pLegal - 1;
        Move m = encodeMove(endsq+2*sqDiff, endsq);
        m = setFlags(m, MOVE_DOUBLE_PAWN);
        checks.add(m);
    }

    uint64_t knights = pieces[color][KNIGHTS] & kingParity;
    uint64_t nAttackMap = getKnightSquares(kingSq);
    while (knights) {
        int stsq = bitScanForward(knights);
        knights &= knights-1;
        uint64_t nSq = getKnightSquares(stsq);
        // Get any bishops, rooks, queens attacking king after knight has moved
        uint64_t xrays = getXRayPieceMap(color, kingSq, indexToBit(stsq), 0);
        // If still no xrayers are giving check, then we have no discovered
        // check. Otherwise, every move by this piece is a (discovered) checking
        // move
        if (!(xrays & potentialXRay))
            nSq &= nAttackMap;

        addMovesToList<MOVEGEN_QUIETS>(checks, stsq, nSq);
    }

    uint64_t occ = getOccupancy();
    uint64_t bishops = pieces[color][BISHOPS] & kingParity;
    uint64_t bAttackMap = getBishopSquares(kingSq, occ);
    while (bishops) {
        int stsq = bitScanForward(bishops);
        bishops &= bishops-1;
        uint64_t bSq = getBishopSquares(stsq, occ);
        uint64_t xrays = getXRayPieceMap(color, kingSq, indexToBit(stsq), 0);
        if (!(xrays & potentialXRay))
            bSq &= bAttackMap;

        addMovesToList<MOVEGEN_QUIETS>(checks, stsq, bSq);
    }

    uint64_t rooks = pieces[color][ROOKS];
    uint64_t rAttackMap = getRookSquares(kingSq, occ);
    while (rooks) {
        int stsq = bitScanForward(rooks);
        rooks &= rooks-1;
        uint64_t rSq = getRookSquares(stsq, occ);
        uint64_t xrays = getXRayPieceMap(color, kingSq, indexToBit(stsq), 0);
        if (!(xrays & potentialXRay))
            rSq &= rAttackMap;

        addMovesToList<MOVEGEN_QUIETS>(checks, stsq, rSq);
    }

    uint64_t queens = pieces[color][QUEENS];
    uint64_t qAttackMap = getQueenSquares(kingSq, occ);
    while (queens) {
        int stsq = bitScanForward(queens);
        queens &= queens-1;
        uint64_t qSq = getQueenSquares(stsq, occ) & qAttackMap;

        addMovesToList<MOVEGEN_QUIETS>(checks, stsq, qSq);
    }
}

// Generate moves that (sort of but not really) get out of check
// This can only be used if we know the side to move is in check
// Optimizations include looking for double check (king moves only),
// otherwise we can only capture the checker or block if it is an xray piece
void Board::getPseudoLegalCheckEscapes(MoveList &escapes, int color) {
    int kingSq = bitScanForward(pieces[color][KINGS]);
    uint64_t otherPieces = allPieces[color^1];
    uint64_t attackMap = getAttackMap(color^1, kingSq);
    // Consider only captures of pieces giving check
    otherPieces &= attackMap;

    // If double check, we can only move the king
    if (count(otherPieces) >= 2) {
        uint64_t kingSqs = getKingSquares(kingSq);

        addMovesToList<MOVEGEN_CAPTURES>(escapes, kingSq, kingSqs, allPieces[color^1]);
        addMovesToList<MOVEGEN_QUIETS>(escapes, kingSq, kingSqs);
        return;
    }

    addPawnCapturesToList(escapes, color, otherPieces, true);

    uint64_t occ = getOccupancy();
    // If bishops, rooks, or queens, get bitboard of attack path so we
    // can intersect with legal moves to get legal block moves
    uint64_t xraySqs = 0;
    int attackerSq = bitScanForward(otherPieces);
    int attackerType = getPieceOnSquare(color^1, attackerSq);
    if (attackerType == BISHOPS)
        xraySqs = getBishopSquares(attackerSq, occ);
    else if (attackerType == ROOKS)
        xraySqs = getRookSquares(attackerSq, occ);
    else if (attackerType == QUEENS)
        xraySqs = getQueenSquares(attackerSq, occ);

    addPieceMovesToList<MOVEGEN_CAPTURES>(escapes, color, otherPieces);

    int stsqK = bitScanForward(pieces[color][KINGS]);
    uint64_t kingSqs = getKingSquares(stsqK);
    addMovesToList<MOVEGEN_CAPTURES>(escapes, stsqK, kingSqs, allPieces[color^1]);

    addPawnMovesToList(escapes, color);
    uint64_t knights = pieces[color][KNIGHTS];
    while (knights) {
        int stSq = bitScanForward(knights);
        knights &= knights-1;
        uint64_t nSq = getKnightSquares(stSq);

        addMovesToList<MOVEGEN_QUIETS>(escapes, stSq, nSq & xraySqs);
    }

    uint64_t bishops = pieces[color][BISHOPS];
    while (bishops) {
        int stSq = bitScanForward(bishops);
        bishops &= bishops-1;
        uint64_t bSq = getBishopSquares(stSq, occ);

        addMovesToList<MOVEGEN_QUIETS>(escapes, stSq, bSq & xraySqs);
    }

    uint64_t rooks = pieces[color][ROOKS];
    while (rooks) {
        int stSq = bitScanForward(rooks);
        rooks &= rooks-1;
        uint64_t rSq = getRookSquares(stSq, occ);

        addMovesToList<MOVEGEN_QUIETS>(escapes, stSq, rSq & xraySqs);
    }

    uint64_t queens = pieces[color][QUEENS];
    while (queens) {
        int stSq = bitScanForward(queens);
        queens &= queens-1;
        uint64_t qSq = getQueenSquares(stSq, occ);

        addMovesToList<MOVEGEN_QUIETS>(escapes, stSq, qSq & xraySqs);
    }

    addMovesToList<MOVEGEN_QUIETS>(escapes, stsqK, kingSqs);
}

//------------------------------------------------------------------------------
//---------------------------Move Generation Helpers----------------------------
//------------------------------------------------------------------------------
// We can do pawns in parallel, since the start square of a pawn move is
// determined by its end square.
void Board::addPawnMovesToList(MoveList &quiets, int color) {
    uint64_t pawns = pieces[color][PAWNS];
    uint64_t finalRank = (color == WHITE) ? RANK_8 : RANK_1;
    int sqDiff = (color == WHITE) ? -8 : 8;

    uint64_t pLegal = (color == WHITE) ? getWPawnSingleMoves(pawns)
                                       : getBPawnSingleMoves(pawns);
    // Promotions occur when a pawn reaches the final rank
    uint64_t promotions = pLegal & finalRank;
    pLegal ^= promotions;

    while (promotions) {
        int endSq = bitScanForward(promotions);
        promotions &= promotions - 1;
        int stSq = endSq + sqDiff;

        addPromotionsToList<MOVEGEN_QUIETS>(quiets, stSq, endSq);
    }
    while (pLegal) {
        int endsq = bitScanForward(pLegal);
        pLegal &= pLegal - 1;
        quiets.add(encodeMove(endsq+sqDiff, endsq));
    }

    pLegal = (color == WHITE) ? getWPawnDoubleMoves(pawns)
                              : getBPawnDoubleMoves(pawns);
    while (pLegal) {
        int endsq = bitScanForward(pLegal);
        pLegal &= pLegal - 1;
        Move m = encodeMove(endsq+2*sqDiff, endsq);
        m = setFlags(m, MOVE_DOUBLE_PAWN);
        quiets.add(m);
    }
}

// For pawn captures, we can use a similar approach, but we must consider
// left-hand and right-hand captures separately so we can tell which
// pawn is doing the capturing.
void Board::addPawnCapturesToList(MoveList &captures, int color, uint64_t otherPieces, bool includePromotions) {
    uint64_t pawns = pieces[color][PAWNS];
    uint64_t finalRank = (color == WHITE) ? RANK_8 : RANK_1;
    int leftDiff = (color == WHITE) ? -7 : 9;
    int rightDiff = (color == WHITE) ? -9 : 7;

    uint64_t legal = (color == WHITE) ? getWPawnLeftCaptures(pawns)
                                      : getBPawnLeftCaptures(pawns);
    legal &= otherPieces;
    uint64_t promotions = legal & finalRank;
    legal ^= promotions;

    if (includePromotions) {
        while (promotions) {
            int endSq = bitScanForward(promotions);
            promotions &= promotions-1;

            addPromotionsToList<MOVEGEN_CAPTURES>(captures, endSq+leftDiff, endSq);
        }
    }
    while (legal) {
        int endsq = bitScanForward(legal);
        legal &= legal-1;
        Move m = encodeMove(endsq+leftDiff, endsq);
        m = setCapture(m, true);
        captures.add(m);
    }

    legal = (color == WHITE) ? getWPawnRightCaptures(pawns)
                             : getBPawnRightCaptures(pawns);
    legal &= otherPieces;
    promotions = legal & finalRank;
    legal ^= promotions;

    if (includePromotions) {
        while (promotions) {
            int endSq = bitScanForward(promotions);
            promotions &= promotions-1;

            addPromotionsToList<MOVEGEN_CAPTURES>(captures, endSq+rightDiff, endSq);
        }
    }
    while (legal) {
        int endsq = bitScanForward(legal);
        legal &= legal-1;
        Move m = encodeMove(endsq+rightDiff, endsq);
        m = setCapture(m, true);
        captures.add(m);
    }

    // If there are en passants possible...
    if (epCaptureFile != NO_EP_POSSIBLE) {
        int victimSq = epVictimSquare(color^1, epCaptureFile);
        // capturer's destination square is either the rank above (white) or
        // below (black) the victim square
        int rankDiff = (color == WHITE) ? 8 : -8;
        // The capturer's start square is either 1 to the left or right of victim
        if ((indexToBit(victimSq) << 1) & NOTA & pieces[color][PAWNS]) {
            Move m = encodeMove(victimSq+1, victimSq+rankDiff);
            m = setFlags(m, MOVE_EP);
            captures.add(m);
        }
        if ((indexToBit(victimSq) >> 1) & NOTH & pieces[color][PAWNS]) {
            Move m = encodeMove(victimSq-1, victimSq+rankDiff);
            m = setFlags(m, MOVE_EP);
            captures.add(m);
        }
    }
}

template <bool isCapture>
void Board::addPieceMovesToList(MoveList &moves, int color, uint64_t otherPieces) {
    uint64_t knights = pieces[color][KNIGHTS];
    while (knights) {
        int stSq = bitScanForward(knights);
        knights &= knights-1;
        uint64_t nSq = getKnightSquares(stSq);

        addMovesToList<isCapture>(moves, stSq, nSq, otherPieces);
    }

    uint64_t occ = getOccupancy();
    uint64_t bishops = pieces[color][BISHOPS];
    while (bishops) {
        int stSq = bitScanForward(bishops);
        bishops &= bishops-1;
        uint64_t bSq = getBishopSquares(stSq, occ);

        addMovesToList<isCapture>(moves, stSq, bSq, otherPieces);
    }

    uint64_t rooks = pieces[color][ROOKS];
    while (rooks) {
        int stSq = bitScanForward(rooks);
        rooks &= rooks-1;
        uint64_t rSq = getRookSquares(stSq, occ);

        addMovesToList<isCapture>(moves, stSq, rSq, otherPieces);
    }

    uint64_t queens = pieces[color][QUEENS];
    while (queens) {
        int stSq = bitScanForward(queens);
        queens &= queens-1;
        uint64_t qSq = getQueenSquares(stSq, occ);

        addMovesToList<isCapture>(moves, stSq, qSq, otherPieces);
    }
}

// Helper function that processes a bitboard of legal moves and adds all
// moves into a list.
template <bool isCapture>
void Board::addMovesToList(MoveList &moves, int stSq, uint64_t allEndSqs,
    uint64_t otherPieces) {

    uint64_t intersect = (isCapture) ? otherPieces : ~getOccupancy();
    uint64_t legal = allEndSqs & intersect;
    while (legal) {
        int endSq = bitScanForward(legal);
        legal &= legal-1;
        Move m = encodeMove(stSq, endSq);
        if (isCapture)
            m = setCapture(m, true);
        moves.add(m);
    }
}

template <bool isCapture>
void Board::addPromotionsToList(MoveList &moves, int stSq, int endSq) {
    Move mk = encodeMove(stSq, endSq);
    mk = setFlags(mk, MOVE_PROMO_N);
    Move mb = encodeMove(stSq, endSq);
    mb = setFlags(mb, MOVE_PROMO_B);
    Move mr = encodeMove(stSq, endSq);
    mr = setFlags(mr, MOVE_PROMO_R);
    Move mq = encodeMove(stSq, endSq);
    mq = setFlags(mq, MOVE_PROMO_Q);
    if (isCapture) {
        mk = setCapture(mk, true);
        mb = setCapture(mb, true);
        mr = setCapture(mr, true);
        mq = setCapture(mq, true);
    }
    // Order promotions queen, knight, rook, bishop
    moves.add(mq);
    moves.add(mk);
    moves.add(mr);
    moves.add(mb);
}

void Board::addCastlesToList(MoveList &moves, int color) {
    // Add all possible castles
    if (color == WHITE) {
        // If castling rights still exist, squares in between king and rook are
        // empty, and player is not in check
        if ((castlingRights & WHITEKSIDE)
         && (getOccupancy() & WHITE_KSIDE_PASSTHROUGH_SQS) == 0
         && !isInCheck(WHITE)) {
            // Check for castling through check
            if (getAttackMap(BLACK, 5) == 0) {
                Move m = encodeMove(4, 6);
                m = setCastle(m, true);
                moves.add(m);
            }
        }
        if ((castlingRights & WHITEQSIDE)
         && (getOccupancy() & WHITE_QSIDE_PASSTHROUGH_SQS) == 0
         && !isInCheck(WHITE)) {
            if (getAttackMap(BLACK, 3) == 0) {
                Move m = encodeMove(4, 2);
                m = setCastle(m, true);
                moves.add(m);
            }
        }
    }
    else {
        if ((castlingRights & BLACKKSIDE)
         && (getOccupancy() & BLACK_KSIDE_PASSTHROUGH_SQS) == 0
         && !isInCheck(BLACK)) {
            if (getAttackMap(WHITE, 61) == 0) {
                Move m = encodeMove(60, 62);
                m = setCastle(m, true);
                moves.add(m);
            }
        }
        if ((castlingRights & BLACKQSIDE)
         && (getOccupancy() & BLACK_QSIDE_PASSTHROUGH_SQS) == 0
         && !isInCheck(BLACK)) {
            if (getAttackMap(WHITE, 59) == 0) {
                Move m = encodeMove(60, 58);
                m = setCastle(m, true);
                moves.add(m);
            }
        }
    }
}


//------------------------------------------------------------------------------
//-----------------------Useful bitboard info generators:-----------------------
//------------------------------attack maps, etc.-------------------------------
//------------------------------------------------------------------------------

// Get the attack map of all potential x-ray pieces (bishops, rooks, queens)
// after a blocker has been removed.
uint64_t Board::getXRayPieceMap(int color, int sq, uint64_t blockerStart, uint64_t blockerEnd) {
    uint64_t occ = getOccupancy();
    occ &= ~blockerStart;
    occ |= blockerEnd;

    uint64_t bishops = pieces[color][BISHOPS];
    uint64_t rooks = pieces[color][ROOKS];
    uint64_t queens = pieces[color][QUEENS];

    uint64_t xRayMap = (getBishopSquares(sq, occ) & (bishops | queens))
                     | (getRookSquares(sq, occ) & (rooks | queens));

    return (xRayMap & ~blockerStart);
}

// Get the attack map of all potential x-ray pieces (bishops, rooks, queens)
// with no blockers removed. Used to compare with the results of the function
// above to find pins/discovered checks
/*uint64_t Board::getInitXRays(int color, int sq) {
    uint64_t bishops = pieces[color][BISHOPS];
    uint64_t rooks = pieces[color][ROOKS];
    uint64_t queens = pieces[color][QUEENS];

    return (getBishopSquares(sq) & (bishops | queens))
         | (getRookSquares(sq) & (rooks | queens));
}*/

// Given a color and a square, returns all pieces of the color that attack the
// square. Useful for checks, captures
uint64_t Board::getAttackMap(int color, int sq) {
    uint64_t occ = getOccupancy();
    uint64_t pawnCap = (color == WHITE)
                     ? getBPawnCaptures(indexToBit(sq))
                     : getWPawnCaptures(indexToBit(sq));
    return (pawnCap & pieces[color][PAWNS])
         | (getKnightSquares(sq) & pieces[color][KNIGHTS])
         | (getBishopSquares(sq, occ) & (pieces[color][BISHOPS] | pieces[color][QUEENS]))
         | (getRookSquares(sq, occ) & (pieces[color][ROOKS] | pieces[color][QUEENS]))
         | (getKingSquares(sq) & pieces[color][KINGS]);
}

// Get all pieces of both colors attacking a square
uint64_t Board::getAttackMap(int sq) {
    uint64_t occ = getOccupancy();
    return (getBPawnCaptures(indexToBit(sq)) & pieces[WHITE][PAWNS])
         | (getWPawnCaptures(indexToBit(sq)) & pieces[BLACK][PAWNS])
         | (getKnightSquares(sq) & (pieces[WHITE][KNIGHTS] | pieces[BLACK][KNIGHTS]))
         | (getBishopSquares(sq, occ) & (pieces[WHITE][BISHOPS] | pieces[WHITE][QUEENS] | pieces[BLACK][BISHOPS] | pieces[BLACK][QUEENS]))
         | (getRookSquares(sq, occ) & (pieces[WHITE][ROOKS] | pieces[WHITE][QUEENS] | pieces[BLACK][ROOKS] | pieces[BLACK][QUEENS]))
         | (getKingSquares(sq) & (pieces[WHITE][KINGS] | pieces[BLACK][KINGS]));
}

// Given the on a given square, used to get either the piece moving or the
// captured piece.
int Board::getPieceOnSquare(int color, int sq) {
    uint64_t endSingle = indexToBit(sq);
    for (int pieceID = 0; pieceID <= 5; pieceID++) {
        if (pieces[color][pieceID] & endSingle)
            return pieceID;
    }
    // If used for captures, the default of an empty square indicates an
    // en passant (and hopefully not an error).
    return -1;
}

// Returns true if a move puts the opponent in check
bool Board::isCheckMove(int color, Move m) {
    int kingSq = bitScanForward(pieces[color^1][KINGS]);

    // Special case for castling
    if (isCastle(m)) {
        uint64_t attackMap = getRookSquares(kingSq, getOccupancy() ^ indexToBit(getStartSq(m)));
        int rookEnd = 0;
        switch (getEndSq(m)) {
            case 6: // white kside
                rookEnd = 5;
                break;
            case 2: // white qside
                rookEnd = 3;
                break;
            case 62: // black kside
                rookEnd = 61;
                break;
            case 58: // black qside
                rookEnd = 59;
                break;
            default:
                return false;
                break;
        }
        return (bool) (indexToBit(rookEnd) & attackMap);
    }

    // See if move is a direct check
    uint64_t attackMap = 0;
    uint64_t occ = getOccupancy() ^ indexToBit(getStartSq(m));
    uint64_t pieceID = isPromotion(m) ? getPromotion(m)
                                      : getPieceOnSquare(color, getStartSq(m));
    switch (pieceID) {
        case PAWNS:
            attackMap = (color == WHITE)
                ? getBPawnCaptures(indexToBit(kingSq))
                : getWPawnCaptures(indexToBit(kingSq));
            break;
        case KNIGHTS:
            attackMap = getKnightSquares(kingSq);
            break;
        case BISHOPS:
            attackMap = getBishopSquares(kingSq, occ);
            break;
        case ROOKS:
            attackMap = getRookSquares(kingSq, occ);
            break;
        case QUEENS:
            attackMap = getQueenSquares(kingSq, occ);
            break;
        case KINGS:
            // keep attackMap 0
            break;
    }
    if (indexToBit(getEndSq(m)) & attackMap)
        return true;

    // See if move is a discovered check
    // Get a bitboard of all pieces that could possibly xray
    uint64_t xrayPieces = pieces[color][BISHOPS] | pieces[color][ROOKS] | pieces[color][QUEENS];

    // The piece that has moved is no longer blocking xray pieces
    uint64_t removedBlockers = indexToBit(getStartSq(m));
    if (isEP(m))
        removedBlockers |= indexToBit(epVictimSquare(color^1, epCaptureFile));

    // Get any bishops, rooks, queens attacking king after piece has moved
    uint64_t xrays = getXRayPieceMap(color, kingSq, removedBlockers, indexToBit(getEndSq(m)));
    // If there is an xray piece attacking the king square after the piece has
    // moved, we have discovered check
    return (bool) (xrays & xrayPieces);
}

/*
 * These two functions return all squares x-rayed by a single rook or bishop, i.e.
 * all squares between the first and second blocker.
 * Algorithm from http://chessprogramming.wikispaces.com/X-ray+Attacks+%28Bitboards%29#ModifyingOccupancy
 */
uint64_t Board::getRookXRays(int sq, uint64_t occ, uint64_t blockers) {
    uint64_t attacks = getRookSquares(sq, occ);
    blockers &= attacks;
    return attacks ^ getRookSquares(sq, occ ^ blockers);
}

uint64_t Board::getBishopXRays(int sq, uint64_t occ, uint64_t blockers) {
    uint64_t attacks = getBishopSquares(sq, occ);
    blockers &= attacks;
    return attacks ^ getBishopSquares(sq, occ ^ blockers);
}

/*
 * This function returns all pinned pieces for a given side.
 * Algorithm from http://chessprogramming.wikispaces.com/Checks+and+Pinned+Pieces+%28Bitboards%29
 */
uint64_t Board::getPinnedMap(int color) {
    uint64_t pinned = 0;
    uint64_t blockers = allPieces[color];
    int kingSq = bitScanForward(pieces[color][KINGS]);

    uint64_t pinners = getRookXRays(kingSq, getOccupancy(), blockers)
        & (pieces[color^1][ROOKS] | pieces[color^1][QUEENS]);
    while (pinners) {
        int sq  = bitScanForward(pinners);
        pinned |= inBetweenSqs[sq][kingSq] & blockers;
        pinners &= pinners - 1;
    }

    pinners = getBishopXRays(kingSq, getOccupancy(), blockers)
        & (pieces[color^1][BISHOPS] | pieces[color^1][QUEENS]);
    while (pinners) {
        int sq  = bitScanForward(pinners);
        pinned |= inBetweenSqs[sq][kingSq] & blockers;
        pinners &= pinners - 1;
    }

    return pinned;
}


//------------------------------------------------------------------------------
//-------------------King: check, draw, insufficient material-------------------
//------------------------------------------------------------------------------
bool Board::isInCheck(int color) {
    int sq = bitScanForward(pieces[color][KINGS]);

    return getAttackMap(color^1, sq);
}

bool Board::isDraw() {
    if (fiftyMoveCounter >= 100) return true;

    if (isInsufficientMaterial())
        return true;

    return false;
}

// Check for guaranteed drawn positions, where helpmate is not possible.
bool Board::isInsufficientMaterial() {
    int numPieces = count(allPieces[WHITE] | allPieces[BLACK]) - 2;
    if (numPieces < 2) {
        if (numPieces == 0)
            return true;
        if (pieces[WHITE][KNIGHTS] || pieces[WHITE][BISHOPS]
         || pieces[BLACK][KNIGHTS] || pieces[BLACK][BISHOPS])
            return true;
    }
    return false;
}

void Board::getCheckMaps(int color, uint64_t *checkMaps) {
    int kingSq = bitScanForward(pieces[color][KINGS]);
    uint64_t occ = getOccupancy();
    checkMaps[KNIGHTS-1] = getKnightSquares(kingSq);
    checkMaps[BISHOPS-1] = getBishopSquares(kingSq, occ);
    checkMaps[ROOKS-1] = getRookSquares(kingSq, occ);
    checkMaps[QUEENS-1] = getQueenSquares(kingSq, occ);
}


//------------------------------------------------------------------------------
//--------------------------------Move Ordering---------------------------------
//------------------------------------------------------------------------------
int Board::getMaterial(int color) {
    int material = 0;
    for (int pieceID = PAWNS; pieceID <= QUEENS; pieceID++)
        material += PIECE_VALUES[MG][pieceID] * count(pieces[color][pieceID]);
    return material;
}

uint64_t Board::getNonPawnMaterial(int color) {
    return pieces[color][KNIGHTS] | pieces[color][BISHOPS]
         | pieces[color][ROOKS]   | pieces[color][QUEENS];
}

// Given a bitboard of attackers, finds the least valuable attacker of color and
// returns a single occupancy bitboard of that piece
uint64_t Board::getLeastValuableAttacker(uint64_t attackers, int color, int &piece) {
    for (piece = 0; piece < 5; piece++) {
        uint64_t single = attackers & pieces[color][piece];
        if (single)
            return single & -single;
    }

    piece = KINGS;
    return attackers & pieces[color][KINGS];
}

// Static exchange evaluation algorithm from
// https://chessprogramming.wikispaces.com/SEE+-+The+Swap+Algorithm
int Board::getSEE(int color, int sq) {
    int gain[32], d = 0, piece = 0;
    uint64_t attackers = getAttackMap(sq);
    // used attackers that may act as blockers for x-ray pieces
    uint64_t used = 0;

    // Get the first attacker
    uint64_t single = getLeastValuableAttacker(attackers, color, piece);
    // If there are no attackers, return immediately
    if (!single)
        return 0;
    // Get value of piece initially being captured. Should never be empty.
    gain[d] = SEE_PIECE_VALS[getPieceOnSquare(color^1, sq)];

    do {
        d++; // next depth
        color ^= 1;
        gain[d] = SEE_PIECE_VALS[piece] - gain[d-1];
        if (-gain[d-1] < 0 && gain[d] < 0) // pruning for stand pat
            break;
        attackers ^= single; // remove used attacker
        used |= single;
        attackers |= getXRayPieceMap(WHITE, sq, used, 0) | getXRayPieceMap(BLACK, sq, used, 0);
        single = getLeastValuableAttacker(attackers, color, piece);
    } while (single);

    // Minimax results
    while (--d)
        gain[d-1]= -((-gain[d-1] > gain[d]) ? -gain[d-1] : gain[d]);

    return gain[0];
}

/**
 * @brief This function calculates a SEE for a move rather than a square, by
 * forcing the initial move (to avoid standing pat if the move is bad).
 */
int Board::getSEEForMove(int color, Move m) {
    int value = 0;
    int startSq = getStartSq(m);
    int endSq = getEndSq(m);
    int pieceID = getPieceOnSquare(color, startSq);

    // TODO temporary hack for EP captures
    if (isEP(m))
        return 0;

    // Do the move temporarily
    pieces[color][pieceID] &= ~indexToBit(startSq);
    pieces[color][pieceID] |= indexToBit(endSq);
    allPieces[color] &= ~indexToBit(startSq);
    allPieces[color] |= indexToBit(endSq);

    // For a capture, we also need to update the captured piece on the board
    if (isCapture(m)) {
        int capturedPiece = getPieceOnSquare(color^1, endSq);
        pieces[color^1][capturedPiece] &= ~indexToBit(endSq);
        allPieces[color^1] &= ~indexToBit(endSq);

        value = SEE_PIECE_VALS[capturedPiece] - getSEE(color^1, endSq);

        pieces[color^1][capturedPiece] |= indexToBit(endSq);
        allPieces[color^1] |= indexToBit(endSq);
    }
    else {
        value = -getSEE(color^1, endSq);
    }

    // Undo the move
    pieces[color][pieceID] |= indexToBit(startSq);
    pieces[color][pieceID] &= ~indexToBit(endSq);
    allPieces[color] |= indexToBit(startSq);
    allPieces[color] &= ~indexToBit(endSq);

    return value;
}

int Board::valueOfPiece(int pieceID) {
    switch(pieceID) {
        // EP capture
        case -1:
            return PIECE_VALUES[MG][PAWNS];
        case PAWNS:
        case KNIGHTS:
        case BISHOPS:
        case ROOKS:
        case QUEENS:
            return PIECE_VALUES[MG][pieceID];
        case KINGS:
            return MATE_SCORE / 2;
    }

    return -1;
}

// Calculates a score for Most Valuable Victim / Least Valuable Attacker
// capture ordering.
int Board::getMVVLVAScore(int color, Move m) {
    int endSq = getEndSq(m);
    int attacker = getPieceOnSquare(color, getStartSq(m));
    if (attacker == KINGS)
        attacker = -1;
    int victim = getPieceOnSquare(color^1, endSq);

    return (victim * 8) + (4 - attacker);
}

// Returns a score from the initial capture
// This helps reduce the number of times SEE must be used in quiescence search,
// since if we have a losing trade after capture-recapture our opponent could
// likely stand pat, or we could've captured with a better piece
int Board::getExchangeScore(int color, Move m) {
    int endSq = getEndSq(m);
    int attacker = getPieceOnSquare(color, getStartSq(m));
    if (attacker == KINGS)
        attacker = -1;
    int victim = getPieceOnSquare(color^1, endSq);
    return victim - attacker;
}


//------------------------------------------------------------------------------
//-----------------------------Move Generation----------------------------------
//------------------------------------------------------------------------------
inline uint64_t Board::getWPawnSingleMoves(uint64_t pawns) {
    return (pawns << 8) & ~getOccupancy();
}

inline uint64_t Board::getBPawnSingleMoves(uint64_t pawns) {
    return (pawns >> 8) & ~getOccupancy();
}

uint64_t Board::getWPawnDoubleMoves(uint64_t pawns) {
    uint64_t open = ~getOccupancy();
    uint64_t temp = (pawns << 8) & open;
    pawns = (temp << 8) & open & RANK_4;
    return pawns;
}

uint64_t Board::getBPawnDoubleMoves(uint64_t pawns) {
    uint64_t open = ~getOccupancy();
    uint64_t temp = (pawns >> 8) & open;
    pawns = (temp >> 8) & open & RANK_5;
    return pawns;
}

inline uint64_t Board::getWPawnLeftCaptures(uint64_t pawns) {
    return (pawns << 7) & NOTH;
}

inline uint64_t Board::getBPawnLeftCaptures(uint64_t pawns) {
    return (pawns >> 9) & NOTH;
}

inline uint64_t Board::getWPawnRightCaptures(uint64_t pawns) {
    return (pawns << 9) & NOTA;
}

inline uint64_t Board::getBPawnRightCaptures(uint64_t pawns) {
    return (pawns >> 7) & NOTA;
}

uint64_t Board::getWPawnCaptures(uint64_t pawns) {
    return getWPawnLeftCaptures(pawns) | getWPawnRightCaptures(pawns);
}

uint64_t Board::getBPawnCaptures(uint64_t pawns) {
    return getBPawnLeftCaptures(pawns) | getBPawnRightCaptures(pawns);
}

inline uint64_t Board::getKnightSquares(int single) {
    return knightAttacks(single);
}

uint64_t Board::getBishopSquares(int single, uint64_t occ) {
    return bishopAttacks(single, occ);
}

uint64_t Board::getRookSquares(int single, uint64_t occ) {
    return rookAttacks(single, occ);
}

uint64_t Board::getQueenSquares(int single, uint64_t occ) {
    return queenAttacks(single, occ);
}

uint64_t Board::getKingSquares(int single) {
    return kingAttacks(single);
}

inline uint64_t Board::getOccupancy() {
    return allPieces[WHITE] | allPieces[BLACK];
}

inline int Board::epVictimSquare(int victimColor, uint16_t file) {
    return 8 * (3 + victimColor) + file;
}


//------------------------------------------------------------------------------
//---------------------------Public Getter Methods------------------------------
//------------------------------------------------------------------------------
bool Board::getWhiteCanKCastle() {
    return castlingRights & WHITEKSIDE;
}

bool Board::getBlackCanKCastle() {
    return castlingRights & BLACKKSIDE;
}

bool Board::getWhiteCanQCastle() {
    return castlingRights & WHITEQSIDE;
}

bool Board::getBlackCanQCastle() {
    return castlingRights & BLACKQSIDE;
}

bool Board::getAnyCanCastle() {
    return (castlingRights != 0);
}

uint16_t Board::getEPCaptureFile() {
    return epCaptureFile;
}

uint8_t Board::getFiftyMoveCounter() {
    return fiftyMoveCounter;
}

uint8_t Board::getCastlingRights() {
    return castlingRights;
}

uint16_t Board::getMoveNumber() {
    return moveNumber;
}

int Board::getPlayerToMove() {
    return playerToMove;
}

uint64_t Board::getPieces(int color, int piece) {
    return pieces[color][piece];
}

uint64_t Board::getAllPieces(int color) {
    return allPieces[color];
}

int *Board::getMailbox() {
    int *result = new int[64];
    for (int i = 0; i < 64; i++) {
        result[i] = -1;
    }
    for (int i = 0; i < 6; i++) {
        uint64_t bitboard = pieces[WHITE][i];
        while (bitboard) {
            result[bitScanForward(bitboard)] = i;
            bitboard &= bitboard - 1;
        }
    }
    for (int i = 0; i < 6; i++) {
        uint64_t bitboard = pieces[BLACK][i];
        while (bitboard) {
            result[bitScanForward(bitboard)] = 6 + i;
            bitboard &= bitboard - 1;
        }
    }
    return result;
}

uint64_t Board::getZobristKey() {
    return zobristKey;
}

void Board::initZobristKey(int *mailbox) {
    zobristKey = 0;
    for (int i = 0; i < 64; i++) {
        if (mailbox[i] != -1) {
            zobristKey ^= zobristTable[mailbox[i] * 64 + i];
        }
    }
    if (playerToMove == BLACK)
        zobristKey ^= zobristTable[768];
    zobristKey ^= zobristTable[769 + castlingRights];
    zobristKey ^= zobristTable[785 + epCaptureFile];
}
