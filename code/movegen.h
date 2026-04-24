#ifndef MOVEGEN_H
#define MOVEGEN_H

#include "board.h"
#include <vector>

std::vector<Move> generatePseudoLegalMoves(const Board &board);
std::vector<Move> generateLegalMoves(Board &board);

void generatePawnMoves(const Board &board, int r, int c, std::vector<Move> &moves);
void generateKnightMoves(const Board &board, int r, int c, std::vector<Move> &moves);
void generateBishopMoves(const Board &board, int r, int c, std::vector<Move> &moves);
void generateRookMoves(const Board &board, int r, int c, std::vector<Move> &moves);
void generateQueenMoves(const Board &board, int r, int c, std::vector<Move> &moves);
void generateKingMoves(const Board &board, int r, int c, std::vector<Move> &moves);

bool isSquareAttacked(const Board &board, int row, int col, int attackerSide);
bool isKingInCheck(const Board &board, int side);

UndoInfo makeMove(Board &board, const Move &move);
void undoMove(Board &board, const UndoInfo &info);

#endif
