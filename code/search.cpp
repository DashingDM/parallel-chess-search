#include "search.h"
#include "movegen.h"
#include "evaluate.h"
#include "utils.h"
#include <algorithm>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace {

constexpr int CHECKMATE_SCORE = 1000000;
constexpr std::size_t MAX_TT_ENTRIES = 200000;

enum class BoundType {
    Exact,
    Lower,
    Upper
};

struct TranspositionEntry {
    int depth;
    int score;
    BoundType bound;
};

thread_local std::unordered_map<std::uint64_t, TranspositionEntry> transpositionTable;

bool isMaximizingSide(int sideToMove) {
    return sideToMove == WHITE;
}

int scoreTerminalPosition(Board &board, int depth) {
    if (!isKingInCheck(board, board.sideToMove)) {
        return 0;
    }

    return isMaximizingSide(board.sideToMove) ? -CHECKMATE_SCORE - depth
                                              : CHECKMATE_SCORE + depth;
}

int movePriority(const Move &move) {
    int score = 0;
    if (move.capturedPiece != EMPTY) {
        score += 10 * getPieceValue(move.capturedPiece) - getPieceValue(move.movedPiece);
    }
    if (move.promotionPiece != EMPTY) {
        score += getPieceValue(move.promotionPiece);
    }
    return score;
}

void orderMoves(std::vector<Move> &moves) {
    std::sort(moves.begin(), moves.end(), [](const Move &lhs, const Move &rhs) {
        return movePriority(lhs) > movePriority(rhs);
    });
}

int pieceIndex(int piece) {
    return piece + 6;
}

std::uint64_t hashBoard(const Board &board) {
    std::uint64_t hash = 1469598103934665603ull;
    constexpr std::uint64_t prime = 1099511628211ull;

    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {
            std::uint64_t value = static_cast<std::uint64_t>(pieceIndex(board.squares[row][col]) + 1);
            value ^= static_cast<std::uint64_t>((row * 8 + col) + 17) << 8;
            hash ^= value;
            hash *= prime;
        }
    }

    hash ^= static_cast<std::uint64_t>(board.sideToMove == WHITE ? 1 : 2);
    hash *= prime;
    return hash;
}

} // namespace

void clearSearchCache() {
    transpositionTable.clear();
}

int minimax(Board &board, int depth, bool maximizingPlayer, long long &nodes) {
    nodes++;

    if (depth == 0) {
        return evaluateBoard(board);
    }

    std::vector<Move> moves = generateLegalMoves(board);
    if (moves.empty()) {
        return scoreTerminalPosition(board, depth);
    }
    orderMoves(moves);

    if (maximizingPlayer) {
        int bestScore = -INF;
        for (const Move &m : moves) {
            UndoInfo info = makeMove(board, m);
            int score = minimax(board, depth - 1, false, nodes);
            undoMove(board, info);
            if (score > bestScore) bestScore = score;
        }
        return bestScore;
    } else {
        int bestScore = INF;
        for (const Move &m : moves) {
            UndoInfo info = makeMove(board, m);
            int score = minimax(board, depth - 1, true, nodes);
            undoMove(board, info);
            if (score < bestScore) bestScore = score;
        }
        return bestScore;
    }
}

int alphaBeta(Board &board, int depth, int alpha, int beta, bool maximizingPlayer, long long &nodes) {
    nodes++;

    const int originalAlpha = alpha;
    const int originalBeta = beta;
    const std::uint64_t boardHash = hashBoard(board);
    auto cached = transpositionTable.find(boardHash);
    if (cached != transpositionTable.end() && cached->second.depth >= depth) {
        const TranspositionEntry &entry = cached->second;
        if (entry.bound == BoundType::Exact) {
            return entry.score;
        }
        if (entry.bound == BoundType::Lower) {
            alpha = std::max(alpha, entry.score);
        } else {
            beta = std::min(beta, entry.score);
        }
        if (alpha >= beta) {
            return entry.score;
        }
    }

    if (depth == 0) {
        return evaluateBoard(board);
    }

    std::vector<Move> moves = generateLegalMoves(board);
    if (moves.empty()) {
        return scoreTerminalPosition(board, depth);
    }
    orderMoves(moves);

    if (maximizingPlayer) {
        int bestScore = -INF;
        for (const Move &m : moves) {
            UndoInfo info = makeMove(board, m);
            int score = alphaBeta(board, depth - 1, alpha, beta, false, nodes);
            undoMove(board, info);

            if (score > bestScore) bestScore = score;
            if (score > alpha) alpha = score;
            if (beta <= alpha) break;
        }
        BoundType bound = BoundType::Exact;
        if (bestScore <= originalAlpha) {
            bound = BoundType::Upper;
        } else if (bestScore >= originalBeta) {
            bound = BoundType::Lower;
        }
        if (transpositionTable.size() >= MAX_TT_ENTRIES) {
            transpositionTable.clear();
        }
        transpositionTable[boardHash] = {depth, bestScore, bound};
        return bestScore;
    } else {
        int bestScore = INF;
        for (const Move &m : moves) {
            UndoInfo info = makeMove(board, m);
            int score = alphaBeta(board, depth - 1, alpha, beta, true, nodes);
            undoMove(board, info);

            if (score < bestScore) bestScore = score;
            if (score < beta) beta = score;
            if (beta <= alpha) break;
        }
        BoundType bound = BoundType::Exact;
        if (bestScore <= originalAlpha) {
            bound = BoundType::Upper;
        } else if (bestScore >= originalBeta) {
            bound = BoundType::Lower;
        }
        if (transpositionTable.size() >= MAX_TT_ENTRIES) {
            transpositionTable.clear();
        }
        transpositionTable[boardHash] = {depth, bestScore, bound};
        return bestScore;
    }
}

SearchResult findBestMoveMinimax(Board &board, int depth) {
    SearchResult result{};
    const int rootSide = board.sideToMove;
    result.bestScore = (rootSide == WHITE) ? -INF : INF;
    result.nodesSearched = 0;

    std::vector<Move> moves = generateLegalMoves(board);
    if (moves.empty()) {
        result.bestScore = scoreTerminalPosition(board, depth);
        return result;
    }
    orderMoves(moves);

    for (const Move &m : moves) {
        UndoInfo info = makeMove(board, m);
        long long localNodes = 0;
        int score = minimax(board, depth - 1, isMaximizingSide(board.sideToMove), localNodes);
        undoMove(board, info);

        result.nodesSearched += localNodes;

        if ((rootSide == WHITE && score > result.bestScore) ||
            (rootSide == BLACK && score < result.bestScore)) {
            result.bestScore = score;
            result.bestMove = m;
        }
    }

    return result;
}

SearchResult findBestMoveAlphaBeta(Board &board, int depth) {
    SearchResult result{};
    const int rootSide = board.sideToMove;
    result.bestScore = (rootSide == WHITE) ? -INF : INF;
    result.nodesSearched = 0;
    int alpha = -INF;
    int beta = INF;
    clearSearchCache();

    std::vector<Move> moves = generateLegalMoves(board);
    if (moves.empty()) {
        result.bestScore = scoreTerminalPosition(board, depth);
        return result;
    }
    orderMoves(moves);

    for (const Move &m : moves) {
        UndoInfo info = makeMove(board, m);
        long long localNodes = 0;
        int score = alphaBeta(board, depth - 1, alpha, beta, isMaximizingSide(board.sideToMove), localNodes);
        undoMove(board, info);

        result.nodesSearched += localNodes;

        if ((rootSide == WHITE && score > result.bestScore) ||
            (rootSide == BLACK && score < result.bestScore)) {
            result.bestScore = score;
            result.bestMove = m;
        }

        if (rootSide == WHITE) {
            alpha = std::max(alpha, result.bestScore);
        } else {
            beta = std::min(beta, result.bestScore);
        }
    }

    return result;
}
