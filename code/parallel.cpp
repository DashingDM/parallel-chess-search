#include "parallel.h"
#include "movegen.h"
#include "evaluate.h"
#include "search.h"
#include "utils.h"
#include <algorithm>
#include <atomic>
#include <vector>

#ifdef _OPENMP
#include <omp.h>
#endif

namespace {

constexpr int CHECKMATE_SCORE = 1000000;

int scoreTerminalPosition(Board &board, int depth) {
    if (!isKingInCheck(board, board.sideToMove)) {
        return 0;
    }

    return board.sideToMove == WHITE ? -CHECKMATE_SCORE - depth
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

void updateAtomicMax(std::atomic<int> &target, int value) {
    int current = target.load(std::memory_order_relaxed);
    while (current < value &&
           !target.compare_exchange_weak(current, value, std::memory_order_relaxed)) {
    }
}

void updateAtomicMin(std::atomic<int> &target, int value) {
    int current = target.load(std::memory_order_relaxed);
    while (current > value &&
           !target.compare_exchange_weak(current, value, std::memory_order_relaxed)) {
    }
}

} // namespace

SearchResult findBestMoveParallel(Board &board, int depth, int numThreads) {
    SearchResult result{};
    std::vector<Move> moves = generateLegalMoves(board);

    if (moves.empty()) {
        result.bestScore = scoreTerminalPosition(board, depth);
        result.nodesSearched = 0;
        return result;
    }
    orderMoves(moves);

    std::vector<int> scores(moves.size(), 0);
    std::vector<long long> nodeCounts(moves.size(), 0);

    int rootSide = board.sideToMove;
    int alpha = -INF;
    int beta = INF;

    clearSearchCache();
    Board firstBoard = copyBoard(board);
    makeMove(firstBoard, moves[0]);
    int firstScore = alphaBeta(firstBoard, depth - 1, alpha, beta, firstBoard.sideToMove == WHITE, nodeCounts[0]);
    scores[0] = firstScore;

    if (rootSide == WHITE) {
        alpha = firstScore;
    } else {
        beta = firstScore;
    }

    std::atomic<int> sharedAlpha(alpha);
    std::atomic<int> sharedBeta(beta);

    #ifdef _OPENMP
    #pragma omp parallel for num_threads(numThreads)
    #endif
    for (int i = 1; i < (int)moves.size(); i++) {
        clearSearchCache();
        Board localBoard = copyBoard(board);
        long long localNodes = 0;
        int localAlpha = sharedAlpha.load(std::memory_order_relaxed);
        int localBeta = sharedBeta.load(std::memory_order_relaxed);

        makeMove(localBoard, moves[i]);
        int score = alphaBeta(localBoard, depth - 1, localAlpha, localBeta,
                              localBoard.sideToMove == WHITE, localNodes);

        scores[i] = score;
        nodeCounts[i] = localNodes;

        if (rootSide == WHITE) {
            updateAtomicMax(sharedAlpha, score);
        } else {
            updateAtomicMin(sharedBeta, score);
        }
    }

    result.bestMove = moves[0];
    result.bestScore = scores[0];
    result.nodesSearched = 0;

    for (int i = 0; i < (int)moves.size(); i++) {
        result.nodesSearched += nodeCounts[i];

        if ((rootSide == WHITE && scores[i] > result.bestScore) ||
            (rootSide == BLACK && scores[i] < result.bestScore)) {
            result.bestScore = scores[i];
            result.bestMove = moves[i];
        }
    }

    return result;
}
