#include "board.h"
#include <cstring>
#include <iostream>

void initializeBoard(Board &board) {
    int setup[8][8] = {
        {BR, BN, BB, BQ, BK, BB, BN, BR},
        {BP, BP, BP, BP, BP, BP, BP, BP},
        {EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY},
        {EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY},
        {EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY},
        {EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY},
        {WP, WP, WP, WP, WP, WP, WP, WP},
        {WR, WN, WB, WQ, WK, WB, WN, WR}
    };

    std::memcpy(board.squares, setup, sizeof(setup));
    board.sideToMove = WHITE;
}

char pieceToChar(int piece) {
    switch (piece) {
        case WP: return 'P';
        case WN: return 'N';
        case WB: return 'B';
        case WR: return 'R';
        case WQ: return 'Q';
        case WK: return 'K';
        case BP: return 'p';
        case BN: return 'n';
        case BB: return 'b';
        case BR: return 'r';
        case BQ: return 'q';
        case BK: return 'k';
        default: return '.';
    }
}

void printBoard(const Board &board) {
    std::cout << "\n  0 1 2 3 4 5 6 7\n";
    for (int r = 0; r < 8; r++) {
        std::cout << r << " ";
        for (int c = 0; c < 8; c++) {
            std::cout << pieceToChar(board.squares[r][c]) << " ";
        }
        std::cout << "\n";
    }
    std::cout << "Side to move: " << (board.sideToMove == WHITE ? "WHITE" : "BLACK") << "\n";
}

bool isInside(int r, int c) {
    return r >= 0 && r < 8 && c >= 0 && c < 8;
}

bool isWhitePiece(int piece) {
    return piece > 0;
}

bool isBlackPiece(int piece) {
    return piece < 0;
}

bool isSameColor(int piece, int side) {
    if (piece == EMPTY) return false;
    return (side == WHITE && isWhitePiece(piece)) || (side == BLACK && isBlackPiece(piece));
}

Board copyBoard(const Board &board) {
    return board;
}
