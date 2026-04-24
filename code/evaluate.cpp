#include "evaluate.h"
#include <cstdlib>

int getPieceValue(int piece) {
    switch (std::abs(piece)) {
        case 1: return 100;
        case 2: return 320;
        case 3: return 330;
        case 4: return 500;
        case 5: return 900;
        case 6: return 20000;
        default: return 0;
    }
}

int evaluateBoard(const Board &board) {
    int score = 0;

    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            int piece = board.squares[r][c];
            if (piece > 0) score += getPieceValue(piece);
            else if (piece < 0) score -= getPieceValue(piece);
        }
    }

    return score;
}
