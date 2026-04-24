#ifndef SEARCH_H
#define SEARCH_H

#include "board.h"

struct SearchResult {
    Move bestMove;
    int bestScore;
    long long nodesSearched;
};

int minimax(Board &board, int depth, bool maximizingPlayer, long long &nodes);
int alphaBeta(Board &board, int depth, int alpha, int beta, bool maximizingPlayer, long long &nodes);
void clearSearchCache();

SearchResult findBestMoveMinimax(Board &board, int depth);
SearchResult findBestMoveAlphaBeta(Board &board, int depth);

#endif
