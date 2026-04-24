#ifndef PARALLEL_H
#define PARALLEL_H

#include "search.h"

SearchResult findBestMoveParallel(Board &board, int depth, int numThreads);

#endif
