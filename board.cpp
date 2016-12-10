/*
    Laser, a UCI chess engine written in C++11.
    Copyright 2015-2016 Jeffrey An and Michael An

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
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include "board.h"
#include "bbinit.h"
#include "eval.h"


const uint64_t WHITE_KSIDE_PASSTHROUGH_SQS = INDEX_TO_BIT[5] | INDEX_TO_BIT[6];
const uint64_t WHITE_QSIDE_PASSTHROUGH_SQS = INDEX_TO_BIT[1] | INDEX_TO_BIT[2] | INDEX_TO_BIT[3];
const uint64_t BLACK_KSIDE_PASSTHROUGH_SQS = INDEX_TO_BIT[61] | INDEX_TO_BIT[62];
const uint64_t BLACK_QSIDE_PASSTHROUGH_SQS = INDEX_TO_BIT[57] | INDEX_TO_BIT[58] | INDEX_TO_BIT[59];

static int KS_TO_SCORE[200];

void initKSArray() {
    for (int i = 0; i < 200; i++) {
        double x = (double) i;
        KS_TO_SCORE[i] = (int) std::min(x * x / KS_ARRAY_FACTOR, 500.0);
    }
}

// Zobrist hashing table and the start position key, both initialized at startup
static uint64_t zobristTable[794];
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

// Magic tables, initialized in bbinit.cpp
extern uint64_t *attackTable;
extern MagicInfo magicBishops[64];
extern MagicInfo magicRooks[64];

// Precalculated bitboard tables
extern uint64_t inBetweenSqs[64][64];


struct EvalDebug {
    int totalEval;
    int totalMaterialMg, totalMaterialEg;
    int whiteMobilityMg, whiteMobilityEg, blackMobilityMg, blackMobilityEg;
    int whiteKingSafety, blackKingSafety;
    Score whitePawnScore, blackPawnScore;

    EvalDebug() {
        clear();
    }

    void clear() {
        totalEval = 0;
        totalMaterialMg = totalMaterialEg = 0;
        whiteMobilityMg = whiteMobilityEg = blackMobilityMg = blackMobilityEg = 0;
        whiteKingSafety = blackKingSafety = 0;
        whitePawnScore = blackPawnScore = EVAL_ZERO;
    }

    void print() {
        std::cerr << std::endl;
        std::cerr << "    Component     |      White      |      Black      |      Total" << std::endl;
        std::cerr << "                  |    MG     EG    |    MG     EG    |    MG     EG" << std::endl;
        std::cerr << std::string(70, '-') << std::endl;
        std::cerr << "    Material      |   "
                  << " -- " << "   "
                  << " -- " << "   |   "
                  << " -- " << "   "
                  << " -- " << "   |   "
                  << std::setw(4) << S(totalMaterialMg) << "   "
                  << std::setw(4) << S(totalMaterialEg) << std::endl;
        std::cerr << "    Mobility      |   "
                  << std::setw(4) << S(whiteMobilityMg) << "   "
                  << std::setw(4) << S(whiteMobilityEg) << "   |   "
                  << std::setw(4) << S(blackMobilityMg) << "   "
                  << std::setw(4) << S(blackMobilityEg) << std::endl;
        std::cerr << "    King Safety   |   "
                  << std::setw(4) << S(whiteKingSafety) << "   "
                  << " -- " << "   |   "
                  << std::setw(4) << S(blackKingSafety) << "   "
                  << " -- " << std::endl;
        std::cerr << "    Pawns         |   "
                  << std::setw(4) << S(decEvalMg(whitePawnScore)) << "   "
                  << std::setw(4) << S(decEvalEg(whitePawnScore)) << "   |   "
                  << std::setw(4) << S(decEvalMg(blackPawnScore)) << "   "
                  << std::setw(4) << S(decEvalEg(blackPawnScore)) << std::endl;
        std::cerr << std::string(70, '-') << std::endl;
        std::cerr << "Static evaluation: " << S(totalEval) << std::endl;
        std::cerr << std::endl;
    }

    // Scales the internal score representation into centipawns
    int S(int v) {
        return (int) (v * 100 / PIECE_VALUES[EG][PAWNS]);
    }
};

static EvalDebug evalDebugStats;


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
            pieces[mailboxBoard[i]/6][mailboxBoard[i]%6] |= INDEX_TO_BIT[i];
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
    std::memcpy(&b, this, sizeof(Board));
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
            pieces[color][PAWNS] &= ~INDEX_TO_BIT[startSq];
            pieces[color][promotionType] |= INDEX_TO_BIT[endSq];
            pieces[color^1][captureType] &= ~INDEX_TO_BIT[endSq];

            allPieces[color] &= ~INDEX_TO_BIT[startSq];
            allPieces[color] |= INDEX_TO_BIT[endSq];
            allPieces[color^1] &= ~INDEX_TO_BIT[endSq];

            zobristKey ^= zobristTable[384*color + startSq];
            zobristKey ^= zobristTable[384*color + 64*promotionType + endSq];
            zobristKey ^= zobristTable[384*(color^1) + 64*captureType + endSq];
        }
        else {
            pieces[color][PAWNS] &= ~INDEX_TO_BIT[startSq];
            pieces[color][promotionType] |= INDEX_TO_BIT[endSq];

            allPieces[color] &= ~INDEX_TO_BIT[startSq];
            allPieces[color] |= INDEX_TO_BIT[endSq];

            zobristKey ^= zobristTable[384*color + startSq];
            zobristKey ^= zobristTable[384*color + 64*promotionType + endSq];
        }
        epCaptureFile = NO_EP_POSSIBLE;
        fiftyMoveCounter = 0;
    } // end promotion
    else if (isCapture(m)) {
        if (isEP(m)) {
            pieces[color][PAWNS] &= ~INDEX_TO_BIT[startSq];
            pieces[color][PAWNS] |= INDEX_TO_BIT[endSq];
            uint64_t epCaptureSq = INDEX_TO_BIT[epVictimSquare(color^1, epCaptureFile)];
            pieces[color^1][PAWNS] &= ~epCaptureSq;

            allPieces[color] &= ~INDEX_TO_BIT[startSq];
            allPieces[color] |= INDEX_TO_BIT[endSq];
            allPieces[color^1] &= ~epCaptureSq;

            int capSq = epVictimSquare(color^1, epCaptureFile);
            zobristKey ^= zobristTable[384*color + startSq];
            zobristKey ^= zobristTable[384*color + endSq];
            zobristKey ^= zobristTable[384*(color^1) + capSq];
        }
        else {
            int captureType = getPieceOnSquare(color^1, endSq);
            pieces[color][pieceID] &= ~INDEX_TO_BIT[startSq];
            pieces[color][pieceID] |= INDEX_TO_BIT[endSq];
            pieces[color^1][captureType] &= ~INDEX_TO_BIT[endSq];

            allPieces[color] &= ~INDEX_TO_BIT[startSq];
            allPieces[color] |= INDEX_TO_BIT[endSq];
            allPieces[color^1] &= ~INDEX_TO_BIT[endSq];

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
                pieces[WHITE][KINGS] &= ~INDEX_TO_BIT[4];
                pieces[WHITE][KINGS] |= INDEX_TO_BIT[6];
                pieces[WHITE][ROOKS] &= ~INDEX_TO_BIT[7];
                pieces[WHITE][ROOKS] |= INDEX_TO_BIT[5];

                allPieces[WHITE] &= ~INDEX_TO_BIT[4];
                allPieces[WHITE] |= INDEX_TO_BIT[6];
                allPieces[WHITE] &= ~INDEX_TO_BIT[7];
                allPieces[WHITE] |= INDEX_TO_BIT[5];

                zobristKey ^= zobristTable[64*KINGS+4];
                zobristKey ^= zobristTable[64*KINGS+6];
                zobristKey ^= zobristTable[64*ROOKS+7];
                zobristKey ^= zobristTable[64*ROOKS+5];
            }
            else if (endSq == 2) { // white qside
                pieces[WHITE][KINGS] &= ~INDEX_TO_BIT[4];
                pieces[WHITE][KINGS] |= INDEX_TO_BIT[2];
                pieces[WHITE][ROOKS] &= ~INDEX_TO_BIT[0];
                pieces[WHITE][ROOKS] |= INDEX_TO_BIT[3];

                allPieces[WHITE] &= ~INDEX_TO_BIT[4];
                allPieces[WHITE] |= INDEX_TO_BIT[2];
                allPieces[WHITE] &= ~INDEX_TO_BIT[0];
                allPieces[WHITE] |= INDEX_TO_BIT[3];

                zobristKey ^= zobristTable[64*KINGS+4];
                zobristKey ^= zobristTable[64*KINGS+2];
                zobristKey ^= zobristTable[64*ROOKS+0];
                zobristKey ^= zobristTable[64*ROOKS+3];
            }
            else if (endSq == 62) { // black kside
                pieces[BLACK][KINGS] &= ~INDEX_TO_BIT[60];
                pieces[BLACK][KINGS] |= INDEX_TO_BIT[62];
                pieces[BLACK][ROOKS] &= ~INDEX_TO_BIT[63];
                pieces[BLACK][ROOKS] |= INDEX_TO_BIT[61];

                allPieces[BLACK] &= ~INDEX_TO_BIT[60];
                allPieces[BLACK] |= INDEX_TO_BIT[62];
                allPieces[BLACK] &= ~INDEX_TO_BIT[63];
                allPieces[BLACK] |= INDEX_TO_BIT[61];

                zobristKey ^= zobristTable[384+64*KINGS+60];
                zobristKey ^= zobristTable[384+64*KINGS+62];
                zobristKey ^= zobristTable[384+64*ROOKS+63];
                zobristKey ^= zobristTable[384+64*ROOKS+61];
            }
            else { // black qside
                pieces[BLACK][KINGS] &= ~INDEX_TO_BIT[60];
                pieces[BLACK][KINGS] |= INDEX_TO_BIT[58];
                pieces[BLACK][ROOKS] &= ~INDEX_TO_BIT[56];
                pieces[BLACK][ROOKS] |= INDEX_TO_BIT[59];

                allPieces[BLACK] &= ~INDEX_TO_BIT[60];
                allPieces[BLACK] |= INDEX_TO_BIT[58];
                allPieces[BLACK] &= ~INDEX_TO_BIT[56];
                allPieces[BLACK] |= INDEX_TO_BIT[59];

                zobristKey ^= zobristTable[384+64*KINGS+60];
                zobristKey ^= zobristTable[384+64*KINGS+58];
                zobristKey ^= zobristTable[384+64*ROOKS+56];
                zobristKey ^= zobristTable[384+64*ROOKS+59];
            }
            epCaptureFile = NO_EP_POSSIBLE;
            fiftyMoveCounter++;
        } // end castling
        else { // other quiet moves
            pieces[color][pieceID] &= ~INDEX_TO_BIT[startSq];
            pieces[color][pieceID] |= INDEX_TO_BIT[endSq];

            allPieces[color] &= ~INDEX_TO_BIT[startSq];
            allPieces[color] |= INDEX_TO_BIT[endSq];

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
    uint64_t endSingle = INDEX_TO_BIT[getEndSq(m)];
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

    uint64_t occ = allPieces[color^1] | pieces[color][PAWNS] | pieces[color][KINGS];
    uint64_t bishops = pieces[color][BISHOPS];
    while (bishops) {
        int stSq = bitScanForward(bishops);
        bishops &= bishops-1;
        uint64_t bSq = getBishopSquares(stSq, occ);

        pml.add(PieceMoveInfo(BISHOPS, stSq, bSq));
    }

    uint64_t rooks = pieces[color][ROOKS];
    while (rooks) {
        int stSq = bitScanForward(rooks);
        rooks &= rooks-1;
        uint64_t rSq = getRookSquares(stSq, occ);

        pml.add(PieceMoveInfo(ROOKS, stSq, rSq));
    }

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
    uint64_t finalRank = (color == WHITE) ? RANKS[7] : RANKS[0];

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
        uint64_t xrays = getXRayPieceMap(color, kingSq, color, INDEX_TO_BIT[stsq]);
        // If moving the pawn caused a new xray piece to attack the king
        if (!(xrays & invAttackMap)) {
            // Every legal move of this pawn is a legal check
            uint64_t legal = (color == WHITE) ? getWPawnSingleMoves(INDEX_TO_BIT[stsq]) | getWPawnDoubleMoves(INDEX_TO_BIT[stsq])
                                              : getBPawnSingleMoves(INDEX_TO_BIT[stsq]) | getBPawnDoubleMoves(INDEX_TO_BIT[stsq]);
            while (legal) {
                int endsq = bitScanForward(legal);
                legal &= legal - 1;
                checks.add(encodeMove(stsq, endsq, PAWNS, false));
            }

            // Remove this pawn from future consideration
            pawns ^= INDEX_TO_BIT[stsq];
        }
    }
    */

    uint64_t pAttackMap = (color == WHITE)
            ? getBPawnCaptures(INDEX_TO_BIT[kingSq])
            : getWPawnCaptures(INDEX_TO_BIT[kingSq]);
    uint64_t finalRank = (color == WHITE) ? RANKS[7] : RANKS[0];
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
        uint64_t xrays = getXRayPieceMap(color, kingSq, color, INDEX_TO_BIT[stsq], 0);
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
        uint64_t xrays = getXRayPieceMap(color, kingSq, color, INDEX_TO_BIT[stsq], 0);
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
        uint64_t xrays = getXRayPieceMap(color, kingSq, color, INDEX_TO_BIT[stsq], 0);
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
    uint64_t finalRank = (color == WHITE) ? RANKS[7] : RANKS[0];
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
    uint64_t finalRank = (color == WHITE) ? RANKS[7] : RANKS[0];
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
        if ((INDEX_TO_BIT[victimSq] << 1) & NOTA & pieces[color][PAWNS]) {
            Move m = encodeMove(victimSq+1, victimSq+rankDiff);
            m = setFlags(m, MOVE_EP);
            captures.add(m);
        }
        if ((INDEX_TO_BIT[victimSq] >> 1) & NOTH & pieces[color][PAWNS]) {
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
uint64_t Board::getXRayPieceMap(int color, int sq, int blockerColor,
    uint64_t blockerStart, uint64_t blockerEnd) {
    uint64_t occ = getOccupancy();
    occ ^= blockerStart;
    occ ^= blockerEnd;

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
                     ? getBPawnCaptures(INDEX_TO_BIT[sq])
                     : getWPawnCaptures(INDEX_TO_BIT[sq]);
    return (pawnCap & pieces[color][PAWNS])
         | (getKnightSquares(sq) & pieces[color][KNIGHTS])
         | (getBishopSquares(sq, occ) & (pieces[color][BISHOPS] | pieces[color][QUEENS]))
         | (getRookSquares(sq, occ) & (pieces[color][ROOKS] | pieces[color][QUEENS]))
         | (getKingSquares(sq) & pieces[color][KINGS]);
}

// Given the on a given square, used to get either the piece moving or the
// captured piece.
int Board::getPieceOnSquare(int color, int sq) {
    uint64_t endSingle = INDEX_TO_BIT[sq];
    for (int pieceID = 0; pieceID <= 5; pieceID++) {
        if (pieces[color][pieceID] & endSingle)
            return pieceID;
    }
    // If used for captures, the default of an empty square indicates an
    // en passant (and hopefully not an error).
    return -1;
}

// Returns true if a move puts the opponent in check
// Precondition: opposing king is not already in check (obviously)
// Does not consider en passant or castling
bool Board::isCheckMove(int color, Move m) {
    int kingSq = bitScanForward(pieces[color^1][KINGS]);

    // See if move is a direct check
    uint64_t attackMap = 0;
    uint64_t occ = getOccupancy();
    switch (getPieceOnSquare(color, getStartSq(m))) {
        case PAWNS:
            attackMap = (color == WHITE)
                ? getBPawnCaptures(INDEX_TO_BIT[kingSq])
                : getWPawnCaptures(INDEX_TO_BIT[kingSq]);
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
    if (INDEX_TO_BIT[getEndSq(m)] & attackMap)
        return true;

    // See if move is a discovered check
    // Get a bitboard of all pieces that could possibly xray
    uint64_t xrayPieces = pieces[color][BISHOPS] | pieces[color][ROOKS] | pieces[color][QUEENS];

    // Get any bishops, rooks, queens attacking king after piece has moved
    uint64_t xrays = getXRayPieceMap(color, kingSq, color, INDEX_TO_BIT[getStartSq(m)], INDEX_TO_BIT[getEndSq(m)]);
    // If there is an xray piece attacking the king square after the piece has
    // moved, we have discovered check
    if (xrays & xrayPieces)
        return true;

    // If not direct or discovered check, then not a check
    return false;
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


//------------------------------------------------------------------------------
//------------------------Evaluation and Move Ordering--------------------------
//------------------------------------------------------------------------------

/*
 * Evaluates the current board position in hundredths of pawns. White is
 * positive and black is negative in traditional negamax format.
 */
template <bool debug>
int Board::evaluate() {
    // Precompute some values
    int pieceCounts[2][6];
    for (int color = 0; color < 2; color++) {
        for (int pieceID = 0; pieceID < 6; pieceID++)
            pieceCounts[color][pieceID] = count(pieces[color][pieceID]);
    }

    // Material
    int whiteMaterial = 0, blackMaterial = 0;
    for (int pieceID = PAWNS; pieceID <= QUEENS; pieceID++)
        whiteMaterial += PIECE_VALUES[MG][pieceID] * pieceCounts[WHITE][pieceID];
    for (int pieceID = PAWNS; pieceID <= QUEENS; pieceID++)
        blackMaterial += PIECE_VALUES[MG][pieceID] * pieceCounts[BLACK][pieceID];

    int whiteEGFactorMat = 0, blackEGFactorMat = 0;
    for (int pieceID = PAWNS; pieceID <= QUEENS; pieceID++)
        whiteEGFactorMat += EG_FACTOR_PIECE_VALS[pieceID] * pieceCounts[WHITE][pieceID];
    for (int pieceID = PAWNS; pieceID <= QUEENS; pieceID++)
        blackEGFactorMat += EG_FACTOR_PIECE_VALS[pieceID] * pieceCounts[BLACK][pieceID];

    // Compute endgame factor which is between 0 and EG_FACTOR_RES, inclusive
    int egFactor = EG_FACTOR_RES - (whiteEGFactorMat + blackEGFactorMat - EG_FACTOR_ALPHA) * EG_FACTOR_RES / EG_FACTOR_BETA;
    egFactor = std::max(0, std::min(EG_FACTOR_RES, egFactor));


    // Check for special endgames
    if (egFactor == EG_FACTOR_RES) {
        int endgameScore = checkEndgameCases();
        if (endgameScore != -INFTY)
            return endgameScore;
    }


    //---------------------------Material terms---------------------------------
    int valueMg = 0;
    int valueEg = 0;

    valueMg += whiteMaterial;
    valueMg -= blackMaterial;
    for (int pieceID = PAWNS; pieceID <= QUEENS; pieceID++)
        valueEg += PIECE_VALUES[EG][pieceID] * pieceCounts[WHITE][pieceID];
    for (int pieceID = PAWNS; pieceID <= QUEENS; pieceID++)
        valueEg -= PIECE_VALUES[EG][pieceID] * pieceCounts[BLACK][pieceID];

    // Tempo bonus
    valueMg += (playerToMove == WHITE) ? TEMPO_VALUE : -TEMPO_VALUE;

    // Bishop pair bonus
    if ((pieces[WHITE][BISHOPS] & LIGHT) && (pieces[WHITE][BISHOPS] & DARK)) {
        valueMg += BISHOP_PAIR_VALUE;
        valueEg += BISHOP_PAIR_VALUE;
    }
    if ((pieces[BLACK][BISHOPS] & LIGHT) && (pieces[BLACK][BISHOPS] & DARK)) {
        valueMg -= BISHOP_PAIR_VALUE;
        valueEg -= BISHOP_PAIR_VALUE;
    }


    // Piece square tables
    // White pieces
    for (int pieceID = 0; pieceID < 6; pieceID++) {
        uint64_t bitboard = pieces[WHITE][pieceID];
        // Invert the board for white side
        bitboard = flipAcrossRanks(bitboard);
        while (bitboard) {
            int sq = bitScanForward(bitboard);
            bitboard &= bitboard - 1;
            valueMg += getPSQTValue(MG, pieceID, sq);
            valueEg += getPSQTValue(EG, pieceID, sq);
        }
    }
    // Black pieces
    for (int pieceID = 0; pieceID < 6; pieceID++)  {
        uint64_t bitboard = pieces[BLACK][pieceID];
        while (bitboard) {
            int sq = bitScanForward(bitboard);
            bitboard &= bitboard - 1;
            valueMg -= getPSQTValue(MG, pieceID, sq);
            valueEg -= getPSQTValue(EG, pieceID, sq);
        }
    }


    // Pawn scaling: a penalty for having too few pawns
    valueMg -= PAWN_SCALING_MG[pieceCounts[WHITE][PAWNS]];
    valueEg -= PAWN_SCALING_EG[pieceCounts[WHITE][PAWNS]];
    valueMg += PAWN_SCALING_MG[pieceCounts[BLACK][PAWNS]];
    valueEg += PAWN_SCALING_EG[pieceCounts[BLACK][PAWNS]];

    // With queens on the board pawns are a target in the endgame
    if (pieces[WHITE][QUEENS])
        valueEg -= QUEEN_PAWN_PENALTY * pieceCounts[BLACK][PAWNS];
    if (pieces[BLACK][QUEENS])
        valueEg += QUEEN_PAWN_PENALTY * pieceCounts[WHITE][PAWNS];

    if (debug) {
        evalDebugStats.totalMaterialMg = valueMg;
        evalDebugStats.totalMaterialEg = valueEg;
    }


    //----------------------------Positional terms------------------------------
    // A 32-bit integer that holds both the midgame and endgame values using SWAR
    Score value = EVAL_ZERO;


    //-----------------------King Safety and Mobility---------------------------
    // Scores based on mobility and basic king safety (which is turned off in
    // the endgame)
    PieceMoveList pmlWhite = getPieceMoveList(WHITE);
    PieceMoveList pmlBlack = getPieceMoveList(BLACK);
    int whiteMobilityMg, whiteMobilityEg;
    int blackMobilityMg, blackMobilityEg;
    getPseudoMobility<WHITE>(pmlWhite, pmlBlack, whiteMobilityMg, whiteMobilityEg);
    getPseudoMobility<BLACK>(pmlBlack, pmlWhite, blackMobilityMg, blackMobilityEg);
    valueMg += whiteMobilityMg - blackMobilityMg;
    valueEg += whiteMobilityEg - blackMobilityEg;

    if (debug) {
        evalDebugStats.whiteMobilityMg = whiteMobilityMg;
        evalDebugStats.whiteMobilityEg = whiteMobilityEg;
        evalDebugStats.blackMobilityMg = blackMobilityMg;
        evalDebugStats.blackMobilityEg = blackMobilityEg;
    }


    // Consider squares near king
    int wKingSq = bitScanForward(pieces[WHITE][KINGS]);
    int bKingSq = bitScanForward(pieces[BLACK][KINGS]);
    uint64_t wKingNeighborhood = getKingSquares(wKingSq);
    uint64_t bKingNeighborhood = getKingSquares(bKingSq);

    // All king safety terms are midgame only, so don't calculate them in the endgame
    if (egFactor < EG_FACTOR_RES) {
        int wKsValue = 0, bKsValue = 0;

        // Pawn shield and storm values
        int wKingFile = wKingSq & 7;
        int bKingFile = bKingSq & 7;

        // White king
        for (int i = wKingFile-1; i <= wKingFile+1; i++) {
            if (i < 0 || i > 7)
                continue;
            int f = std::min(i, 7-i);

            uint64_t wPawnShield = pieces[WHITE][PAWNS] & FILES[i];
            if (wPawnShield) {
                int wPawnSq = bitScanForward(wPawnShield);
                int wr = wPawnSq >> 3;

                wKsValue += PAWN_SHIELD_VALUE[f][wr];
            }
            // Semi-open file: no pawn shield
            else
                wKsValue += PAWN_SHIELD_VALUE[f][0];

            uint64_t bPawnStorm = pieces[BLACK][PAWNS] & FILES[i];
            if (bPawnStorm) {
                int bPawnSq = bitScanForward(bPawnStorm);
                int br = bPawnSq >> 3;

                wKsValue -= PAWN_STORM_VALUE[
                    (pieces[WHITE][PAWNS] & FILES[i]) == 0                  ? 0 :
                    (pieces[WHITE][PAWNS] & INDEX_TO_BIT[bPawnSq - 8]) != 0 ? 1 : 2][f][br];
            }
            // Semi-open file: no pawn for attacker
            else
                wKsValue -= PAWN_STORM_VALUE[0][f][0];
        }

        // Black king
        for (int i = bKingFile-1; i <= bKingFile+1; i++) {
            if (i < 0 || i > 7)
                continue;
            int f = std::min(i, 7-i);

            uint64_t bPawnShield = pieces[BLACK][PAWNS] & FILES[i];
            if (bPawnShield) {
                int bPawnSq = bitScanReverse(bPawnShield);
                int br = 7 - (bPawnSq >> 3);

                bKsValue += PAWN_SHIELD_VALUE[f][br];
            }
            else
                bKsValue += PAWN_SHIELD_VALUE[f][0];

            uint64_t wPawnStorm = pieces[WHITE][PAWNS] & FILES[i];
            if (wPawnStorm) {
                int wPawnSq = bitScanReverse(wPawnStorm);
                int wr = 7 - (wPawnSq >> 3);

                bKsValue -= PAWN_STORM_VALUE[
                    (pieces[BLACK][PAWNS] & FILES[i]) == 0                  ? 0 :
                    (pieces[BLACK][PAWNS] & INDEX_TO_BIT[wPawnSq + 8]) != 0 ? 1 : 2][f][wr];
            }
            else
                bKsValue -= PAWN_STORM_VALUE[0][f][0];
        }

        // Piece attacks
        bKsValue -= getKingSafety<WHITE>(pmlWhite, pmlBlack, bKingNeighborhood, bKsValue);
        wKsValue -= getKingSafety<BLACK>(pmlBlack, pmlWhite, wKingNeighborhood, wKsValue);

        // Castling rights
        wKsValue += CASTLING_RIGHTS_VALUE[count(castlingRights & WHITECASTLE)];
        bKsValue += CASTLING_RIGHTS_VALUE[count(castlingRights & BLACKCASTLE)];

        valueMg += wKsValue - bKsValue;
        if (debug) {
            evalDebugStats.whiteKingSafety = wKsValue;
            evalDebugStats.blackKingSafety = bKsValue;
        }
    }


    // Current pawn attacks
    // Used for outposts and backwards pawns
    uint64_t wPawnAtt = getWPawnCaptures(pieces[WHITE][PAWNS]);
    uint64_t bPawnAtt = getBPawnCaptures(pieces[BLACK][PAWNS]);
    // Get all squares attackable by pawns in the future
    // Used for outposts and backwards pawns
    uint64_t wPawnFrontSpan = pieces[WHITE][PAWNS] << 8;
    uint64_t bPawnFrontSpan = pieces[BLACK][PAWNS] >> 8;
    for (int i = 0; i < 5; i++) {
        wPawnFrontSpan |= wPawnFrontSpan << 8;
        bPawnFrontSpan |= bPawnFrontSpan >> 8;
    }
    uint64_t wPawnStopAtt = ((wPawnFrontSpan >> 1) & NOTH) | ((wPawnFrontSpan << 1) & NOTA);
    uint64_t bPawnStopAtt = ((bPawnFrontSpan >> 1) & NOTH) | ((bPawnFrontSpan << 1) & NOTA);


    //------------------------------Minor Pieces--------------------------------
    // Bishops tend to be worse if too many pawns are on squares of their color
    if (pieces[WHITE][BISHOPS] & LIGHT) {
        value += BISHOP_PAWN_COLOR_PENALTY * count(pieces[WHITE][PAWNS] & LIGHT);
    }
    if (pieces[WHITE][BISHOPS] & DARK) {
        value += BISHOP_PAWN_COLOR_PENALTY * count(pieces[WHITE][PAWNS] & DARK);
    }
    if (pieces[BLACK][BISHOPS] & LIGHT) {
        value -= BISHOP_PAWN_COLOR_PENALTY * count(pieces[BLACK][PAWNS] & LIGHT);
    }
    if (pieces[BLACK][BISHOPS] & DARK) {
        value -= BISHOP_PAWN_COLOR_PENALTY * count(pieces[BLACK][PAWNS] & DARK);
    }

    // Knights do better when the opponent has many pawns
    value += KNIGHT_PAWN_BONUS * pieceCounts[WHITE][KNIGHTS] * pieceCounts[BLACK][PAWNS];
    value -= KNIGHT_PAWN_BONUS * pieceCounts[BLACK][KNIGHTS] * pieceCounts[WHITE][PAWNS];

    // Knight outposts: knights that cannot be attacked by opposing pawns
    const uint64_t RANKS_456 = RANKS[3] | RANKS[4] | RANKS[5];
    const uint64_t RANKS_543 = RANKS[4] | RANKS[3] | RANKS[2];
    const uint64_t FILES_CDEF = FILES[2] | FILES[3] | FILES[4] | FILES[5];
    if (uint64_t wOutpost = pieces[WHITE][KNIGHTS] & ~bPawnStopAtt & RANKS_456 & FILES_CDEF) {
        value += KNIGHT_OUTPOST_BONUS * count(wOutpost);
        // An additional bonus if the outpost knight is defended by a pawn
        value += KNIGHT_OUTPOST_PAWN_DEF_BONUS * count(wOutpost & wPawnAtt);
    }
    if (uint64_t bOutpost = pieces[BLACK][KNIGHTS] & ~wPawnStopAtt & RANKS_543 & FILES_CDEF) {
        value -= KNIGHT_OUTPOST_BONUS * count(bOutpost);
        value -= KNIGHT_OUTPOST_PAWN_DEF_BONUS * count(bOutpost & bPawnAtt);
    }

    // Bishop outposts
    if (uint64_t wOutpost = pieces[WHITE][BISHOPS] & ~bPawnStopAtt & RANKS_456 & FILES_CDEF) {
        value += BISHOP_OUTPOST_BONUS * count(wOutpost);
        value += BISHOP_OUTPOST_PAWN_DEF_BONUS * count(wOutpost & wPawnAtt);
    }
    if (uint64_t bOutpost = pieces[BLACK][BISHOPS] & ~wPawnStopAtt & RANKS_543 & FILES_CDEF) {
        value -= BISHOP_OUTPOST_BONUS * count(bOutpost);
        value -= BISHOP_OUTPOST_PAWN_DEF_BONUS * count(bOutpost & bPawnAtt);
    }

    // Special case: Nc3 blocking c2-c4 in closed openings (pawn on d4, no pawn on e4)
    if ((pieces[WHITE][KNIGHTS] & 0x40000)
     && (pieces[WHITE][PAWNS] & 0x400)
     && (pieces[WHITE][PAWNS] & 0x8000000)
    && !(pieces[WHITE][PAWNS] & 0x10000000))
        value += KNIGHT_C3_CLOSED_PENALTY;
    if ((pieces[BLACK][KNIGHTS] & 0x40000000000)
     && (pieces[BLACK][PAWNS] & 0x4000000000000)
     && (pieces[BLACK][PAWNS] & 0x800000000)
    && !(pieces[BLACK][PAWNS] & 0x1000000000))
        value -= KNIGHT_C3_CLOSED_PENALTY;


    //-------------------------------Rooks--------------------------------------
    // Bonus for having rooks on open files
    uint64_t wRooksOpen = pieces[WHITE][ROOKS];
    while (wRooksOpen) {
        int rookSq = bitScanForward(wRooksOpen);
        wRooksOpen &= wRooksOpen - 1;
        int file = rookSq & 7;

        if (!(FILES[file] & (pieces[WHITE][PAWNS] | pieces[BLACK][PAWNS])))
            value += ROOK_OPEN_FILE_BONUS;
        else if (!(FILES[file] & pieces[WHITE][PAWNS]))
            value += ROOK_SEMIOPEN_FILE_BONUS;
    }

    uint64_t bRooksOpen = pieces[BLACK][ROOKS];
    while (bRooksOpen) {
        int rookSq = bitScanForward(bRooksOpen);
        bRooksOpen &= bRooksOpen - 1;
        int file = rookSq & 7;

        if (!(FILES[file] & (pieces[WHITE][PAWNS] | pieces[BLACK][PAWNS])))
            value -= ROOK_OPEN_FILE_BONUS;
        else if (!(FILES[file] & pieces[BLACK][PAWNS]))
            value -= ROOK_SEMIOPEN_FILE_BONUS;
    }


    //----------------------------Pawn structure--------------------------------
    Score whitePawnScore = EVAL_ZERO, blackPawnScore = EVAL_ZERO;
    // Passed pawns
    uint64_t wPassedBlocker = pieces[BLACK][PAWNS] >> 8;
    uint64_t bPassedBlocker = pieces[WHITE][PAWNS] << 8;
    // If opposing pawns are on the same or an adjacent file on a pawn's front
    // span, then the pawn is not passed
    wPassedBlocker |= ((wPassedBlocker >> 1) & NOTH) | ((wPassedBlocker << 1) & NOTA);
    bPassedBlocker |= ((bPassedBlocker >> 1) & NOTH) | ((bPassedBlocker << 1) & NOTA);
    // Include own pawns as blockers to prevent doubled pawns from both being
    // scored as passers
    wPassedBlocker |= (pieces[WHITE][PAWNS] >> 8);
    bPassedBlocker |= (pieces[BLACK][PAWNS] << 8);
    // Find opposing pawn front spans
    for(int i = 0; i < 4; i++) {
        wPassedBlocker |= (wPassedBlocker >> 8);
        bPassedBlocker |= (bPassedBlocker << 8);
    }
    // Passers are pawns outside the opposing pawn front span
    uint64_t wPassedPawns = pieces[WHITE][PAWNS] & ~wPassedBlocker;
    uint64_t bPassedPawns = pieces[BLACK][PAWNS] & ~bPassedBlocker;

    // Get the overall attack maps
    uint64_t wAttackMap = 0, bAttackMap = 0;
    for (unsigned int i = 0; i < pmlWhite.size(); i++) {
        wAttackMap |= pmlWhite.get(i).legal;
    }
    wAttackMap |= getWPawnCaptures(pieces[WHITE][PAWNS]);
    for (unsigned int i = 0; i < pmlBlack.size(); i++) {
        bAttackMap |= pmlBlack.get(i).legal;
    }
    bAttackMap |= getBPawnCaptures(pieces[BLACK][PAWNS]);

    uint64_t wPasserTemp = wPassedPawns;
    uint64_t wBlock = allPieces[BLACK] | bAttackMap;
    while (wPasserTemp) {
        int passerSq = bitScanForward(wPasserTemp);
        wPasserTemp &= wPasserTemp - 1;
        int file = passerSq & 7;
        int rank = passerSq >> 3;
        whitePawnScore += PASSER_BONUS[rank];
        whitePawnScore += PASSER_FILE_BONUS[file];

        uint64_t pathToQueen = INDEX_TO_BIT[passerSq];
        pathToQueen |= pathToQueen << 8;
        pathToQueen |= pathToQueen << 16;
        pathToQueen |= pathToQueen << 32;

        // Non-linear bonus based on rank
        int rFactor = (rank-1) * (rank-2) / 2;
        // Path to queen is completely undefended by opponent
        if (!(pathToQueen & wBlock))
            whitePawnScore += rFactor * FREE_PROMOTION_BONUS;
        // Stop square is undefended by opponent
        else if (!((INDEX_TO_BIT[passerSq] << 8) & wBlock))
            whitePawnScore += rFactor * FREE_STOP_BONUS;
        // Path to queen is completely defended by own pieces
        if ((pathToQueen & wAttackMap) == pathToQueen)
            whitePawnScore += rFactor * FULLY_DEFENDED_PASSER_BONUS;
        // Stop square is defended by own pieces
        else if ((INDEX_TO_BIT[passerSq] << 8) & wAttackMap)
            whitePawnScore += rFactor * DEFENDED_PASSER_BONUS;

        // Bonuses and penalties for king distance
        whitePawnScore -= OWN_KING_DIST * getManhattanDistance(passerSq, wKingSq) * rFactor;
        whitePawnScore += OPP_KING_DIST * getManhattanDistance(passerSq, bKingSq) * rFactor;
    }
    uint64_t bPasserTemp = bPassedPawns;
    uint64_t bBlock = allPieces[WHITE] | wAttackMap;
    while (bPasserTemp) {
        int passerSq = bitScanForward(bPasserTemp);
        bPasserTemp &= bPasserTemp - 1;
        int file = passerSq & 7;
        int rank = 7 - (passerSq >> 3);
        blackPawnScore += PASSER_BONUS[rank];
        blackPawnScore += PASSER_FILE_BONUS[file];

        uint64_t pathToQueen = INDEX_TO_BIT[passerSq];
        pathToQueen |= pathToQueen >> 8;
        pathToQueen |= pathToQueen >> 16;
        pathToQueen |= pathToQueen >> 32;

        int rFactor = (rank-1) * (rank-2) / 2;
        if (!(pathToQueen & bBlock))
            blackPawnScore += rFactor * FREE_PROMOTION_BONUS;
        else if (!((INDEX_TO_BIT[passerSq] >> 8) & bBlock))
            blackPawnScore += rFactor * FREE_STOP_BONUS;
        if ((pathToQueen & bAttackMap) == pathToQueen)
            blackPawnScore += rFactor * FULLY_DEFENDED_PASSER_BONUS;
        else if ((INDEX_TO_BIT[passerSq] >> 8) & bAttackMap)
            blackPawnScore += rFactor * DEFENDED_PASSER_BONUS;

        blackPawnScore += OPP_KING_DIST * getManhattanDistance(passerSq, wKingSq) * rFactor;
        blackPawnScore -= OWN_KING_DIST * getManhattanDistance(passerSq, bKingSq) * rFactor;
    }

    int wPawnCtByFile[8];
    int bPawnCtByFile[8];
    for (int i = 0; i < 8; i++) {
        wPawnCtByFile[i] = count(pieces[WHITE][PAWNS] & FILES[i]);
        bPawnCtByFile[i] = count(pieces[BLACK][PAWNS] & FILES[i]);
    }

    // Doubled pawns
    for (int i = 0; i < 8; i++) {
        whitePawnScore += DOUBLED_PENALTY[wPawnCtByFile[i]] * DOUBLED_PENALTY_SCALE[pieceCounts[WHITE][PAWNS]];
        blackPawnScore += DOUBLED_PENALTY[bPawnCtByFile[i]] * DOUBLED_PENALTY_SCALE[pieceCounts[BLACK][PAWNS]];
    }

    // Isolated pawns
    // Fill a bitmap of which files have pawns
    uint64_t wp = 0, bp = 0;
    for (int i = 0; i < 8; i++) {
        wp |= (bool) (wPawnCtByFile[i]);
        bp |= (bool) (bPawnCtByFile[i]);
        wp <<= 1;
        bp <<= 1;
    }
    wp >>= 1;
    bp >>= 1;
    // If there are pawns on either adjacent file, we remove this pawn
    wp &= ~((wp >> 1) | (wp << 1));
    bp &= ~((bp >> 1) | (bp << 1));
    int whiteIsolated = count(wp);
    int blackIsolated = count(bp);
    whitePawnScore += ISOLATED_PENALTY * whiteIsolated;
    blackPawnScore += ISOLATED_PENALTY * blackIsolated;
    whitePawnScore += CENTRAL_ISOLATED_PENALTY * count(wp & 0x7E);
    blackPawnScore += CENTRAL_ISOLATED_PENALTY * count(bp & 0x7E);

    // Isolated, doubled pawns
    // x1 for isolated, doubled pawns
    // x3 for isolated, tripled pawns
    for (int i = 0; i < 8; i++) {
        if ((wPawnCtByFile[i] > 1) && (wp & INDEX_TO_BIT[7-i])) {
            whitePawnScore += ISOLATED_DOUBLED_PENALTY * (((wPawnCtByFile[i] - 1) * wPawnCtByFile[i]) / 2);
        }
        if ((bPawnCtByFile[i] > 1) && (bp & INDEX_TO_BIT[7-i])) {
            blackPawnScore += ISOLATED_DOUBLED_PENALTY * (((bPawnCtByFile[i] - 1) * bPawnCtByFile[i]) / 2);
        }
    }

    // Backward pawns
    uint64_t wBadStopSqs = ~wPawnStopAtt & bPawnAtt;
    uint64_t bBadStopSqs = ~bPawnStopAtt & wPawnAtt;
    for (int i = 0; i < 6; i++) {
        wBadStopSqs |= wBadStopSqs >> 8;
        bBadStopSqs |= bBadStopSqs << 8;
    }

    uint64_t wBackwards = wBadStopSqs & pieces[WHITE][PAWNS];
    uint64_t bBackwards = bBadStopSqs & pieces[BLACK][PAWNS];
    whitePawnScore += BACKWARD_PENALTY * count(wBackwards);
    blackPawnScore += BACKWARD_PENALTY * count(bBackwards);

    // Semi-open files with backwards pawns
    while (wBackwards) {
        int pawnSq = bitScanForward(wBackwards);
        wBackwards &= wBackwards - 1;
        int f = pawnSq & 7;
        if (!(FILES[f] & pieces[BLACK][PAWNS])) {
            whitePawnScore += BACKWARD_SEMIOPEN_PENALTY;
            // if (FILES[f] & pieces[BLACK][ROOKS])
            //     blackPawnScore += BACKWARD_SEMIOPEN_ROOK_BONUS;
        }
    }
    while (bBackwards) {
        int pawnSq = bitScanForward(bBackwards);
        bBackwards &= bBackwards - 1;
        int f = pawnSq & 7;
        if (!(FILES[f] & pieces[WHITE][PAWNS])) {
            blackPawnScore += BACKWARD_SEMIOPEN_PENALTY;
            // if (FILES[f] & pieces[WHITE][ROOKS])
            //     whitePawnScore += BACKWARD_SEMIOPEN_ROOK_BONUS;
        }
    }

    valueMg += decEvalMg(whitePawnScore) - decEvalMg(blackPawnScore);
    valueEg += decEvalEg(whitePawnScore) - decEvalEg(blackPawnScore);

    if (debug) {
        evalDebugStats.whitePawnScore = whitePawnScore;
        evalDebugStats.blackPawnScore = blackPawnScore;
    }


    // King-pawn tropism
    int kingPawnTropism = 0;
    if (egFactor > 0) {
        uint64_t pawnBits = pieces[WHITE][PAWNS] | pieces[BLACK][PAWNS];
        int pawnCount = pieceCounts[WHITE][PAWNS] + pieceCounts[BLACK][PAWNS];

        int wTropismTotal = 0, bTropismTotal = 0;
        while (pawnBits) {
            int pawnSq = bitScanForward(pawnBits);
            // int rank = pawnSq >> 3;
            pawnBits &= pawnBits - 1;
            if (INDEX_TO_BIT[pawnSq] & (wBackwards | bBackwards)) {
                wTropismTotal += 2 * getManhattanDistance(pawnSq, wKingSq);
                bTropismTotal += 2 * getManhattanDistance(pawnSq, bKingSq);
            }
            else {
                wTropismTotal += getManhattanDistance(pawnSq, wKingSq);
                bTropismTotal += getManhattanDistance(pawnSq, bKingSq);
            }
        }

        if (pawnCount)
            kingPawnTropism = (bTropismTotal - wTropismTotal) / pawnCount;

        valueEg += kingPawnTropism;
    }


    valueMg += decEvalMg(value);
    valueEg += decEvalEg(value);
    int totalEval = (valueMg * (EG_FACTOR_RES - egFactor) + valueEg * egFactor) / EG_FACTOR_RES;

    // Scale factors
    // Opposite colored bishops
    if (egFactor > 3 * EG_FACTOR_RES / 4) {
        if (pieceCounts[WHITE][BISHOPS] == 1
         && pieceCounts[BLACK][BISHOPS] == 1
         && (((pieces[WHITE][BISHOPS] & LIGHT) && (pieces[BLACK][BISHOPS] & DARK))
          || ((pieces[WHITE][BISHOPS] & DARK) && (pieces[BLACK][BISHOPS] & LIGHT)))) {
            if ((getNonPawnMaterial(WHITE) == pieces[WHITE][BISHOPS])
             && (getNonPawnMaterial(BLACK) == pieces[BLACK][BISHOPS]))
                totalEval = totalEval / 2;
            else
                totalEval = (384 - 128 * egFactor / EG_FACTOR_RES) * totalEval / 256;
        }
    }


    if (debug) {
        evalDebugStats.totalEval = totalEval;
        evalDebugStats.print();
    }

    return totalEval;
}

// Explicitly instantiate templates
template int Board::evaluate<true>();
template int Board::evaluate<false>();


/* Scores the board for a player based on estimates of mobility
 * This function also provides a bonus for having mobile pieces near the
 * opponent's king, and deals with control of center.
 */
template <int color>
void Board::getPseudoMobility(PieceMoveList &pml, PieceMoveList &oppPml,
    int &valueMg, int &valueEg) {
    // Bitboard of center 4 squares: d4, e4, d5, e5
    const uint64_t CENTER_SQS = 0x0000001818000000;
    // Extended center: center, plus c4, f4, c5, f5, and d6/d3, e6/e3
    const uint64_t EXTENDED_CENTER_SQS = 0x0000183C3C180000;
    // Holds the mobility values and the final result
    int mgMobility = 0, egMobility = 0;
    // Holds the center control score
    int centerControl = 0;
    // All squares the side to move could possibly move to
    uint64_t openSqs = ~allPieces[color];

    // Calculate center control only for pawns
    uint64_t pawns = pieces[color][PAWNS];
    uint64_t pawnAttackMap = (color == WHITE) ? getWPawnCaptures(pawns)
                                              : getBPawnCaptures(pawns);
    centerControl += EXTENDED_CENTER_VAL * count(pawnAttackMap & EXTENDED_CENTER_SQS);
    centerControl += CENTER_BONUS * count(pawnAttackMap & CENTER_SQS);


    uint64_t oppPawns = pieces[color^1][PAWNS];
    uint64_t oppPawnAttackMap = (color == WHITE) ? getBPawnCaptures(oppPawns)
                                                 : getWPawnCaptures(oppPawns);
    // We count mobility for captures or moves to open squares not controlled
    // by an opponent's pawn
    openSqs = allPieces[color^1] | (openSqs & ~oppPawnAttackMap);
    // Or for a queen, squares not controlled by an opponent's pawn, minor, or rook
    uint64_t oppAttackMap = 0;
    for (unsigned int i = 0; i < oppPml.size(); i++) {
        PieceMoveInfo pmi = oppPml.get(i);
        if (pmi.pieceID <= ROOKS)
            oppAttackMap |= pmi.legal;
    }

    // Iterate over piece move information to extract all mobility-related scores
    for (unsigned int i = 0; i < pml.size(); i++) {
        PieceMoveInfo pmi = pml.get(i);

        int pieceIndex = pmi.pieceID - 1;
        // Get all potential legal moves
        uint64_t legal = pmi.legal;
        // Get mobility score
        if (pieceIndex == QUEENS - 1) {
            mgMobility += mobilityScore[MG][pieceIndex][count(legal & openSqs & ~oppAttackMap)];
            egMobility += mobilityScore[EG][pieceIndex][count(legal & openSqs & ~oppAttackMap)];
        }
        else {
            mgMobility += mobilityScore[MG][pieceIndex][count(legal & openSqs)];
            egMobility += mobilityScore[EG][pieceIndex][count(legal & openSqs)];
        }

        // Get center control score
        if (pieceIndex == QUEENS - 1) {
            centerControl += EXTENDED_CENTER_VAL * count(legal & EXTENDED_CENTER_SQS & ~oppPawnAttackMap & ~oppAttackMap);
            centerControl += CENTER_BONUS * count(legal & CENTER_SQS & ~oppPawnAttackMap & ~oppAttackMap);
        }
        else {
            centerControl += EXTENDED_CENTER_VAL * count(legal & EXTENDED_CENTER_SQS & ~oppPawnAttackMap);
            centerControl += CENTER_BONUS * count(legal & CENTER_SQS & ~oppPawnAttackMap);
        }
    }

    valueMg = mgMobility + centerControl;
    valueEg = egMobility;
}

// King safety, based on the number of opponent pieces near the king
// The lookup table approach is inspired by Ed Schroder's Rebel chess engine
template <int attackingColor>
int Board::getKingSafety(PieceMoveList &attackers, PieceMoveList &defenders,
    uint64_t kingSqs, int pawnScore) {

    // Holds the king safety score
    int kingSafetyPts = 0;
    // Counts the number of pieces participating in the king attack
    int kingAttackPieces = 0;

    uint64_t kingNeighborhood = (attackingColor == WHITE) ? (kingSqs | (kingSqs >> 8))
                                                          : (kingSqs | (kingSqs << 8));
    uint64_t defendingPawns = pieces[attackingColor^1][PAWNS];


    // Analyze undefended squares directly adjacent to king
    uint64_t kingDefenseless = 0;
    for (unsigned int i = 0; i < defenders.size(); i++) {
        uint64_t legal = defenders.get(i).legal;
        kingDefenseless |= legal & kingSqs;
    }
    // Add in pawns
    kingDefenseless |= (attackingColor == WHITE) ? (getBPawnCaptures(defendingPawns) & kingSqs)
                                                 : (getWPawnCaptures(defendingPawns) & kingSqs);
    kingDefenseless ^= kingSqs;


    // Iterate over piece move information to extract all mobility-related scores
    for (unsigned int i = 0; i < attackers.size(); i++) {
        PieceMoveInfo pmi = attackers.get(i);

        int pieceIndex = pmi.pieceID - 1;
        // Get all potential legal moves
        uint64_t legal = pmi.legal;

        // Get king safety score
        int kingSqCount = count(legal & kingNeighborhood);
        if (kingSqCount) {
            kingAttackPieces++;
            kingSafetyPts += KING_THREAT_MULTIPLIER[pieceIndex] * count(legal & kingSqs);

            // Bonus for overloading on defenseless squares
            kingSafetyPts += KING_DEFENSELESS_SQUARE * count(legal & kingDefenseless);
        }
    }

    // Give a decent bonus for each additional piece participating
    kingSafetyPts += std::min(60, 2 * kingAttackPieces * (kingAttackPieces+1));

    // Adjust based on pawn shield and pawn storms
    kingSafetyPts -= pawnScore / 4;

    return KS_TO_SCORE[std::max(0, std::min(199, kingSafetyPts))];
}

// Check special endgame cases: where help mate is possible (detecting this
// is delegated to search), but forced mate is not, or where a simple
// forced mate is possible.
int Board::checkEndgameCases() {
    int numWPieces = count(allPieces[WHITE]) - 1;
    int numBPieces = count(allPieces[BLACK]) - 1;
    int numPieces = numWPieces + numBPieces;

    // Rook or queen + anything else vs. lone king is a forced win
    if (numBPieces == 0 && (pieces[WHITE][ROOKS] || pieces[WHITE][QUEENS])) {
        return scoreSimpleKnownWin(WHITE);
    }
    if (numWPieces == 0 && (pieces[BLACK][ROOKS] || pieces[BLACK][QUEENS])) {
        return scoreSimpleKnownWin(BLACK);
    }

    if (numPieces == 1) {
        if (pieces[WHITE][PAWNS]) {
            int wPawn = bitScanForward(flipAcrossRanks(pieces[WHITE][PAWNS]));
            return 3 * PIECE_VALUES[EG][PAWNS] / 2 + getPSQTValue(EG, PAWNS, wPawn);
        }
        if (pieces[BLACK][PAWNS]) {
            int bPawn = bitScanForward(pieces[BLACK][PAWNS]);
            return -3 * PIECE_VALUES[EG][PAWNS] / 2 - getPSQTValue(EG, PAWNS, bPawn);
        }
    }

    else if (numPieces == 2) {
        // If white has one piece, the other must be black's
        if (numWPieces == 1) {
            // If each side has one minor piece, then draw
            if ((pieces[WHITE][KNIGHTS] | pieces[WHITE][BISHOPS])
             && (pieces[BLACK][KNIGHTS] | pieces[BLACK][BISHOPS]))
                return 0;
            // If each side has a rook, then draw
            if (pieces[WHITE][ROOKS] && pieces[BLACK][ROOKS])
                return 0;
            // If each side has a queen, then draw
            if (pieces[WHITE][QUEENS] && pieces[BLACK][QUEENS])
                return 0;
        }
        // Otherwise, one side has both pieces
        else {
            // Pawn + anything is a win
            // TODO bishop can block losing king's path to queen square
            if (pieces[WHITE][PAWNS]) {
                int value = KNOWN_WIN / 2;
                int wKingSq = bitScanForward(pieces[WHITE][KINGS]);
                int bKingSq = bitScanForward(pieces[BLACK][KINGS]);
                int wPawnSq = bitScanForward(pieces[WHITE][PAWNS]);
                int wf = wPawnSq & 7;
                int wr = wPawnSq >> 3;
                if (pieces[WHITE][BISHOPS]
                 && ((wf == 0 && (pieces[WHITE][BISHOPS] & DARK))
                  || (wf == 7 && (pieces[WHITE][BISHOPS] & LIGHT)))) {
                    int wDist = std::max(7 - (wKingSq >> 3), std::abs((wKingSq & 7) - wf));
                    int bDist = std::max(7 - (bKingSq >> 3), std::abs((bKingSq & 7) - wf));
                    int wQueenDist = std::min(7-wr, 5) + 1;
                    if (playerToMove == BLACK)
                        bDist--;
                    if (bDist < std::min(wDist, wQueenDist))
                        return 0;
                }

                value += 8 * wr * wr;
                value += scoreCornerDistance(WHITE, wKingSq, bKingSq);
                return value;
            }
            if (pieces[BLACK][PAWNS]) {
                int value = -KNOWN_WIN / 2;
                int wKingSq = bitScanForward(pieces[WHITE][KINGS]);
                int bKingSq = bitScanForward(pieces[BLACK][KINGS]);
                int bPawnSq = bitScanForward(pieces[BLACK][PAWNS]);
                int bf = bPawnSq & 7;
                int br = bPawnSq >> 3;
                if (pieces[BLACK][BISHOPS]
                 && ((bf == 0 && (pieces[BLACK][BISHOPS] & LIGHT))
                  || (bf == 7 && (pieces[BLACK][BISHOPS] & DARK)))) {
                    int wDist = std::max((wKingSq >> 3), std::abs((wKingSq & 7) - bf));
                    int bDist = std::max((bKingSq >> 3), std::abs((bKingSq & 7) - bf));
                    int bQueenDist = std::min(br, 5) + 1;
                    if (playerToMove == WHITE)
                        wDist--;
                    if (wDist < std::min(bDist, bQueenDist))
                        return 0;
                }

                value -= 8 * br * br;
                value += scoreCornerDistance(WHITE, wKingSq, bKingSq);
                return value;
            }
            // Two knights is a draw
            if (count(pieces[WHITE][KNIGHTS]) == 2 || count(pieces[BLACK][KNIGHTS]) == 2)
                return 0;
            // Two bishops is a win
            if (count(pieces[WHITE][BISHOPS]) == 2)
                return scoreSimpleKnownWin(WHITE);
            if (count(pieces[BLACK][BISHOPS]) == 2)
                return scoreSimpleKnownWin(BLACK);

            // Mating with knight and bishop
            if (pieces[WHITE][KNIGHTS] && pieces[WHITE][BISHOPS]) {
                int value = KNOWN_WIN;
                int wKingSq = bitScanForward(pieces[WHITE][KINGS]);
                int bKingSq = bitScanForward(pieces[BLACK][KINGS]);
                value += scoreCornerDistance(WHITE, wKingSq, bKingSq);

                // Light squared corners are H1 (7) and A8 (56)
                if (pieces[WHITE][BISHOPS] & LIGHT)
                    value -= 20 * std::min(getManhattanDistance(bKingSq, 7), getManhattanDistance(bKingSq, 56));
                // Dark squared corners are A1 (0) and H8 (63)
                else
                    value -= 20 * std::min(getManhattanDistance(bKingSq, 0), getManhattanDistance(bKingSq, 63));
                return value;
            }
            if (pieces[BLACK][KNIGHTS] && pieces[BLACK][BISHOPS]) {
                int value = -KNOWN_WIN;
                int wKingSq = bitScanForward(pieces[WHITE][KINGS]);
                int bKingSq = bitScanForward(pieces[BLACK][KINGS]);
                value += scoreCornerDistance(BLACK, wKingSq, bKingSq);

                // Light squared corners are H1 (7) and A8 (56)
                if (pieces[BLACK][BISHOPS] & LIGHT)
                    value += 20 * std::min(getManhattanDistance(wKingSq, 7), getManhattanDistance(wKingSq, 56));
                // Dark squared corners are A1 (0) and H8 (63)
                else
                    value += 20 * std::min(getManhattanDistance(wKingSq, 0), getManhattanDistance(wKingSq, 63));
                return value;
            }
        }
    }

    // If not an endgame case, return -INFTY
    return -INFTY;
}

// A function for scoring the most basic mating cases, when it is only necessary
// to get the opposing king into a corner.
int Board::scoreSimpleKnownWin(int winningColor) {
    int wKingSq = bitScanForward(pieces[WHITE][KINGS]);
    int bKingSq = bitScanForward(pieces[BLACK][KINGS]);
    int winScore = (winningColor == WHITE) ? KNOWN_WIN : -KNOWN_WIN;
    return winScore + scoreCornerDistance(winningColor, wKingSq, bKingSq);
}

inline int Board::scoreCornerDistance(int winningColor, int wKingSq, int bKingSq) {
    int wf = wKingSq & 7;
    int wr = wKingSq >> 3;
    int bf = bKingSq & 7;
    int br = bKingSq >> 3;
    int wDist = std::min(wf, 7-wf) + std::min(wr, 7-wr);
    int bDist = std::min(bf, 7-bf) + std::min(br, 7-br);
    return (winningColor == WHITE) ? wDist - 2*bDist : 2*wDist - bDist;
}

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

int Board::getManhattanDistance(int sq1, int sq2) {
    return std::abs((sq1 >> 3) - (sq2 >> 3)) + std::abs((sq1 & 7) - (sq2 & 7));
}

inline int Board::getPSQTValue(int isEndgame, int pieceID, int sq) {
    int f = sq & 7;
    sq = ((sq >> 3) << 2) + std::min(f, 7-f);
    return pieceSquareTable[isEndgame][pieceID][sq];
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
    uint64_t attackers = getAttackMap(WHITE, sq) | getAttackMap(BLACK, sq);
    // used attackers that may act as blockers for x-ray pieces
    uint64_t used = 0;

    // Get the first attacker
    uint64_t single = getLeastValuableAttacker(attackers, color, piece);
    // If there are no attackers, return immediately
    if (!single)
        return 0;
    // Get value of piece initially being captured, or 0 if the square is
    // empty
    gain[d] = SEE_PIECE_VALS[getPieceOnSquare(color^1, sq)];

    do {
        d++; // next depth
        color ^= 1;
        gain[d]  = SEE_PIECE_VALS[piece] - gain[d-1];
        if (-gain[d-1] < 0 && gain[d] < 0) // pruning for stand pat
            break;
        attackers ^= single; // remove used attacker
        used |= single;
        attackers |= getXRayPieceMap(WHITE, sq, color, used, 0) | getXRayPieceMap(BLACK, sq, color, used, 0);
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
        return -PIECE_VALUES[MG][PAWNS];

    // Do the move temporarily
    pieces[color][pieceID] &= ~INDEX_TO_BIT[startSq];
    pieces[color][pieceID] |= INDEX_TO_BIT[endSq];
    allPieces[color] &= ~INDEX_TO_BIT[startSq];
    allPieces[color] |= INDEX_TO_BIT[endSq];

    // For a capture, we also need to update the captured piece on the board
    if (isCapture(m)) {
        int capturedPiece = getPieceOnSquare(color^1, endSq);
        pieces[color^1][capturedPiece] &= ~INDEX_TO_BIT[endSq];
        allPieces[color^1] &= ~INDEX_TO_BIT[endSq];

        value = SEE_PIECE_VALS[capturedPiece] - getSEE(color^1, endSq);

        pieces[color^1][capturedPiece] |= INDEX_TO_BIT[endSq];
        allPieces[color^1] |= INDEX_TO_BIT[endSq];
    }
    else {
        value = -getSEE(color^1, endSq);
    }

    // Undo the move
    pieces[color][pieceID] |= INDEX_TO_BIT[startSq];
    pieces[color][pieceID] &= ~INDEX_TO_BIT[endSq];
    allPieces[color] |= INDEX_TO_BIT[startSq];
    allPieces[color] &= ~INDEX_TO_BIT[endSq];

    return value;
}

int Board::valueOfPiece(int pieceID) {
    switch(pieceID) {
        case -1:
            return 0;
        case PAWNS:
            return PIECE_VALUES[MG][PAWNS];
        case KNIGHTS:
            return PIECE_VALUES[MG][KNIGHTS];
        case BISHOPS:
            return PIECE_VALUES[MG][BISHOPS];
        case ROOKS:
            return PIECE_VALUES[MG][ROOKS];
        case QUEENS:
            return PIECE_VALUES[MG][QUEENS];
        case KINGS:
            return MATE_SCORE;
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
//-----------------------------MOVE GENERATION----------------------------------
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
    pawns = (temp << 8) & open & RANKS[3];
    return pawns;
}

uint64_t Board::getBPawnDoubleMoves(uint64_t pawns) {
    uint64_t open = ~getOccupancy();
    uint64_t temp = (pawns >> 8) & open;
    pawns = (temp >> 8) & open & RANKS[4];
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

inline uint64_t Board::getWPawnCaptures(uint64_t pawns) {
    return getWPawnLeftCaptures(pawns) | getWPawnRightCaptures(pawns);
}

inline uint64_t Board::getBPawnCaptures(uint64_t pawns) {
    return getBPawnLeftCaptures(pawns) | getBPawnRightCaptures(pawns);
}

inline uint64_t Board::getKnightSquares(int single) {
    return KNIGHTMOVES[single];
}

uint64_t Board::getBishopSquares(int single, uint64_t occ) {
    uint64_t *attTableLoc = magicBishops[single].table;
    occ &= magicBishops[single].mask;
    occ *= magicBishops[single].magic;
    occ >>= magicBishops[single].shift;
    return attTableLoc[occ];
}

uint64_t Board::getRookSquares(int single, uint64_t occ) {
    uint64_t *attTableLoc = magicRooks[single].table;
    occ &= magicRooks[single].mask;
    occ *= magicRooks[single].magic;
    occ >>= magicRooks[single].shift;
    return attTableLoc[occ];
}

uint64_t Board::getQueenSquares(int single, uint64_t occ) {
    return getBishopSquares(single, occ) | getRookSquares(single, occ);
}

inline uint64_t Board::getKingSquares(int single) {
    return KINGMOVES[single];
}

inline uint64_t Board::getOccupancy() {
    return allPieces[WHITE] | allPieces[BLACK];
}

inline int Board::epVictimSquare(int victimColor, uint16_t file) {
    return 8 * (3 + victimColor) + file;
}


//------------------------------------------------------------------------------
//---------------------------PUBLIC GETTER METHODS------------------------------
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

uint16_t Board::getEPCaptureFile() {
    return epCaptureFile;
}

uint8_t Board::getFiftyMoveCounter() {
    return fiftyMoveCounter;
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
