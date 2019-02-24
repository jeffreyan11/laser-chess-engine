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

#ifndef __BOARD_H__
#define __BOARD_H__

#include "common.h"


constexpr uint8_t WHITEKSIDE = 0x1;
constexpr uint8_t WHITEQSIDE = 0x2;
constexpr uint8_t BLACKKSIDE = 0x4;
constexpr uint8_t BLACKQSIDE = 0x8;
constexpr uint8_t WHITECASTLE = 0x3;
constexpr uint8_t BLACKCASTLE = 0xC;

constexpr uint16_t NO_EP_POSSIBLE = 0x8;

constexpr bool MOVEGEN_CAPTURES = true;
constexpr bool MOVEGEN_QUIETS = false;


struct PieceMoveInfo {
    int pieceID;
    int startSq;
    uint64_t legal;

    PieceMoveInfo() {}
    PieceMoveInfo(int _pieceID, int _startSq, uint64_t _legal) {
        pieceID = _pieceID;
        startSq = _startSq;
        legal = _legal;
    }
};

struct PieceMoveList {
    PieceMoveInfo arrayList[32];
    unsigned int length;
    unsigned int starts[5];

    PieceMoveList() {
        length = 0;
    }
    ~PieceMoveList() {}

    unsigned int size() { return length; }

    void add(PieceMoveInfo o) {
        arrayList[length] = o;
        length++;
    }

    PieceMoveInfo get(int i) { return arrayList[i]; }
};

void initZobristTable();


/**
 * @brief A chess board and its associated functionality, including get legal
 *        moves and evaluation.
 */
class Board {
public:
    Board();
    Board(int *mailboxBoard, bool _whiteCanKCastle, bool _blackCanKCastle,
          bool _whiteCanQCastle, bool _blackCanQCastle, uint16_t _epCaptureFile,
          int _fiftyMoveCounter, int _moveNumber, int _playerToMove);
    ~Board();
    Board staticCopy() const;

    void doMove(Move m, int color);
    bool doPseudoLegalMove(Move m, int color);
    bool doHashMove(Move m, int color);
    void doNullMove();
    void undoNullMove(uint16_t _epCaptureFile);

    PieceMoveList getPieceMoveList(int color) const;
    MoveList getAllLegalMoves(int color) const;
    void getAllPseudoLegalMoves(MoveList &legalMoves, int color) const;
    void getPseudoLegalQuiets(MoveList &quiets, int color) const;
    void getPseudoLegalCaptures(MoveList &captures, int color, bool includePromotions) const;
    void getPseudoLegalPromotions(MoveList &moves, int color) const;
    void getPseudoLegalChecks(MoveList &checks, int color) const;
    void getPseudoLegalCheckEscapes(MoveList &escapes, int color) const;

    // Get a bitboard of all xray-ers attacking a square if a blocker has been moved or removed
    uint64_t getXRayPieceMap(int color, int sq, uint64_t occ) const;
    // Get all pieces of that color attacking the square
    uint64_t getAttackMap(int color, int sq) const;
    uint64_t getAttackMap(int sq, uint64_t occ) const;
    int getPieceOnSquare(int color, int sq) const;
    bool isCheckMove(int color, Move m) const;
    uint64_t getRookXRays(int sq, uint64_t occ, uint64_t blockers) const;
    uint64_t getBishopXRays(int sq, uint64_t occ, uint64_t blockers) const;
    uint64_t getPinnedMap(int color) const;

    bool isInCheck(int color) const;
    bool isDraw() const;
    bool isInsufficientMaterial() const;
    void getCheckMaps(int color, uint64_t *checkMaps) const;

    // Useful for turning off some pruning late endgame
    uint64_t getNonPawnMaterial(int color) const;
    // Static exchange evaluation code: for checking material trades on a single square
    uint64_t getLeastValuableAttacker(uint64_t attackers, int color, int &piece) const;
    bool isSEEAbove(int color, Move m, int cutoff) const;
    int valueOfPiece(int piece) const;
    // Most Valuable Victim / Least Valuable Attacker
    int getMVVLVAScore(int color, Move m) const;

    // Public move generators
    uint64_t getWPawnCaptures(uint64_t pawns) const;
    uint64_t getBPawnCaptures(uint64_t pawns) const;
    uint64_t getKnightSquares(int single) const;
    uint64_t getBishopSquares(int single, uint64_t occ) const;
    uint64_t getKingSquares(int single) const;

    // Getter methods
    bool getWhiteCanKCastle() const;
    bool getBlackCanKCastle() const;
    bool getWhiteCanQCastle() const;
    bool getBlackCanQCastle() const;
    bool getAnyCanCastle() const;
    uint16_t getEPCaptureFile() const;
    uint8_t getFiftyMoveCounter() const;
    uint8_t getCastlingRights() const;
    uint16_t getMoveNumber() const;
    int getPlayerToMove() const;
    uint64_t getPieces(int color, int piece) const;
    uint64_t getAllPieces(int color) const;
    int getKingSq(int color) const;
    int *getMailbox() const;
    uint64_t getZobristKey() const;

    void initZobristKey(int *mailbox);

private:
    // Bitboards for all white or all black pieces
    uint64_t allPieces[2];
    // 12 bitboards, one for each of the 12 piece types, indexed by the
    // constants given in common.h
    uint64_t pieces[2][6];
    // Zobrist key for hash table use
    uint64_t zobristKey;
    // 8 if cannot en passant, if en passant is possible, the file of the
    // pawn being captured is stored here (0-7)
    uint16_t epCaptureFile;
    // Whose move is it?
    uint16_t playerToMove;
    // Move number
    uint16_t moveNumber;
    // Booleans indicating whether castling is possible
    // Bit 0: white kingside
    // Bit 1: white queenside
    // Bit 2: black kingside
    // Bit 3: black queenside
    uint8_t castlingRights;
    // Counts half moves for the 50-move rule
    uint8_t fiftyMoveCounter;

    // Precomputed tables
    int kingSqs[2];

    void addPawnMovesToList(MoveList &quiets, int color) const;
    void addPawnCapturesToList(MoveList &captures, int color, uint64_t otherPieces, bool includePromotions) const;
    template <bool isCapture>
    void addPieceMovesToList(MoveList &moves, int color, uint64_t otherPieces = 0) const;
    template <bool isCapture>
    void addMovesToList(MoveList &moves, int stSq, uint64_t allEndSqs, uint64_t otherPieces = 0) const;
    template <bool isCapture>
    void addPromotionsToList(MoveList &moves, int stSq, int endSq) const;
    void addCastlesToList(MoveList &moves, int color) const;

    // Move generation
    // Takes into account blocking for sliders, but otherwise leaves
    // the occupancy of the end square up to the move generation function
    // This is necessary for captures
    uint64_t getWPawnSingleMoves(uint64_t pawns) const;
    uint64_t getBPawnSingleMoves(uint64_t pawns) const;
    uint64_t getWPawnDoubleMoves(uint64_t pawns) const;
    uint64_t getBPawnDoubleMoves(uint64_t pawns) const;
    uint64_t getWPawnLeftCaptures(uint64_t pawns) const;
    uint64_t getBPawnLeftCaptures(uint64_t pawns) const;
    uint64_t getWPawnRightCaptures(uint64_t pawns) const;
    uint64_t getBPawnRightCaptures(uint64_t pawns) const;
    uint64_t getRookSquares(int single, uint64_t occ) const;
    uint64_t getQueenSquares(int single, uint64_t occ) const;
    uint64_t getOccupancy() const;
    int epVictimSquare(int victimColor, uint16_t file) const;
};

#endif
