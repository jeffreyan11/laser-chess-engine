#include "board.h"

Board::Board() {
    pieces = new uint64_t[12];
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
    whiteCanEP = 0;
    blackCanEP = 0;
    mailbox = new int[64];
    for(int i = 0; i < 64; i++) {
        mailbox[i] = initMailbox[i];
    }
}

Board::~Board() {
    delete[] pieces;
    delete[] mailbox;
}

Board *Board::copy() {
    Board *result = new Board();
    for(int i = 0; i < 12; i++) {
        result->pieces[i] = pieces[i];
    }
    result->whitePieces = whitePieces;
    result->blackPieces = blackPieces;
    result->whiteCanKCastle = whiteCanKCastle;
    result->whiteCanQCastle = whiteCanQCastle;
    result->blackCanKCastle = blackCanKCastle;
    result->blackCanQCastle = blackCanQCastle;
    result->whiteCanEP = whiteCanEP;
    result->blackCanEP = blackCanEP;
    for(int i = 0; i < 64; i++) {
        result->mailbox[i] = mailbox[i];
    }
    return result;
}

void Board::doMove(Move *m, int color) {
/* TODO undo move stuff
    BMove record = new BMove(color, m->startsq, m->endsq, mailbox[m->endsq], whiteCanEP, blackCanEP, m->isCastle);
    if(m->promotion != -1)
        record->isPromotion = true;
    record->whiteCanKCastle = whiteCanKCastle;
    record->whiteCanQCastle = whiteCanQCastle;
    record->blackCanKCastle = blackCanKCastle;
    record->blackCanQCastle = blackCanQCastle;
    history.push(record);
*/

    if(m->isCastle) {
        if(color == WHITE && m->endsq == 6) { // w kside
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
        else if(color == WHITE && m->endsq == 2) { // w qside
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
        else if(color == BLACK && m->endsq == 62) { // b kside
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
        else { // b qside
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
        whiteCanEP = 0;
        blackCanEP = 0;
    } // end castling
    else if(m->promotion != -1) {
        if(m->isCapture) {
            pieces[PAWNS+color] &= ~MOVEMASK[m->startsq];
            pieces[m->promotion+color] |= MOVEMASK[m->endsq];
            pieces[mailbox[m->endsq]] &= ~MOVEMASK[m->endsq];

            if(color == WHITE) {
                whitePieces &= ~MOVEMASK[m->startsq];
                whitePieces |= MOVEMASK[m->endsq];
                blackPieces &= ~MOVEMASK[m->endsq];
            }
            else {
                blackPieces &= ~MOVEMASK[m->startsq];
                blackPieces |= MOVEMASK[m->endsq];
                whitePieces &= ~MOVEMASK[m->endsq];
            }
        }
        else {
            pieces[PAWNS+color] &= ~MOVEMASK[m->startsq];
            pieces[m->promotion+color] |= MOVEMASK[m->endsq];

            if(color == WHITE) {
                whitePieces &= ~MOVEMASK[m->startsq];
                whitePieces |= MOVEMASK[m->endsq];
            }
            else {
                blackPieces &= ~MOVEMASK[m->startsq];
                blackPieces |= MOVEMASK[m->endsq];
            }
        }

        mailbox[m->startsq] = -1;
        mailbox[m->endsq] = m->promotion + color;
        whiteCanEP = 0;
        blackCanEP = 0;
    } // end promotion
    else if(m->isCapture) {
        if(mailbox[m->endsq] == -1 && m->piece == PAWNS) {
            pieces[PAWNS+color] &= ~MOVEMASK[m->startsq];
            pieces[PAWNS+color] |= MOVEMASK[m->endsq];
            pieces[PAWNS-color] &= ~((color == WHITE) ? whiteCanEP : blackCanEP);

            if(color == WHITE) {
                whitePieces &= ~MOVEMASK[m->startsq];
                whitePieces |= MOVEMASK[m->endsq];
                blackPieces &= ~((color == WHITE) ? whiteCanEP : blackCanEP);
                mailbox[bitScanForward(whiteCanEP)] = -1;
            }
            else {
                blackPieces &= ~MOVEMASK[m->startsq];
                blackPieces |= MOVEMASK[m->endsq];
                whitePieces &= ~((color == WHITE) ? whiteCanEP : blackCanEP);
                mailbox[bitScanForward(blackCanEP)] = -1;
            }
        
            mailbox[m->startsq] = -1;
            mailbox[m->endsq] = m->piece + color;
        }
        else {
            pieces[m->piece+color] &= ~MOVEMASK[m->startsq];
            pieces[m->piece+color] |= MOVEMASK[m->endsq];
            pieces[mailbox[m->endsq]] &= ~MOVEMASK[m->endsq];

            if(color == WHITE) {
                whitePieces &= ~MOVEMASK[m->startsq];
                whitePieces |= MOVEMASK[m->endsq];
                blackPieces &= ~MOVEMASK[m->endsq];
            }
            else {
                blackPieces &= ~MOVEMASK[m->startsq];
                blackPieces |= MOVEMASK[m->endsq];
                whitePieces &= ~MOVEMASK[m->endsq];
            }

            mailbox[m->startsq] = -1;
            mailbox[m->endsq] = m->piece + color;
        }
        whiteCanEP = 0;
        blackCanEP = 0;
    } // end capture
    else {
        pieces[m->piece+color] &= ~MOVEMASK[m->startsq];
        pieces[m->piece+color] |= MOVEMASK[m->endsq];

        if(color == WHITE) {
            whitePieces &= ~MOVEMASK[m->startsq];
            whitePieces |= MOVEMASK[m->endsq];
        }
        else {
            blackPieces &= ~MOVEMASK[m->startsq];
            blackPieces |= MOVEMASK[m->endsq];
        }

        mailbox[m->startsq] = -1;
        mailbox[m->endsq] = m->piece + color;

        // check for en passant
        if(m->piece == PAWNS) {
            if(color == WHITE && m->startsq/8 == 1 && m->endsq/8 == 3) {
                blackCanEP = MOVEMASK[m->endsq];
                whiteCanEP = 0;
            }
            else if(m->startsq/8 == 6 && m->endsq/8 == 4) {
                whiteCanEP = MOVEMASK[m->endsq];
                blackCanEP = 0;
            }
            else {
                whiteCanEP = 0;
                blackCanEP = 0;
            }
        }
        else {
            whiteCanEP = 0;
            blackCanEP = 0;
        }
    } // end normal move

    // change castling flags
    if(m->piece == KINGS) {
        if(color == WHITE) {
            whiteCanKCastle = false;
            whiteCanQCastle = false;
        }
        else {
            blackCanKCastle = false;
            blackCanQCastle = false;
        }
    }
    else {
        if(whiteCanKCastle || whiteCanQCastle) {
            int whiteR = (int)(RANKS[0] & pieces[WHITE+ROOKS]);
            if((whiteR & 0x80) == 0)
                whiteCanKCastle = false;
            if((whiteR & 1) == 0)
                whiteCanQCastle = false;
        }
        if(blackCanKCastle || blackCanQCastle) {
            int blackR = (int)((RANKS[7] & pieces[BLACK+ROOKS]) >> 56);
            if((blackR & 0x80) == 0)
                blackCanKCastle = false;
            if((blackR & 1) == 0)
                blackCanQCastle = false;
        }
    } // end castling flags
}

bool Board::doPLMove(Move *m, int color) {
    doMove(m, color);

    if((color == WHITE) ? getWinCheck() : getBinCheck())
        return false;
    else return true;
}

/*
void Board::undoMove() {
    // BMove last = history.pop();

    if(last.isCastle) {
        if(last.color == WHITE && last.endsq == 6) { // w kside
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
        else if(last.color == WHITE && last.endsq == 2) { // w qside
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
        else if(last.color == BLACK && last.endsq == 62) { // b kside
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
        whiteCanEP = last.wEP;
        blackCanEP = last.bEP;
        return;
    } // end castling
    else if(last.isPromotion) {
        if(last.capture != -1) {
            pieces[mailbox[last.endsq]] &= ~MOVEMASK[last.endsq];
            pieces[last.capture] |= MOVEMASK[last.endsq];
            pieces[PAWNS+last.color] |= MOVEMASK[last.stsq];

            if(last.color == WHITE) {
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

            if(last.color == WHITE) {
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
    else if(last.capture == -1 && mailbox[last.endsq]-last.color == PAWNS && (last.endsq-last.stsq) % 8 != 0) {
        pieces[PAWNS+last.color] &= ~MOVEMASK[last.endsq];
        pieces[PAWNS+last.color] |= MOVEMASK[last.stsq];
        pieces[PAWNS-last.color] |= ((last.color == WHITE) ? last.wEP : last.bEP);

        if(last.color == WHITE) {
            whitePieces &= ~MOVEMASK[last.endsq];
            whitePieces |= MOVEMASK[last.stsq];
            blackPieces |= whiteCanEP;
            mailbox[bitScanForward(whiteCanEP)] = 0;
        }
        else {
            blackPieces &= ~MOVEMASK[last.endsq];
            blackPieces |= MOVEMASK[last.stsq];
            whitePieces |= blackCanEP;
            mailbox[bitScanForward(blackCanEP)] = 2;
        }

        mailbox[last.endsq] = -1;
        mailbox[last.stsq] = PAWNS + last.color;
    }
    else if(last.capture != -1) {
        int pieceMoved = mailbox[last.endsq];
        pieces[pieceMoved] &= ~MOVEMASK[last.endsq];
        pieces[pieceMoved] |= MOVEMASK[last.stsq];
        pieces[last.capture] |= MOVEMASK[last.endsq];

        if(last.color == WHITE) {
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

        if(last.color == WHITE) {
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
    whiteCanEP = last.wEP;
    blackCanEP = last.bEP;

    // change castling flags
    whiteCanKCastle = last.whiteCanKCastle;
    whiteCanQCastle = last.whiteCanQCastle;
    blackCanKCastle = last.blackCanKCastle;
    blackCanQCastle = last.blackCanQCastle;
}
*/

bool Board::isLegalMove(Move *m, int color) {
    uint64_t legalM = 0;
    uint64_t legalC = 0;
    uint64_t moved = 0;

    if(m->isCastle) { //TODO if parsing user move is changed: castle and promotion
        return true;
    /*if(color == WHITE && m->endsq == 6 && whiteCanKCastle) {

    }
    else if(color == WHITE && m->endsq == 2 && whiteCanQCastle) {

    }
    else if(color == BLACK && m->endsq == 62 && blackCanKCastle) {

    }
    else if(color == BLACK && m->endsq == 58 && blackCanQCastle) {

    }*/
    }

    switch(m->piece) {
        case PAWNS:
            moved = pieces[color+PAWNS] & MOVEMASK[m->startsq];
            legalM = (color == WHITE) ? getWPawnMoves(moved) : getBPawnMoves(moved);
            legalC = (color == WHITE) ? (getWPawnCaptures(moved) & blackPieces) :
            (getBPawnCaptures(moved) & whitePieces);
            break;
        case KNIGHTS:
            moved = pieces[color+KNIGHTS] & MOVEMASK[m->startsq];
            legalM = getKnightSquares(m->startsq);
            legalC = legalM;
            legalM &= ~(whitePieces & blackPieces);
            legalC &= (color == WHITE) ? blackPieces : whitePieces;
            break;
        case BISHOPS:
            moved = pieces[color+BISHOPS] & MOVEMASK[m->startsq];
            legalM = (color == WHITE) ? getWBishopMoves(moved) : getBBishopMoves(moved);
            legalC = legalM;
            legalM &= ~(whitePieces & blackPieces);
            legalC &= (color == WHITE) ? blackPieces : whitePieces;
            break;
        case ROOKS:
            moved = pieces[color+ROOKS] & MOVEMASK[m->startsq];
            legalM = (color == WHITE) ? getWRookMoves(moved) : getBRookMoves(moved);
            legalC = legalM;
            legalM &= ~(whitePieces & blackPieces);
            legalC &= (color == WHITE) ? blackPieces : whitePieces;
            break;
        case QUEENS:
            moved = pieces[color+QUEENS] & MOVEMASK[m->startsq];
            legalM = (color == WHITE) ? getWQueenMoves(moved) : getBQueenMoves(moved);
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

    if(m->isCapture) {
        if((MOVEMASK[m->endsq] & legalC) == 0)
            return false;
    }
    else {
        if((MOVEMASK[m->endsq] & legalM) == 0)
            return false;
    }

    Board *b = copy();
    b->doMove(m, color);
    if((color == WHITE) ? b->getWinCheck() : b->getBinCheck()) {
        delete b;
        return false;
    }

    delete b;
    return true;
}

vector<Move *> Board::getLegalWMoves() {
    vector<Move *> moveList = getPseudoLegalWMoves();

    for(unsigned int i = 0; i < moveList.size(); i++) {
        Board *b = copy();
        b->doMove(moveList.at(i), WHITE);

        if(b->getWinCheck()) {
            moveList.erase(moveList.begin() + i);
            i--;
        }

        delete b;
    }

    return moveList;
}

vector<Move *> Board::getLegalBMoves() {
    vector<Move *> moveList = getPseudoLegalBMoves();

    for(unsigned int i = 0; i < moveList.size(); i++) {
        Board *b = copy();
        b->doMove(moveList.at(i), BLACK);

        if(b->getBinCheck()) {
            moveList.erase(moveList.begin() + i);
            i--;
        }

        delete b;
    }

    return moveList;
}

vector<Move *> Board::getLegalMoves(int color) {
    return (color == WHITE) ? getLegalWMoves() : getLegalBMoves();
}

//---------------------Pseudo-legal Moves---------------------------
vector<Move *> Board::getPseudoLegalWMoves() {
    vector<Move *> result;
    uint64_t pawns = pieces[WHITE+PAWNS];
    uint64_t knights = pieces[WHITE+KNIGHTS];
    uint64_t bishops = pieces[WHITE+BISHOPS];
    uint64_t rooks = pieces[WHITE+ROOKS];
    uint64_t queens = pieces[WHITE+QUEENS];
    uint64_t kings = pieces[WHITE+KINGS];
    uint64_t legal;

    while(pawns != 0) {
        uint64_t single = pawns & -pawns;
        pawns &= pawns-1;
        int stsq = bitScanForward(single);

        legal = getWPawnMoves(single);
        uint64_t promotions = legal & RANKS[7];

        if(promotions != 0) {
            int endsq = bitScanForward(promotions);
            Move *mk = new Move(PAWNS, false, stsq, endsq);
            mk->promotion = KNIGHTS;
            Move *mb = new Move(PAWNS, false, stsq, endsq);
            mk->promotion = BISHOPS;
            Move *mr = new Move(PAWNS, false, stsq, endsq);
            mk->promotion = ROOKS;
            Move *mq = new Move(PAWNS, false, stsq, endsq);
            mk->promotion = QUEENS;
            result.push_back(mk);
            result.push_back(mb);
            result.push_back(mr);
            result.push_back(mq);
        }
        else {
            while(legal != 0) {
                int endsq = bitScanForward(legal & -legal);
                legal &= legal-1;

                result.push_back(new Move(PAWNS, false, stsq, endsq));
            }
        }
    }

    while(knights != 0) {
        uint64_t single = knights & -knights;
        knights &= knights-1;
        int stsq = bitScanForward(single);

        legal = getKnightSquares(stsq) & ~(whitePieces | blackPieces);
        while(legal != 0) {
            int endsq = bitScanForward(legal & -legal);
            legal &= legal-1;

            result.push_back(new Move(KNIGHTS, false, stsq, endsq));
        }
    }

    while(bishops != 0) {
        int stsq = bitScanForward(bishops & -bishops);
        bishops &= bishops-1;

        legal = getBishopSquares(stsq) & ~(whitePieces | blackPieces);
        while(legal != 0) {
            int endsq = bitScanForward(legal & -legal);
            legal &= legal-1;

            result.push_back(new Move(BISHOPS, false, stsq, endsq));
        }
    }

    while(rooks != 0) {
        int stsq = bitScanForward(rooks & -rooks);
        rooks &= rooks-1;

        legal = getRookSquares(stsq) & ~(whitePieces | blackPieces);
        while(legal != 0) {
            int endsq = bitScanForward(legal & -legal);
            legal &= legal-1;

            result.push_back(new Move(ROOKS, false, stsq, endsq));
        }
    }

    while(queens != 0) {
        int stsq = bitScanForward(queens & -queens);
        queens &= queens-1;

        legal = getQueenSquares(stsq) & ~(whitePieces | blackPieces);
        while(legal != 0) {
            int endsq = bitScanForward(legal & -legal);
            legal &= legal-1;

            result.push_back(new Move(QUEENS, false, stsq, endsq));
        }
    }

    int stsq = bitScanForward(kings);
    legal = getKingSquares(stsq) & ~(whitePieces | blackPieces);
    while(legal != 0) {
        int endsq = bitScanForward(legal & -legal);
        legal &= legal-1;

        result.push_back(new Move(KINGS, false, stsq, endsq));
    }

    if(whiteCanKCastle) {
        if(((whitePieces|blackPieces) & (MOVEMASK[5] | MOVEMASK[6])) == 0 && !getWinCheck()) {
            uint64_t bAtt = 0;
            bAtt |= getBPawnCaptures(pieces[BLACK+PAWNS]) | getBKnightSquares(pieces[BLACK+KNIGHTS]) |
            getBBishopMoves(pieces[BLACK+BISHOPS]) | getBRookMoves(pieces[BLACK+ROOKS]) |
            getBQueenMoves(pieces[BLACK+QUEENS]) | getKingAttacks(BLACK);
            if((bAtt & (MOVEMASK[5] | MOVEMASK[6])) == 0) {
                Move *m = new Move(KINGS, false, 4, 6);
                m->isCastle = true;
                result.push_back(m);
            }
        }
    }
    if(whiteCanQCastle) {
        if(((whitePieces|blackPieces) & (MOVEMASK[1] | MOVEMASK[2] | MOVEMASK[3])) == 0 && !getWinCheck()) {
            uint64_t bAtt = 0;
            bAtt |= getBPawnCaptures(pieces[BLACK+PAWNS]) | getBKnightSquares(pieces[BLACK+KNIGHTS]) |
            getBBishopMoves(pieces[BLACK+BISHOPS]) | getBRookMoves(pieces[BLACK+ROOKS]) |
            getBQueenMoves(pieces[BLACK+QUEENS]) | getKingAttacks(BLACK);
            if((bAtt & (MOVEMASK[1] | MOVEMASK[2] | MOVEMASK[3])) == 0) {
                Move *m = new Move(KINGS, false, 4, 2);
                m->isCastle = true;
                result.push_back(m);
            }
        }
    }

    return result;
}

vector<Move *> Board::getPseudoLegalBMoves() {
    vector<Move *> result;
    uint64_t pawns = pieces[BLACK+PAWNS];
    uint64_t knights = pieces[BLACK+KNIGHTS];
    uint64_t bishops = pieces[BLACK+BISHOPS];
    uint64_t rooks = pieces[BLACK+ROOKS];
    uint64_t queens = pieces[BLACK+QUEENS];
    uint64_t kings = pieces[BLACK+KINGS];
    uint64_t legal;

    while(pawns != 0) {
        uint64_t single = pawns & -pawns;
        pawns &= pawns-1;
        int stsq = bitScanForward(single);

        legal = getBPawnMoves(single);
        uint64_t promotions = legal & RANKS[0];

        if(promotions != 0) {
            int endsq = bitScanForward(promotions);
            Move *mk = new Move(PAWNS, false, stsq, endsq);
            mk->promotion = KNIGHTS;
            Move *mb = new Move(PAWNS, false, stsq, endsq);
            mk->promotion = BISHOPS;
            Move *mr = new Move(PAWNS, false, stsq, endsq);
            mk->promotion = ROOKS;
            Move *mq = new Move(PAWNS, false, stsq, endsq);
            mk->promotion = QUEENS;
            result.push_back(mk);
            result.push_back(mb);
            result.push_back(mr);
            result.push_back(mq);
        }
        else {
            while(legal != 0) {
                int endsq = bitScanForward(legal & -legal);
                legal &= legal-1;

                result.push_back(new Move(PAWNS, false, stsq, endsq));
            }
        }
    }

    while(knights != 0) {
        uint64_t single = knights & -knights;
        knights &= knights-1;
        int stsq = bitScanForward(single);

        legal = getKnightSquares(stsq) & ~(whitePieces | blackPieces);
        while(legal != 0) {
            int endsq = bitScanForward(legal & -legal);
            legal &= legal-1;

            result.push_back(new Move(KNIGHTS, false, stsq, endsq));
        }
    }

    while(bishops != 0) {
        int stsq = bitScanForward(bishops & -bishops);
        bishops &= bishops-1;

        legal = getBishopSquares(stsq) & ~(whitePieces | blackPieces);
        while(legal != 0) {
            int endsq = bitScanForward(legal & -legal);
            legal &= legal-1;

            result.push_back(new Move(BISHOPS, false, stsq, endsq));
        }
    }

    while(rooks != 0) {
        int stsq = bitScanForward(rooks & -rooks);
        rooks &= rooks-1;

        legal = getRookSquares(stsq) & ~(whitePieces | blackPieces);
        while(legal != 0) {
            int endsq = bitScanForward(legal & -legal);
            legal &= legal-1;

            result.push_back(new Move(ROOKS, false, stsq, endsq));
        }
    }

    while(queens != 0) {
        int stsq = bitScanForward(queens & -queens);
        queens &= queens-1;

        legal = getQueenSquares(stsq) & ~(whitePieces | blackPieces);
        while(legal != 0) {
            int endsq = bitScanForward(legal & -legal);
            legal &= legal-1;

            result.push_back(new Move(QUEENS, false, stsq, endsq));
        }
    }

    int stsq = bitScanForward(kings);
    legal = getKingSquares(stsq) & ~(whitePieces | blackPieces);
    while(legal != 0) {
        int endsq = bitScanForward(legal & -legal);
        legal &= legal-1;

        result.push_back(new Move(KINGS, false, stsq, endsq));
    }

    if(blackCanKCastle) {
        if(((whitePieces|blackPieces) & (MOVEMASK[61] | MOVEMASK[62])) == 0 && !getBinCheck()) {
            uint64_t wAtt = 0;
            wAtt |= getWPawnCaptures(pieces[WHITE+PAWNS]) | getWKnightSquares(pieces[WHITE+KNIGHTS]) |
            getWBishopMoves(pieces[WHITE+BISHOPS]) | getWRookMoves(pieces[WHITE+ROOKS]) |
            getWQueenMoves(pieces[WHITE+QUEENS]) | getKingAttacks(WHITE);
            if((wAtt & (MOVEMASK[61] | MOVEMASK[62])) == 0) {
                Move *m = new Move(KINGS, false, 60, 62);
                m->isCastle = true;
                result.push_back(m);
            }
        }
    }
    if(blackCanQCastle) {
        if(((whitePieces|blackPieces) & (MOVEMASK[57] | MOVEMASK[58] | MOVEMASK[59])) == 0 && !getBinCheck()) {
            uint64_t wAtt = 0;
            wAtt |= getWPawnCaptures(pieces[WHITE+PAWNS]) | getWKnightSquares(pieces[WHITE+KNIGHTS]) |
            getWBishopMoves(pieces[WHITE+BISHOPS]) | getWRookMoves(pieces[WHITE+ROOKS]) |
            getWQueenMoves(pieces[WHITE+QUEENS]) | getKingAttacks(WHITE);
            if((wAtt & (MOVEMASK[57] | MOVEMASK[58] | MOVEMASK[59])) == 0) {
                Move *m = new Move(KINGS, false, 60, 58);
                m->isCastle = true;
                result.push_back(m);
            }
        }
    }

    return result;
}

vector<Move *> Board::getPseudoLegalMoves(int color) {
    return (color == WHITE) ? getPseudoLegalWMoves() : getPseudoLegalBMoves();
}

vector<Move *> Board::getLegalWCaptures() {
    vector<Move *> moveList = getPLWCaptures();

    for(unsigned int i = 0; i < moveList.size(); i++) {
        Board *b = copy();
        b->doMove(moveList.at(i), WHITE);

        if(b->getWinCheck()) {
            moveList.erase(moveList.begin() + i);
            i--;
        }

        delete b;
    }

    return moveList;
}

vector<Move *> Board::getLegalBCaptures() {
    vector<Move *> moveList = getPLBCaptures();

    for(unsigned int i = 0; i < moveList.size(); i++) {
        Board *b = copy();
        b->doMove(moveList.at(i), BLACK);

        if(b->getBinCheck()) {
            moveList.erase(moveList.begin() + i);
            i--;
        }

        delete b;
    }

    return moveList;
}

vector<Move *> Board::getLegalCaptures(int color) {
    return (color == WHITE) ? getLegalWCaptures() : getLegalBCaptures();
}

vector<Move *> Board::getPLWCaptures() {
    vector<Move *> result;
    uint64_t pawns = pieces[WHITE+PAWNS];
    uint64_t knights = pieces[WHITE+KNIGHTS];
    uint64_t bishops = pieces[WHITE+BISHOPS];
    uint64_t rooks = pieces[WHITE+ROOKS];
    uint64_t queens = pieces[WHITE+QUEENS];
    uint64_t kings = pieces[WHITE+KINGS];

    while(pawns != 0) {
        uint64_t single = pawns & -pawns;
        pawns &= pawns-1;
        int stsq = bitScanForward(single);

        uint64_t legal = getWPawnCaptures(single) & blackPieces;
        uint64_t promotions = legal & RANKS[7];

        if(promotions != 0) {
            int endsq = bitScanForward(promotions & -promotions);
            promotions &= promotions-1;

            Move *mk = new Move(PAWNS, true, stsq, endsq);
            mk->promotion = KNIGHTS;
            Move *mb = new Move(PAWNS, true, stsq, endsq);
            mk->promotion = BISHOPS;
            Move *mr = new Move(PAWNS, true, stsq, endsq);
            mk->promotion = ROOKS;
            Move *mq = new Move(PAWNS, true, stsq, endsq);
            mk->promotion = QUEENS;
            result.push_back(mk);
            result.push_back(mb);
            result.push_back(mr);
            result.push_back(mq);

            if(promotions != 0) {
                endsq = bitScanForward(promotions);
                Move *mk2 = new Move(PAWNS, true, stsq, endsq);
                mk2->promotion = KNIGHTS;
                Move *mb2 = new Move(PAWNS, true, stsq, endsq);
                mk2->promotion = BISHOPS;
                Move *mr2 = new Move(PAWNS, true, stsq, endsq);
                mk2->promotion = ROOKS;
                Move *mq2 = new Move(PAWNS, true, stsq, endsq);
                mk2->promotion = QUEENS;
                result.push_back(mk2);
                result.push_back(mb2);
                result.push_back(mr2);
                result.push_back(mq2);
            }
        }
        else {
            while(legal != 0) {
                int endsq = bitScanForward(legal & -legal);
                legal &= legal-1;

                result.push_back(new Move(PAWNS, true, stsq, endsq));
            }
        }
    }

    if(whiteCanEP != 0) {
        uint64_t taker = (whiteCanEP << 1) & NOTA & pieces[WHITE+PAWNS];
        if(taker != 0) {
            result.push_back(new Move(PAWNS, true, bitScanForward(taker), bitScanForward(whiteCanEP << 8)));
        }
        else {
            taker = (whiteCanEP >> 1) & NOTH & pieces[WHITE+PAWNS];
            if(taker != 0) {
                result.push_back(new Move(PAWNS, true, bitScanForward(taker), bitScanForward(whiteCanEP << 8)));
            }
        }
    }

    while(knights != 0) {
        int stsq = bitScanForward(knights & -knights);
        knights &= knights-1;

        uint64_t legal = getKnightSquares(stsq) & blackPieces;
        while(legal != 0) {
            int endsq = bitScanForward(legal & -legal);
            legal &= legal-1;

            result.push_back(new Move(KNIGHTS, true, stsq, endsq));
        }
    }

    while(bishops != 0) {
        int stsq = bitScanForward(bishops & -bishops);
        bishops &= bishops-1;

        uint64_t legal = getBishopSquares(stsq) & blackPieces;
        while(legal != 0) {
            int endsq = bitScanForward(legal & -legal);
            legal &= legal-1;

            result.push_back(new Move(BISHOPS, true, stsq, endsq));
        }
    }

    while(rooks != 0) {
        int stsq = bitScanForward(rooks & -rooks);
        rooks &= rooks-1;

        uint64_t legal = getRookSquares(stsq) & blackPieces;
        while(legal != 0) {
            int endsq = bitScanForward(legal & -legal);
            legal &= legal-1;

            result.push_back(new Move(ROOKS, true, stsq, endsq));
        }
    }

    while(queens != 0) {
        int stsq = bitScanForward(queens & -queens);
        queens &= queens-1;

        uint64_t legal = getQueenSquares(stsq) & blackPieces;
        while(legal != 0) {
            int endsq = bitScanForward(legal & -legal);
            legal &= legal-1;

            result.push_back(new Move(QUEENS, true, stsq, endsq));
        }
    }

    int stsq = bitScanForward(kings);
    uint64_t legal = getKingSquares(stsq) & blackPieces;
    while(legal != 0) {
        int endsq = bitScanForward(legal & -legal);
        legal &= legal-1;

        result.push_back(new Move(KINGS, true, stsq, endsq));
    }

    return result;
}

vector<Move *> Board::getPLBCaptures() {
    vector<Move *> result;
    uint64_t pawns = pieces[BLACK+PAWNS];
    uint64_t knights = pieces[BLACK+KNIGHTS];
    uint64_t bishops = pieces[BLACK+BISHOPS];
    uint64_t rooks = pieces[BLACK+ROOKS];
    uint64_t queens = pieces[BLACK+QUEENS];
    uint64_t kings = pieces[BLACK+KINGS];

    while(pawns != 0) {
        uint64_t single = pawns & -pawns;
        pawns &= pawns-1;
        int stsq = bitScanForward(single);

        uint64_t legal = getBPawnCaptures(single) & whitePieces;
        uint64_t promotions = legal & RANKS[0];
        if(promotions != 0) {
            int endsq = bitScanForward(promotions & -promotions);
            promotions &= promotions-1;

            Move *mk = new Move(PAWNS, true, stsq, endsq);
            mk->promotion = KNIGHTS;
            Move *mb = new Move(PAWNS, true, stsq, endsq);
            mk->promotion = BISHOPS;
            Move *mr = new Move(PAWNS, true, stsq, endsq);
            mk->promotion = ROOKS;
            Move *mq = new Move(PAWNS, true, stsq, endsq);
            mk->promotion = QUEENS;
            result.push_back(mk);
            result.push_back(mb);
            result.push_back(mr);
            result.push_back(mq);

            if(promotions != 0) {
                endsq = bitScanForward(promotions);
                Move *mk2 = new Move(PAWNS, true, stsq, endsq);
                mk2->promotion = KNIGHTS;
                Move *mb2 = new Move(PAWNS, true, stsq, endsq);
                mk2->promotion = BISHOPS;
                Move *mr2 = new Move(PAWNS, true, stsq, endsq);
                mk2->promotion = ROOKS;
                Move *mq2 = new Move(PAWNS, true, stsq, endsq);
                mk2->promotion = QUEENS;
                result.push_back(mk2);
                result.push_back(mb2);
                result.push_back(mr2);
                result.push_back(mq2);
            }
        }
        else {
            while(legal != 0) {
                int endsq = bitScanForward(legal & -legal);
                legal &= legal-1;

                result.push_back(new Move(PAWNS, true, stsq, endsq));
            }
        }
    }

    if(blackCanEP != 0) {
        uint64_t taker = (blackCanEP << 1) & NOTA & pieces[BLACK+PAWNS];
        if(taker != 0) {
            result.push_back(new Move(PAWNS, true, bitScanForward(taker), bitScanForward(blackCanEP >> 8)));
        }
        else {
            taker = (blackCanEP >> 1) & NOTH & pieces[BLACK+PAWNS];
            if(taker != 0) {
                result.push_back(new Move(PAWNS, true, bitScanForward(taker), bitScanForward(blackCanEP >> 8)));
            }
        }
    }

    while(knights != 0) {
        int stsq = bitScanForward(knights & -knights);
        knights &= knights-1;

        uint64_t legal = getKnightSquares(stsq) & whitePieces;
        while(legal != 0) {
            int endsq = bitScanForward(legal & -legal);
            legal &= legal-1;

            result.push_back(new Move(KNIGHTS, true, stsq, endsq));
        }
    }

    while(bishops != 0) {
        int stsq = bitScanForward(bishops & -bishops);
        bishops &= bishops-1;

        uint64_t legal = getBishopSquares(stsq) & whitePieces;
        while(legal != 0) {
            int endsq = bitScanForward(legal & -legal);
            legal &= legal-1;

            result.push_back(new Move(BISHOPS, true, stsq, endsq));
        }
    }

    while(rooks != 0) {
        int stsq = bitScanForward(rooks & -rooks);
        rooks &= rooks-1;

        uint64_t legal = getRookSquares(stsq) & whitePieces;
        while(legal != 0) {
            int endsq = bitScanForward(legal & -legal);
            legal &= legal-1;

            result.push_back(new Move(ROOKS, true, stsq, endsq));
        }
    }

    while(queens != 0) {
        int stsq = bitScanForward(queens & -queens);
        queens &= queens-1;

        uint64_t legal = getQueenSquares(stsq) & whitePieces;
        while(legal != 0) {
            int endsq = bitScanForward(legal & -legal);
            legal &= legal-1;

            result.push_back(new Move(QUEENS, true, stsq, endsq));
        }
    }

    int stsq = bitScanForward(kings);
    uint64_t legal = getKingSquares(stsq) & whitePieces;
    while(legal != 0) {
        int endsq = bitScanForward(legal & -legal);
        legal &= legal-1;

        result.push_back(new Move(KINGS, true, stsq, endsq));
    }

    return result;
}

vector<Move *> Board::getPLCaptures(int color) {
    return (color == WHITE) ? getPLWCaptures() : getPLBCaptures();
}



// ---------------------King: check, checkmate, stalemate----------------------
bool Board::getWinCheck() {
    uint64_t bAtt = 0;
    bAtt |= getBPawnCaptures(pieces[BLACK+PAWNS]) | getBKnightSquares(pieces[BLACK+KNIGHTS]) |
    getBBishopMoves(pieces[BLACK+BISHOPS]) | getBRookMoves(pieces[BLACK+ROOKS]) |
    getBQueenMoves(pieces[BLACK+QUEENS]) | getKingAttacks(BLACK);
    if((bAtt & pieces[WHITE+KINGS]) != 0)
        return true;
    else return false;
}

bool Board::getBinCheck() {
    uint64_t wAtt = 0;
    wAtt |= getWPawnCaptures(pieces[WHITE+PAWNS]) | getWKnightSquares(pieces[WHITE+KNIGHTS]) |
    getWBishopMoves(pieces[WHITE+BISHOPS]) | getWRookMoves(pieces[WHITE+ROOKS]) |
    getWQueenMoves(pieces[WHITE+QUEENS]) | getKingAttacks(WHITE);
    if((wAtt & pieces[BLACK+KINGS]) != 0)
        return true;
    else return false;
}

bool Board::isWinMate() {
    vector<Move *> temp = getLegalWMoves();
    if(temp.size() == 0 && getWinCheck())
        return true;
    else return false;
}

bool Board::isBinMate() {
    vector<Move *> temp = getLegalBMoves();
    if(temp.size() == 0 && getBinCheck())
        return true;
    else return false;
}

bool Board::isStalemate(int sideToMove) {
    vector<Move *> temp = (sideToMove == WHITE) ? getLegalWMoves() : getLegalBMoves();

    if(temp.size() == 0 && !((sideToMove == WHITE) ? getBinCheck() : getWinCheck()))
        return true;
    else return false;
}

// TODO evaluation function
/*
 * Evaluates the current board position in hundredths of pawns. White is
 * positive and black is negative in traditional negamax format.
 */
int Board::evaluate() {
    // special cases
    if(isWinMate())
        return 99999;
    else if(isBinMate())
        return -99999;
    //else if(isStalemate())
    //    return 0;

    int value = 0;

    // material
    value += 100*count(pieces[WHITE+PAWNS]) + 310*count(pieces[WHITE+KNIGHTS]) +
    320*count(pieces[WHITE+BISHOPS]) + 500*count(pieces[WHITE+ROOKS]) + 900*count(pieces[WHITE+QUEENS]);
    value -= 100*count(pieces[BLACK+PAWNS]) + 310*count(pieces[BLACK+KNIGHTS]) +
    320*count(pieces[BLACK+BISHOPS]) + 500*count(pieces[BLACK+ROOKS]) + 900*count(pieces[BLACK+QUEENS]);

    // piece square tables
    for(int i = 0; i < 64; i++) {
        switch(mailbox[i]) {
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
                break;
            case 9: // black king
                break;
            default:
                cout << "Error in piece table switch statement." << endl;
                break;
        }
    }

    // king safety
    int wfile = bitScanForward(pieces[WHITE+KINGS]) % 8;
    int bfile = bitScanForward(pieces[BLACK+KINGS]) % 8;
    if(wfile == 4 || wfile == 5)
        value -= 20;
    if(bfile == 4 || bfile == 5)
        value += 20;

    uint64_t wksq = getKingAttacks(WHITE);
    uint64_t bksq = getKingAttacks(BLACK);
    uint64_t bAtt = 0;
    bAtt = getBPawnCaptures(pieces[BLACK+PAWNS]) | getBKnightSquares(pieces[BLACK+KNIGHTS]) |
    getBBishopMoves(pieces[BLACK+BISHOPS]) | getBRookMoves(pieces[BLACK+ROOKS]) |
    getBQueenMoves(pieces[BLACK+QUEENS]) | getKingAttacks(BLACK);
    uint64_t wAtt = 0;
    wAtt = getWPawnCaptures(pieces[WHITE+PAWNS]) | getWKnightSquares(pieces[WHITE+KNIGHTS]) |
    getWBishopMoves(pieces[WHITE+BISHOPS]) | getWRookMoves(pieces[WHITE+ROOKS]) |
    getWQueenMoves(pieces[WHITE+QUEENS]) | getKingAttacks(WHITE);

    value -= 25 * count(wksq & bAtt);
    value += 25 * count(bksq & wAtt);

    uint64_t wpawnShield = (wksq & pieces[WHITE+KINGS]) << 8;
    uint64_t bpawnShield = (bksq & pieces[BLACK+KINGS]) >> 8;
    value += 30 * count(wpawnShield & pieces[WHITE+PAWNS]);
    value -= 30 * count(bpawnShield & pieces[BLACK+PAWNS]);

    // mobility
    //value += 10 * getWPseudoMobility();
    //value -= 10 * getBPseudoMobility();
    return value;
}

bool Board::pieceOn(int color, int x, int y) {
    if(color == WHITE)
        return (whitePieces & MOVEMASK[x + 8*y]) != 0;
    else
        return (blackPieces & MOVEMASK[x + 8*y]) != 0;
}

// Faster estimates of mobility (number of legal moves)
int Board::getWPseudoMobility() {
    int result = 0;
    uint64_t pawns = pieces[WHITE+PAWNS];
    uint64_t knights = pieces[WHITE+KNIGHTS];
    uint64_t bishops = pieces[WHITE+BISHOPS];
    uint64_t rooks = pieces[WHITE+ROOKS];
    uint64_t queens = pieces[WHITE+QUEENS];

    while(pawns != 0) {
        uint64_t single = pawns & -pawns;
        pawns &= pawns-1;

        uint64_t legal = getWPawnCaptures(single) & blackPieces;
        legal |= getWPawnMoves(single);

        result += count(legal);
    }

    while(knights != 0) {
        uint64_t single = knights & -knights;
        knights &= knights-1;

        uint64_t legal = getWKnightSquares(single) & (blackPieces | ~(whitePieces | blackPieces));

        result += count(legal);
    }

    while(bishops != 0) {
        uint64_t single = bishops & -bishops;
        bishops &= bishops-1;

        uint64_t legal = getWBishopMoves(single) & (blackPieces | ~(whitePieces | blackPieces));

        result += count(legal);
    }

    while(rooks != 0) {
        uint64_t single = rooks & -rooks;
        rooks &= rooks-1;

        uint64_t legal = getWRookMoves(single) & (blackPieces | ~(whitePieces | blackPieces));

        result += count(legal);
    }

    while(queens != 0) {
        uint64_t single = queens & -queens;
        queens &= queens-1;

        uint64_t legal = getWQueenMoves(single) & (blackPieces | ~(whitePieces | blackPieces));

        result += count(legal);
    }

    uint64_t legal = getKingAttacks(WHITE) & (blackPieces | ~(whitePieces | blackPieces));
    result += count(legal);

    if(whiteCanKCastle)
        result++;
    if(whiteCanQCastle)
        result++;

    return result;
}

int Board::getBPseudoMobility() {
    int result = 0;
    uint64_t pawns = pieces[BLACK+PAWNS];
    uint64_t knights = pieces[BLACK+KNIGHTS];
    uint64_t bishops = pieces[BLACK+BISHOPS];
    uint64_t rooks = pieces[BLACK+ROOKS];
    uint64_t queens = pieces[BLACK+QUEENS];

    while(pawns != 0) {
        uint64_t single = pawns & -pawns;
        pawns &= pawns-1;

        uint64_t legal = getBPawnCaptures(single) & whitePieces;
        legal |= getBPawnMoves(single);

        result += count(legal);
    }

    while(knights != 0) {
        uint64_t single = knights & -knights;
        knights &= knights-1;

        uint64_t legal = getBKnightSquares(single) & (whitePieces | ~(whitePieces | blackPieces));

        result += count(legal);
    }

    while(bishops != 0) {
        uint64_t single = bishops & -bishops;
        bishops &= bishops-1;

        uint64_t legal = getBBishopMoves(single) & (whitePieces | ~(whitePieces | blackPieces));

        result += count(legal);
    }

    while(rooks != 0) {
        uint64_t single = rooks & -rooks;
        rooks &= rooks-1;

        uint64_t legal = getBRookMoves(single) & (whitePieces | ~(whitePieces | blackPieces));

        result += count(legal);
    }

    while(queens != 0) {
        uint64_t single = queens & -queens;
        queens &= queens-1;

        uint64_t legal = getBQueenMoves(single) & (whitePieces | ~(whitePieces | blackPieces));

        result += count(legal);
    }

    uint64_t legal = getKingAttacks(BLACK) & (whitePieces | ~(whitePieces | blackPieces));
    result += count(legal);

    if(blackCanKCastle)
        result++;
    if(blackCanQCastle)
        result++;

    return result;
}

// Returns a list of pieces on the board. For rendering purposes.
/*
public ArrayList<Piece> getPieces(PieceImage pi) {
ArrayList<Piece> result = new ArrayList<Piece>();

uint64_t pawns = pieces[WHITE+PAWNS];
uint64_t knights = pieces[WHITE+KNIGHTS];
uint64_t bishops = pieces[WHITE+BISHOPS];
uint64_t rooks = pieces[WHITE+ROOKS];
uint64_t queens = pieces[WHITE+QUEENS];
uint64_t kings = pieces[WHITE+KINGS];

while(pawns != 0) {
uint64_t single = pawns & -pawns;
pawns &= pawns-1;
int index = bitScanForward(single);
result.push_back(new Piece(index%8, index/8, WHITE*PAWNS, pi));
}
while(knights != 0) {
uint64_t single = knights & -knights;
knights &= knights-1;
int index = bitScanForward(single);
result.push_back(new Piece(index%8, index/8, WHITE*KNIGHTS, pi));
}
while(bishops != 0) {
int index = bitScanForward(bishops);
bishops &= bishops-1;
result.push_back(new Piece(index%8, index/8, WHITE*BISHOPS, pi));
}
while(rooks != 0) {
int index = bitScanForward(rooks);
rooks &= rooks-1;
result.push_back(new Piece(index%8, index/8, WHITE*ROOKS, pi));
}
while(queens != 0) {
int index = bitScanForward(queens);
queens &= queens-1;
result.push_back(new Piece(index%8, index/8, WHITE*QUEENS, pi));
}
while(kings != 0) {
int index = bitScanForward(kings);
kings &= kings-1;
result.push_back(new Piece(index%8, index/8, WHITE*KINGS, pi));
}

pawns = pieces[BLACK+PAWNS];
knights = pieces[BLACK+KNIGHTS];
bishops = pieces[BLACK+BISHOPS];
rooks = pieces[BLACK+ROOKS];
queens = pieces[BLACK+QUEENS];
kings = pieces[BLACK+KINGS];

while(pawns != 0) {
uint64_t single = pawns & -pawns;
pawns &= pawns-1;
int index = bitScanForward(single);
result.push_back(new Piece(index%8, index/8, BLACK*PAWNS, pi));
}
while(knights != 0) {
uint64_t single = knights & -knights;
knights &= knights-1;
int index = bitScanForward(single);
result.push_back(new Piece(index%8, index/8, BLACK*KNIGHTS, pi));
}
while(bishops != 0) {
int index = bitScanForward(bishops);
bishops &= bishops-1;
result.push_back(new Piece(index%8, index/8, BLACK*BISHOPS, pi));
}
while(rooks != 0) {
int index = bitScanForward(rooks);
rooks &= rooks-1;
result.push_back(new Piece(index%8, index/8, BLACK*ROOKS, pi));
}
while(queens != 0) {
int index = bitScanForward(queens);
queens &= queens-1;
result.push_back(new Piece(index%8, index/8, BLACK*QUEENS, pi));
}
while(kings != 0) {
int index = bitScanForward(kings);
kings &= kings-1;
result.push_back(new Piece(index%8, index/8, BLACK*KINGS, pi));
}

return result;
}
*/

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

uint64_t Board::getKnightSquares(int single) {
    return KNIGHTMOVES[single];
}

uint64_t Board::getWKnightSquares(uint64_t knights) {
    uint64_t wn = knights;
    uint64_t l1 = (wn >> 1) & 0x7f7f7f7f7f7f7f7f;
    uint64_t l2 = (wn >> 2) & 0x3f3f3f3f3f3f3f3f;
    uint64_t r1 = (wn << 1) & 0xfefefefefefefefe;
    uint64_t r2 = (wn << 2) & 0xfcfcfcfcfcfcfcfc;
    uint64_t h1 = l1 | r1;
    uint64_t h2 = l2 | r2;
    return (h1<<16) | (h1>>16) | (h2<<8) | (h2>>8);
}

uint64_t Board::getBKnightSquares(uint64_t knights) {
    uint64_t bn = knights;
    uint64_t l1 = (bn >> 1) & 0x7f7f7f7f7f7f7f7f;
    uint64_t l2 = (bn >> 2) & 0x3f3f3f3f3f3f3f3f;
    uint64_t r1 = (bn << 1) & 0xfefefefefefefefe;
    uint64_t r2 = (bn << 2) & 0xfcfcfcfcfcfcfcfc;
    uint64_t h1 = l1 | r1;
    uint64_t h2 = l2 | r2;
    return (h1<<16) | (h1>>16) | (h2<<8) | (h2>>8);
}

uint64_t Board::getBishopSquares(int single) {
    uint64_t nwatt = NWRAY[single];
    uint64_t blocker = nwatt & (whitePieces | blackPieces);
    int nws = bitScanForward(blocker | 0x8000000000000000);
    nwatt ^= NWRAY[nws];

    uint64_t neatt = NERAY[single];
    blocker = neatt & (whitePieces | blackPieces);
    int nes = bitScanForward(blocker | 0x8000000000000000);
    neatt ^= NERAY[nes];

    uint64_t swatt = SWRAY[single];
    blocker = swatt & (whitePieces | blackPieces);
    int sws = bitScanReverse(blocker | 1);
    swatt ^= SWRAY[sws];

    uint64_t seatt = SERAY[single];
    blocker = seatt & (whitePieces | blackPieces);
    int ses = bitScanReverse(blocker | 1);
    seatt ^= SERAY[ses];

    return nwatt | neatt | swatt | seatt;
}

uint64_t Board::getWBishopMoves(uint64_t bishops) {
    uint64_t open = ~(whitePieces | blackPieces);
    uint64_t result = neAttacks(bishops, open);
    result |= seAttacks(bishops, open);
    result |= nwAttacks(bishops, open);
    result |= swAttacks(bishops, open);
    return result;
}

uint64_t Board::getBBishopMoves(uint64_t bishops) {
    uint64_t open = ~(whitePieces | blackPieces);
    uint64_t result = neAttacks(bishops, open);
    result |= seAttacks(bishops, open);
    result |= nwAttacks(bishops, open);
    result |= swAttacks(bishops, open);
    return result;
}

uint64_t Board::getRookSquares(int single) {
    uint64_t satt = SOUTHRAY[single];
    uint64_t blocker = satt & (whitePieces | blackPieces);
    int ss = bitScanReverse(blocker | 1);
    satt ^= SOUTHRAY[ss];

    uint64_t watt = WESTRAY[single];
    blocker = watt & (whitePieces | blackPieces);
    int ws = bitScanReverse(blocker | 1);
    watt ^= WESTRAY[ws];

    uint64_t natt = NORTHRAY[single];
    blocker = natt & (whitePieces | blackPieces);
    int ns = bitScanForward(blocker | 0x8000000000000000);
    natt ^= NORTHRAY[ns];

    uint64_t eatt = EASTRAY[single];
    blocker = eatt & (whitePieces | blackPieces);
    int es = bitScanForward(blocker | 0x8000000000000000);
    eatt ^= EASTRAY[es];

    return satt | watt | natt | eatt;
}

uint64_t Board::getWRookMoves(uint64_t rooks) {
    uint64_t open = ~(whitePieces | blackPieces);
    uint64_t result = southAttacks(rooks, open);
    result |= northAttacks(rooks, open);
    result |= eastAttacks(rooks, open);
    result |= westAttacks(rooks, open);
    return result;
}

uint64_t Board::getBRookMoves(uint64_t rooks) {
    uint64_t open = ~(whitePieces | blackPieces);
    uint64_t result = southAttacks(rooks, open);
    result |= northAttacks(rooks, open);
    result |= eastAttacks(rooks, open);
    result |= westAttacks(rooks, open);
    return result;
}

uint64_t Board::getQueenSquares(int single) {
    uint64_t occ = whitePieces | blackPieces;

    uint64_t nwatt = NWRAY[single];
    uint64_t blocker = nwatt & occ;
    int nws = bitScanForward(blocker | 0x8000000000000000);
    nwatt ^= NWRAY[nws];

    uint64_t neatt = NERAY[single];
    blocker = neatt & occ;
    int nes = bitScanForward(blocker | 0x8000000000000000);
    neatt ^= NERAY[nes];

    uint64_t swatt = SWRAY[single];
    blocker = swatt & occ;
    int sws = bitScanReverse(blocker | 1);
    swatt ^= SWRAY[sws];

    uint64_t seatt = SERAY[single];
    blocker = seatt & occ;
    int ses = bitScanReverse(blocker | 1);
    seatt ^= SERAY[ses];

    uint64_t satt = SOUTHRAY[single];
    blocker = satt & occ;
    int ss = bitScanReverse(blocker | 1);
    satt ^= SOUTHRAY[ss];

    uint64_t watt = WESTRAY[single];
    blocker = watt & occ;
    int ws = bitScanReverse(blocker | 1);
    watt ^= WESTRAY[ws];

    uint64_t natt = NORTHRAY[single];
    blocker = natt & occ;
    int ns = bitScanForward(blocker | 0x8000000000000000);
    natt ^= NORTHRAY[ns];

    uint64_t eatt = EASTRAY[single];
    blocker = eatt & occ;
    int es = bitScanForward(blocker | 0x8000000000000000);
    eatt ^= EASTRAY[es];

    return nwatt | neatt | swatt | seatt | satt | watt | natt | eatt;
}

uint64_t Board::getWQueenMoves(uint64_t queens) {
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

uint64_t Board::getBQueenMoves(uint64_t queens) {
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

// BSF and BSR algorithms from https://chessprogramming.wikispaces.com/BitScan
int Board::bitScanForward(uint64_t bb) {
    uint64_t debruijn64 = 0x03f79d71b4cb0a89;
    int i = (int)(((bb ^ (bb-1)) * debruijn64) >> 58);
    return index64[i];
}

int Board::bitScanReverse(uint64_t bb) {
    uint64_t debruijn64 = 0x03f79d71b4cb0a89;
    bb |= bb >> 1; 
    bb |= bb >> 2;
    bb |= bb >> 4;
    bb |= bb >> 8;
    bb |= bb >> 16;
    bb |= bb >> 32;
    return index64[(int) ((bb * debruijn64) >> 58)];
}

int Board::count(uint64_t bb) {
/*int n = 0;
while(bb != 0) {
bb &= bb - 1;
n++;
}
return n;*/
    bb = bb - ((bb >> 1) & 0x5555555555555555);
    bb = (bb & 0x3333333333333333) + ((bb >> 2) & 0x3333333333333333);
    bb = (((bb + (bb >> 4)) & 0x0F0F0F0F0F0F0F0F) * 0x0101010101010101) >> 56;
    return (int) bb;
}

/*
public String toString() {
String result = "";
for(int i = 0; i < 64; i++) {
if(mailbox[i] != -1)
result += (100*i + mailbox[i]) + ",";
}
return result;
}
*/

// TODO not updated or tested
/*
public boolean equals(Object o) {
    if(!(o instanceof Board))
        return false;
    Board other = (Board)o;
    if(mailbox.equals(other.mailbox) && whiteCanKCastle == other.whiteCanKCastle
    && whiteCanQCastle == other.whiteCanQCastle && blackCanKCastle == other.blackCanKCastle
    && blackCanQCastle == other.blackCanQCastle && whiteCanEP == other.whiteCanEP
    && blackCanEP == other.blackCanEP)
        return true;
    else return false;
}
*/

// Not tested
/*
public int hashCode() {
    int result = 221; //2166136261
    for(int i = 0; i < 64; i++) {
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
