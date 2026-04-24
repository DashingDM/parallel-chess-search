#include "fen.h"

#include <cstring>
#include <sstream>

namespace {

int pieceFromChar(char piece) {
    switch (piece) {
        case 'P': return WP;
        case 'N': return WN;
        case 'B': return WB;
        case 'R': return WR;
        case 'Q': return WQ;
        case 'K': return WK;
        case 'p': return BP;
        case 'n': return BN;
        case 'b': return BB;
        case 'r': return BR;
        case 'q': return BQ;
        case 'k': return BK;
        default: return EMPTY;
    }
}

} // namespace

bool loadFEN(Board &board, const std::string &fen, std::string &errorMessage) {
    std::memset(board.squares, 0, sizeof(board.squares));
    board.sideToMove = WHITE;

    std::istringstream input(fen);
    std::string placement;
    std::string activeColor;
    if (!(input >> placement >> activeColor)) {
        errorMessage = "FEN must include piece placement and active color.";
        return false;
    }

    int row = 0;
    int col = 0;
    for (char token : placement) {
        if (token == '/') {
            if (col != 8) {
                errorMessage = "Each FEN rank must contain exactly 8 squares.";
                return false;
            }
            ++row;
            col = 0;
            continue;
        }

        if (row >= 8) {
            errorMessage = "FEN contains more than 8 ranks.";
            return false;
        }

        if (token >= '1' && token <= '8') {
            col += token - '0';
            if (col > 8) {
                errorMessage = "FEN rank overflows past 8 squares.";
                return false;
            }
            continue;
        }

        int piece = pieceFromChar(token);
        if (piece == EMPTY) {
            errorMessage = "FEN contains an unknown piece character.";
            return false;
        }
        if (col >= 8) {
            errorMessage = "FEN rank overflows past 8 squares.";
            return false;
        }
        board.squares[row][col] = piece;
        ++col;
    }

    if (row != 7 || col != 8) {
        errorMessage = "FEN must describe exactly 8 complete ranks.";
        return false;
    }

    if (activeColor == "w") {
        board.sideToMove = WHITE;
    } else if (activeColor == "b") {
        board.sideToMove = BLACK;
    } else {
        errorMessage = "Active color must be 'w' or 'b'.";
        return false;
    }

    return true;
}

std::string boardToFEN(const Board &board) {
    std::ostringstream output;
    for (int row = 0; row < 8; ++row) {
        int emptySquares = 0;
        for (int col = 0; col < 8; ++col) {
            int piece = board.squares[row][col];
            if (piece == EMPTY) {
                ++emptySquares;
                continue;
            }

            if (emptySquares > 0) {
                output << emptySquares;
                emptySquares = 0;
            }
            output << pieceToChar(piece);
        }

        if (emptySquares > 0) {
            output << emptySquares;
        }

        if (row != 7) {
            output << '/';
        }
    }

    output << ' ' << (board.sideToMove == WHITE ? 'w' : 'b');
    return output.str();
}
