#ifndef FEN_H
#define FEN_H

#include "board.h"

#include <string>

bool loadFEN(Board &board, const std::string &fen, std::string &errorMessage);
std::string boardToFEN(const Board &board);

#endif
