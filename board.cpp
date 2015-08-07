#include <random>
#include "board.h"
#include "btables.h"

static uint64_t rankArray[8][64];
static uint64_t fileArray[8][64];
static uint64_t zobristTable[794];
static uint64_t startPosZobristKey = 0;

// Dumb7fill methods
uint64_t southAttacks(uint64_t rooks, uint64_t empty);
uint64_t northAttacks(uint64_t rooks, uint64_t empty);
uint64_t eastAttacks(uint64_t rooks, uint64_t empty);
uint64_t neAttacks(uint64_t bishops, uint64_t empty);
uint64_t seAttacks(uint64_t bishops, uint64_t empty);
uint64_t westAttacks(uint64_t rooks, uint64_t empty);
uint64_t swAttacks(uint64_t bishops, uint64_t empty);
uint64_t nwAttacks(uint64_t bishops, uint64_t empty);

void initKindergartenTables() {
    // Generate the kindergarten bitboard attack table.
    // We index by the file the slider is on, and the middle 6 bits occupancy of
    // the rank.
    for(int file = 0; file < 8; file++) {
        for(int occ = 0; occ < 64; occ++) {
            // East/west attacks since we are considering one rank
            // occ << 1 to map the occupancy to the middle 6 bits
            // This gives the result in the lowest 8 bits
            uint64_t rankEmpty = RANKS[0] & ~(occ << 1);
            uint64_t result = eastAttacks((1 << file), rankEmpty)
                            | westAttacks((1 << file), rankEmpty);
            // We must now copy these 8 bits 7 times to fill the bitboard...
            result |= result << 8;
            result |= result << 16;
            result |= result << 32;
            // ...and record the result
            rankArray[file][occ] = result;
        }
    }

    // Files use a slightly different table, with a single instance of the A-file
    for(uint64_t rank = 0; rank < 8; rank++) {
        for(uint64_t occ = 0; occ < 64; occ++) {
            uint64_t fileEmpty = 0;
            // Move occupancy from the first rank to the A file
            for(int i = 0; i < 6; i++) {
                fileEmpty |= (occ & (1 << i)) << (7 * i);
            }
            fileEmpty = AFILE & ~(fileEmpty << 8);
            uint64_t result = northAttacks((1ULL << (rank * 8)), fileEmpty)
                            | southAttacks((1ULL << (rank * 8)), fileEmpty);
            fileArray[rank][occ] = result;
        }
    }
}

void initZobristTable() {
    mt19937_64 rng (61280152908);
    for (int i = 0; i < 794; i++)
        zobristTable[i] = rng();

    Board b;
    int *mailbox = b.getMailbox();
    b.initZobristKey(mailbox);
    startPosZobristKey = b.getZobristKey();
    delete[] mailbox;
}

int epVictimSquare(int victimColor, uint16_t file) {
    return 8 * (3 + victimColor) + file;
}

/*
 * Performs a PERFT. Useful for testing/debugging
 * 7/8/15: PERFT 5, 1.46 s (i5-2450m)
 * 7/11/15: PERFT 5, 1.22 s (i5-2450m)
 * 7/13/15: PERFT 5, 1.27/1.08 s (i5-2450m) before/after pass Board by reference
 * 7/14/15: PERFT 5, 0.86 s (i5-2450m)
 * 7/17/15: PERFT 5, 0.32 s (i5-2450m)
 */
uint64_t perft(Board &b, int color, int depth, uint64_t &captures) {
    if (depth == 0)
        return 1;

    MoveList pl = b.getPseudoLegalQuiets(color);
    uint64_t nodes = 0;
    
    for (unsigned int i = 0; i < pl.size(); i++) {
        Board copy = b.staticCopy();
        if (!copy.doPseudoLegalMove(pl.get(i), color))
            continue;
        
        nodes += perft(copy, color^1, depth-1, captures);
    }

    MoveList pc = b.getPseudoLegalCaptures(color, true);
    for (unsigned int i = 0; i < pc.size(); i++) {
        Board copy = b.staticCopy();
        if (!copy.doPseudoLegalMove(pc.get(i), color))
            continue;
        
        captures++;
        nodes += perft(copy, color^1, depth-1, captures);
    }
/*    
    MoveList pl = b.getAllPseudoLegalMoves(color);
    uint64_t nodes = 0;
    
    for (unsigned int i = 0; i < pl.size(); i++) {
        Board copy = b.staticCopy();
        if (!copy.doPseudoLegalMove(pl.get(i), color))
            continue;

        if (isCapture(pl.get(i)))
            captures++;
        
        nodes += perft(copy, color^1, depth-1, captures);
    }
*/
    return nodes;
}

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

    epCaptureFile = NO_EP_POSSIBLE;
    playerToMove = WHITE;
    twoFoldSqs = RESET_TWOFOLD;
    zobristKey = startPosZobristKey;
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
            pieces[mailboxBoard[i]/6][mailboxBoard[i]%6] |= MOVEMASK[i];
        }
        else if (mailboxBoard[i] > 11)
            cerr << "Error in constructor." << endl;
    }
    allPieces[WHITE] = 0;
    for (int i = 0; i < 6; i++)
        allPieces[WHITE] |= pieces[0][i];
    allPieces[BLACK] = 0;
    for (int i = 0; i < 6; i++)
        allPieces[BLACK] |= pieces[1][i];

    epCaptureFile = _epCaptureFile;
    playerToMove = _playerToMove;
    twoFoldSqs = RESET_TWOFOLD;
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

// Creates a copy of a board
// Private constructor used only with staticCopy()
Board::Board(Board *b) {
    allPieces[WHITE] = b->allPieces[WHITE];
    allPieces[BLACK] = b->allPieces[BLACK];
    for (int i = 0; i < 6; i++) {
        pieces[0][i] = b->pieces[0][i];
    }
    for (int i = 0; i < 6; i++) {
        pieces[1][i] = b->pieces[1][i];
    }

    epCaptureFile = b->epCaptureFile;
    playerToMove = b->playerToMove;
    twoFoldSqs = b->twoFoldSqs;
    zobristKey = b->zobristKey;
    moveNumber = b->moveNumber;
    castlingRights = b->castlingRights;
    fiftyMoveCounter = b->fiftyMoveCounter;
}

Board Board::staticCopy() {
    return Board(this);
}

/*
Board *Board::dynamicCopy() {
    Board *result = new Board();
    for (int i = 0; i < 6; i++) {
        result->pieces[0][i] = pieces[0][i];
    }
    for (int i = 0; i < 6; i++) {
        result->pieces[1][i] = pieces[1][i];
    }
    result->whitePieces = whitePieces;
    result->blackPieces = blackPieces;
    result->epCaptureFile = epCaptureFile;
    result->playerToMove = playerToMove;
    result->twoFoldSqs = twoFoldSqs;
    result->zobristKey = zobristKey;
    result->moveNumber = moveNumber;
    result->castlingRights = castlingRights;
    result->fiftyMoveCounter = fiftyMoveCounter;
    return result;
}
*/

void Board::doMove(Move m, int color) {
    int pieceID = getPiece(m);
    int startSq = getStartSq(m);
    int endSq = getEndSq(m);

    // Handle null moves for null move pruning
    if (m == NULL_MOVE) {
        playerToMove = color ^ 1;
        zobristKey ^= zobristTable[768];
        return;
    }

    // Update flag based elements of Zobrist key
    zobristKey ^= zobristTable[769 + castlingRights];
    zobristKey ^= zobristTable[785 + epCaptureFile];

    // Record current board position for two-fold repetition
    if (isCapture(m) || pieceID == PAWNS || isCastle(m)) {
        twoFoldSqs = RESET_TWOFOLD;
    }
    else {
        twoFoldSqs <<= 16;
        twoFoldSqs |= (startSq << 8) | endSq;
    }

    if (getPromotion(m)) {
        if (isCapture(m)) {
            int captureType = getCapturedPiece(color^1, endSq);
            pieces[color][PAWNS] &= ~MOVEMASK[startSq];
            pieces[color][getPromotion(m)] |= MOVEMASK[endSq];
            pieces[color^1][captureType] &= ~MOVEMASK[endSq];

            allPieces[color] &= ~MOVEMASK[startSq];
            allPieces[color] |= MOVEMASK[endSq];
            allPieces[color^1] &= ~MOVEMASK[endSq];

            zobristKey ^= zobristTable[384*color + startSq];
            zobristKey ^= zobristTable[384*color + 64*getPromotion(m) + endSq];
            zobristKey ^= zobristTable[384*(color^1) + 64*captureType + endSq];
        }
        else {
            pieces[color][PAWNS] &= ~MOVEMASK[startSq];
            pieces[color][getPromotion(m)] |= MOVEMASK[endSq];

            allPieces[color] &= ~MOVEMASK[startSq];
            allPieces[color] |= MOVEMASK[endSq];

            zobristKey ^= zobristTable[384*color + startSq];
            zobristKey ^= zobristTable[384*color + 64*getPromotion(m) + endSq];
        }
        epCaptureFile = NO_EP_POSSIBLE;
        fiftyMoveCounter = 0;
    } // end promotion
    else if (isCapture(m)) {
        int captureType = getCapturedPiece(color^1, endSq);
        if (captureType == -1) {
            pieces[color][PAWNS] &= ~MOVEMASK[startSq];
            pieces[color][PAWNS] |= MOVEMASK[endSq];
            uint64_t epCaptureSq = MOVEMASK[epVictimSquare(color^1, epCaptureFile)];
            pieces[color^1][PAWNS] &= ~epCaptureSq;

            allPieces[color] &= ~MOVEMASK[startSq];
            allPieces[color] |= MOVEMASK[endSq];
            allPieces[color^1] &= ~epCaptureSq;

            int capSq = epVictimSquare(color^1, epCaptureFile);
            zobristKey ^= zobristTable[384*color + startSq];
            zobristKey ^= zobristTable[384*color + endSq];
            zobristKey ^= zobristTable[384*(color^1) + capSq];
        }
        else {
            pieces[color][pieceID] &= ~MOVEMASK[startSq];
            pieces[color][pieceID] |= MOVEMASK[endSq];
            pieces[color^1][captureType] &= ~MOVEMASK[endSq];

            allPieces[color] &= ~MOVEMASK[startSq];
            allPieces[color] |= MOVEMASK[endSq];
            allPieces[color^1] &= ~MOVEMASK[endSq];

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
                pieces[WHITE][KINGS] &= ~MOVEMASK[4];
                pieces[WHITE][KINGS] |= MOVEMASK[6];
                pieces[WHITE][ROOKS] &= ~MOVEMASK[7];
                pieces[WHITE][ROOKS] |= MOVEMASK[5];

                allPieces[WHITE] &= ~MOVEMASK[4];
                allPieces[WHITE] |= MOVEMASK[6];
                allPieces[WHITE] &= ~MOVEMASK[7];
                allPieces[WHITE] |= MOVEMASK[5];

                zobristKey ^= zobristTable[64*KINGS+4];
                zobristKey ^= zobristTable[64*KINGS+6];
                zobristKey ^= zobristTable[64*ROOKS+7];
                zobristKey ^= zobristTable[64*ROOKS+5];
            }
            else if (endSq == 2) { // white qside
                pieces[WHITE][KINGS] &= ~MOVEMASK[4];
                pieces[WHITE][KINGS] |= MOVEMASK[2];
                pieces[WHITE][ROOKS] &= ~MOVEMASK[0];
                pieces[WHITE][ROOKS] |= MOVEMASK[3];

                allPieces[WHITE] &= ~MOVEMASK[4];
                allPieces[WHITE] |= MOVEMASK[2];
                allPieces[WHITE] &= ~MOVEMASK[0];
                allPieces[WHITE] |= MOVEMASK[3];

                zobristKey ^= zobristTable[64*KINGS+4];
                zobristKey ^= zobristTable[64*KINGS+2];
                zobristKey ^= zobristTable[64*ROOKS+0];
                zobristKey ^= zobristTable[64*ROOKS+3];
            }
            else if (endSq == 62) { // black kside
                pieces[BLACK][KINGS] &= ~MOVEMASK[60];
                pieces[BLACK][KINGS] |= MOVEMASK[62];
                pieces[BLACK][ROOKS] &= ~MOVEMASK[63];
                pieces[BLACK][ROOKS] |= MOVEMASK[61];

                allPieces[BLACK] &= ~MOVEMASK[60];
                allPieces[BLACK] |= MOVEMASK[62];
                allPieces[BLACK] &= ~MOVEMASK[63];
                allPieces[BLACK] |= MOVEMASK[61];

                zobristKey ^= zobristTable[384+64*KINGS+60];
                zobristKey ^= zobristTable[384+64*KINGS+62];
                zobristKey ^= zobristTable[384+64*ROOKS+63];
                zobristKey ^= zobristTable[384+64*ROOKS+61];
            }
            else { // black qside
                pieces[BLACK][KINGS] &= ~MOVEMASK[60];
                pieces[BLACK][KINGS] |= MOVEMASK[58];
                pieces[BLACK][ROOKS] &= ~MOVEMASK[56];
                pieces[BLACK][ROOKS] |= MOVEMASK[59];

                allPieces[BLACK] &= ~MOVEMASK[60];
                allPieces[BLACK] |= MOVEMASK[58];
                allPieces[BLACK] &= ~MOVEMASK[56];
                allPieces[BLACK] |= MOVEMASK[59];

                zobristKey ^= zobristTable[384+64*KINGS+60];
                zobristKey ^= zobristTable[384+64*KINGS+58];
                zobristKey ^= zobristTable[384+64*ROOKS+56];
                zobristKey ^= zobristTable[384+64*ROOKS+59];
            }
            epCaptureFile = NO_EP_POSSIBLE;
            fiftyMoveCounter++;
        } // end castling
        else { // other quiet moves
            pieces[color][pieceID] &= ~MOVEMASK[startSq];
            pieces[color][pieceID] |= MOVEMASK[endSq];

            allPieces[color] &= ~MOVEMASK[startSq];
            allPieces[color] |= MOVEMASK[endSq];

            zobristKey ^= zobristTable[384*color + 64*pieceID + startSq];
            zobristKey ^= zobristTable[384*color + 64*pieceID + endSq];

            // check for en passant
            if (pieceID == PAWNS) {
                if (color == WHITE && startSq/8 == 1 && endSq/8 == 3) {
                    epCaptureFile = startSq & 7;
                }
                else if (startSq/8 == 6 && endSq/8 == 4) {
                    epCaptureFile = startSq & 7;
                }
                else {
                    epCaptureFile = NO_EP_POSSIBLE;
                }
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
        if (color == WHITE) {
            castlingRights &= ~WHITECASTLE;
        }
        else {
            castlingRights &= ~BLACKCASTLE;
        }
    }
    else {
        if (castlingRights & WHITECASTLE) {
            int whiteR = (int)(RANKS[0] & pieces[WHITE][ROOKS]);
            if ((whiteR & 0x80) == 0)
                castlingRights &= ~WHITEKSIDE;
            if ((whiteR & 1) == 0)
                castlingRights &= ~WHITEQSIDE;
        }
        if (castlingRights & BLACKCASTLE) {
            int blackR = (int)((RANKS[7] & pieces[BLACK][ROOKS]) >> 56);
            if ((blackR & 0x80) == 0)
                castlingRights &= ~BLACKKSIDE;
            if ((blackR & 1) == 0)
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

    if (isInCheck(color))
        return false;
    else return true;
}

// Do a hash move, which requires a few more checks in case of a Type-1 error.
bool Board::doHashMove(Move m, int color) {
    int pieceID = getPiece(m);
    // Check that the correct piece is on the start square
    if (!(pieces[color][pieceID] & MOVEMASK[getStartSq(m)]))
        return false;
    // Check that the end square has correct occupancy
    uint64_t otherPieces = allPieces[color^1];
    uint64_t endSingle = MOVEMASK[getEndSq(m)];
    bool captureRoutes = (isCapture(m) && (otherPieces & endSingle))
                      || (isCapture(m) && pieceID == PAWNS && (~otherPieces & endSingle));
    uint64_t empty = ~(allPieces[WHITE] | allPieces[BLACK]);
    if (!(captureRoutes || (!isCapture(m) && (empty & endSingle))))
        return false;

    return doPseudoLegalMove(m, color);
}

// Get all legal moves and captures
MoveList Board::getAllLegalMoves(int color) {
    MoveList moves = getAllPseudoLegalMoves(color);

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
MoveList Board::getAllPseudoLegalMoves(int color) {
    MoveList quiets, captures;

    uint64_t otherPieces = allPieces[color^1];

    addPawnMovesToList(quiets, color);
    addPawnCapturesToList(captures, color, otherPieces, true);

    uint64_t knights = pieces[color][KNIGHTS];
    while (knights) {
        int stsq = bitScanForward(knights);
        knights &= knights-1;
        uint64_t nSq = getKnightSquares(stsq);

        addMovesToList(quiets, KNIGHTS, stsq, nSq, false);
        addMovesToList(captures, KNIGHTS, stsq, nSq, true, otherPieces);
    }

    uint64_t bishops = pieces[color][BISHOPS];
    while (bishops) {
        int stsq = bitScanForward(bishops);
        bishops &= bishops-1;
        uint64_t bSq = getBishopSquares(stsq);

        addMovesToList(quiets, BISHOPS, stsq, bSq, false);
        addMovesToList(captures, BISHOPS, stsq, bSq, true, otherPieces);
    }

    uint64_t rooks = pieces[color][ROOKS];
    while (rooks) {
        int stsq = bitScanForward(rooks);
        rooks &= rooks-1;
        uint64_t rSq = getRookSquares(stsq);

        addMovesToList(quiets, ROOKS, stsq, rSq, false);
        addMovesToList(captures, ROOKS, stsq, rSq, true, otherPieces);
    }

    uint64_t queens = pieces[color][QUEENS];
    while (queens) {
        int stsq = bitScanForward(queens);
        queens &= queens-1;
        uint64_t qSq = getQueenSquares(stsq);

        addMovesToList(quiets, QUEENS, stsq, qSq, false);
        addMovesToList(captures, QUEENS, stsq, qSq, true, otherPieces);
    }

    uint64_t kings = pieces[color][KINGS];
    int stsqK = bitScanForward(kings);
    uint64_t kingSqs = getKingSquares(stsqK);

    addMovesToList(quiets, KINGS, stsqK, kingSqs, false);
    addMovesToList(captures, KINGS, stsqK, kingSqs, true, otherPieces);

    addCastlesToList(quiets, color);

    // Put captures before quiet moves
    for (unsigned int i = 0; i < quiets.size(); i++) {
        captures.add(quiets.get(i));
    }
    return captures;
}

MoveList Board::getPseudoLegalQuiets(int color) {
    MoveList quiets;

    addPawnMovesToList(quiets, color);

    uint64_t knights = pieces[color][KNIGHTS];
    while (knights) {
        int stsq = bitScanForward(knights);
        knights &= knights-1;
        uint64_t nSq = getKnightSquares(stsq);

        addMovesToList(quiets, KNIGHTS, stsq, nSq, false);
    }

    uint64_t bishops = pieces[color][BISHOPS];
    while (bishops) {
        int stsq = bitScanForward(bishops);
        bishops &= bishops-1;
        uint64_t bSq = getBishopSquares(stsq);

        addMovesToList(quiets, BISHOPS, stsq, bSq, false);
    }

    uint64_t rooks = pieces[color][ROOKS];
    while (rooks) {
        int stsq = bitScanForward(rooks);
        rooks &= rooks-1;
        uint64_t rSq = getRookSquares(stsq);

        addMovesToList(quiets, ROOKS, stsq, rSq, false);
    }

    uint64_t queens = pieces[color][QUEENS];
    while (queens) {
        int stsq = bitScanForward(queens);
        queens &= queens-1;
        uint64_t qSq = getQueenSquares(stsq);

        addMovesToList(quiets, QUEENS, stsq, qSq, false);
    }

    uint64_t kings = pieces[color][KINGS];
    int stsqK = bitScanForward(kings);
    uint64_t kingSqs = getKingSquares(stsqK);

    addMovesToList(quiets, KINGS, stsqK, kingSqs, false);

    addCastlesToList(quiets, color);

    return quiets;
}

// Do not include promotions for quiescence search, include promotions in normal search.
MoveList Board::getPseudoLegalCaptures(int color, bool includePromotions) {
    MoveList captures;

    uint64_t otherPieces = allPieces[color^1];

    addPawnCapturesToList(captures, color, otherPieces, includePromotions);

    uint64_t knights = pieces[color][KNIGHTS];
    while (knights) {
        int stsq = bitScanForward(knights);
        knights &= knights-1;
        uint64_t nSq = getKnightSquares(stsq);

        addMovesToList(captures, KNIGHTS, stsq, nSq, true, otherPieces);
    }

    uint64_t bishops = pieces[color][BISHOPS];
    while (bishops) {
        int stsq = bitScanForward(bishops);
        bishops &= bishops-1;
        uint64_t bSq = getBishopSquares(stsq);

        addMovesToList(captures, BISHOPS, stsq, bSq, true, otherPieces);
    }

    uint64_t rooks = pieces[color][ROOKS];
    while (rooks) {
        int stsq = bitScanForward(rooks);
        rooks &= rooks-1;
        uint64_t rSq = getRookSquares(stsq);

        addMovesToList(captures, ROOKS, stsq, rSq, true, otherPieces);
    }

    uint64_t queens = pieces[color][QUEENS];
    while (queens) {
        int stsq = bitScanForward(queens);
        queens &= queens-1;
        uint64_t qSq = getQueenSquares(stsq);

        addMovesToList(captures, QUEENS, stsq, qSq, true, otherPieces);
    }

    uint64_t kings = pieces[color][KINGS];
    int stsq = bitScanForward(kings);
    uint64_t kingSqs = getKingSquares(stsq);

    addMovesToList(captures, KINGS, stsq, kingSqs, true, otherPieces);

    return captures;
}

// Generates all queen promotions for quiescence search
MoveList Board::getPseudoLegalPromotions(int color) {
    MoveList moves;
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

        Move mq = encodeMove(endSq+leftDiff, endSq, PAWNS, true);
        mq = setPromotion(mq, QUEENS);
        moves.add(mq);
    }

    legal = (color == WHITE) ? getWPawnRightCaptures(pawns)
                             : getBPawnRightCaptures(pawns);
    legal &= otherPieces;
    promotions = legal & finalRank;

    while (promotions) {
        int endSq = bitScanForward(promotions);
        promotions &= promotions-1;

        Move mq = encodeMove(endSq+rightDiff, endSq, PAWNS, true);
        mq = setPromotion(mq, QUEENS);
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

        Move mq = encodeMove(stSq, endSq, PAWNS, false);
        mq = setPromotion(mq, QUEENS);
        moves.add(mq);
    }

    return moves;
}

/*
 * Get all pseudo-legal checks for a position. Used in quiescence search.
 * This function can be optimized compared to a normal getLegalMoves() because
 * for each piece, we can intersect the legal moves of the piece with the attack
 * map of this piece to the opposing king square to determine direct checks.
 * Discovered checks have to be handled separately.
 *
 * For simplicity, promotions and en passant are left out of this function.
 */
MoveList Board::getPseudoLegalChecks(int color) {
    MoveList checkCaptures, checks;
    int kingSq = bitScanForward(pieces[color^1][KINGS]);
    // Square parity for knight and bishop moves
    uint64_t kingParity = (pieces[color^1][KINGS] & LIGHT) ? LIGHT : DARK;
    uint64_t otherPieces = allPieces[color^1];
    uint64_t invAttackMap = ~(getInitXRays(color, kingSq));

    // We can do pawns in parallel, since the start square of a pawn move is
    // determined by its end square.
    uint64_t pawns = pieces[color][PAWNS];

    // First, deal with discovered checks
    /*
    uint64_t tempPawns = pawns;
    while (tempPawns) {
        int stsq = bitScanForward(tempPawns);
        tempPawns &= tempPawns - 1;
        uint64_t xrays = getXRays(color, kingSq, color, MOVEMASK[stsq]);
        // If moving the pawn caused a new xray piece to attack the king
        if (!(xrays & invAttackMap)) {
            // Every legal move of this pawn is a legal check
            uint64_t legal = (color == WHITE) ? getWPawnSingleMoves(MOVEMASK[stsq]) | getWPawnDoubleMoves(MOVEMASK[stsq])
                                              : getBPawnSingleMoves(MOVEMASK[stsq]) | getBPawnDoubleMoves(MOVEMASK[stsq]);
            while (legal) {
                int endsq = bitScanForward(legal);
                legal &= legal - 1;
                checks.add(encodeMove(stsq, endsq, PAWNS, false));
            }

            legal = (color == WHITE) ? getWPawnLeftCaptures(MOVEMASK[stsq]) | getWPawnRightCaptures(MOVEMASK[stsq])
                                     : getBPawnLeftCaptures(MOVEMASK[stsq]) | getBPawnRightCaptures(MOVEMASK[stsq]);
            while (legal) {
                int endsq = bitScanForward(legal);
                legal &= legal - 1;
                checkCaptures.add(encodeMove(stsq, endsq, PAWNS, true));
            }
            // Remove this pawn from future consideration
            pawns ^= MOVEMASK[stsq];
        }
    }
    */

    uint64_t pAttackMap = (color == WHITE) 
            ? getBPawnLeftCaptures(MOVEMASK[kingSq]) | getBPawnRightCaptures(MOVEMASK[kingSq])
            : getWPawnLeftCaptures(MOVEMASK[kingSq]) | getWPawnRightCaptures(MOVEMASK[kingSq]);
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
        checks.add(encodeMove(endsq+sqDiff, endsq, PAWNS, false));
    }

    pLegal = (color == WHITE) ? getWPawnDoubleMoves(pawns)
                              : getBPawnDoubleMoves(pawns);
    pLegal &= pAttackMap;
    while (pLegal) {
        int endsq = bitScanForward(pLegal);
        pLegal &= pLegal - 1;
        checks.add(encodeMove(endsq+2*sqDiff, endsq, PAWNS, false));
    }

    // For pawn captures, we can use a similar approach, but we must consider
    // left-hand and right-hand captures separately so we can tell which
    // pawn is doing the capturing.
    int leftDiff = (color == WHITE) ? -7 : 9;
    int rightDiff = (color == WHITE) ? -9 : 7;

    pLegal = (color == WHITE) ? getWPawnLeftCaptures(pawns)
                              : getBPawnLeftCaptures(pawns);
    pLegal &= otherPieces;
    promotions = pLegal & finalRank;
    pLegal ^= promotions;
    pLegal &= pAttackMap;

    while (pLegal) {
        int endsq = bitScanForward(pLegal);
        pLegal &= pLegal-1;
        checkCaptures.add(encodeMove(endsq+leftDiff, endsq, PAWNS, true));
    }

    pLegal = (color == WHITE) ? getWPawnRightCaptures(pawns)
                              : getBPawnRightCaptures(pawns);
    pLegal &= otherPieces;
    promotions = pLegal & finalRank;
    pLegal ^= promotions;
    pLegal &= pAttackMap;

    while (pLegal) {
        int endsq = bitScanForward(pLegal);
        pLegal &= pLegal-1;
        checkCaptures.add(encodeMove(endsq+rightDiff, endsq, PAWNS, true));
    }

    uint64_t knights = pieces[color][KNIGHTS] & kingParity;
    uint64_t nAttackMap = getKnightSquares(kingSq);
    while (knights) {
        int stsq = bitScanForward(knights);
        knights &= knights-1;
        uint64_t nSq = getKnightSquares(stsq);
        // Get any bishops, rooks, queens attacking king after knight has moved
        uint64_t xrays = getXRays(color, kingSq, color, MOVEMASK[stsq]);
        // If none of these xray-ers are new, no discovered check.
        // Otherwise, we have an xray-er, and any move by this piece will
        // give discovered check
        if (!(xrays & invAttackMap))
            nSq &= nAttackMap;

        addMovesToList(checks, KNIGHTS, stsq, nSq, false);
        addMovesToList(checkCaptures, KNIGHTS, stsq, nSq, true, otherPieces);
    }

    uint64_t bishops = pieces[color][BISHOPS] & kingParity;
    uint64_t bAttackMap = getBishopSquares(kingSq);
    while (bishops) {
        int stsq = bitScanForward(bishops);
        bishops &= bishops-1;
        uint64_t bSq = getBishopSquares(stsq);
        uint64_t xrays = getXRays(color, kingSq, color, MOVEMASK[stsq]);
        if (!(xrays & invAttackMap))
            bSq &= bAttackMap;

        addMovesToList(checks, BISHOPS, stsq, bSq, false);
        addMovesToList(checkCaptures, BISHOPS, stsq, bSq, true, otherPieces);
    }

    uint64_t rooks = pieces[color][ROOKS];
    uint64_t rAttackMap = getRookSquares(kingSq);
    while (rooks) {
        int stsq = bitScanForward(rooks);
        rooks &= rooks-1;
        uint64_t rSq = getRookSquares(stsq);
        uint64_t xrays = getXRays(color, kingSq, color, MOVEMASK[stsq]);
        if (!(xrays & invAttackMap))
            rSq &= rAttackMap;

        addMovesToList(checks, ROOKS, stsq, rSq, false);
        addMovesToList(checkCaptures, ROOKS, stsq, rSq, true, otherPieces);
    }

    uint64_t queens = pieces[color][QUEENS];
    uint64_t qAttackMap = getQueenSquares(kingSq);
    while (queens) {
        int stsq = bitScanForward(queens);
        queens &= queens-1;
        uint64_t qSq = getQueenSquares(stsq) & qAttackMap;

        addMovesToList(checks, QUEENS, stsq, qSq, false);
        addMovesToList(checkCaptures, QUEENS, stsq, qSq, true, otherPieces);
    }

    // Put captures before quiet moves
    for (unsigned int i = 0; i < checks.size(); i++) {
        checkCaptures.add(checks.get(i));
    }
    return checkCaptures;
}

// Generate moves that (sort of but not really) get out of check
// This can only be used if we know the side to move is in check
// Optimizations include looking for double check (king moves only),
// otherwise we can only capture the checker or block if it is an xray piece
MoveList Board::getPseudoLegalCheckEscapes(int color) {
    MoveList captures, blocks;

    int kingSq = bitScanForward(pieces[color][KINGS]);
    uint64_t otherPieces = allPieces[color^1];
    uint64_t attackMap = getAttackMap(color^1, kingSq);
    // Consider only captures of pieces giving check
    otherPieces &= attackMap;

    // If double check, we can only move the king
    if (count(otherPieces) >= 2) {
        uint64_t kingSqs = getKingSquares(kingSq);

        addMovesToList(captures, KINGS, kingSq, kingSqs, true,
            allPieces[color^1]);
        addMovesToList(captures, KINGS, kingSq, kingSqs, false);
        return captures;
    }

    addPawnMovesToList(blocks, color);
    addPawnCapturesToList(captures, color, otherPieces, true);

    // If bishops, rooks, or queens, get bitboard of attack path so we
    // can intersect with legal moves to get legal block moves
    uint64_t xraySqs = 0;
    int attackerSq = bitScanForward(otherPieces);
    int attackerType = getCapturedPiece(color^1, attackerSq);
    if (attackerType == BISHOPS)
        xraySqs = getBishopSquares(attackerSq);
    else if (attackerType == ROOKS)
        xraySqs = getRookSquares(attackerSq);
    else if (attackerType == QUEENS)
        xraySqs = getQueenSquares(attackerSq);

    uint64_t knights = pieces[color][KNIGHTS];
    while (knights) {
        int stsq = bitScanForward(knights);
        knights &= knights-1;
        uint64_t nSq = getKnightSquares(stsq);

        addMovesToList(blocks, KNIGHTS, stsq, nSq & xraySqs, false);

        uint64_t legal = nSq & otherPieces;
        while (legal) {
            int endsq = bitScanForward(legal);
            legal &= legal-1;
            captures.add(encodeMove(stsq, endsq, KNIGHTS, true));
        }
    }

    uint64_t bishops = pieces[color][BISHOPS];
    while (bishops) {
        int stsq = bitScanForward(bishops);
        bishops &= bishops-1;
        uint64_t bSq = getBishopSquares(stsq);

        addMovesToList(blocks, BISHOPS, stsq, bSq & xraySqs, false);

        uint64_t legal = bSq & otherPieces;
        while (legal) {
            int endsq = bitScanForward(legal);
            legal &= legal-1;
            captures.add(encodeMove(stsq, endsq, BISHOPS, true));
        }
    }

    uint64_t rooks = pieces[color][ROOKS];
    while (rooks) {
        int stsq = bitScanForward(rooks);
        rooks &= rooks-1;
        uint64_t rSq = getRookSquares(stsq);

        addMovesToList(blocks, ROOKS, stsq, rSq & xraySqs, false);

        uint64_t legal = rSq & otherPieces;
        while (legal) {
            int endsq = bitScanForward(legal);
            legal &= legal-1;
            captures.add(encodeMove(stsq, endsq, ROOKS, true));
        }
    }

    uint64_t queens = pieces[color][QUEENS];
    while (queens) {
        int stsq = bitScanForward(queens);
        queens &= queens-1;
        uint64_t qSq = getQueenSquares(stsq);

        addMovesToList(blocks, QUEENS, stsq, qSq & xraySqs, false);

        uint64_t legal = qSq & otherPieces;
        while (legal) {
            int endsq = bitScanForward(legal);
            legal &= legal-1;
            captures.add(encodeMove(stsq, endsq, QUEENS, true));
        }
    }

    uint64_t kings = pieces[color][KINGS];
    int stsqK = bitScanForward(kings);
    uint64_t kingSqs = getKingSquares(stsqK);

    addMovesToList(blocks, KINGS, stsqK, kingSqs, false);
    addMovesToList(captures, KINGS, stsqK, kingSqs, true, allPieces[color^1]);

    // Put captures before blocking moves
    for (unsigned int i = 0; i < blocks.size(); i++) {
        captures.add(blocks.get(i));
    }

    return captures;
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

        addPromotionsToList(quiets, stSq, endSq, false);
    }
    while (pLegal) {
        int endsq = bitScanForward(pLegal);
        pLegal &= pLegal - 1;
        quiets.add(encodeMove(endsq+sqDiff, endsq, PAWNS, false));
    }

    pLegal = (color == WHITE) ? getWPawnDoubleMoves(pawns)
                              : getBPawnDoubleMoves(pawns);
    while (pLegal) {
        int endsq = bitScanForward(pLegal);
        pLegal &= pLegal - 1;
        quiets.add(encodeMove(endsq+2*sqDiff, endsq, PAWNS, false));
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

            addPromotionsToList(captures, endSq+leftDiff, endSq, true);
        }
    }
    while (legal) {
        int endsq = bitScanForward(legal);
        legal &= legal-1;
        captures.add(encodeMove(endsq+leftDiff, endsq, PAWNS, true));
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

            addPromotionsToList(captures, endSq+rightDiff, endSq, true);
        }
    }
    while (legal) {
        int endsq = bitScanForward(legal);
        legal &= legal-1;
        captures.add(encodeMove(endsq+rightDiff, endsq, PAWNS, true));
    }

    // If there are en passants possible...
    if (epCaptureFile != NO_EP_POSSIBLE) {
        int victimSq = epVictimSquare(color^1, epCaptureFile);
        // capturer's destination square is either the rank above (white) or
        // below (black) the victim square
        int rankDiff = (color == WHITE) ? 8 : -8;
        // The capturer's start square is either 1 to the left or right of victim
        uint64_t taker = (MOVEMASK[victimSq] << 1) & NOTA & pieces[color][PAWNS];
        if (taker)
            captures.add(encodeMove(victimSq+1, victimSq+rankDiff, PAWNS, true));
        else {
            taker = (MOVEMASK[victimSq] >> 1) & NOTH & pieces[color][PAWNS];
            if (taker)
                captures.add(encodeMove(victimSq-1, victimSq+rankDiff, PAWNS, true));
        }
    }
}

// Helper function that processes a bitboard of legal moves and adds all
// moves into a list.
void Board::addMovesToList(MoveList &moves, int pieceID, int stSq,
    uint64_t allEndSqs, bool isCapture, uint64_t otherPieces) {

    uint64_t intersect = (isCapture) ? otherPieces : ~(allPieces[WHITE] | allPieces[BLACK]);
    uint64_t legal = allEndSqs & intersect;
    while (legal) {
        int endSq = bitScanForward(legal);
        legal &= legal-1;
        moves.add(encodeMove(stSq, endSq, pieceID, isCapture));
    }
}

void Board::addPromotionsToList(MoveList &moves, int stSq, int endSq, bool isCapture) {
    Move mk = encodeMove(stSq, endSq, PAWNS, isCapture);
    mk = setPromotion(mk, KNIGHTS);
    Move mb = encodeMove(stSq, endSq, PAWNS, isCapture);
    mb = setPromotion(mb, BISHOPS);
    Move mr = encodeMove(stSq, endSq, PAWNS, isCapture);
    mr = setPromotion(mr, ROOKS);
    Move mq = encodeMove(stSq, endSq, PAWNS, isCapture);
    mq = setPromotion(mq, QUEENS);
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
         && ((allPieces[WHITE] | allPieces[BLACK]) & WHITE_KSIDE_PASSTHROUGH_SQS) == 0
         && !isInCheck(WHITE)) {
            // Check for castling through check
            if (getAttackMap(BLACK, 5) == 0) {
                Move m = encodeMove(4, 6, KINGS, false);
                m = setCastle(m, true);
                moves.add(m);
            }
        }
        if ((castlingRights & WHITEQSIDE)
         && ((allPieces[WHITE] | allPieces[BLACK]) & WHITE_QSIDE_PASSTHROUGH_SQS) == 0
         && !isInCheck(WHITE)) {
            if (getAttackMap(BLACK, 3) == 0) {
                Move m = encodeMove(4, 2, KINGS, false);
                m = setCastle(m, true);
                moves.add(m);
            }
        }
    }
    else {
        if ((castlingRights & BLACKKSIDE)
         && ((allPieces[WHITE] | allPieces[BLACK]) & BLACK_KSIDE_PASSTHROUGH_SQS) == 0
         && !isInCheck(BLACK)) {
            if (getAttackMap(WHITE, 61) == 0) {
                Move m = encodeMove(60, 62, KINGS, false);
                m = setCastle(m, true);
                moves.add(m);
            }
        }
        if ((castlingRights & BLACKQSIDE)
         && ((allPieces[WHITE] | allPieces[BLACK]) & BLACK_QSIDE_PASSTHROUGH_SQS) == 0
         && !isInCheck(BLACK)) {
            if (getAttackMap(WHITE, 59) == 0) {
                Move m = encodeMove(60, 58, KINGS, false);
                m = setCastle(m, true);
                moves.add(m);
            }
        }
    }
}


//-----------------------Useful bitboard info generators:-----------------------
//------------------------------attack maps, etc.-------------------------------

// Get the attack map of all potential x-ray pieces (bishops, rooks, queens)
// after a blocker has been removed.
uint64_t Board::getXRays(int color, int sq, int blockerColor, uint64_t blocker) {
    allPieces[blockerColor] ^= blocker;

    uint64_t bishops = pieces[color][BISHOPS] & ~blocker;
    uint64_t rooks = pieces[color][ROOKS] & ~blocker;
    uint64_t queens = pieces[color][QUEENS] & ~blocker;

    uint64_t xRayMap = (getBishopSquares(sq) & (bishops | queens))
                     | (getRookSquares(sq) & (rooks | queens));

    allPieces[blockerColor] ^= blocker;

    return xRayMap;
}

// Get the attack map of all potential x-ray pieces (bishops, rooks, queens)
// with no blockers removed. Used to compare with the results of the function
// above to find pins/discovered checks
uint64_t Board::getInitXRays(int color, int sq) {
    uint64_t bishops = pieces[color][BISHOPS];
    uint64_t rooks = pieces[color][ROOKS];
    uint64_t queens = pieces[color][QUEENS];

    return (getBishopSquares(sq) & (bishops | queens))
         | (getRookSquares(sq) & (rooks | queens));
}

// Given a color and a square, returns all pieces of the color that attack the
// square. Useful for checks, captures
uint64_t Board::getAttackMap(int color, int sq) {
    uint64_t pawnCap = (color == WHITE)
                     ? getBPawnLeftCaptures(MOVEMASK[sq]) | getBPawnRightCaptures(MOVEMASK[sq])
                     : getWPawnLeftCaptures(MOVEMASK[sq]) | getWPawnRightCaptures(MOVEMASK[sq]);
    return (pawnCap & pieces[color][PAWNS])
         | (getKnightSquares(sq) & pieces[color][KNIGHTS])
         | (getBishopSquares(sq) & (pieces[color][BISHOPS] | pieces[color][QUEENS]))
         | (getRookSquares(sq) & (pieces[color][ROOKS] | pieces[color][QUEENS]))
         | (getKingSquares(sq) & pieces[color][KINGS]);
}

// Given the end square of a capture, find the opposing piece that is captured.
int Board::getCapturedPiece(int colorCaptured, int endSq) {
    uint64_t endSingle = MOVEMASK[endSq];
    for (int pieceID = 0; pieceID < 5; pieceID++) {
        if (pieces[colorCaptured][pieceID] & endSingle)
            return pieceID;
    }
    // The default is when the capture destination is an empty square.
    // This indicates an en passant (and hopefully not an error).
    return -1;
}

// Returns true if a move puts the opponent in check
// Precondition: opposing king is not already in check (obviously)
bool Board::isCheckMove(Move m, int color) {
    int kingSq = bitScanForward(pieces[color^1][KINGS]);

    // See if move is a direct check
    uint64_t attackMap = 0;
    switch (getPiece(m)) {
        case PAWNS:
            attackMap = (color == WHITE) 
                ? getBPawnLeftCaptures(MOVEMASK[kingSq]) | getBPawnRightCaptures(MOVEMASK[kingSq])
                : getWPawnLeftCaptures(MOVEMASK[kingSq]) | getWPawnRightCaptures(MOVEMASK[kingSq]);
            break;
        case KNIGHTS:
            attackMap = getKnightSquares(kingSq);
            break;
        case BISHOPS:
            attackMap = getBishopSquares(kingSq);
            break;
        case ROOKS:
            attackMap = getRookSquares(kingSq);
            break;
        case QUEENS:
            attackMap = getQueenSquares(kingSq);
            break;
        case KINGS:
            // keep attackMap 0
            break;
    }
    if (MOVEMASK[getEndSq(m)] & attackMap)
        return true;

    // See if move is a discovered check
    // Get a bitboard of all pieces that could possibly xray
    uint64_t xrayPieces = pieces[color][BISHOPS] | pieces[color][ROOKS] | pieces[color][QUEENS];

    // Get any bishops, rooks, queens attacking king after piece has moved
    uint64_t xrays = getXRays(color, kingSq, color, MOVEMASK[getStartSq(m)]);
    // If there is an xray piece attacking the king square after the piece has
    // moved, we have discovered check
    if (xrays & xrayPieces)
        return true;

    // If not direct or discovered check, then not a check
    return false;
}


//----------------------King: check, checkmate, stalemate-----------------------
bool Board::isInCheck(int color) {
    int sq = bitScanForward(pieces[color][KINGS]);

    return getAttackMap(color^1, sq);
}

bool Board::isWInMate() {
    MoveList moves = getAllLegalMoves(WHITE);
    bool isInMate = false;
    if (moves.size() == 0 && isInCheck(WHITE))
        isInMate = true;
    
    return isInMate;
}

bool Board::isBInMate() {
    MoveList moves = getAllLegalMoves(BLACK);
    bool isInMate = false;
    if (moves.size() == 0 && isInCheck(BLACK))
        isInMate = true;

    return isInMate;
}

// TODO Includes 3-fold repetition draw for now.
bool Board::isStalemate(int sideToMove) {
    MoveList moves = getAllLegalMoves(sideToMove);
    bool isInStalemate = false;

    if (moves.size() == 0 && !isInCheck(sideToMove))
        isInStalemate = true;

    return isInStalemate;
}

bool Board::isDraw() {
    if (fiftyMoveCounter >= 100) return true;

    if ((twoFoldSqs >> 63) == 0) {
        uint8_t *pTwoFoldSqs = (uint8_t*) (&twoFoldSqs);
        
        if ( (pTwoFoldSqs[7] == pTwoFoldSqs[2])
          && (pTwoFoldSqs[3] == pTwoFoldSqs[6])
          && (pTwoFoldSqs[5] == pTwoFoldSqs[0])
          && (pTwoFoldSqs[1] == pTwoFoldSqs[4])) {
            return true;
        }
    }
    
    return false;
}

//------------------------Evaluation and Move Ordering--------------------------
/*
 * Evaluates the current board position in hundredths of pawns. White is
 * positive and black is negative in traditional negamax format.
 */
int Board::evaluate() {
    // Tempo bonus
    int value = (playerToMove == WHITE) ? TEMPO_VALUE : -TEMPO_VALUE;

    // material
    int whiteMaterial = PAWN_VALUE * count(pieces[WHITE][PAWNS])
            + KNIGHT_VALUE * count(pieces[WHITE][KNIGHTS])
            + BISHOP_VALUE * count(pieces[WHITE][BISHOPS])
            + ROOK_VALUE * count(pieces[WHITE][ROOKS])
            + QUEEN_VALUE * count(pieces[WHITE][QUEENS]);
    int blackMaterial = PAWN_VALUE * count(pieces[BLACK][PAWNS])
            + KNIGHT_VALUE * count(pieces[BLACK][KNIGHTS])
            + BISHOP_VALUE * count(pieces[BLACK][BISHOPS])
            + ROOK_VALUE * count(pieces[BLACK][ROOKS])
            + QUEEN_VALUE * count(pieces[BLACK][QUEENS]);
    
    // compute endgame factor which is between 0 and EG_FACTOR_RES, inclusive
    int egFactor = EG_FACTOR_RES - (whiteMaterial + blackMaterial - START_VALUE / 2) * EG_FACTOR_RES / START_VALUE;
    egFactor = max(0, min(EG_FACTOR_RES, egFactor));
    
    value += whiteMaterial + (PAWN_VALUE_EG - PAWN_VALUE) * count(pieces[WHITE][PAWNS]) * egFactor / EG_FACTOR_RES;
    value -= blackMaterial + (PAWN_VALUE_EG - PAWN_VALUE) * count(pieces[BLACK][PAWNS]) * egFactor / EG_FACTOR_RES;
    
    // bishop pair bonus
    if ((pieces[WHITE][BISHOPS] & LIGHT) && (pieces[WHITE][BISHOPS] & DARK))
        value += BISHOP_PAIR_VALUE;
    if ((pieces[BLACK][BISHOPS] & LIGHT) && (pieces[BLACK][BISHOPS] & DARK))
        value -= BISHOP_PAIR_VALUE;
    
    // piece square tables
    int midgamePSTVal = 0;
    int endgamePSTVal = 0;
    // White pieces
    for (int pieceID = 0; pieceID < 6; pieceID++) {
        uint64_t bitboard = pieces[0][pieceID];
        while (bitboard) {
            int i = bitScanForward(bitboard);
            // Invert the board for white side
            i = ((7 - (i >> 3)) << 3) + (i & 7);
            bitboard &= bitboard - 1;
            midgamePSTVal += midgamePieceValues[pieceID][i];
            endgamePSTVal += endgamePieceValues[pieceID][i];
        }
    }
    // Black pieces
    for (int pieceID = 0; pieceID < 6; pieceID++)  {
        uint64_t bitboard = pieces[1][pieceID];
        while (bitboard) {
            int i = bitScanForward(bitboard);
            bitboard &= bitboard - 1;
            midgamePSTVal -= midgamePieceValues[pieceID][i];
            endgamePSTVal -= endgamePieceValues[pieceID][i];
        }
    }
    // Adjust values according to material left on board
    value += midgamePSTVal * (EG_FACTOR_RES - egFactor) / EG_FACTOR_RES;
    value += endgamePSTVal * egFactor / EG_FACTOR_RES;

    // Consider attacked squares near king
    uint64_t wksq = getKingSquares(bitScanForward(pieces[WHITE][KINGS]));
    uint64_t bksq = getKingSquares(bitScanForward(pieces[BLACK][KINGS]));
    
    // Pawn shield bonus (files ABC, FGH)
    value += (25 * egFactor / EG_FACTOR_RES) * count(wksq & pieces[WHITE][PAWNS] & 0xe7e7e7e7e7e7e7e7);
    value -= (25 * egFactor / EG_FACTOR_RES) * count(bksq & pieces[BLACK][PAWNS] & 0xe7e7e7e7e7e7e7e7);
    
    value += getPseudoMobility(WHITE);
    value -= getPseudoMobility(BLACK);

    // Pawn structure
    // Passed pawns
    uint64_t notwp = pieces[WHITE][PAWNS];
    uint64_t notbp = pieces[BLACK][PAWNS];
    // These act as blockers for the flood fill: if opposing pawns are on the
    // same or an adjacent rank, your pawn is not passed.
    notwp |= ((notwp >> 1) & NOTH) | ((notwp << 1) & NOTA);
    notbp |= ((notbp >> 1) & NOTH) | ((notbp << 1) & NOTA);
    notwp = ~notwp;
    notbp = ~notbp;
    uint64_t tempwp = pieces[WHITE][PAWNS];
    uint64_t tempbp = pieces[BLACK][PAWNS];
    // Flood fill to simulate pushing the pawn to the 7th (or 2nd) rank
    for(int i = 0; i < 6; i++) {
        tempwp |= (tempwp << 8) & notbp;
        tempbp |= (tempbp >> 8) & notwp;
    }
    // Pawns that made it without being blocked are passed
    value += (10 + 50 * egFactor / EG_FACTOR_RES) * count(tempwp & RANKS[7]);
    value -= (10 + 50 * egFactor / EG_FACTOR_RES) * count(tempbp & RANKS[0]);

    int wPawnCtByFile[8];
    int bPawnCtByFile[8];
    for (int i = 0; i < 8; i++) {
        wPawnCtByFile[i] = count(pieces[WHITE][PAWNS] & FILES[i]);
        bPawnCtByFile[i] = count(pieces[BLACK][PAWNS] & FILES[i]);
    }

    // Doubled pawns
    // 0 pawns on file: 0 cp
    // 1 pawn on file: 0 cp (each pawn worth 100 cp)
    // 2 pawns on file: -24 cp (each pawn worth 88 cp)
    // 3 pawns on file: -48 cp (each pawn worth 84 cp)
    // 4 pawns on file: -144 cp (each pawn worth 64 cp)
    for (int i = 0; i < 8; i++) {
        value -= 24 * (wPawnCtByFile[i] - 1) * (wPawnCtByFile[i] / 2);
        value += 24 * (bPawnCtByFile[i] - 1) * (bPawnCtByFile[i] / 2);
    }

    // Isolated pawns
    uint64_t wp = 0, bp = 0;
    for (int i = 0; i < 8; i++) {
        wp |= (bool) (wPawnCtByFile[i]);
        bp |= (bool) (bPawnCtByFile[i]);
        wp <<= 1;
        bp <<= 1;
    }
    // If there are pawns on either adjacent file, we remove this pawn
    wp &= ~((wp >> 1) | (wp << 1));
    bp &= ~((bp >> 1) | (bp << 1));
    value -= 20 * count(wp);
    value += 20 * count(bp);

    return value;
}

// Scores the board for a player based on estimates of mobility
int Board::getPseudoMobility(int color) {
    int result = 0;
    uint64_t knights = pieces[color][KNIGHTS];
    uint64_t bishops = pieces[color][BISHOPS];
    uint64_t rooks = pieces[color][ROOKS];
    uint64_t queens = pieces[color][QUEENS];
    uint64_t pieces = allPieces[color];

    while (knights != 0) {
        int single = bitScanForward(knights);
        knights &= knights-1;

        uint64_t legal = getKnightSquares(single) & ~pieces;

        result += knightMobility[count(legal)];
    }

    while (bishops != 0) {
        int single = bitScanForward(bishops);
        bishops &= bishops-1;

        uint64_t legal = getBishopSquares(single) & ~pieces;

        result += bishopMobility[count(legal)];
    }

    while (rooks != 0) {
        int single = bitScanForward(rooks);
        rooks &= rooks-1;

        uint64_t legal = getRookSquares(single) & ~pieces;

        result += rookMobility[count(legal)];
    }

    while (queens != 0) {
        int single = bitScanForward(queens);
        queens &= queens-1;

        uint64_t legal = getQueenSquares(single) & ~pieces;

        result += queenMobility[count(legal)];
    }

    return result;
}

// Gets the endgame factor, which adjusts the evaluation based on how much
// material is left on the board.
int Board::getEGFactor() {
    int whiteMaterial = PAWN_VALUE * count(pieces[WHITE][PAWNS])
            + KNIGHT_VALUE * count(pieces[WHITE][KNIGHTS])
            + BISHOP_VALUE * count(pieces[WHITE][BISHOPS])
            + ROOK_VALUE * count(pieces[WHITE][ROOKS])
            + QUEEN_VALUE * count(pieces[WHITE][QUEENS]);
    int blackMaterial = PAWN_VALUE * count(pieces[BLACK][PAWNS])
            + KNIGHT_VALUE * count(pieces[BLACK][KNIGHTS])
            + BISHOP_VALUE * count(pieces[BLACK][BISHOPS])
            + ROOK_VALUE * count(pieces[BLACK][ROOKS])
            + QUEEN_VALUE * count(pieces[BLACK][QUEENS]);
    int egFactor = EG_FACTOR_RES - (whiteMaterial + blackMaterial - START_VALUE / 2) * EG_FACTOR_RES / START_VALUE;
    return max(0, min(EG_FACTOR_RES, egFactor));
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
    uint64_t single = getLeastValuableAttacker(attackers, color, piece);
    // Get value of piece initially being captured. If the destination square is
    // empty, then the capture is an en passant.
    gain[d] = valueOfPiece(getCapturedPiece(color^1, sq));

    do {
        d++; // next depth
        color ^= 1;
        gain[d]  = valueOfPiece(piece) - gain[d-1];
        if (-gain[d-1] < 0 && gain[d] < 0) // pruning for stand pat
            break;
        attackers ^= single; // remove used attacker
        used |= single;
        attackers |= getXRays(WHITE, sq, color, used) | getXRays(BLACK, sq, color, used);
        single = getLeastValuableAttacker(attackers, color, piece);
    } while (single);

    while (--d)
        gain[d-1]= -((-gain[d-1] > gain[d]) ? -gain[d-1] : gain[d]);

    return gain[0];
}

int Board::valueOfPiece(int piece) {
    switch(piece) {
        case -1: // en passant
        case PAWNS:
            return PAWN_VALUE;
        case KNIGHTS:
            return KNIGHT_VALUE;
        case BISHOPS:
            return BISHOP_VALUE;
        case ROOKS:
            return ROOK_VALUE;
        case QUEENS:
            return QUEEN_VALUE;
        case KINGS:
            return MATE_SCORE;
    }
    cerr << "Error in Board::valueOfPiece() " << piece << endl;
    return -1;
}

// Calculates a score for Most Valuable Victim / Least Valuable Attacker
// capture ordering.
int Board::getMVVLVAScore(int color, Move m) {
    int endSq = getEndSq(m);
    int attacker = getPiece(m);
    int victim = getCapturedPiece(color^1, endSq);
    if (attacker == KINGS)
        attacker = -1;

    return (victim * 8) + (4 - attacker);
}

// Returns a score from the initial capture
// This helps reduce the number of times SEE must be used in quiescence search,
// since if we have a losing trade after capture-recapture our opponent could
// likely stand pat, or we could've captured with a better piece
int Board::getExchangeScore(int color, Move m) {
    int endSq = getEndSq(m);
    int attacker = getPiece(m);
    int victim = getCapturedPiece(color^1, endSq);
    return victim - attacker;
}


//-----------------------------MOVE GENERATION----------------------------------
uint64_t Board::getWPawnSingleMoves(uint64_t pawns) {
    uint64_t open = ~(allPieces[WHITE] | allPieces[BLACK]);
    return (pawns << 8) & open;
}

uint64_t Board::getBPawnSingleMoves(uint64_t pawns) {
    uint64_t open = ~(allPieces[WHITE] | allPieces[BLACK]);
    return (pawns >> 8) & open;
}

uint64_t Board::getWPawnDoubleMoves(uint64_t pawns) {
    uint64_t open = ~(allPieces[WHITE] | allPieces[BLACK]);
    uint64_t temp = (pawns << 8) & open;
    pawns = (temp << 8) & open & RANKS[3];
    return pawns;
}

uint64_t Board::getBPawnDoubleMoves(uint64_t pawns) {
    uint64_t open = ~(allPieces[WHITE] | allPieces[BLACK]);
    uint64_t temp = (pawns >> 8) & open;
    pawns = (temp >> 8) & open & RANKS[4];
    return pawns;
}

uint64_t Board::getWPawnLeftCaptures(uint64_t pawns) {
    return (pawns << 7) & NOTH;
}

uint64_t Board::getBPawnLeftCaptures(uint64_t pawns) {
    return (pawns >> 9) & NOTH;
}

uint64_t Board::getWPawnRightCaptures(uint64_t pawns) {
    return (pawns << 9) & NOTA;
}

uint64_t Board::getBPawnRightCaptures(uint64_t pawns) {
    return (pawns >> 7) & NOTA;
}

uint64_t Board::getKnightSquares(int single) {
    return KNIGHTMOVES[single];
}

uint64_t Board::getBishopSquares(int single) {
    uint64_t occ = allPieces[WHITE] | allPieces[BLACK];

    uint64_t diagAtt = diagAttacks(occ, single);
    uint64_t antiDiagAtt = antiDiagAttacks(occ, single);

    return diagAtt | antiDiagAtt;
}

uint64_t Board::getRookSquares(int single) {
    uint64_t occ = allPieces[WHITE] | allPieces[BLACK];

    uint64_t rankAtt = rankAttacks(occ, single);
    uint64_t fileAtt = fileAttacks(occ, single);

    return rankAtt | fileAtt;
}

uint64_t Board::getQueenSquares(int single) {
    uint64_t occ = allPieces[WHITE] | allPieces[BLACK];

    uint64_t rankAtt = rankAttacks(occ, single);
    uint64_t fileAtt = fileAttacks(occ, single);
    uint64_t diagAtt = diagAttacks(occ, single);
    uint64_t antiDiagAtt = antiDiagAttacks(occ, single);

    return rankAtt | fileAtt | diagAtt | antiDiagAtt;
}

uint64_t Board::getKingSquares(int single) {
    return KINGMOVES[single];
}

// Kindergarten bitboard slider attacks
// http://chessprogramming.wikispaces.com/Kindergarten+Bitboards
uint64_t Board::rankAttacks(uint64_t occ, int single) {
    occ = (RANKRAY[single] & occ) * FILES[1] >> 58;
    return (RANKRAY[single] & rankArray[single&7][occ]);
}

uint64_t Board::fileAttacks(uint64_t occ, int single) {
    occ = AFILE & (occ >> (single & 7));
    occ = (0x0004081020408000 * occ) >> 58;
    return (fileArray[single>>3][occ] << (single & 7));
}

uint64_t Board::diagAttacks(uint64_t occ, int single) {
    occ = (DIAGRAY[single] & occ) * FILES[1] >> 58;
    return (DIAGRAY[single] & rankArray[single&7][occ]);
}

uint64_t Board::antiDiagAttacks(uint64_t occ, int single) {
    occ = (ANTIDIAGRAY[single] & occ) * FILES[1] >> 58;
    return (ANTIDIAGRAY[single] & rankArray[single&7][occ]);
}

// Getter methods
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

uint64_t Board::getWhitePieces() {
    return allPieces[WHITE];
}

uint64_t Board::getBlackPieces() {
    return allPieces[BLACK];
}

int *Board::getMailbox() {
    int *result = new int[64];
    for (int i = 0; i < 64; i++) {
        result[i] = -1;
    }
    for (int i = 0; i < 6; i++) {
        uint64_t bitboard = pieces[0][i];
        while (bitboard) {
            result[bitScanForward(bitboard)] = i;
            bitboard &= bitboard - 1;
        }
    }
    for (int i = 0; i < 6; i++) {
        uint64_t bitboard = pieces[1][i];
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

string Board::toString() {
    int *mailbox = getMailbox();
    string result = "";
    for (int i = 56; i >= 0; i++) {
        switch (mailbox[i]) {
            case -1: // empty
                result += "-";
                break;
            case 0: // white pawn
                result += "P";
                break;
            case 6: // black pawn
                result += "p";
                break;
            case 1: // white knight
                result += "N";
                break;
            case 7: // black knight
                result += "n";
                break;
            case 2: // white bishop
                result += "B";
                break;
            case 8: // black bishop
                result += "b";
                break;
            case 3: // white rook
                result += "R";
                break;
            case 9: // black rook
                result += "r";
                break;
            case 4: // white queen
                result += "Q";
                break;
            case 10: // black queen
                result += "q";
                break;
            case 5: // white king
                result += "K";
                break;
            case 11: // black king
                result += "k";
                break;
        }
        if (i % 8 == 7) {
            result += "\n";
            i -= 16;
        }
    }
    delete[] mailbox;
    return result;
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

// Dumb7Fill
uint64_t southAttacks(uint64_t rooks, uint64_t empty) {
    uint64_t flood = rooks;
    flood |= rooks = (rooks >> 8) & empty;
    flood |= rooks = (rooks >> 8) & empty;
    flood |= rooks = (rooks >> 8) & empty;
    flood |= rooks = (rooks >> 8) & empty;
    flood |= rooks = (rooks >> 8) & empty;
    flood |=         (rooks >> 8) & empty;
    return           (flood >> 8);
}

uint64_t northAttacks(uint64_t rooks, uint64_t empty) {
    uint64_t flood = rooks;
    flood |= rooks = (rooks << 8) & empty;
    flood |= rooks = (rooks << 8) & empty;
    flood |= rooks = (rooks << 8) & empty;
    flood |= rooks = (rooks << 8) & empty;
    flood |= rooks = (rooks << 8) & empty;
    flood |=         (rooks << 8) & empty;
    return           (flood << 8);
}

uint64_t eastAttacks(uint64_t rooks, uint64_t empty) {
    uint64_t flood = rooks;
    empty &= NOTA;
    flood |= rooks = (rooks << 1) & empty;
    flood |= rooks = (rooks << 1) & empty;
    flood |= rooks = (rooks << 1) & empty;
    flood |= rooks = (rooks << 1) & empty;
    flood |= rooks = (rooks << 1) & empty;
    flood |=         (rooks << 1) & empty;
    return           (flood << 1) & NOTA ;
}

uint64_t neAttacks(uint64_t bishops, uint64_t empty) {
    uint64_t flood = bishops;
    empty &= NOTA;
    flood |= bishops = (bishops << 9) & empty;
    flood |= bishops = (bishops << 9) & empty;
    flood |= bishops = (bishops << 9) & empty;
    flood |= bishops = (bishops << 9) & empty;
    flood |= bishops = (bishops << 9) & empty;
    flood |=         (bishops << 9) & empty;
    return           (flood << 9) & NOTA ;
}

uint64_t seAttacks(uint64_t bishops, uint64_t empty) {
    uint64_t flood = bishops;
    empty &= NOTA;
    flood |= bishops = (bishops >> 7) & empty;
    flood |= bishops = (bishops >> 7) & empty;
    flood |= bishops = (bishops >> 7) & empty;
    flood |= bishops = (bishops >> 7) & empty;
    flood |= bishops = (bishops >> 7) & empty;
    flood |=         (bishops >> 7) & empty;
    return           (flood >> 7) & NOTA ;
}

uint64_t westAttacks(uint64_t rooks, uint64_t empty) {
    uint64_t flood = rooks;
    empty &= NOTH;
    flood |= rooks = (rooks >> 1) & empty;
    flood |= rooks = (rooks >> 1) & empty;
    flood |= rooks = (rooks >> 1) & empty;
    flood |= rooks = (rooks >> 1) & empty;
    flood |= rooks = (rooks >> 1) & empty;
    flood |=         (rooks >> 1) & empty;
    return           (flood >> 1) & NOTH ;
}

uint64_t swAttacks(uint64_t bishops, uint64_t empty) {
    uint64_t flood = bishops;
    empty &= NOTH;
    flood |= bishops = (bishops >> 9) & empty;
    flood |= bishops = (bishops >> 9) & empty;
    flood |= bishops = (bishops >> 9) & empty;
    flood |= bishops = (bishops >> 9) & empty;
    flood |= bishops = (bishops >> 9) & empty;
    flood |=         (bishops >> 9) & empty;
    return           (flood >> 9) & NOTH ;
}

uint64_t nwAttacks(uint64_t bishops, uint64_t empty) {
    uint64_t flood = bishops;
    empty &= NOTH;
    flood |= bishops = (bishops << 7) & empty;
    flood |= bishops = (bishops << 7) & empty;
    flood |= bishops = (bishops << 7) & empty;
    flood |= bishops = (bishops << 7) & empty;
    flood |= bishops = (bishops << 7) & empty;
    flood |=         (bishops << 7) & empty;
    return           (flood << 7) & NOTH ;
}