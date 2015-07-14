#include "board.h"

Board::Board() {
    pieces[2] = 0x000000000000FF00; // white pawns
    pieces[0] = 0x00FF000000000000; // black pawns
    pieces[3] = 0x0000000000000042; // white knights
    pieces[1] = 0x4200000000000000; // black knights
    pieces[6] = 0x0000000000000024; // white bishops
    pieces[4] = 0x2400000000000000; // black bishops
    pieces[7] = 0x0000000000000081; // white rooks
    pieces[5] = 0x8100000000000000; // black rooks
    pieces[10] = 0x0000000000000008; // white queens
    pieces[8] = 0x0800000000000000; // black queens
    pieces[11] = 0x0000000000000010; // white kings
    pieces[9] = 0x1000000000000000; // black kings
    whitePieces = 0x000000000000FFFF;
    blackPieces = 0xFFFF000000000000;

    whiteCanKCastle = true;
    blackCanKCastle = true;
    whiteCanQCastle = true;
    blackCanQCastle = true;
    whiteEPCaptureSq = 0;
    blackEPCaptureSq = 0;

    for (int i = 0; i < 64; i++) {
        mailbox[i] = initMailbox[i];
    }

    fiftyMoveCounter = 0;
    moveNumber = 1;
    playerToMove = WHITE;
}

Board::Board(int *mailboxBoard, bool _whiteCanKCastle, bool _blackCanKCastle,
        bool _whiteCanQCastle, bool _blackCanQCastle, uint64_t _whiteEPCaptureSq,
        uint64_t _blackEPCaptureSq, int _fiftyMoveCounter, int _moveNumber,
        int _playerToMove) {
    // Copy mailbox
    for (int i = 0; i < 64; i++) {
        mailbox[i] = mailboxBoard[i];
    }

    // Initialize bitboards
    for (int i = 0; i < 12; i++) {
        pieces[i] = 0;
    }
    for (int i = 0; i < 64; i++) {
        if (0 <= mailbox[i] && mailbox[i] <= 11) {
            pieces[mailbox[i]] |= MOVEMASK[i];
        }
        
        if (mailbox[i] > 11) cerr << "Error in constructor." << endl;
    }
    whitePieces = pieces[2] | pieces[3] | pieces[6] | pieces[7] | pieces[10]
                | pieces[11];
    blackPieces = pieces[0] | pieces[1] | pieces[4] | pieces[5] | pieces[8]
                | pieces[9];
    
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
    for (int i = 0; i < 12; i++) {
        result.pieces[i] = pieces[i];
    }
    result.whitePieces = whitePieces;
    result.blackPieces = blackPieces;
    result.whiteCanKCastle = whiteCanKCastle;
    result.whiteCanQCastle = whiteCanQCastle;
    result.blackCanKCastle = blackCanKCastle;
    result.blackCanQCastle = blackCanQCastle;
    result.whiteEPCaptureSq = whiteEPCaptureSq;
    result.blackEPCaptureSq = blackEPCaptureSq;
    for (int i = 0; i < 64; i++) {
        result.mailbox[i] = mailbox[i];
    }
    result.fiftyMoveCounter = fiftyMoveCounter;
    result.moveNumber = moveNumber;
    result.playerToMove = playerToMove;
    result.twoFoldStartSqs = twoFoldStartSqs;
    result.twoFoldEndSqs = twoFoldEndSqs;
    result.twoFoldPTM = twoFoldPTM;
    return result;
}

Board *Board::dynamicCopy() {
    Board *result = new Board();
    for (int i = 0; i < 12; i++) {
        result->pieces[i] = pieces[i];
    }
    result->whitePieces = whitePieces;
    result->blackPieces = blackPieces;
    result->whiteCanKCastle = whiteCanKCastle;
    result->whiteCanQCastle = whiteCanQCastle;
    result->blackCanKCastle = blackCanKCastle;
    result->blackCanQCastle = blackCanQCastle;
    result->whiteEPCaptureSq = whiteEPCaptureSq;
    result->blackEPCaptureSq = blackEPCaptureSq;
    for (int i = 0; i < 64; i++) {
        result->mailbox[i] = mailbox[i];
    }
    result->fiftyMoveCounter = fiftyMoveCounter;
    result->moveNumber = moveNumber;
    result->playerToMove = playerToMove;
    result->twoFoldStartSqs = twoFoldStartSqs;
    result->twoFoldEndSqs = twoFoldEndSqs;
    result->twoFoldPTM = twoFoldPTM;
    return result;
}

void Board::doMove(Move m, int color) {
/* TODO undo move stuff
    BMove record = new BMove(color, m->startsq, m->endsq, mailbox[m->endsq], whiteEPCaptureSq, blackEPCaptureSq, m->isCastle);
    if (m->promotion != -1)
        record->isPromotion = true;
    record->whiteCanKCastle = whiteCanKCastle;
    record->whiteCanQCastle = whiteCanQCastle;
    record->blackCanKCastle = blackCanKCastle;
    record->blackCanQCastle = blackCanQCastle;
    history.push(record);
*/

    int pieceID = getPiece(m);
    int startSq = getStartSq(m);
    int endSq = getEndSq(m);

    // Handle null moves for null move pruning
    if (m == NULL_MOVE) {
        playerToMove = -color;
        return;
    }

    // Record current board position for two-fold repetition
    if (isCapture(m) || pieceID == PAWNS || isCastle(m)) {
        twoFoldStartSqs = 0x80008000;
        twoFoldEndSqs = 0x80008000;
        twoFoldPTM = 0;
    }
    else {
        twoFoldStartSqs <<= 8;
        twoFoldEndSqs <<= 8;
        twoFoldPTM <<= 8;
        twoFoldStartSqs |= (uint8_t) startSq;
        twoFoldEndSqs |= (uint8_t) endSq;
        twoFoldPTM |= (uint8_t) playerToMove;
    }

    if (isCastle(m)) {
        if (endSq == 6) { // white kside
            pieces[11] &= ~MOVEMASK[4];
            pieces[11] |= MOVEMASK[6];
            pieces[7] &= ~MOVEMASK[7];
            pieces[7] |= MOVEMASK[5];

            whitePieces &= ~MOVEMASK[4];
            whitePieces |= MOVEMASK[6];
            whitePieces &= ~MOVEMASK[7];
            whitePieces |= MOVEMASK[5];

            mailbox[4] = -1;
            mailbox[6] = WHITE+KINGS;
            mailbox[7] = -1;
            mailbox[5] = WHITE+ROOKS;

            whiteCanKCastle = false;
            whiteCanQCastle = false;
        }
        else if (endSq == 2) { // white qside
            pieces[11] &= ~MOVEMASK[4];
            pieces[11] |= MOVEMASK[2];
            pieces[7] &= ~MOVEMASK[0];
            pieces[7] |= MOVEMASK[3];

            whitePieces &= ~MOVEMASK[4];
            whitePieces |= MOVEMASK[2];
            whitePieces &= ~MOVEMASK[0];
            whitePieces |= MOVEMASK[3];

            mailbox[4] = -1;
            mailbox[2] = WHITE+KINGS;
            mailbox[0] = -1;
            mailbox[3] = WHITE+ROOKS;

            whiteCanKCastle = false;
            whiteCanQCastle = false;
        }
        else if (endSq == 62) { // black kside
            pieces[9] &= ~MOVEMASK[60];
            pieces[9] |= MOVEMASK[62];
            pieces[5] &= ~MOVEMASK[63];
            pieces[5] |= MOVEMASK[61];

            blackPieces &= ~MOVEMASK[60];
            blackPieces |= MOVEMASK[62];
            blackPieces &= ~MOVEMASK[63];
            blackPieces |= MOVEMASK[61];

            mailbox[60] = -1;
            mailbox[62] = BLACK+KINGS;
            mailbox[63] = -1;
            mailbox[61] = BLACK+ROOKS;

            blackCanKCastle = false;
            blackCanQCastle = false;
        }
        else { // black qside
            pieces[9] &= ~MOVEMASK[60];
            pieces[9] |= MOVEMASK[58];
            pieces[5] &= ~MOVEMASK[56];
            pieces[5] |= MOVEMASK[59];

            blackPieces &= ~MOVEMASK[60];
            blackPieces |= MOVEMASK[58];
            blackPieces &= ~MOVEMASK[56];
            blackPieces |= MOVEMASK[59];

            mailbox[60] = -1;
            mailbox[58] = BLACK+KINGS;
            mailbox[56] = -1;
            mailbox[59] = BLACK+ROOKS;

            blackCanKCastle = false;
            blackCanQCastle = false;
        }
        whiteEPCaptureSq = 0;
        blackEPCaptureSq = 0;
        fiftyMoveCounter++;
    } // end castling
    else if (getPromotion(m)) {
        if (isCapture(m)) {
            pieces[PAWNS+color] &= ~MOVEMASK[startSq];
            pieces[getPromotion(m)+color] |= MOVEMASK[endSq];
            pieces[mailbox[endSq]] &= ~MOVEMASK[endSq];

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
            pieces[PAWNS+color] &= ~MOVEMASK[startSq];
            pieces[getPromotion(m)+color] |= MOVEMASK[endSq];

            if (color == WHITE) {
                whitePieces &= ~MOVEMASK[startSq];
                whitePieces |= MOVEMASK[endSq];
            }
            else {
                blackPieces &= ~MOVEMASK[startSq];
                blackPieces |= MOVEMASK[endSq];
            }
        }

        mailbox[startSq] = -1;
        mailbox[endSq] = getPromotion(m) + color;
        whiteEPCaptureSq = 0;
        blackEPCaptureSq = 0;
        fiftyMoveCounter = 0;
    } // end promotion
    else if (isCapture(m)) {
        if (mailbox[endSq] == -1) {
            pieces[PAWNS+color] &= ~MOVEMASK[startSq];
            pieces[PAWNS+color] |= MOVEMASK[endSq];
            pieces[PAWNS-color] &= ~((color == WHITE) ? whiteEPCaptureSq : blackEPCaptureSq);

            if (color == WHITE) {
                whitePieces &= ~MOVEMASK[startSq];
                whitePieces |= MOVEMASK[endSq];
                blackPieces &= ~whiteEPCaptureSq;
                mailbox[bitScanForward(whiteEPCaptureSq)] = -1;
            }
            else {
                blackPieces &= ~MOVEMASK[startSq];
                blackPieces |= MOVEMASK[endSq];
                whitePieces &= ~blackEPCaptureSq;
                mailbox[bitScanForward(blackEPCaptureSq)] = -1;
            }
        
            mailbox[startSq] = -1;
            mailbox[endSq] = PAWNS + color;
        }
        else {
            pieces[pieceID+color] &= ~MOVEMASK[startSq];
            pieces[pieceID+color] |= MOVEMASK[endSq];
            pieces[mailbox[endSq]] &= ~MOVEMASK[endSq];

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

            mailbox[startSq] = -1;
            mailbox[endSq] = pieceID + color;
        }
        whiteEPCaptureSq = 0;
        blackEPCaptureSq = 0;
        fiftyMoveCounter = 0;
    } // end capture
    else {
        pieces[pieceID+color] &= ~MOVEMASK[startSq];
        pieces[pieceID+color] |= MOVEMASK[endSq];

        if (color == WHITE) {
            whitePieces &= ~MOVEMASK[startSq];
            whitePieces |= MOVEMASK[endSq];
        }
        else {
            blackPieces &= ~MOVEMASK[startSq];
            blackPieces |= MOVEMASK[endSq];
        }

        mailbox[startSq] = -1;
        mailbox[endSq] = pieceID + color;

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
            int whiteR = (int)(RANKS[0] & pieces[WHITE+ROOKS]);
            if ((whiteR & 0x80) == 0)
                whiteCanKCastle = false;
            if ((whiteR & 1) == 0)
                whiteCanQCastle = false;
        }
        if (blackCanKCastle || blackCanQCastle) {
            int blackR = (int)((RANKS[7] & pieces[BLACK+ROOKS]) >> 56);
            if ((blackR & 0x80) == 0)
                blackCanKCastle = false;
            if ((blackR & 1) == 0)
                blackCanQCastle = false;
        }
    } // end castling flags

    if (color == BLACK)
        moveNumber++;
    playerToMove = -color;
}

bool Board::doPseudoLegalMove(Move m, int color) {
    doMove(m, color);

    if (getInCheck(color))
        return false;
    else return true;
}

/* TODO:
 * If using this function, fifty move rule, move number, and player to move need
 * to be included.
void Board::undoMove() {
    // BMove last = history.pop();

    if (last.isCastle) {
        if (last.color == WHITE && last.endsq == 6) { // w kside
            pieces[11] &= ~MOVEMASK[6];
            pieces[11] |= MOVEMASK[4];
            pieces[7] &= ~MOVEMASK[5];
            pieces[7] |= MOVEMASK[7];

            whitePieces &= ~MOVEMASK[6];
            whitePieces |= MOVEMASK[4];
            whitePieces &= ~MOVEMASK[5];
            whitePieces |= MOVEMASK[7];

            mailbox[6] = -1;
            mailbox[4] = WHITE+KINGS;
            mailbox[5] = -1;
            mailbox[7] = WHITE+ROOKS;

            whiteCanKCastle = last.whiteCanKCastle;
            whiteCanQCastle = last.whiteCanQCastle;
        }
        else if (last.color == WHITE && last.endsq == 2) { // w qside
            pieces[11] &= ~MOVEMASK[2];
            pieces[11] |= MOVEMASK[4];
            pieces[7] &= ~MOVEMASK[3];
            pieces[7] |= MOVEMASK[0];

            whitePieces &= ~MOVEMASK[2];
            whitePieces |= MOVEMASK[4];
            whitePieces &= ~MOVEMASK[3];
            whitePieces |= MOVEMASK[0];

            mailbox[2] = -1;
            mailbox[4] = WHITE+KINGS;
            mailbox[3] = -1;
            mailbox[0] = WHITE+ROOKS;

            whiteCanKCastle = last.whiteCanKCastle;
            whiteCanQCastle = last.whiteCanQCastle;
        }
        else if (last.color == BLACK && last.endsq == 62) { // b kside
            pieces[9] &= ~MOVEMASK[62];
            pieces[9] |= MOVEMASK[60];
            pieces[5] &= ~MOVEMASK[61];
            pieces[5] |= MOVEMASK[63];

            blackPieces &= ~MOVEMASK[62];
            blackPieces |= MOVEMASK[60];
            blackPieces &= ~MOVEMASK[61];
            blackPieces |= MOVEMASK[63];

            mailbox[62] = -1;
            mailbox[60] = BLACK+KINGS;
            mailbox[61] = -1;
            mailbox[63] = BLACK+ROOKS;

            blackCanKCastle = last.blackCanKCastle;
            blackCanQCastle = last.blackCanQCastle;
        }
        else { // b qside
            pieces[9] &= ~MOVEMASK[58];
            pieces[9] |= MOVEMASK[60];
            pieces[5] &= ~MOVEMASK[59];
            pieces[5] |= MOVEMASK[56];

            blackPieces &= ~MOVEMASK[58];
            blackPieces |= MOVEMASK[60];
            blackPieces &= ~MOVEMASK[59];
            blackPieces |= MOVEMASK[56];

            mailbox[58] = -1;
            mailbox[60] = BLACK+KINGS;
            mailbox[59] = -1;
            mailbox[56] = BLACK+ROOKS;

            blackCanKCastle = last.blackCanKCastle;
            blackCanQCastle = last.blackCanQCastle;
        }
        whiteEPCaptureSq = last.wEP;
        blackEPCaptureSq = last.bEP;
        return;
    } // end castling
    else if (last.isPromotion) {
        if (last.capture != -1) {
            pieces[mailbox[last.endsq]] &= ~MOVEMASK[last.endsq];
            pieces[last.capture] |= MOVEMASK[last.endsq];
            pieces[PAWNS+last.color] |= MOVEMASK[last.stsq];

            if (last.color == WHITE) {
                whitePieces &= ~MOVEMASK[last.endsq];
                whitePieces |= MOVEMASK[last.stsq];
                blackPieces |= MOVEMASK[last.endsq];
            }
            else {
                blackPieces &= ~MOVEMASK[last.endsq];
                blackPieces |= MOVEMASK[last.stsq];
                whitePieces |= MOVEMASK[last.endsq];
            }
        }
        else {
            pieces[mailbox[last.endsq]] &= ~MOVEMASK[last.endsq];
            pieces[PAWNS+last.color] |= MOVEMASK[last.stsq];

            if (last.color == WHITE) {
            whitePieces &= ~MOVEMASK[last.endsq];
            whitePieces |= MOVEMASK[last.stsq];
            }
            else {
            blackPieces &= ~MOVEMASK[last.endsq];
            blackPieces |= MOVEMASK[last.stsq];
            }
        }

        mailbox[last.stsq] = PAWNS+last.color;
        mailbox[last.endsq] = last.capture;
    } // end promotion
    else if (last.capture == -1 && mailbox[last.endsq]-last.color == PAWNS && (last.endsq-last.stsq) % 8 != 0) {
        pieces[PAWNS+last.color] &= ~MOVEMASK[last.endsq];
        pieces[PAWNS+last.color] |= MOVEMASK[last.stsq];
        pieces[PAWNS-last.color] |= ((last.color == WHITE) ? last.wEP : last.bEP);

        if (last.color == WHITE) {
            whitePieces &= ~MOVEMASK[last.endsq];
            whitePieces |= MOVEMASK[last.stsq];
            blackPieces |= whiteEPCaptureSq;
            mailbox[bitScanForward(whiteEPCaptureSq)] = 0;
        }
        else {
            blackPieces &= ~MOVEMASK[last.endsq];
            blackPieces |= MOVEMASK[last.stsq];
            whitePieces |= blackEPCaptureSq;
            mailbox[bitScanForward(blackEPCaptureSq)] = 2;
        }

        mailbox[last.endsq] = -1;
        mailbox[last.stsq] = PAWNS + last.color;
    }
    else if (last.capture != -1) {
        int pieceMoved = mailbox[last.endsq];
        pieces[pieceMoved] &= ~MOVEMASK[last.endsq];
        pieces[pieceMoved] |= MOVEMASK[last.stsq];
        pieces[last.capture] |= MOVEMASK[last.endsq];

        if (last.color == WHITE) {
            whitePieces &= ~MOVEMASK[last.endsq];
            whitePieces |= MOVEMASK[last.stsq];
            blackPieces |= MOVEMASK[last.endsq];
        }
        else {
            blackPieces &= ~MOVEMASK[last.endsq];
            blackPieces |= MOVEMASK[last.stsq];
            whitePieces |= MOVEMASK[last.endsq];
        }

        mailbox[last.stsq] = pieceMoved;
        mailbox[last.endsq] = last.capture;
    }
    else {
        int pieceMoved = mailbox[last.endsq];
        pieces[pieceMoved] &= ~MOVEMASK[last.endsq];
        pieces[pieceMoved] |= MOVEMASK[last.stsq];

        if (last.color == WHITE) {
            whitePieces &= ~MOVEMASK[last.endsq];
            whitePieces |= MOVEMASK[last.stsq];
        }
        else {
            blackPieces &= ~MOVEMASK[last.endsq];
            blackPieces |= MOVEMASK[last.stsq];
        }

        mailbox[last.endsq] = -1;
        mailbox[last.stsq] = pieceMoved;
    }

    // reset en passant
    whiteEPCaptureSq = last.wEP;
    blackEPCaptureSq = last.bEP;

    // change castling flags
    whiteCanKCastle = last.whiteCanKCastle;
    whiteCanQCastle = last.whiteCanQCastle;
    blackCanKCastle = last.blackCanKCastle;
    blackCanQCastle = last.blackCanQCastle;
}
*/

bool Board::isLegalMove(Move m, int color) {
    uint64_t legalM = 0;
    uint64_t legalC = 0;
    uint64_t moved = 0;

    if (isCastle(m)) {
        return true;
    }

    switch (getPiece(m)) {
        case PAWNS:
            moved = pieces[color+PAWNS] & MOVEMASK[getStartSq(m)];
            legalM = (color == WHITE) ? getWPawnMoves(moved) : getBPawnMoves(moved);
            legalC = (color == WHITE) ? (getWPawnCaptures(moved) & blackPieces) :
            (getBPawnCaptures(moved) & whitePieces);
            break;
        case KNIGHTS:
            legalM = getKnightSquares(getStartSq(m));
            legalC = legalM;
            legalM &= ~(whitePieces & blackPieces);
            legalC &= (color == WHITE) ? blackPieces : whitePieces;
            break;
        case BISHOPS:
            legalM = getBishopSquares(getStartSq(m));
            legalC = legalM;
            legalM &= ~(whitePieces & blackPieces);
            legalC &= (color == WHITE) ? blackPieces : whitePieces;
            break;
        case ROOKS:
            legalM = getRookSquares(getStartSq(m));
            legalC = legalM;
            legalM &= ~(whitePieces & blackPieces);
            legalC &= (color == WHITE) ? blackPieces : whitePieces;
            break;
        case QUEENS:
            legalM = getQueenSquares(getStartSq(m));
            legalC = legalM;
            legalM &= ~(whitePieces & blackPieces);
            legalC &= (color == WHITE) ? blackPieces : whitePieces;
            break;
        case KINGS:
            legalM = getKingAttacks(color);
            legalC = legalM;
            legalM &= ~(whitePieces & blackPieces);
            legalC &= (color == WHITE) ? blackPieces : whitePieces;
            break;
        default:
            cout << "Error: Invalid piece type." << endl;
            return false;
    }

    if (isCapture(m)) {
        if ((MOVEMASK[getEndSq(m)] & legalC) == 0)
            return false;
    }
    else {
        if ((MOVEMASK[getEndSq(m)] & legalM) == 0)
            return false;
    }

    Board b = staticCopy();
    b.doMove(m, color);
    if (b.getInCheck(color)) {
        return false;
    }

    return true;
}

// Get all legal moves and captures
MoveList Board::getAllLegalMoves(int color) {
    MoveList nonCaptures = getLegalMoves(color);
    MoveList moves = getLegalCaptures(color);
    for (unsigned int i = 0; i < nonCaptures.size(); i++) {
        moves.add(nonCaptures.get(i));
    }
    return moves;
}

MoveList Board::getLegalMoves(int color) {
    MoveList moves = getPseudoLegalMoves(color);

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
MoveList Board::getPseudoLegalMoves(int color) {
    MoveList result;
    uint64_t pawns = pieces[color+PAWNS];
    uint64_t knights = pieces[color+KNIGHTS];
    uint64_t bishops = pieces[color+BISHOPS];
    uint64_t rooks = pieces[color+ROOKS];
    uint64_t queens = pieces[color+QUEENS];
    uint64_t kings = pieces[color+KINGS];
    uint64_t legal;

    if (color == WHITE) {
        while (pawns) {
            uint64_t single = pawns & -pawns;
            pawns &= pawns-1;
            int stsq = bitScanForward(single);

            legal = getWPawnMoves(single);
            uint64_t promotions = legal & RANKS[7];

            if (promotions) {
                int endsq = bitScanForward(promotions);
                Move mk = encodeMove(stsq, endsq, PAWNS, false);
                mk = setPromotion(mk, KNIGHTS);
                Move mb = encodeMove(stsq, endsq, PAWNS, false);
                mb = setPromotion(mb, BISHOPS);
                Move mr = encodeMove(stsq, endsq, PAWNS, false);
                mr = setPromotion(mr, ROOKS);
                Move mq = encodeMove(stsq, endsq, PAWNS, false);
                mq = setPromotion(mq, QUEENS);
                result.add(mk);
                result.add(mb);
                result.add(mr);
                result.add(mq);
            }
            else {
                while (legal) {
                    int endsq = bitScanForward(legal);
                    legal &= legal-1;

                    result.add(encodeMove(stsq, endsq, PAWNS, false));
                }
            }
        }
    }
    else {
        while (pawns) {
            uint64_t single = pawns & -pawns;
            pawns &= pawns-1;
            int stsq = bitScanForward(single);

            legal = getBPawnMoves(single);
            uint64_t promotions = legal & RANKS[0];

            if (promotions) {
                int endsq = bitScanForward(promotions);
                Move mk = encodeMove(stsq, endsq, PAWNS, false);
                mk = setPromotion(mk, KNIGHTS);
                Move mb = encodeMove(stsq, endsq, PAWNS, false);
                mb = setPromotion(mb, BISHOPS);
                Move mr = encodeMove(stsq, endsq, PAWNS, false);
                mr = setPromotion(mr, ROOKS);
                Move mq = encodeMove(stsq, endsq, PAWNS, false);
                mq = setPromotion(mq, QUEENS);
                result.add(mk);
                result.add(mb);
                result.add(mr);
                result.add(mq);
            }
            else {
                while (legal) {
                    int endsq = bitScanForward(legal);
                    legal &= legal-1;

                    result.add(encodeMove(stsq, endsq, PAWNS, false));
                }
            }
        }
    }

    while (knights) {
        int stsq = bitScanForward(knights);
        knights &= knights-1;

        legal = getKnightSquares(stsq) & ~(whitePieces | blackPieces);
        while (legal) {
            int endsq = bitScanForward(legal);
            legal &= legal-1;

            result.add(encodeMove(stsq, endsq, KNIGHTS, false));
        }
    }

    while (bishops) {
        int stsq = bitScanForward(bishops);
        bishops &= bishops-1;

        legal = getBishopSquares(stsq) & ~(whitePieces | blackPieces);
        while (legal) {
            int endsq = bitScanForward(legal);
            legal &= legal-1;

            result.add(encodeMove(stsq, endsq, BISHOPS, false));
        }
    }

    while (rooks) {
        int stsq = bitScanForward(rooks);
        rooks &= rooks-1;

        legal = getRookSquares(stsq) & ~(whitePieces | blackPieces);
        while (legal) {
            int endsq = bitScanForward(legal);
            legal &= legal-1;

            result.add(encodeMove(stsq, endsq, ROOKS, false));
        }
    }

    while (queens) {
        int stsq = bitScanForward(queens);
        queens &= queens-1;

        legal = getQueenSquares(stsq) & ~(whitePieces | blackPieces);
        while (legal) {
            int endsq = bitScanForward(legal);
            legal &= legal-1;

            result.add(encodeMove(stsq, endsq, QUEENS, false));
        }
    }

    int stsq = bitScanForward(kings);
    legal = getKingSquares(stsq) & ~(whitePieces | blackPieces);
    while (legal) {
        int endsq = bitScanForward(legal);
        legal &= legal-1;

        result.add(encodeMove(stsq, endsq, KINGS, false));
    }

    if (color == WHITE) {
        if (whiteCanKCastle) {
            if (((whitePieces|blackPieces) & (MOVEMASK[5] | MOVEMASK[6])) == 0 && !getInCheck(WHITE)) {
                // Check for castling through check
                int sq = 5;
                uint64_t attacked =
                    (getWPawnCaptures(MOVEMASK[sq]) & pieces[BLACK+PAWNS])
                  | (getKnightSquares(sq) & pieces[BLACK+KNIGHTS])
                  | (getBishopSquares(sq) & (pieces[BLACK+BISHOPS] | pieces[BLACK+QUEENS]))
                  | (getRookSquares(sq) & (pieces[BLACK+ROOKS] | pieces[BLACK+QUEENS]))
                  | (getKingSquares(sq) & pieces[BLACK+KINGS]);

                if (attacked == 0) {
                    Move m = encodeMove(4, 6, KINGS, false);
                    m = setCastle(m, true);
                    result.add(m);
                }
            }
        }
        if (whiteCanQCastle) {
            if (((whitePieces|blackPieces) & (MOVEMASK[1] | MOVEMASK[2] | MOVEMASK[3])) == 0 && !getInCheck(WHITE)) {
                // Check for castling through check
                int sq = 3;
                uint64_t attacked =
                    (getWPawnCaptures(MOVEMASK[sq]) & pieces[BLACK+PAWNS])
                  | (getKnightSquares(sq) & pieces[BLACK+KNIGHTS])
                  | (getBishopSquares(sq) & (pieces[BLACK+BISHOPS] | pieces[BLACK+QUEENS]))
                  | (getRookSquares(sq) & (pieces[BLACK+ROOKS] | pieces[BLACK+QUEENS]))
                  | (getKingSquares(sq) & pieces[BLACK+KINGS]);

                if (attacked == 0) {
                    Move m = encodeMove(4, 2, KINGS, false);
                    m = setCastle(m, true);
                    result.add(m);
                }
            }
        }
    }
    else {
        if (blackCanKCastle) {
            if (((whitePieces|blackPieces) & (MOVEMASK[61] | MOVEMASK[62])) == 0 && !getInCheck(BLACK)) {
                int sq = 61;
                uint64_t attacked =
                    (getBPawnCaptures(pieces[BLACK+KINGS]) & pieces[WHITE+PAWNS])
                  | (getKnightSquares(sq) & pieces[WHITE+KNIGHTS])
                  | (getBishopSquares(sq) & (pieces[WHITE+BISHOPS] | pieces[WHITE+QUEENS]))
                  | (getRookSquares(sq) & (pieces[WHITE+ROOKS] | pieces[WHITE+QUEENS]))
                  | (getKingSquares(sq) & pieces[WHITE+KINGS]);

                if (attacked == 0) {
                    Move m = encodeMove(60, 62, KINGS, false);
                    m = setCastle(m, true);
                    result.add(m);
                }
            }
        }
        if (blackCanQCastle) {
            if (((whitePieces|blackPieces) & (MOVEMASK[57] | MOVEMASK[58] | MOVEMASK[59])) == 0 && !getInCheck(BLACK)) {
                int sq = 59;
                uint64_t attacked =
                    (getBPawnCaptures(pieces[BLACK+KINGS]) & pieces[WHITE+PAWNS])
                  | (getKnightSquares(sq) & pieces[WHITE+KNIGHTS])
                  | (getBishopSquares(sq) & (pieces[WHITE+BISHOPS] | pieces[WHITE+QUEENS]))
                  | (getRookSquares(sq) & (pieces[WHITE+ROOKS] | pieces[WHITE+QUEENS]))
                  | (getKingSquares(sq) & pieces[WHITE+KINGS]);

                if (attacked == 0) {
                    Move m = encodeMove(60, 58, KINGS, false);
                    m = setCastle(m, true);
                    result.add(m);
                }
            }
        }
    }

    return result;
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
    uint64_t pawns = pieces[color+PAWNS];
    uint64_t knights = pieces[color+KNIGHTS];
    uint64_t bishops = pieces[color+BISHOPS];
    uint64_t rooks = pieces[color+ROOKS];
    uint64_t queens = pieces[color+QUEENS];
    uint64_t kings = pieces[color+KINGS];
    uint64_t otherPieces = (color == WHITE) ? blackPieces : whitePieces;

    if (color == WHITE) {
        while (pawns) {
            uint64_t single = pawns & -pawns;
            pawns &= pawns-1;
            int stsq = bitScanForward(single);

            uint64_t legal = getWPawnCaptures(single) & otherPieces;
            uint64_t promotions = legal & RANKS[7];

            if (promotions) {
                int endsq = bitScanForward(promotions);
                promotions &= promotions-1;

                Move mk = encodeMove(stsq, endsq, PAWNS, true);
                mk = setPromotion(mk, KNIGHTS);
                Move mb = encodeMove(stsq, endsq, PAWNS, true);
                mb = setPromotion(mb, BISHOPS);
                Move mr = encodeMove(stsq, endsq, PAWNS, true);
                mr = setPromotion(mr, ROOKS);
                Move mq = encodeMove(stsq, endsq, PAWNS, true);
                mq = setPromotion(mq, QUEENS);
                result.add(mk);
                result.add(mb);
                result.add(mr);
                result.add(mq);

                if (promotions) {
                    endsq = bitScanForward(promotions);
                    Move mk2 = encodeMove(stsq, endsq, PAWNS, true);
                    mk2 = setPromotion(mk2, KNIGHTS);
                    Move mb2 = encodeMove(stsq, endsq, PAWNS, true);
                    mb2 = setPromotion(mb2, BISHOPS);
                    Move mr2 = encodeMove(stsq, endsq, PAWNS, true);
                    mr2 = setPromotion(mr2, ROOKS);
                    Move mq2 = encodeMove(stsq, endsq, PAWNS, true);
                    mq2 = setPromotion(mq2, QUEENS);
                    result.add(mk2);
                    result.add(mb2);
                    result.add(mr2);
                    result.add(mq2);
                }
            }
            else {
                while (legal) {
                    int endsq = bitScanForward(legal);
                    legal &= legal-1;

                    result.add(encodeMove(stsq, endsq, PAWNS, true));
                }
            }
        }

        if (whiteEPCaptureSq) {
            uint64_t taker = (whiteEPCaptureSq << 1) & NOTA & pieces[WHITE+PAWNS];
            if (taker) {
                result.add(encodeMove(bitScanForward(taker),
                        bitScanForward(whiteEPCaptureSq << 8), PAWNS, true));
            }
            else {
                taker = (whiteEPCaptureSq >> 1) & NOTH & pieces[WHITE+PAWNS];
                if (taker) {
                    result.add(encodeMove(bitScanForward(taker),
                            bitScanForward(whiteEPCaptureSq << 8), PAWNS, true));
                }
            }
        }
    }
    else {
        while (pawns) {
            uint64_t single = pawns & -pawns;
            pawns &= pawns-1;
            int stsq = bitScanForward(single);

            uint64_t legal = getBPawnCaptures(single) & otherPieces;
            uint64_t promotions = legal & RANKS[0];

            if (promotions) {
                int endsq = bitScanForward(promotions);
                promotions &= promotions-1;

                Move mk = encodeMove(stsq, endsq, PAWNS, true);
                mk = setPromotion(mk, KNIGHTS);
                Move mb = encodeMove(stsq, endsq, PAWNS, true);
                mb = setPromotion(mb, BISHOPS);
                Move mr = encodeMove(stsq, endsq, PAWNS, true);
                mr = setPromotion(mr, ROOKS);
                Move mq = encodeMove(stsq, endsq, PAWNS, true);
                mq = setPromotion(mq, QUEENS);
                result.add(mk);
                result.add(mb);
                result.add(mr);
                result.add(mq);

                if (promotions) {
                    endsq = bitScanForward(promotions);
                    Move mk2 = encodeMove(stsq, endsq, PAWNS, true);
                    mk2 = setPromotion(mk2, KNIGHTS);
                    Move mb2 = encodeMove(stsq, endsq, PAWNS, true);
                    mb2 = setPromotion(mb2, BISHOPS);
                    Move mr2 = encodeMove(stsq, endsq, PAWNS, true);
                    mr2 = setPromotion(mr2, ROOKS);
                    Move mq2 = encodeMove(stsq, endsq, PAWNS, true);
                    mq2 = setPromotion(mq2, QUEENS);
                    result.add(mk2);
                    result.add(mb2);
                    result.add(mr2);
                    result.add(mq2);
                }
            }
            else {
                while (legal) {
                    int endsq = bitScanForward(legal);
                    legal &= legal-1;

                    result.add(encodeMove(stsq, endsq, PAWNS, true));
                }
            }
        }

        if (blackEPCaptureSq) {
            uint64_t taker = (blackEPCaptureSq << 1) & NOTA & pieces[BLACK+PAWNS];
            if (taker) {
                result.add(encodeMove(bitScanForward(taker),
                        bitScanForward(blackEPCaptureSq >> 8), PAWNS, true));
            }
            else {
                taker = (blackEPCaptureSq >> 1) & NOTH & pieces[BLACK+PAWNS];
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
    uint64_t legal = getKingSquares(stsq) & otherPieces;
    while (legal) {
        int endsq = bitScanForward(legal);
        legal &= legal-1;

        result.add(encodeMove(stsq, endsq, KINGS, true));
    }

    return result;
}


// ---------------------King: check, checkmate, stalemate----------------------
bool Board::getInCheck(int color) {
    int sq = bitScanForward(pieces[color+KINGS]);
    uint64_t pawns = (color == WHITE) ? getWPawnCaptures(pieces[WHITE+KINGS])
                                      : getBPawnCaptures(pieces[BLACK+KINGS]);
    int other = -color;

    return (pawns & pieces[other+PAWNS])
         | (getKnightSquares(sq) & pieces[other+KNIGHTS])
         | (getBishopSquares(sq) & (pieces[other+BISHOPS] | pieces[other+QUEENS]))
         | (getRookSquares(sq) & (pieces[other+ROOKS] | pieces[other+QUEENS]))
         | (getKingSquares(sq) & pieces[other+KINGS]);
}

bool Board::isWinMate() {
    MoveList moves = getLegalMoves(WHITE);
    MoveList captures = getLegalCaptures(WHITE);
    bool isInMate = false;
    if (moves.size() == 0 && captures.size() == 0 && getInCheck(WHITE))
        isInMate = true;
    
    return isInMate;
}

bool Board::isBinMate() {
    MoveList moves = getLegalMoves(BLACK);
    MoveList captures = getLegalCaptures(BLACK);
    bool isInMate = false;
    if (moves.size() == 0 && captures.size() == 0 && getInCheck(BLACK))
        isInMate = true;

    return isInMate;
}

// TODO Includes 3-fold repetition draw for now.
bool Board::isStalemate(int sideToMove) {
    if(!(twoFoldStartSqs & (1 << 31))) {
        bool isTwoFold = true;
        if (((twoFoldPTM >> 8) & 0xFF) != (uint8_t) playerToMove
         || ((twoFoldPTM >> 24) & 0xFF) != (uint8_t) playerToMove)
            isTwoFold = false;

        if ( (((twoFoldStartSqs >> 24) & 0xFF) != ((twoFoldEndSqs >> 8) & 0xFF))
          || (((twoFoldStartSqs >> 8) & 0xFF) != ((twoFoldEndSqs >> 24) & 0xFF))
          || (((twoFoldStartSqs >> 16) & 0xFF) != (twoFoldEndSqs & 0xFF))
          || ((twoFoldStartSqs & 0xFF) != ((twoFoldEndSqs >> 16) & 0xFF))) {
            isTwoFold = false;
        }

        if (isTwoFold) return true;
    }

    MoveList temp = getLegalMoves(sideToMove);
    bool isInStalemate = false;

    if (temp.size() == 0 && !getInCheck(sideToMove))
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
    int whiteMaterial = PAWN_VALUE * count(pieces[WHITE+PAWNS])
            + KNIGHT_VALUE * count(pieces[WHITE+KNIGHTS])
            + BISHOP_VALUE * count(pieces[WHITE+BISHOPS])
            + ROOK_VALUE * count(pieces[WHITE+ROOKS])
            + QUEEN_VALUE * count(pieces[WHITE+QUEENS]);
    int blackMaterial = PAWN_VALUE * count(pieces[BLACK+PAWNS])
            + KNIGHT_VALUE * count(pieces[BLACK+KNIGHTS])
            + BISHOP_VALUE * count(pieces[BLACK+BISHOPS])
            + ROOK_VALUE * count(pieces[BLACK+ROOKS])
            + QUEEN_VALUE * count(pieces[BLACK+QUEENS]);
    
    // compute endgame factor which is between 0 and EG_FACTOR_RES, inclusive
    int egFactor = (whiteMaterial + blackMaterial - START_VALUE / 2) * EG_FACTOR_RES / START_VALUE;
    egFactor = max(0, min(EG_FACTOR_RES, egFactor));
    
    value += whiteMaterial + (PAWN_VALUE_EG - PAWN_VALUE) * count(pieces[WHITE+PAWNS]) * egFactor / EG_FACTOR_RES;
    value -= blackMaterial + (PAWN_VALUE_EG - PAWN_VALUE) * count(pieces[BLACK+PAWNS]) * egFactor / EG_FACTOR_RES;
    
    // bishop pair bonus
    if ((pieces[WHITE+BISHOPS] & LIGHT) && (pieces[WHITE+BISHOPS] & DARK))
        value += BISHOP_PAIR_VALUE;
    if ((pieces[BLACK+BISHOPS] & LIGHT) && (pieces[BLACK+BISHOPS] & DARK))
        value -= BISHOP_PAIR_VALUE;
    
    // piece square tables
    for (int i = 0; i < 64; i++) {
        switch (mailbox[i]) {
            case -1: // empty
                break;
            case 2: // white pawn
                value += pawnValues[(7 - i/8)*8 + i%8];
                break;
            case 0: // black pawn
                value -= pawnValues[i];
                break;
            case 3: // white knight
                value += knightValues[(7 - i/8)*8 + i%8];
                break;
            case 1: // black knight
                value -= knightValues[i];
                break;
            case 6: // white bishop
                value += bishopValues[(7 - i/8)*8 + i%8];
                break;
            case 4: // black bishop
                value -= bishopValues[i];
                break;
            case 7: // white rook
                value += rookValues[(7 - i/8)*8 + i%8];
                break;
            case 5: // black rook
                value -= rookValues[i];
                break;
            case 10: // white queen
                value += queenValues[(7 - i/8)*8 + i%8];
                break;
            case 8: // black queen
                value -= queenValues[i];
                break;
            case 11: // white king
                value += kingValues[(7 - i/8)*8 + i%8] * (EG_FACTOR_RES - egFactor) / EG_FACTOR_RES;
                break;
            case 9: // black king
                value -= kingValues[i] * (EG_FACTOR_RES - egFactor) / EG_FACTOR_RES;
                break;
            default:
                cout << "Error in piece table switch statement." << endl;
                break;
        }
    }

    // king safety
    /*
    int wfile = bitScanForward(pieces[WHITE+KINGS]) % 8;
    int bfile = bitScanForward(pieces[BLACK+KINGS]) % 8;
    if (wfile == 3 || wfile == 4)
        value -= 50 * (EG_FACTOR_RES - egFactor) / EG_FACTOR_RES;
    if (bfile == 3 || bfile == 4)
        value += 50 * (EG_FACTOR_RES - egFactor) / EG_FACTOR_RES;
    */
    
    uint64_t wksq = getKingAttacks(WHITE);
    uint64_t bksq = getKingAttacks(BLACK);
    uint64_t bAtt = 0;
    bAtt = getBPawnCaptures(pieces[BLACK+PAWNS]) | getKnightMoves(pieces[BLACK+KNIGHTS]) |
    getBishopMoves(pieces[BLACK+BISHOPS]) | getRookMoves(pieces[BLACK+ROOKS]) |
    getQueenMoves(pieces[BLACK+QUEENS]) | getKingAttacks(BLACK);
    uint64_t wAtt = 0;
    wAtt = getWPawnCaptures(pieces[WHITE+PAWNS]) | getKnightMoves(pieces[WHITE+KNIGHTS]) |
    getBishopMoves(pieces[WHITE+BISHOPS]) | getRookMoves(pieces[WHITE+ROOKS]) |
    getQueenMoves(pieces[WHITE+QUEENS]) | getKingAttacks(WHITE);
    
    value -= 25 * count(wksq & bAtt);
    value += 25 * count(bksq & wAtt);
    
    uint64_t wpawnShield = (wksq | pieces[WHITE+KINGS]) << 8;
    uint64_t bpawnShield = (bksq | pieces[BLACK+KINGS]) >> 8;
    // Have only pawns on ABC, FGH files count towards the pawn shield
    value += 30 * count(wpawnShield & pieces[WHITE+PAWNS] & 0xe7e7e7e7e7e7e7e7);
    value -= 30 * count(bpawnShield & pieces[BLACK+PAWNS] & 0xe7e7e7e7e7e7e7e7);
    
    value += getPseudoMobility(WHITE);
    value -= getPseudoMobility(BLACK);
    return value;
}

bool Board::pieceOn(int color, int x, int y) {
    if (color == WHITE)
        return (whitePieces & MOVEMASK[x + 8*y]) != 0;
    else
        return (blackPieces & MOVEMASK[x + 8*y]) != 0;
}

// Faster estimates of piece mobility (number of legal moves)
int Board::getPseudoMobility(int color) {
    int result = 0;
    uint64_t knights = pieces[color+KNIGHTS];
    uint64_t bishops = pieces[color+BISHOPS];
    uint64_t rooks = pieces[color+ROOKS];
    uint64_t queens = pieces[color+QUEENS];
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

uint64_t Board::getAttackMap(int color, int sq) {
    uint64_t pawnCap = (color == WHITE)
                     ? getBPawnCaptures(MOVEMASK[sq])
                     : getWPawnCaptures(MOVEMASK[sq]);
    return (pawnCap & pieces[color+PAWNS])
         | (getKnightSquares(sq) & pieces[color+KNIGHTS])
         | (getBishopSquares(sq) & (pieces[color+BISHOPS] | pieces[color+QUEENS]))
         | (getRookSquares(sq) & (pieces[color+ROOKS] | pieces[color+QUEENS]))
         | (getKingSquares(sq) & pieces[color+KINGS]);
}

// TODO come up with a better way to do this
uint64_t Board::getLeastValuableAttacker(uint64_t attackers, int color, int &piece) {
    uint64_t single = attackers & pieces[color+PAWNS];
    if(single) {
        piece = PAWNS;
        return single & -single;
    }
    single = attackers & pieces[color+KNIGHTS];
    if(single) {
        piece = KNIGHTS;
        return single & -single;
    }
    single = attackers & pieces[color+BISHOPS];
    if(single) {
        piece = BISHOPS;
        return single & -single;
    }
    single = attackers & pieces[color+ROOKS];
    if(single) {
        piece = ROOKS;
        return single & -single;
    }
    single = attackers & pieces[color+QUEENS];
    if(single) {
        piece = QUEENS;
        return single & -single;
    }

    piece = KINGS;
    return attackers & pieces[color+KINGS];
}

// TODO consider xrays
// Static exchange evaluation algorithm from
// https://chessprogramming.wikispaces.com/SEE+-+The+Swap+Algorithm
int Board::getSEE(int color, int sq) {
    int gain[32], d = 0, piece = 0;
    uint64_t attackers = getAttackMap(color, sq) | getAttackMap(-color, sq);
    uint64_t single = getLeastValuableAttacker(attackers, color, piece);
    // Get value of piece initially being captured. If the destination square is
    // empty, then the capture is an en passant.
    gain[d] = valueOfPiece((mailbox[sq] == -1) ? PAWNS : (mailbox[sq] + color));

    do {
        d++; // next depth
        color = -color;
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
uint64_t Board::getWPawnMoves(uint64_t pawns) {
    uint64_t result = pawns;
    uint64_t open = ~(whitePieces | blackPieces);

    result = (result << 8) & open;
    result |= (result << 8) & open & RANKS[3];

    return result;
}

uint64_t Board::getBPawnMoves(uint64_t pawns) {
    uint64_t result = pawns;
    uint64_t open = ~(whitePieces | blackPieces);

    result = (result >> 8) & open;
    result |= (result >> 8) & open & RANKS[4];

    return result;
}

uint64_t Board::getWPawnCaptures(uint64_t pawns) {
    uint64_t result = (pawns << 9) & NOTA;
    result |= (pawns << 7) & NOTH;
    return result;
}

uint64_t Board::getBPawnCaptures(uint64_t pawns) {
    uint64_t result = (pawns >> 7) & NOTA;
    result |= (pawns >> 9) & NOTH;
    return result;
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
    uint64_t kings = pieces[color+KINGS];
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
        result[i] = mailbox[i];
    }
    return result;
}

string Board::toString() {
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
    return result;
}

// TODO not updated or tested
/*
public boolean equals(Object o) {
    if (!(o instanceof Board))
        return false;
    Board other = (Board)o;
    if (mailbox.equals(other.mailbox) && whiteCanKCastle == other.whiteCanKCastle
    && whiteCanQCastle == other.whiteCanQCastle && blackCanKCastle == other.blackCanKCastle
    && blackCanQCastle == other.blackCanQCastle && whiteEPCaptureSq == other.whiteEPCaptureSq
    && blackEPCaptureSq == other.blackEPCaptureSq)
        return true;
    else return false;
}
*/

// Not tested
/*
public int hashCode() {
    int result = 221; //2166136261
    for (int i = 0; i < 64; i++) {
        result ^= mailbox[i];
        result *= 16777619;
    }
    return (result & 0x7FFFFFFF);
}
*/

/* TODO java code not yet converted
private class BMove {
    public int color, stsq, endsq, capture;
    public boolean whiteCanKCastle = false;
    public boolean blackCanKCastle = false;
    public boolean whiteCanQCastle = false;
    public boolean blackCanQCastle = false;
    public uint64_t wEP, bEP;
    public boolean isCastle;
    public boolean isPromotion = false;

    public BMove(int _color, int _stsq, int _endsq, int _capture, uint64_t _wEP, uint64_t _bEP, boolean _isCastle) {
        color = _color;
        stsq = _stsq;
        endsq = _endsq;
        capture = _capture;
        wEP = _wEP;
        bEP = _bEP;
        isCastle = _isCastle;
    }
}
*/
