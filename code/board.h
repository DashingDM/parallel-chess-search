#ifndef BOARD_H
#define BOARD_H

#include <vector>
#include <string>

enum Piece {
    EMPTY = 0,

    WP = 1, WN = 2, WB = 3, WR = 4, WQ = 5, WK = 6,
    BP = -1, BN = -2, BB = -3, BR = -4, BQ = -5, BK = -6
};

enum Color {
    WHITE = 1,
    BLACK = -1
};

struct Move {
    int fromRow, fromCol;
    int toRow, toCol;
    int movedPiece;
    int capturedPiece;
    int promotionPiece;
};

struct UndoInfo {
    Move move;
    int previousSideToMove;
};

struct Board {
    int squares[8][8];
    int sideToMove;
};

void initializeBoard(Board &board);
void printBoard(const Board &board);
bool isInside(int r, int c);
bool isWhitePiece(int piece);
bool isBlackPiece(int piece);
bool isSameColor(int piece, int side);
Board copyBoard(const Board &board);
char pieceToChar(int piece);

#endif
