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
            bool _whiteCanQCastle, bool _blackCanQCastle,
            uint16_t _epCaptureFile, int _fiftyMoveCounter, int _moveNumber,
            int _playerToMove);
    ~Board();
    Board staticCopy();

    void doMove(Move m, int color);
    bool doPseudoLegalMove(Move m, int color);
    bool doHashMove(Move m, int color);
    void doNullMove();
    void undoNullMove(uint16_t _epCaptureFile);

    PieceMoveList getPieceMoveList(int color);
    MoveList getAllLegalMoves(int color);
    void getAllPseudoLegalMoves(MoveList &legalMoves, int color);
    void getPseudoLegalQuiets(MoveList &quiets, int color);
    void getPseudoLegalCaptures(MoveList &captures, int color, bool includePromotions);
    void getPseudoLegalPromotions(MoveList &moves, int color);
    void getPseudoLegalChecks(MoveList &checks, int color);
    void getPseudoLegalCheckEscapes(MoveList &escapes, int color);

    // Get a bitboard of all xray-ers attacking a square if a blocker has been moved or removed
    uint64_t getXRayPieceMap(int color, int sq, uint64_t blockerStart, uint64_t blockerEnd);
    // Get a bitboard of all xray-ers attacking a square with no blocker removed
    //uint64_t getInitXRays(int color, int sq);
    // Get all pieces of that color attacking the square
    uint64_t getAttackMap(int color, int sq);
    uint64_t getAttackMap(int sq);
    int getPieceOnSquare(int color, int sq);
    bool isCheckMove(int color, Move m);
    uint64_t getRookXRays(int sq, uint64_t occ, uint64_t blockers);
    uint64_t getBishopXRays(int sq, uint64_t occ, uint64_t blockers);
    uint64_t getPinnedMap(int color);

    bool isInCheck(int color);
    bool isDraw();
    bool isInsufficientMaterial();
    void getCheckMaps(int color, uint64_t *checkMaps);

    int getMaterial(int color);
    // Useful for turning off some pruning late endgame
    uint64_t getNonPawnMaterial(int color);
    // Static exchange evaluation code: for checking material trades on a single square
    uint64_t getLeastValuableAttacker(uint64_t attackers, int color, int &piece);
    int getSEE(int color, int sq);
    int getSEEForMove(int color, Move m);
    int valueOfPiece(int piece);
    // Most Valuable Victim / Least Valuable Attacker
    int getMVVLVAScore(int color, Move m);
    int getExchangeScore(int color, Move m);

    // Public move generators
    uint64_t getWPawnCaptures(uint64_t pawns);
    uint64_t getBPawnCaptures(uint64_t pawns);
    uint64_t getKingSquares(int single);

    // Getter methods
    bool getWhiteCanKCastle();
    bool getBlackCanKCastle();
    bool getWhiteCanQCastle();
    bool getBlackCanQCastle();
    bool getAnyCanCastle();
    uint16_t getEPCaptureFile();
    uint8_t getFiftyMoveCounter();
    uint8_t getCastlingRights();
    uint16_t getMoveNumber();
    int getPlayerToMove();
    uint64_t getPieces(int color, int piece);
    uint64_t getAllPieces(int color);
    int *getMailbox();
    uint64_t getZobristKey();

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

    void addPawnMovesToList(MoveList &quiets, int color);
    void addPawnCapturesToList(MoveList &captures, int color, uint64_t otherPieces, bool includePromotions);
    template <bool isCapture>
    void addPieceMovesToList(MoveList &moves, int color, uint64_t otherPieces = 0);
    template <bool isCapture>
    void addMovesToList(MoveList &moves, int stSq, uint64_t allEndSqs, uint64_t otherPieces = 0);
    template <bool isCapture>
    void addPromotionsToList(MoveList &moves, int stSq, int endSq);
    void addCastlesToList(MoveList &moves, int color);

    // Move generation
    // Takes into account blocking for sliders, but otherwise leaves
    // the occupancy of the end square up to the move generation function
    // This is necessary for captures
    uint64_t getWPawnSingleMoves(uint64_t pawns);
    uint64_t getBPawnSingleMoves(uint64_t pawns);
    uint64_t getWPawnDoubleMoves(uint64_t pawns);
    uint64_t getBPawnDoubleMoves(uint64_t pawns);
    uint64_t getWPawnLeftCaptures(uint64_t pawns);
    uint64_t getBPawnLeftCaptures(uint64_t pawns);
    uint64_t getWPawnRightCaptures(uint64_t pawns);
    uint64_t getBPawnRightCaptures(uint64_t pawns);
    uint64_t getOccupancy();
    int epVictimSquare(int victimColor, uint16_t file);

    // Kindergarten bitboard methods
    uint64_t rankAttacks(uint64_t occ, int single);
    uint64_t fileAttacks(uint64_t occ, int single);
    uint64_t diagAttacks(uint64_t occ, int single);
    uint64_t antiDiagAttacks(uint64_t occ, int single);
};

struct Magic {
    uint64_t magic;
    uint64_t mask;
    uint64_t shift;
    uint64_t *offset;
};

void initAttacks();

uint64_t knightAttacks(int sq);
uint64_t bishopAttacks(int sq, uint64_t occupied);
uint64_t rookAttacks(int sq, uint64_t occupied);
uint64_t queenAttacks(int sq, uint64_t occupied);
uint64_t kingAttacks(int sq);

static const uint64_t RookMagics[64] = {
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

static const uint64_t BishopMagics[64] = {
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

#endif
