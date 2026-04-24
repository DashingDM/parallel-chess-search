#include "movegen.h"
#include <cstdlib>

static void addMove(const Board &board, int fr, int fc, int tr, int tc, std::vector<Move> &moves,
                    int promotionPiece = EMPTY) {
    Move m;
    m.fromRow = fr;
    m.fromCol = fc;
    m.toRow = tr;
    m.toCol = tc;
    m.movedPiece = board.squares[fr][fc];
    m.capturedPiece = board.squares[tr][tc];
    m.promotionPiece = promotionPiece;
    moves.push_back(m);
}

std::vector<Move> generatePseudoLegalMoves(const Board &board) {
    std::vector<Move> moves;

    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            int piece = board.squares[r][c];
            if (!isSameColor(piece, board.sideToMove)) continue;

            switch (std::abs(piece)) {
                case 1: generatePawnMoves(board, r, c, moves); break;
                case 2: generateKnightMoves(board, r, c, moves); break;
                case 3: generateBishopMoves(board, r, c, moves); break;
                case 4: generateRookMoves(board, r, c, moves); break;
                case 5: generateQueenMoves(board, r, c, moves); break;
                case 6: generateKingMoves(board, r, c, moves); break;
            }
        }
    }

    return moves;
}

std::vector<Move> generateLegalMoves(Board &board) {
    std::vector<Move> pseudo = generatePseudoLegalMoves(board);
    std::vector<Move> legal;

    int side = board.sideToMove;

    for (const Move &m : pseudo) {
        UndoInfo info = makeMove(board, m);
        if (!isKingInCheck(board, side)) {
            legal.push_back(m);
        }
        undoMove(board, info);
    }

    return legal;
}

void generatePawnMoves(const Board &board, int r, int c, std::vector<Move> &moves) {
    int piece = board.squares[r][c];
    int dir = (piece > 0) ? -1 : 1;
    int startRow = (piece > 0) ? 6 : 1;
    int promotionRow = (piece > 0) ? 0 : 7;

    auto addPromotionMoves = [&](int fromRow, int fromCol, int toRow, int toCol) {
        if (piece > 0) {
            addMove(board, fromRow, fromCol, toRow, toCol, moves, WQ);
            addMove(board, fromRow, fromCol, toRow, toCol, moves, WR);
            addMove(board, fromRow, fromCol, toRow, toCol, moves, WB);
            addMove(board, fromRow, fromCol, toRow, toCol, moves, WN);
        } else {
            addMove(board, fromRow, fromCol, toRow, toCol, moves, BQ);
            addMove(board, fromRow, fromCol, toRow, toCol, moves, BR);
            addMove(board, fromRow, fromCol, toRow, toCol, moves, BB);
            addMove(board, fromRow, fromCol, toRow, toCol, moves, BN);
        }
    };

    int oneStep = r + dir;
    if (isInside(oneStep, c) && board.squares[oneStep][c] == EMPTY) {
        if (oneStep == promotionRow) {
            addPromotionMoves(r, c, oneStep, c);
        } else {
            addMove(board, r, c, oneStep, c, moves);
        }

        int twoStep = r + 2 * dir;
        if (r == startRow && isInside(twoStep, c) && board.squares[twoStep][c] == EMPTY) {
            addMove(board, r, c, twoStep, c, moves);
        }
    }

    for (int dc = -1; dc <= 1; dc += 2) {
        int nr = r + dir;
        int nc = c + dc;
        if (!isInside(nr, nc)) continue;

        int target = board.squares[nr][nc];
        if (target != EMPTY && !isSameColor(target, board.sideToMove)) {
            if (nr == promotionRow) {
                addPromotionMoves(r, c, nr, nc);
            } else {
                addMove(board, r, c, nr, nc, moves);
            }
        }
    }
}

void generateKnightMoves(const Board &board, int r, int c, std::vector<Move> &moves) {
    const int dr[8] = {-2, -2, -1, -1, 1, 1, 2, 2};
    const int dc[8] = {-1, 1, -2, 2, -2, 2, -1, 1};

    for (int i = 0; i < 8; i++) {
        int nr = r + dr[i];
        int nc = c + dc[i];
        if (!isInside(nr, nc)) continue;

        int target = board.squares[nr][nc];
        if (target == EMPTY || !isSameColor(target, board.sideToMove)) {
            addMove(board, r, c, nr, nc, moves);
        }
    }
}

void generateBishopMoves(const Board &board, int r, int c, std::vector<Move> &moves) {
    const int dr[4] = {-1, -1, 1, 1};
    const int dc[4] = {-1, 1, -1, 1};

    for (int d = 0; d < 4; d++) {
        int nr = r + dr[d];
        int nc = c + dc[d];
        while (isInside(nr, nc)) {
            int target = board.squares[nr][nc];
            if (target == EMPTY) {
                addMove(board, r, c, nr, nc, moves);
            } else {
                if (!isSameColor(target, board.sideToMove)) {
                    addMove(board, r, c, nr, nc, moves);
                }
                break;
            }
            nr += dr[d];
            nc += dc[d];
        }
    }
}

void generateRookMoves(const Board &board, int r, int c, std::vector<Move> &moves) {
    const int dr[4] = {-1, 1, 0, 0};
    const int dc[4] = {0, 0, -1, 1};

    for (int d = 0; d < 4; d++) {
        int nr = r + dr[d];
        int nc = c + dc[d];
        while (isInside(nr, nc)) {
            int target = board.squares[nr][nc];
            if (target == EMPTY) {
                addMove(board, r, c, nr, nc, moves);
            } else {
                if (!isSameColor(target, board.sideToMove)) {
                    addMove(board, r, c, nr, nc, moves);
                }
                break;
            }
            nr += dr[d];
            nc += dc[d];
        }
    }
}

void generateQueenMoves(const Board &board, int r, int c, std::vector<Move> &moves) {
    generateBishopMoves(board, r, c, moves);
    generateRookMoves(board, r, c, moves);
}

void generateKingMoves(const Board &board, int r, int c, std::vector<Move> &moves) {
    for (int dr = -1; dr <= 1; dr++) {
        for (int dc = -1; dc <= 1; dc++) {
            if (dr == 0 && dc == 0) continue;
            int nr = r + dr;
            int nc = c + dc;
            if (!isInside(nr, nc)) continue;

            int target = board.squares[nr][nc];
            if (target == EMPTY || !isSameColor(target, board.sideToMove)) {
                addMove(board, r, c, nr, nc, moves);
            }
        }
    }
}

bool isSquareAttacked(const Board &board, int row, int col, int attackerSide) {
    const int pawnDir = (attackerSide == WHITE) ? -1 : 1;
    const int pawnRow = row - pawnDir;
    for (int dc = -1; dc <= 1; dc += 2) {
        int pawnCol = col + dc;
        if (!isInside(pawnRow, pawnCol)) continue;
        if (board.squares[pawnRow][pawnCol] == (attackerSide == WHITE ? WP : BP)) {
            return true;
        }
    }

    const int knightDr[8] = {-2, -2, -1, -1, 1, 1, 2, 2};
    const int knightDc[8] = {-1, 1, -2, 2, -2, 2, -1, 1};
    for (int i = 0; i < 8; ++i) {
        int nr = row + knightDr[i];
        int nc = col + knightDc[i];
        if (!isInside(nr, nc)) continue;
        if (board.squares[nr][nc] == (attackerSide == WHITE ? WN : BN)) {
            return true;
        }
    }

    const int bishopDr[4] = {-1, -1, 1, 1};
    const int bishopDc[4] = {-1, 1, -1, 1};
    for (int d = 0; d < 4; ++d) {
        int nr = row + bishopDr[d];
        int nc = col + bishopDc[d];
        while (isInside(nr, nc)) {
            int piece = board.squares[nr][nc];
            if (piece != EMPTY) {
                if (piece == (attackerSide == WHITE ? WB : BB) ||
                    piece == (attackerSide == WHITE ? WQ : BQ)) {
                    return true;
                }
                break;
            }
            nr += bishopDr[d];
            nc += bishopDc[d];
        }
    }

    const int rookDr[4] = {-1, 1, 0, 0};
    const int rookDc[4] = {0, 0, -1, 1};
    for (int d = 0; d < 4; ++d) {
        int nr = row + rookDr[d];
        int nc = col + rookDc[d];
        while (isInside(nr, nc)) {
            int piece = board.squares[nr][nc];
            if (piece != EMPTY) {
                if (piece == (attackerSide == WHITE ? WR : BR) ||
                    piece == (attackerSide == WHITE ? WQ : BQ)) {
                    return true;
                }
                break;
            }
            nr += rookDr[d];
            nc += rookDc[d];
        }
    }

    for (int dr = -1; dr <= 1; ++dr) {
        for (int dc = -1; dc <= 1; ++dc) {
            if (dr == 0 && dc == 0) continue;
            int nr = row + dr;
            int nc = col + dc;
            if (!isInside(nr, nc)) continue;
            if (board.squares[nr][nc] == (attackerSide == WHITE ? WK : BK)) {
                return true;
            }
        }
    }

    return false;
}

bool isKingInCheck(const Board &board, int side) {
    int kingPiece = (side == WHITE) ? WK : BK;

    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            if (board.squares[r][c] == kingPiece) {
                return isSquareAttacked(board, r, c, -side);
            }
        }
    }

    return false;
}

UndoInfo makeMove(Board &board, const Move &move) {
    UndoInfo info;
    info.move = move;
    info.previousSideToMove = board.sideToMove;

    board.squares[move.toRow][move.toCol] = board.squares[move.fromRow][move.fromCol];
    board.squares[move.fromRow][move.fromCol] = EMPTY;
    if (move.promotionPiece != EMPTY) {
        board.squares[move.toRow][move.toCol] = move.promotionPiece;
    }
    board.sideToMove = -board.sideToMove;

    return info;
}

void undoMove(Board &board, const UndoInfo &info) {
    const Move &move = info.move;
    board.squares[move.fromRow][move.fromCol] = move.movedPiece;
    board.squares[move.toRow][move.toCol] = move.capturedPiece;
    board.sideToMove = info.previousSideToMove;
}
