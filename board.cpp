#include "board.h"
#include "btables.h"

// Create a board object initialized to the start position.
Board::Board() {
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
    whitePieces = 0x000000000000FFFF;
    blackPieces = 0xFFFF000000000000;

    whiteCanKCastle = true;
    blackCanKCastle = true;
    whiteCanQCastle = true;
    blackCanQCastle = true;
    whiteEPCaptureSq = 0;
    blackEPCaptureSq = 0;

    fiftyMoveCounter = 0;
    moveNumber = 1;
    playerToMove = WHITE;
}

// Create a board object from a mailbox of the current board state.
Board::Board(int *mailboxBoard, bool _whiteCanKCastle, bool _blackCanKCastle,
        bool _whiteCanQCastle, bool _blackCanQCastle, uint64_t _whiteEPCaptureSq,
        uint64_t _blackEPCaptureSq, int _fiftyMoveCounter, int _moveNumber,
        int _playerToMove) {
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
    whitePieces = pieces[0][0] | pieces[0][1] | pieces[0][2] | pieces[0][3]
                | pieces[0][4] | pieces[0][5];
    blackPieces = pieces[1][0] | pieces[1][1] | pieces[1][2] | pieces[1][3]
                | pieces[1][4] | pieces[1][5];
    
    whiteCanKCastle = _whiteCanKCastle;
    whiteCanQCastle = _whiteCanQCastle;
    blackCanKCastle = _blackCanKCastle;
    blackCanQCastle = _blackCanQCastle;
    whiteEPCaptureSq = _whiteEPCaptureSq;
    blackEPCaptureSq = _blackEPCaptureSq;
    fiftyMoveCounter = _fiftyMoveCounter;
    moveNumber = _moveNumber;
    playerToMove = _playerToMove;
}

Board::~Board() {}

Board Board::staticCopy() {
    Board result;
    for (int i = 0; i < 6; i++) {
        result.pieces[0][i] = pieces[0][i];
    }
    for (int i = 0; i < 6; i++) {
        result.pieces[1][i] = pieces[1][i];
    }
    result.whitePieces = whitePieces;
    result.blackPieces = blackPieces;
    result.whiteCanKCastle = whiteCanKCastle;
    result.whiteCanQCastle = whiteCanQCastle;
    result.blackCanKCastle = blackCanKCastle;
    result.blackCanQCastle = blackCanQCastle;
    result.whiteEPCaptureSq = whiteEPCaptureSq;
    result.blackEPCaptureSq = blackEPCaptureSq;
    result.fiftyMoveCounter = fiftyMoveCounter;
    result.moveNumber = moveNumber;
    result.playerToMove = playerToMove;
    result.twoFoldStartSqs = twoFoldStartSqs;
    result.twoFoldEndSqs = twoFoldEndSqs;
    return result;
}

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
    result->whiteCanKCastle = whiteCanKCastle;
    result->whiteCanQCastle = whiteCanQCastle;
    result->blackCanKCastle = blackCanKCastle;
    result->blackCanQCastle = blackCanQCastle;
    result->whiteEPCaptureSq = whiteEPCaptureSq;
    result->blackEPCaptureSq = blackEPCaptureSq;
    result->fiftyMoveCounter = fiftyMoveCounter;
    result->moveNumber = moveNumber;
    result->playerToMove = playerToMove;
    result->twoFoldStartSqs = twoFoldStartSqs;
    result->twoFoldEndSqs = twoFoldEndSqs;
    return result;
}

void Board::doMove(Move m, int color) {
    int pieceID = getPiece(m);
    int startSq = getStartSq(m);
    int endSq = getEndSq(m);

    // Handle null moves for null move pruning
    if (m == NULL_MOVE) {
        playerToMove = color ^ 1;
        return;
    }

    // Record current board position for two-fold repetition
    if (isCapture(m) || pieceID == PAWNS || isCastle(m)) {
        twoFoldStartSqs = 0x80808080;
        twoFoldEndSqs = 0x80808080;
    }
    else {
        twoFoldStartSqs <<= 8;
        twoFoldEndSqs <<= 8;
        twoFoldStartSqs |= (uint8_t) startSq;
        twoFoldEndSqs |= (uint8_t) endSq;
    }

    if (isCastle(m)) {
        if (endSq == 6) { // white kside
            pieces[WHITE][KINGS] &= ~MOVEMASK[4];
            pieces[WHITE][KINGS] |= MOVEMASK[6];
            pieces[WHITE][ROOKS] &= ~MOVEMASK[7];
            pieces[WHITE][ROOKS] |= MOVEMASK[5];

            whitePieces &= ~MOVEMASK[4];
            whitePieces |= MOVEMASK[6];
            whitePieces &= ~MOVEMASK[7];
            whitePieces |= MOVEMASK[5];

            whiteCanKCastle = false;
            whiteCanQCastle = false;
        }
        else if (endSq == 2) { // white qside
            pieces[WHITE][KINGS] &= ~MOVEMASK[4];
            pieces[WHITE][KINGS] |= MOVEMASK[2];
            pieces[WHITE][ROOKS] &= ~MOVEMASK[0];
            pieces[WHITE][ROOKS] |= MOVEMASK[3];

            whitePieces &= ~MOVEMASK[4];
            whitePieces |= MOVEMASK[2];
            whitePieces &= ~MOVEMASK[0];
            whitePieces |= MOVEMASK[3];

            whiteCanKCastle = false;
            whiteCanQCastle = false;
        }
        else if (endSq == 62) { // black kside
            pieces[BLACK][KINGS] &= ~MOVEMASK[60];
            pieces[BLACK][KINGS] |= MOVEMASK[62];
            pieces[BLACK][ROOKS] &= ~MOVEMASK[63];
            pieces[BLACK][ROOKS] |= MOVEMASK[61];

            blackPieces &= ~MOVEMASK[60];
            blackPieces |= MOVEMASK[62];
            blackPieces &= ~MOVEMASK[63];
            blackPieces |= MOVEMASK[61];

            blackCanKCastle = false;
            blackCanQCastle = false;
        }
        else { // black qside
            pieces[BLACK][KINGS] &= ~MOVEMASK[60];
            pieces[BLACK][KINGS] |= MOVEMASK[58];
            pieces[BLACK][ROOKS] &= ~MOVEMASK[56];
            pieces[BLACK][ROOKS] |= MOVEMASK[59];

            blackPieces &= ~MOVEMASK[60];
            blackPieces |= MOVEMASK[58];
            blackPieces &= ~MOVEMASK[56];
            blackPieces |= MOVEMASK[59];

            blackCanKCastle = false;
            blackCanQCastle = false;
        }
        whiteEPCaptureSq = 0;
        blackEPCaptureSq = 0;
        fiftyMoveCounter++;
    } // end castling
    else if (getPromotion(m)) {
        if (isCapture(m)) {
            int captureType = getCapturedPiece(color^1, endSq);
            pieces[color][PAWNS] &= ~MOVEMASK[startSq];
            pieces[color][getPromotion(m)] |= MOVEMASK[endSq];
            pieces[color^1][captureType] &= ~MOVEMASK[endSq];

            if (color == WHITE) {
                whitePieces &= ~MOVEMASK[startSq];
                whitePieces |= MOVEMASK[endSq];
                blackPieces &= ~MOVEMASK[endSq];
            }
            else {
                blackPieces &= ~MOVEMASK[startSq];
                blackPieces |= MOVEMASK[endSq];
                whitePieces &= ~MOVEMASK[endSq];
            }
        }
        else {
            pieces[color][PAWNS] &= ~MOVEMASK[startSq];
            pieces[color][getPromotion(m)] |= MOVEMASK[endSq];

            if (color == WHITE) {
                whitePieces &= ~MOVEMASK[startSq];
                whitePieces |= MOVEMASK[endSq];
            }
            else {
                blackPieces &= ~MOVEMASK[startSq];
                blackPieces |= MOVEMASK[endSq];
            }
        }

        whiteEPCaptureSq = 0;
        blackEPCaptureSq = 0;
        fiftyMoveCounter = 0;
    } // end promotion
    else if (isCapture(m)) {
        int captureType = getCapturedPiece(color^1, endSq);
        if (captureType == -1) {
            pieces[color][PAWNS] &= ~MOVEMASK[startSq];
            pieces[color][PAWNS] |= MOVEMASK[endSq];
            pieces[color^1][PAWNS] &= ~((color == WHITE) ? whiteEPCaptureSq : blackEPCaptureSq);

            if (color == WHITE) {
                whitePieces &= ~MOVEMASK[startSq];
                whitePieces |= MOVEMASK[endSq];
                blackPieces &= ~whiteEPCaptureSq;
            }
            else {
                blackPieces &= ~MOVEMASK[startSq];
                blackPieces |= MOVEMASK[endSq];
                whitePieces &= ~blackEPCaptureSq;
            }
        }
        else {
            pieces[color][pieceID] &= ~MOVEMASK[startSq];
            pieces[color][pieceID] |= MOVEMASK[endSq];
            pieces[color^1][captureType] &= ~MOVEMASK[endSq];

            if (color == WHITE) {
                whitePieces &= ~MOVEMASK[startSq];
                whitePieces |= MOVEMASK[endSq];
                blackPieces &= ~MOVEMASK[endSq];
            }
            else {
                blackPieces &= ~MOVEMASK[startSq];
                blackPieces |= MOVEMASK[endSq];
                whitePieces &= ~MOVEMASK[endSq];
            }
        }
        whiteEPCaptureSq = 0;
        blackEPCaptureSq = 0;
        fiftyMoveCounter = 0;
    } // end capture
    else {
        pieces[color][pieceID] &= ~MOVEMASK[startSq];
        pieces[color][pieceID] |= MOVEMASK[endSq];

        if (color == WHITE) {
            whitePieces &= ~MOVEMASK[startSq];
            whitePieces |= MOVEMASK[endSq];
        }
        else {
            blackPieces &= ~MOVEMASK[startSq];
            blackPieces |= MOVEMASK[endSq];
        }

        // check for en passant
        if (pieceID == PAWNS) {
            if (color == WHITE && startSq/8 == 1 && endSq/8 == 3) {
                blackEPCaptureSq = MOVEMASK[endSq];
                whiteEPCaptureSq = 0;
            }
            else if (startSq/8 == 6 && endSq/8 == 4) {
                whiteEPCaptureSq = MOVEMASK[endSq];
                blackEPCaptureSq = 0;
            }
            else {
                whiteEPCaptureSq = 0;
                blackEPCaptureSq = 0;
            }
            fiftyMoveCounter = 0;
        }
        else {
            whiteEPCaptureSq = 0;
            blackEPCaptureSq = 0;
            fiftyMoveCounter++;
        }
    } // end normal move

    // change castling flags
    if (pieceID == KINGS) {
        if (color == WHITE) {
            whiteCanKCastle = false;
            whiteCanQCastle = false;
        }
        else {
            blackCanKCastle = false;
            blackCanQCastle = false;
        }
    }
    else {
        if (whiteCanKCastle || whiteCanQCastle) {
            int whiteR = (int)(RANKS[0] & pieces[WHITE][ROOKS]);
            if ((whiteR & 0x80) == 0)
                whiteCanKCastle = false;
            if ((whiteR & 1) == 0)
                whiteCanQCastle = false;
        }
        if (blackCanKCastle || blackCanQCastle) {
            int blackR = (int)((RANKS[7] & pieces[BLACK][ROOKS]) >> 56);
            if ((blackR & 0x80) == 0)
                blackCanKCastle = false;
            if ((blackR & 1) == 0)
                blackCanQCastle = false;
        }
    } // end castling flags

    if (color == BLACK)
        moveNumber++;
    playerToMove = color^1;
}

bool Board::doPseudoLegalMove(Move m, int color) {
    doMove(m, color);

    if (getInCheck(color))
        return false;
    else return true;
}

// Get all legal moves and captures
MoveList Board::getAllLegalMoves(int color) {
    MoveList moves = getAllPseudoLegalMoves(color);

    for (unsigned int i = 0; i < moves.size(); i++) {
        Board b = staticCopy();
        b.doMove(moves.get(i), color);

        if (b.getInCheck(color)) {
            moves.remove(i);
            i--;
        }
    }

    return moves;
}

//---------------------Pseudo-legal Moves---------------------------
/* Pseudo-legal moves disregard whether the player's king is left in check
 * The pseudo-legal move and capture generators all follow a similar scheme:
 * Bitscan to obtain the square number for each piece (a1 is 0, a2 is 1, h8 is 63).
 * Get the legal moves as a bitboard, then bitscan this to get the destination
 * square and store as a Move object.
 */
MoveList Board::getAllPseudoLegalMoves(int color) {
    MoveList quiets, captures;

    uint64_t otherPieces = (color == WHITE) ? blackPieces : whitePieces;

    // We can do pawns in parallel, since the start square of a pawn move is
    // determined by its end square.
    uint64_t pawns = pieces[color][PAWNS];
    uint64_t finalRank = (color == WHITE) ? RANKS[7] : RANKS[0];
    int sqDiff = (color == WHITE) ? -8 : 8;

    uint64_t pLegal = (color == WHITE) ? getWPawnSingleMoves(pawns)
                                      : getBPawnSingleMoves(pawns);
    uint64_t promotions = pLegal & finalRank;
    pLegal ^= promotions;

    while (promotions) {
        int endsq = bitScanForward(promotions);
        promotions &= promotions - 1;
        int startSq = endsq + sqDiff;
        Move mk = encodeMove(startSq, endsq, PAWNS, false);
        mk = setPromotion(mk, KNIGHTS);
        Move mb = encodeMove(startSq, endsq, PAWNS, false);
        mb = setPromotion(mb, BISHOPS);
        Move mr = encodeMove(startSq, endsq, PAWNS, false);
        mr = setPromotion(mr, ROOKS);
        Move mq = encodeMove(startSq, endsq, PAWNS, false);
        mq = setPromotion(mq, QUEENS);
        quiets.add(mk);
        quiets.add(mb);
        quiets.add(mr);
        quiets.add(mq);
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

    // For pawn captures, we can use a similar approach, but we must consider
    // left-hand and right-hand captures separately so we can tell which
    // pawn is doing the capturing.
    int leftDiff = (color == WHITE) ? -7 : 9;
    int rightDiff = (color == WHITE) ? -9 : 7;

    uint64_t legal = (color == WHITE) ? getWPawnLeftCaptures(pawns)
                                      : getBPawnLeftCaptures(pawns);
    legal &= otherPieces;
    promotions = legal & finalRank;

    while (promotions) {
        int endsq = bitScanForward(promotions);
        promotions &= promotions-1;

        Move mk = encodeMove(endsq+leftDiff, endsq, PAWNS, true);
        mk = setPromotion(mk, KNIGHTS);
        Move mb = encodeMove(endsq+leftDiff, endsq, PAWNS, true);
        mb = setPromotion(mb, BISHOPS);
        Move mr = encodeMove(endsq+leftDiff, endsq, PAWNS, true);
        mr = setPromotion(mr, ROOKS);
        Move mq = encodeMove(endsq+leftDiff, endsq, PAWNS, true);
        mq = setPromotion(mq, QUEENS);
        captures.add(mk);
        captures.add(mb);
        captures.add(mr);
        captures.add(mq);
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

    while (promotions) {
        int endsq = bitScanForward(promotions);
        promotions &= promotions-1;

        Move mk = encodeMove(endsq+rightDiff, endsq, PAWNS, true);
        mk = setPromotion(mk, KNIGHTS);
        Move mb = encodeMove(endsq+rightDiff, endsq, PAWNS, true);
        mb = setPromotion(mb, BISHOPS);
        Move mr = encodeMove(endsq+rightDiff, endsq, PAWNS, true);
        mr = setPromotion(mr, ROOKS);
        Move mq = encodeMove(endsq+rightDiff, endsq, PAWNS, true);
        mq = setPromotion(mq, QUEENS);
        captures.add(mk);
        captures.add(mb);
        captures.add(mr);
        captures.add(mq);
    }
    while (legal) {
        int endsq = bitScanForward(legal);
        legal &= legal-1;
        captures.add(encodeMove(endsq+rightDiff, endsq, PAWNS, true));
    }

    if (color == WHITE) {
        if (whiteEPCaptureSq) {
            uint64_t taker = (whiteEPCaptureSq << 1) & NOTA & pieces[WHITE][PAWNS];
            if (taker) {
                captures.add(encodeMove(bitScanForward(taker),
                        bitScanForward(whiteEPCaptureSq << 8), PAWNS, true));
            }
            else {
                taker = (whiteEPCaptureSq >> 1) & NOTH & pieces[WHITE][PAWNS];
                if (taker) {
                    captures.add(encodeMove(bitScanForward(taker),
                            bitScanForward(whiteEPCaptureSq << 8), PAWNS, true));
                }
            }
        }
    }
    else {
        if (blackEPCaptureSq) {
            uint64_t taker = (blackEPCaptureSq << 1) & NOTA & pieces[BLACK][PAWNS];
            if (taker) {
                captures.add(encodeMove(bitScanForward(taker),
                        bitScanForward(blackEPCaptureSq >> 8), PAWNS, true));
            }
            else {
                taker = (blackEPCaptureSq >> 1) & NOTH & pieces[BLACK][PAWNS];
                if (taker) {
                    captures.add(encodeMove(bitScanForward(taker),
                            bitScanForward(blackEPCaptureSq >> 8), PAWNS, true));
                }
            }
        }
    }

    uint64_t knights = pieces[color][KNIGHTS];
    while (knights) {
        int stsq = bitScanForward(knights);
        knights &= knights-1;
        uint64_t nSq = getKnightSquares(stsq);

        uint64_t legal = nSq & ~(whitePieces | blackPieces);
        while (legal) {
            int endsq = bitScanForward(legal);
            legal &= legal-1;
            quiets.add(encodeMove(stsq, endsq, KNIGHTS, false));
        }

        legal = nSq & otherPieces;
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

        uint64_t legal = bSq & ~(whitePieces | blackPieces);
        while (legal) {
            int endsq = bitScanForward(legal);
            legal &= legal-1;
            quiets.add(encodeMove(stsq, endsq, BISHOPS, false));
        }

        legal = bSq & otherPieces;
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

        uint64_t legal = rSq & ~(whitePieces | blackPieces);
        while (legal) {
            int endsq = bitScanForward(legal);
            legal &= legal-1;
            quiets.add(encodeMove(stsq, endsq, ROOKS, false));
        }

        legal = rSq & otherPieces;
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

        uint64_t legal = qSq & ~(whitePieces | blackPieces);
        while (legal) {
            int endsq = bitScanForward(legal);
            legal &= legal-1;
            quiets.add(encodeMove(stsq, endsq, QUEENS, false));
        }

        legal = qSq & otherPieces;
        while (legal) {
            int endsq = bitScanForward(legal);
            legal &= legal-1;
            captures.add(encodeMove(stsq, endsq, QUEENS, true));
        }
    }

    uint64_t kings = pieces[color][KINGS];
    int stsqK = bitScanForward(kings);
    uint64_t legalK = getKingSquares(stsqK) & ~(whitePieces | blackPieces);
    while (legalK) {
        int endsq = bitScanForward(legalK);
        legalK &= legalK-1;

        quiets.add(encodeMove(stsqK, endsq, KINGS, false));
    }

    legalK = getKingSquares(stsqK) & otherPieces;
    while (legalK) {
        int endsq = bitScanForward(legalK);
        legalK &= legalK-1;

        captures.add(encodeMove(stsqK, endsq, KINGS, true));
    }

    if (color == WHITE) {
        if (whiteCanKCastle
         && ((whitePieces | blackPieces) & (MOVEMASK[5] | MOVEMASK[6])) == 0
         && !getInCheck(WHITE)) {
            // Check for castling through check
            if (getAttackMap(BLACK, 5) == 0) {
                Move m = encodeMove(4, 6, KINGS, false);
                m = setCastle(m, true);
                quiets.add(m);
            }
        }
        else if (whiteCanQCastle
              && ((whitePieces | blackPieces) & (MOVEMASK[1] | MOVEMASK[2] | MOVEMASK[3])) == 0
              && !getInCheck(WHITE)) {
            // Check for castling through check
            if (getAttackMap(BLACK, 3) == 0) {
                Move m = encodeMove(4, 2, KINGS, false);
                m = setCastle(m, true);
                quiets.add(m);
            }
        }
    }
    else {
        if (blackCanKCastle
         && ((whitePieces | blackPieces) & (MOVEMASK[61] | MOVEMASK[62])) == 0
         && !getInCheck(BLACK)) {
            if (getAttackMap(WHITE, 61) == 0) {
                Move m = encodeMove(60, 62, KINGS, false);
                m = setCastle(m, true);
                quiets.add(m);
            }
        }
        if (blackCanQCastle
         && ((whitePieces | blackPieces) & (MOVEMASK[57] | MOVEMASK[58] | MOVEMASK[59])) == 0
         && !getInCheck(BLACK)) {
            if (getAttackMap(WHITE, 59) == 0) {
                Move m = encodeMove(60, 58, KINGS, false);
                m = setCastle(m, true);
                quiets.add(m);
            }
        }
    }

    for (unsigned int i = 0; i < quiets.size(); i++) {
        captures.add(quiets.get(i));
    }
    return captures;
}

MoveList Board::getLegalCaptures(int color) {
    MoveList moves = getPseudoLegalCaptures(color);

    for (unsigned int i = 0; i < moves.size(); i++) {
        Board b = staticCopy();
        b.doMove(moves.get(i), color);

        if (b.getInCheck(color)) {
            moves.remove(i);
            i--;
        }
    }

    return moves;
}

MoveList Board::getPseudoLegalCaptures(int color) {
    MoveList result;
    uint64_t pawns = pieces[color][PAWNS];
    uint64_t knights = pieces[color][KNIGHTS];
    uint64_t bishops = pieces[color][BISHOPS];
    uint64_t rooks = pieces[color][ROOKS];
    uint64_t queens = pieces[color][QUEENS];
    uint64_t kings = pieces[color][KINGS];
    uint64_t otherPieces = (color == WHITE) ? blackPieces : whitePieces;

    uint64_t finalRank = (color == WHITE) ? RANKS[7] : RANKS[0];
    int leftDiff = (color == WHITE) ? -7 : 9;
    int rightDiff = (color == WHITE) ? -9 : 7;

    uint64_t legal = (color == WHITE) ? getWPawnLeftCaptures(pawns)
                                      : getBPawnLeftCaptures(pawns);
    legal &= otherPieces;
    uint64_t promotions = legal & finalRank;

    while (promotions) {
        int endsq = bitScanForward(promotions);
        promotions &= promotions-1;

        Move mk = encodeMove(endsq+leftDiff, endsq, PAWNS, true);
        mk = setPromotion(mk, KNIGHTS);
        Move mb = encodeMove(endsq+leftDiff, endsq, PAWNS, true);
        mb = setPromotion(mb, BISHOPS);
        Move mr = encodeMove(endsq+leftDiff, endsq, PAWNS, true);
        mr = setPromotion(mr, ROOKS);
        Move mq = encodeMove(endsq+leftDiff, endsq, PAWNS, true);
        mq = setPromotion(mq, QUEENS);
        result.add(mk);
        result.add(mb);
        result.add(mr);
        result.add(mq);
    }
    while (legal) {
        int endsq = bitScanForward(legal);
        legal &= legal-1;
        result.add(encodeMove(endsq+leftDiff, endsq, PAWNS, true));
    }

    legal = (color == WHITE) ? getWPawnRightCaptures(pawns)
                                      : getBPawnRightCaptures(pawns);
    legal &= otherPieces;
    promotions = legal & finalRank;

    while (promotions) {
        int endsq = bitScanForward(promotions);
        promotions &= promotions-1;

        Move mk = encodeMove(endsq+rightDiff, endsq, PAWNS, true);
        mk = setPromotion(mk, KNIGHTS);
        Move mb = encodeMove(endsq+rightDiff, endsq, PAWNS, true);
        mb = setPromotion(mb, BISHOPS);
        Move mr = encodeMove(endsq+rightDiff, endsq, PAWNS, true);
        mr = setPromotion(mr, ROOKS);
        Move mq = encodeMove(endsq+rightDiff, endsq, PAWNS, true);
        mq = setPromotion(mq, QUEENS);
        result.add(mk);
        result.add(mb);
        result.add(mr);
        result.add(mq);
    }
    while (legal) {
        int endsq = bitScanForward(legal);
        legal &= legal-1;
        result.add(encodeMove(endsq+rightDiff, endsq, PAWNS, true));
    }

    if (color == WHITE) {
        if (whiteEPCaptureSq) {
            uint64_t taker = (whiteEPCaptureSq << 1) & NOTA & pieces[WHITE][PAWNS];
            if (taker) {
                result.add(encodeMove(bitScanForward(taker),
                        bitScanForward(whiteEPCaptureSq << 8), PAWNS, true));
            }
            else {
                taker = (whiteEPCaptureSq >> 1) & NOTH & pieces[WHITE][PAWNS];
                if (taker) {
                    result.add(encodeMove(bitScanForward(taker),
                            bitScanForward(whiteEPCaptureSq << 8), PAWNS, true));
                }
            }
        }
    }
    else {
        if (blackEPCaptureSq) {
            uint64_t taker = (blackEPCaptureSq << 1) & NOTA & pieces[BLACK][PAWNS];
            if (taker) {
                result.add(encodeMove(bitScanForward(taker),
                        bitScanForward(blackEPCaptureSq >> 8), PAWNS, true));
            }
            else {
                taker = (blackEPCaptureSq >> 1) & NOTH & pieces[BLACK][PAWNS];
                if (taker) {
                    result.add(encodeMove(bitScanForward(taker),
                            bitScanForward(blackEPCaptureSq >> 8), PAWNS, true));
                }
            }
        }
    }

    while (knights) {
        int stsq = bitScanForward(knights);
        knights &= knights-1;

        uint64_t legal = getKnightSquares(stsq) & otherPieces;
        while (legal) {
            int endsq = bitScanForward(legal);
            legal &= legal-1;

            result.add(encodeMove(stsq, endsq, KNIGHTS, true));
        }
    }

    while (bishops) {
        int stsq = bitScanForward(bishops);
        bishops &= bishops-1;

        uint64_t legal = getBishopSquares(stsq) & otherPieces;
        while (legal) {
            int endsq = bitScanForward(legal);
            legal &= legal-1;

            result.add(encodeMove(stsq, endsq, BISHOPS, true));
        }
    }

    while (rooks) {
        int stsq = bitScanForward(rooks);
        rooks &= rooks-1;

        uint64_t legal = getRookSquares(stsq) & otherPieces;
        while (legal) {
            int endsq = bitScanForward(legal);
            legal &= legal-1;

            result.add(encodeMove(stsq, endsq, ROOKS, true));
        }
    }

    while (queens) {
        int stsq = bitScanForward(queens);
        queens &= queens-1;

        uint64_t legal = getQueenSquares(stsq) & otherPieces;
        while (legal) {
            int endsq = bitScanForward(legal);
            legal &= legal-1;

            result.add(encodeMove(stsq, endsq, QUEENS, true));
        }
    }

    int stsq = bitScanForward(kings);
    uint64_t legalK = getKingSquares(stsq) & otherPieces;
    while (legalK) {
        int endsq = bitScanForward(legalK);
        legalK &= legalK-1;

        result.add(encodeMove(stsq, endsq, KINGS, true));
    }

    return result;
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
    if (pieces[colorCaptured][PAWNS] & endSingle)
        return PAWNS;
    else if (pieces[colorCaptured][KNIGHTS] & endSingle)
        return KNIGHTS;
    else if (pieces[colorCaptured][BISHOPS] & endSingle)
        return BISHOPS;
    else if (pieces[colorCaptured][ROOKS] & endSingle)
        return ROOKS;
    else if (pieces[colorCaptured][QUEENS] & endSingle)
        return QUEENS;
    else {
        // The default is when the capture destination is an empty square.
        // This indicates an en passant (and hopefully not an error).
        return -1;
    }
}


// ---------------------King: check, checkmate, stalemate----------------------
bool Board::getInCheck(int color) {
    int sq = bitScanForward(pieces[color][KINGS]);

    return getAttackMap(color^1, sq);
}

bool Board::isWinMate() {
    MoveList moves = getAllLegalMoves(WHITE);
    bool isInMate = false;
    if (moves.size() == 0 && getInCheck(WHITE))
        isInMate = true;
    
    return isInMate;
}

bool Board::isBinMate() {
    MoveList moves = getAllLegalMoves(BLACK);
    bool isInMate = false;
    if (moves.size() == 0 && getInCheck(BLACK))
        isInMate = true;

    return isInMate;
}

// TODO Includes 3-fold repetition draw for now.
bool Board::isStalemate(int sideToMove) {
    if (fiftyMoveCounter >= 100) return true;

    if(!(twoFoldStartSqs & (1 << 31))) {
        bool isTwoFold = true;

        if ( (((twoFoldStartSqs >> 24) & 0xFF) != ((twoFoldEndSqs >> 8) & 0xFF))
          || (((twoFoldStartSqs >> 8) & 0xFF) != ((twoFoldEndSqs >> 24) & 0xFF))
          || (((twoFoldStartSqs >> 16) & 0xFF) != (twoFoldEndSqs & 0xFF))
          || ((twoFoldStartSqs & 0xFF) != ((twoFoldEndSqs >> 16) & 0xFF))) {
            isTwoFold = false;
        }

        if (isTwoFold) return true;
    }

    MoveList moves = getAllLegalMoves(sideToMove);
    bool isInStalemate = false;

    if (moves.size() == 0 && !getInCheck(sideToMove))
        isInStalemate = true;

    return isInStalemate;
}

// TODO Tune evaluation function
/*
 * Evaluates the current board position in hundredths of pawns. White is
 * positive and black is negative in traditional negamax format.
 */
int Board::evaluate() {
    // special cases
    if (fiftyMoveCounter >= 100)
        return 0;
    else if (isWinMate())
        return MATE_SCORE - 100 - moveNumber;
    else if (isBinMate())
        return -MATE_SCORE + 100 + moveNumber;
    else if (isStalemate(playerToMove))
        return 0;

    int value = 0;

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
    
    // TODO make this faster
    // piece square tables
    int *mailbox = getMailbox();
    for (int i = 0; i < 64; i++) {
        switch (mailbox[i]) {
            case -1: // empty
                break;
            case PAWNS: // white pawn
                value += pawnValues[(7 - i/8)*8 + i%8] * (EG_FACTOR_RES - egFactor) / EG_FACTOR_RES;
                value += egPawnValues[(7 - i/8*8 + i%8)] * egFactor / EG_FACTOR_RES;
                break;
            case 6+PAWNS: // black pawn
                value -= pawnValues[i] * (EG_FACTOR_RES - egFactor) / EG_FACTOR_RES;
                value -= egPawnValues[i] * egFactor / EG_FACTOR_RES;
                break;
            case KNIGHTS: // white knight
                value += knightValues[(7 - i/8)*8 + i%8];
                break;
            case 6+KNIGHTS: // black knight
                value -= knightValues[i];
                break;
            case BISHOPS: // white bishop
                value += bishopValues[(7 - i/8)*8 + i%8];
                break;
            case 6+BISHOPS: // black bishop
                value -= bishopValues[i];
                break;
            case ROOKS: // white rook
                value += rookValues[(7 - i/8)*8 + i%8];
                break;
            case 6+ROOKS: // black rook
                value -= rookValues[i];
                break;
            case QUEENS: // white queen
                value += queenValues[(7 - i/8)*8 + i%8];
                break;
            case 6+QUEENS: // black queen
                value -= queenValues[i];
                break;
            case KINGS: // white king
                value += kingValues[(7 - i/8)*8 + i%8] * (EG_FACTOR_RES - egFactor) / EG_FACTOR_RES;
                value += egKingValues[(7 - i/8)*8 + i%8] * egFactor / EG_FACTOR_RES;
                break;
            case 6+KINGS: // black king
                value -= kingValues[i] * (EG_FACTOR_RES - egFactor) / EG_FACTOR_RES;
                value -= egKingValues[i] * egFactor / EG_FACTOR_RES;
                break;
            default:
                cout << "Error in piece table switch statement." << endl;
                break;
        }
    }
    delete[] mailbox;
    
    // Consider attacks on squares near king
    uint64_t wksq = getKingAttacks(WHITE);
    uint64_t bksq = getKingAttacks(BLACK);
    uint64_t bAtt = getBPawnLeftCaptures(pieces[BLACK][PAWNS])
                  | getBPawnRightCaptures(pieces[BLACK][PAWNS])
                  | getKnightMoves(pieces[BLACK][KNIGHTS])
                  | getBishopMoves(pieces[BLACK][BISHOPS])
                  | getRookMoves(pieces[BLACK][ROOKS])
                  | getQueenMoves(pieces[BLACK][QUEENS]);
    uint64_t wAtt = getWPawnLeftCaptures(pieces[WHITE][PAWNS])
                  | getWPawnRightCaptures(pieces[WHITE][PAWNS])
                  | getKnightMoves(pieces[WHITE][KNIGHTS])
                  | getBishopMoves(pieces[WHITE][BISHOPS])
                  | getRookMoves(pieces[WHITE][ROOKS])
                  | getQueenMoves(pieces[WHITE][QUEENS]);
    
    value -= 25 * count(wksq & bAtt) * (EG_FACTOR_RES - egFactor) / EG_FACTOR_RES;
    value += 25 * count(bksq & wAtt) * (EG_FACTOR_RES - egFactor) / EG_FACTOR_RES;
    
    uint64_t wpawnShield = (wksq | pieces[WHITE][KINGS]) << 8;
    uint64_t bpawnShield = (bksq | pieces[BLACK][KINGS]) >> 8;
    // Have only pawns on ABC, FGH files count towards the pawn shield
    value += (30 * egFactor / EG_FACTOR_RES) * count(wpawnShield & pieces[WHITE][PAWNS] & 0xe7e7e7e7e7e7e7e7);
    value -= (30 * egFactor / EG_FACTOR_RES) * count(bpawnShield & pieces[BLACK][PAWNS] & 0xe7e7e7e7e7e7e7e7);
    
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
    // Flood fill to simulate pushing the pawn to the 8th (or 1st) rank
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

// Faster estimates of piece mobility (number of legal moves)
int Board::getPseudoMobility(int color) {
    int result = 0;
    uint64_t knights = pieces[color][KNIGHTS];
    uint64_t bishops = pieces[color][BISHOPS];
    uint64_t rooks = pieces[color][ROOKS];
    uint64_t queens = pieces[color][QUEENS];
    uint64_t pieces = (color == WHITE) ? whitePieces : blackPieces;

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

// TODO come up with a better way to do this
uint64_t Board::getLeastValuableAttacker(uint64_t attackers, int color, int &piece) {
    uint64_t single = attackers & pieces[color][PAWNS];
    if(single) {
        piece = PAWNS;
        return single & -single;
    }
    single = attackers & pieces[color][KNIGHTS];
    if(single) {
        piece = KNIGHTS;
        return single & -single;
    }
    single = attackers & pieces[color][BISHOPS];
    if(single) {
        piece = BISHOPS;
        return single & -single;
    }
    single = attackers & pieces[color][ROOKS];
    if(single) {
        piece = ROOKS;
        return single & -single;
    }
    single = attackers & pieces[color][QUEENS];
    if(single) {
        piece = QUEENS;
        return single & -single;
    }

    piece = KINGS;
    return attackers & pieces[color][KINGS];
}

// TODO consider xrays
// Static exchange evaluation algorithm from
// https://chessprogramming.wikispaces.com/SEE+-+The+Swap+Algorithm
int Board::getSEE(int color, int sq) {
    int gain[32], d = 0, piece = 0;
    uint64_t attackers = getAttackMap(WHITE, sq) | getAttackMap(BLACK, sq);
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


//----------------------------MOVE GENERATION-------------------------------
uint64_t Board::getWPawnSingleMoves(uint64_t pawns) {
    uint64_t open = ~(whitePieces | blackPieces);
    return (pawns << 8) & open;
}

uint64_t Board::getBPawnSingleMoves(uint64_t pawns) {
    uint64_t open = ~(whitePieces | blackPieces);
    return (pawns >> 8) & open;
}

uint64_t Board::getWPawnDoubleMoves(uint64_t pawns) {
    uint64_t open = ~(whitePieces | blackPieces);
    uint64_t temp = (pawns << 8) & open;
    pawns = (temp << 8) & open & RANKS[3];
    return pawns;
}

uint64_t Board::getBPawnDoubleMoves(uint64_t pawns) {
    uint64_t open = ~(whitePieces | blackPieces);
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

// Parallel-prefix knight move generation
// l1, l2, r1, r2 are the 4 possible directions a knight can move for the first
// half of its "L" shaped movement
// Then, l1 and r1 must be moved up or down 2 squares (shift 16)
// Similarly, l2 and r2 are moved up or down 1 square to complete the "L".
uint64_t Board::getKnightMoves(uint64_t knights) {
    uint64_t kn = knights;
    uint64_t l1 = (kn >> 1) & 0x7f7f7f7f7f7f7f7f;
    uint64_t l2 = (kn >> 2) & 0x3f3f3f3f3f3f3f3f;
    uint64_t r1 = (kn << 1) & 0xfefefefefefefefe;
    uint64_t r2 = (kn << 2) & 0xfcfcfcfcfcfcfcfc;
    uint64_t h1 = l1 | r1;
    uint64_t h2 = l2 | r2;
    return (h1<<16) | (h1>>16) | (h2<<8) | (h2>>8);
}

uint64_t Board::getBishopSquares(int single) {
    uint64_t occ = whitePieces | blackPieces;

    uint64_t diagAtt = diagAttacks(occ, single);
    uint64_t antiDiagAtt = antiDiagAttacks(occ, single);

    return diagAtt | antiDiagAtt;
}

uint64_t Board::getBishopMoves(uint64_t bishops) {
    uint64_t open = ~(whitePieces | blackPieces);
    uint64_t result = neAttacks(bishops, open);
    result |= seAttacks(bishops, open);
    result |= nwAttacks(bishops, open);
    result |= swAttacks(bishops, open);
    return result;
}

uint64_t Board::getRookSquares(int single) {
    uint64_t occ = whitePieces | blackPieces;

    uint64_t rankAtt = rankAttacks(occ, single);
    uint64_t fileAtt = fileAttacks(occ, single);

    return rankAtt | fileAtt;
}

uint64_t Board::getRookMoves(uint64_t rooks) {
    uint64_t open = ~(whitePieces | blackPieces);
    uint64_t result = southAttacks(rooks, open);
    result |= northAttacks(rooks, open);
    result |= eastAttacks(rooks, open);
    result |= westAttacks(rooks, open);
    return result;
}

uint64_t Board::getQueenSquares(int single) {
    uint64_t occ = whitePieces | blackPieces;

    uint64_t rankAtt = rankAttacks(occ, single);
    uint64_t fileAtt = fileAttacks(occ, single);
    uint64_t diagAtt = diagAttacks(occ, single);
    uint64_t antiDiagAtt = antiDiagAttacks(occ, single);

    return rankAtt | fileAtt | diagAtt | antiDiagAtt;
}

uint64_t Board::getQueenMoves(uint64_t queens) {
    uint64_t open = ~(whitePieces | blackPieces);
    uint64_t result = southAttacks(queens, open);
    result |= northAttacks(queens, open);
    result |= eastAttacks(queens, open);
    result |= westAttacks(queens, open);
    result |= neAttacks(queens, open);
    result |= seAttacks(queens, open);
    result |= nwAttacks(queens, open);
    result |= swAttacks(queens, open);
    return result;
}

uint64_t Board::getKingSquares(int single) {
    return KINGMOVES[single];
}

uint64_t Board::getKingAttacks(int color) {
    uint64_t kings = pieces[color][KINGS];
    uint64_t attacks = ((kings << 1) & NOTA) | ((kings >> 1) & NOTH);
    kings |= attacks;
    attacks |= (kings >> 8) | (kings << 8);
    return attacks;
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

// Dumb7Fill
uint64_t Board::southAttacks(uint64_t rooks, uint64_t empty) {
    uint64_t flood = rooks;
    flood |= rooks = (rooks >> 8) & empty;
    flood |= rooks = (rooks >> 8) & empty;
    flood |= rooks = (rooks >> 8) & empty;
    flood |= rooks = (rooks >> 8) & empty;
    flood |= rooks = (rooks >> 8) & empty;
    flood |=         (rooks >> 8) & empty;
    return           (flood >> 8);
}

uint64_t Board::northAttacks(uint64_t rooks, uint64_t empty) {
    uint64_t flood = rooks;
    flood |= rooks = (rooks << 8) & empty;
    flood |= rooks = (rooks << 8) & empty;
    flood |= rooks = (rooks << 8) & empty;
    flood |= rooks = (rooks << 8) & empty;
    flood |= rooks = (rooks << 8) & empty;
    flood |=         (rooks << 8) & empty;
    return           (flood << 8);
}

uint64_t Board::eastAttacks(uint64_t rooks, uint64_t empty) {
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

uint64_t Board::neAttacks(uint64_t bishops, uint64_t empty) {
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

uint64_t Board::seAttacks(uint64_t bishops, uint64_t empty) {
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

uint64_t Board::westAttacks(uint64_t rooks, uint64_t empty) {
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

uint64_t Board::swAttacks(uint64_t bishops, uint64_t empty) {
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

uint64_t Board::nwAttacks(uint64_t bishops, uint64_t empty) {
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

// Getter methods
bool Board::getWhiteCanKCastle() {
    return whiteCanKCastle;
}

bool Board::getBlackCanKCastle() {
    return blackCanKCastle;
}

bool Board::getWhiteCanQCastle() {
    return whiteCanQCastle;
}

bool Board::getBlackCanQCastle() {
    return blackCanQCastle;
}

uint64_t Board::getWhiteEPCaptureSq() {
    return whiteEPCaptureSq;
}

uint64_t Board::getBlackEPCaptureSq() {
    return blackEPCaptureSq;
}

int Board::getFiftyMoveCounter() {
    return fiftyMoveCounter;
}

int Board::getMoveNumber() {
    return moveNumber;
}

int Board::getPlayerToMove() {
    return playerToMove;
}

uint64_t Board::getWhitePieces() {
    return whitePieces;
}

uint64_t Board::getBlackPieces() {
    return blackPieces;
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

string Board::toString() {
    int *mailbox = getMailbox();
    string result = "";
    for (int i = 56; i >= 0; i++) {
        switch (mailbox[i]) {
            case -1: // empty
                result += "-";
                break;
            case 2: // white pawn
                result += "P";
                break;
            case 0: // black pawn
                result += "p";
                break;
            case 3: // white knight
                result += "N";
                break;
            case 1: // black knight
                result += "n";
                break;
            case 6: // white bishop
                result += "B";
                break;
            case 4: // black bishop
                result += "b";
                break;
            case 7: // white rook
                result += "R";
                break;
            case 5: // black rook
                result += "r";
                break;
            case 10: // white queen
                result += "Q";
                break;
            case 8: // black queen
                result += "q";
                break;
            case 11: // white king
                result += "K";
                break;
            case 9: // black king
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
