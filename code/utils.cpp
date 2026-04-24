#include "utils.h"
#include <sstream>

std::string moveToString(const Move &move) {
    std::ostringstream out;
    out << "(" << move.fromRow << "," << move.fromCol << ") -> ("
        << move.toRow << "," << move.toCol << ")";
    if (move.promotionPiece != EMPTY) {
        out << " = " << pieceToChar(move.promotionPiece);
    }
    return out.str();
}
