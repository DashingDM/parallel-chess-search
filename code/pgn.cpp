#include "pgn.h"

#include "board.h"
#include "fen.h"
#include "movegen.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace {

struct PgnState {
    bool whiteKingSideCastle = true;
    bool whiteQueenSideCastle = true;
    bool blackKingSideCastle = true;
    bool blackQueenSideCastle = true;
    int enPassantRow = -1;
    int enPassantCol = -1;
};

std::string trim(const std::string &value) {
    std::size_t start = 0;
    while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start]))) {
        ++start;
    }

    std::size_t end = value.size();
    while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1]))) {
        --end;
    }

    return value.substr(start, end - start);
}

std::string stripSanSuffixes(std::string token) {
    token = trim(token);
    while (!token.empty()) {
        char last = token.back();
        if (last == '+' || last == '#' || last == '!' || last == '?') {
            token.pop_back();
            continue;
        }
        break;
    }

    std::size_t ep = token.find("e.p.");
    if (ep != std::string::npos) {
        token.erase(ep, 4);
    }
    token.erase(std::remove_if(token.begin(), token.end(),
                               [](unsigned char c) { return std::isspace(c); }),
                token.end());
    return token;
}

bool isResultToken(const std::string &token) {
    return token == "1-0" || token == "0-1" || token == "1/2-1/2" || token == "*";
}

bool isMoveNumberToken(const std::string &token) {
    if (token.empty()) {
        return false;
    }
    std::size_t index = 0;
    while (index < token.size() && std::isdigit(static_cast<unsigned char>(token[index]))) {
        ++index;
    }
    if (index == 0) {
        return false;
    }
    while (index < token.size() && token[index] == '.') {
        ++index;
    }
    return index == token.size();
}

std::string stripLeadingMoveNumber(std::string token) {
    std::size_t index = 0;
    while (index < token.size() && std::isdigit(static_cast<unsigned char>(token[index]))) {
        ++index;
    }
    if (index == 0) {
        return token;
    }
    std::size_t dots = index;
    while (dots < token.size() && token[dots] == '.') {
        ++dots;
    }
    if (dots == index) {
        return token;
    }
    return token.substr(dots);
}

char fileFromCol(int col) {
    return static_cast<char>('a' + col);
}

char rankFromRow(int row) {
    return static_cast<char>('8' - row);
}

std::string squareToString(int row, int col) {
    std::string square;
    square.push_back(fileFromCol(col));
    square.push_back(rankFromRow(row));
    return square;
}

int pieceFromSanLetter(char letter, int side) {
    switch (letter) {
        case 'N': return side == WHITE ? WN : BN;
        case 'B': return side == WHITE ? WB : BB;
        case 'R': return side == WHITE ? WR : BR;
        case 'Q': return side == WHITE ? WQ : BQ;
        case 'K': return side == WHITE ? WK : BK;
        default: return EMPTY;
    }
}

std::string moveToSan(const Board &board, const Move &move, const std::vector<Move> &legalMoves) {
    std::string san;
    int piece = move.movedPiece;
    bool isPawn = std::abs(piece) == 1;
    bool isCapture = move.capturedPiece != EMPTY;

    if (!isPawn) {
        san.push_back(static_cast<char>(std::toupper(pieceToChar(piece))));

        bool sameFileConflict = false;
        bool sameRankConflict = false;
        bool needsDisambiguation = false;
        for (const Move &candidate : legalMoves) {
            if (candidate.fromRow == move.fromRow && candidate.fromCol == move.fromCol &&
                candidate.toRow == move.toRow && candidate.toCol == move.toCol &&
                candidate.promotionPiece == move.promotionPiece) {
                continue;
            }

            if (candidate.toRow == move.toRow &&
                candidate.toCol == move.toCol &&
                candidate.movedPiece == move.movedPiece) {
                needsDisambiguation = true;
                if (candidate.fromCol == move.fromCol) {
                    sameFileConflict = true;
                }
                if (candidate.fromRow == move.fromRow) {
                    sameRankConflict = true;
                }
            }
        }

        if (needsDisambiguation) {
            if (!sameFileConflict) {
                san.push_back(fileFromCol(move.fromCol));
            } else if (!sameRankConflict) {
                san.push_back(rankFromRow(move.fromRow));
            } else {
                san.push_back(fileFromCol(move.fromCol));
                san.push_back(rankFromRow(move.fromRow));
            }
        }
    } else if (isCapture) {
        san.push_back(fileFromCol(move.fromCol));
    }

    if (isCapture) {
        san.push_back('x');
    }

    san += squareToString(move.toRow, move.toCol);

    if (move.promotionPiece != EMPTY) {
        san.push_back('=');
        san.push_back(static_cast<char>(std::toupper(pieceToChar(move.promotionPiece))));
    }

    return san;
}

void updateCastleRightsForRookSquare(PgnState &state, int row, int col) {
    if (row == 7 && col == 0) {
        state.whiteQueenSideCastle = false;
    } else if (row == 7 && col == 7) {
        state.whiteKingSideCastle = false;
    } else if (row == 0 && col == 0) {
        state.blackQueenSideCastle = false;
    } else if (row == 0 && col == 7) {
        state.blackKingSideCastle = false;
    }
}

void updateStateAfterMove(PgnState &state, const Move &move) {
    state.enPassantRow = -1;
    state.enPassantCol = -1;

    if (move.movedPiece == WK) {
        state.whiteKingSideCastle = false;
        state.whiteQueenSideCastle = false;
    } else if (move.movedPiece == BK) {
        state.blackKingSideCastle = false;
        state.blackQueenSideCastle = false;
    } else if (move.movedPiece == WR || move.movedPiece == BR) {
        updateCastleRightsForRookSquare(state, move.fromRow, move.fromCol);
    }

    if (move.capturedPiece == WR || move.capturedPiece == BR) {
        updateCastleRightsForRookSquare(state, move.toRow, move.toCol);
    }

    if (std::abs(move.movedPiece) == 1 && std::abs(move.toRow - move.fromRow) == 2) {
        state.enPassantRow = (move.fromRow + move.toRow) / 2;
        state.enPassantCol = move.fromCol;
    }
}

bool applyCastling(Board &board, PgnState &state, const std::string &token, std::string &errorMessage) {
    std::string normalized = stripSanSuffixes(token);
    bool kingSide = normalized == "O-O" || normalized == "0-0";
    bool queenSide = normalized == "O-O-O" || normalized == "0-0-0";
    if (!kingSide && !queenSide) {
        return false;
    }

    int side = board.sideToMove;
    int row = side == WHITE ? 7 : 0;
    int kingPiece = side == WHITE ? WK : BK;
    int rookPiece = side == WHITE ? WR : BR;

    if (board.squares[row][4] != kingPiece) {
        errorMessage = "Castling failed because the king is not on its home square.";
        return true;
    }

    if (kingSide) {
        bool allowed = side == WHITE ? state.whiteKingSideCastle : state.blackKingSideCastle;
        if (!allowed || board.squares[row][7] != rookPiece ||
            board.squares[row][5] != EMPTY || board.squares[row][6] != EMPTY ||
            isSquareAttacked(board, row, 4, -side) ||
            isSquareAttacked(board, row, 5, -side) ||
            isSquareAttacked(board, row, 6, -side)) {
            errorMessage = "Kingside castling is not legal in the current position.";
            return true;
        }

        board.squares[row][6] = kingPiece;
        board.squares[row][5] = rookPiece;
        board.squares[row][4] = EMPTY;
        board.squares[row][7] = EMPTY;
    } else {
        bool allowed = side == WHITE ? state.whiteQueenSideCastle : state.blackQueenSideCastle;
        if (!allowed || board.squares[row][0] != rookPiece ||
            board.squares[row][1] != EMPTY || board.squares[row][2] != EMPTY || board.squares[row][3] != EMPTY ||
            isSquareAttacked(board, row, 4, -side) ||
            isSquareAttacked(board, row, 3, -side) ||
            isSquareAttacked(board, row, 2, -side)) {
            errorMessage = "Queenside castling is not legal in the current position.";
            return true;
        }

        board.squares[row][2] = kingPiece;
        board.squares[row][3] = rookPiece;
        board.squares[row][4] = EMPTY;
        board.squares[row][0] = EMPTY;
    }

    if (side == WHITE) {
        state.whiteKingSideCastle = false;
        state.whiteQueenSideCastle = false;
    } else {
        state.blackKingSideCastle = false;
        state.blackQueenSideCastle = false;
    }

    state.enPassantRow = -1;
    state.enPassantCol = -1;
    board.sideToMove = -board.sideToMove;
    return true;
}

bool applyEnPassant(Board &board, PgnState &state, const std::string &token, std::string &errorMessage) {
    std::string normalized = stripSanSuffixes(token);
    if (normalized.size() < 4 || normalized.find('x') == std::string::npos) {
        return false;
    }
    if (normalized[0] < 'a' || normalized[0] > 'h') {
        return false;
    }

    std::size_t destinationStart = normalized.find('x') + 1;
    if (destinationStart + 1 >= normalized.size()) {
        return false;
    }
    char file = normalized[destinationStart];
    char rank = normalized[destinationStart + 1];
    if (file < 'a' || file > 'h' || rank < '1' || rank > '8') {
        return false;
    }

    int toCol = file - 'a';
    int toRow = '8' - rank;
    if (toRow != state.enPassantRow || toCol != state.enPassantCol) {
        return false;
    }

    int side = board.sideToMove;
    int fromCol = normalized[0] - 'a';
    int fromRow = side == WHITE ? toRow + 1 : toRow - 1;
    int pawnPiece = side == WHITE ? WP : BP;
    if (!isInside(fromRow, fromCol) || board.squares[fromRow][fromCol] != pawnPiece) {
        errorMessage = "En passant move does not match a pawn in the current position.";
        return true;
    }

    int capturedRow = side == WHITE ? toRow + 1 : toRow - 1;
    int capturedPiece = side == WHITE ? BP : WP;
    if (!isInside(capturedRow, toCol) || board.squares[capturedRow][toCol] != capturedPiece) {
        errorMessage = "En passant target pawn was not found on the board.";
        return true;
    }

    board.squares[toRow][toCol] = board.squares[fromRow][fromCol];
    board.squares[fromRow][fromCol] = EMPTY;
    board.squares[capturedRow][toCol] = EMPTY;
    state.enPassantRow = -1;
    state.enPassantCol = -1;
    board.sideToMove = -board.sideToMove;
    return true;
}

bool applySanMove(Board &board, PgnState &state, const std::string &token, std::string &errorMessage) {
    if (applyCastling(board, state, token, errorMessage)) {
        return errorMessage.empty();
    }
    if (applyEnPassant(board, state, token, errorMessage)) {
        return errorMessage.empty();
    }

    std::string normalized = stripSanSuffixes(token);
    std::vector<Move> legalMoves = generateLegalMoves(board);
    for (const Move &move : legalMoves) {
        if (moveToSan(board, move, legalMoves) == normalized) {
            UndoInfo info = makeMove(board, move);
            (void)info;
            updateStateAfterMove(state, move);
            return true;
        }
    }

    errorMessage = "Could not match SAN move '" + token + "' in the current position.";
    return false;
}

void initializeStateFromFen(const std::string &fen, PgnState &state) {
    state = {};
    std::istringstream input(fen);
    std::string placement;
    std::string activeColor;
    std::string castling;
    input >> placement >> activeColor >> castling;
    if (!castling.empty()) {
        state.whiteKingSideCastle = castling.find('K') != std::string::npos;
        state.whiteQueenSideCastle = castling.find('Q') != std::string::npos;
        state.blackKingSideCastle = castling.find('k') != std::string::npos;
        state.blackQueenSideCastle = castling.find('q') != std::string::npos;
    }
}

std::vector<std::string> extractMoveTokens(const std::string &movetext) {
    std::vector<std::string> tokens;
    std::string current;
    int variationDepth = 0;
    bool inBraceComment = false;
    bool inSemicolonComment = false;

    auto flushToken = [&]() {
        if (!current.empty()) {
            tokens.push_back(current);
            current.clear();
        }
    };

    for (char ch : movetext) {
        if (inSemicolonComment) {
            if (ch == '\n') {
                inSemicolonComment = false;
            }
            continue;
        }
        if (inBraceComment) {
            if (ch == '}') {
                inBraceComment = false;
            }
            continue;
        }

        if (ch == '{') {
            flushToken();
            inBraceComment = true;
            continue;
        }
        if (ch == '(') {
            flushToken();
            ++variationDepth;
            continue;
        }
        if (ch == ')') {
            flushToken();
            if (variationDepth > 0) {
                --variationDepth;
            }
            continue;
        }
        if (variationDepth > 0) {
            continue;
        }
        if (ch == ';') {
            flushToken();
            inSemicolonComment = true;
            continue;
        }
        if (std::isspace(static_cast<unsigned char>(ch))) {
            flushToken();
            continue;
        }

        current.push_back(ch);
    }
    flushToken();
    return tokens;
}

bool replayGameToPositions(const std::vector<std::string> &moveTokens, const std::string &startFen,
                           const std::string &gameName, int everyNPly, bool includeFinalPosition,
                           std::vector<InputPosition> &positions, std::string &errorMessage) {
    Board board;
    PgnState state;
    std::string fenError;

    if (startFen.empty()) {
        initializeBoard(board);
        state = {};
    } else {
        if (!loadFEN(board, startFen, fenError)) {
            errorMessage = "Game '" + gameName + "' has an invalid FEN header: " + fenError;
            return false;
        }
        initializeStateFromFen(startFen, state);
    }

    int plyCount = 0;
    bool sampledFinalPosition = false;
    for (const std::string &rawToken : moveTokens) {
        std::string token = stripLeadingMoveNumber(trim(rawToken));
        if (token.empty() || isMoveNumberToken(token) || isResultToken(token) || token[0] == '$') {
            continue;
        }

        std::string moveError;
        if (!applySanMove(board, state, token, moveError)) {
            errorMessage = "Game '" + gameName + "' failed on move '" + token + "': " + moveError;
            return false;
        }

        ++plyCount;
        if (everyNPly > 0 && plyCount % everyNPly == 0) {
            positions.push_back({gameName + "_ply_" + std::to_string(plyCount), boardToFEN(board)});
            sampledFinalPosition = true;
        } else {
            sampledFinalPosition = false;
        }
    }

    if (includeFinalPosition && !sampledFinalPosition) {
        positions.push_back({gameName, boardToFEN(board)});
    }

    return true;
}

} // namespace

bool loadPgnAsFinalPositions(const std::string &path, std::vector<InputPosition> &positions,
                             std::string &errorMessage, int maxGames, int startGame) {
    if (startGame <= 0) {
        errorMessage = "PGN start game must be a positive integer.";
        return false;
    }

    std::ifstream input(path);
    if (!input) {
        errorMessage = "Could not open PGN file: " + path;
        return false;
    }

    std::vector<std::string> currentGameLines;
    std::string line;
    while (std::getline(input, line)) {
        currentGameLines.push_back(line);
    }

    std::vector<std::vector<std::string>> games;
    std::vector<std::string> currentGame;
    for (const std::string &gameLine : currentGameLines) {
        if (!currentGame.empty() && gameLine.rfind("[Event ", 0) == 0) {
            games.push_back(currentGame);
            currentGame.clear();
        }
        if (!gameLine.empty() || !currentGame.empty()) {
            currentGame.push_back(gameLine);
        }
    }
    if (!currentGame.empty()) {
        games.push_back(currentGame);
    }

    if (games.empty()) {
        errorMessage = "No PGN games found in file: " + path;
        return false;
    }

    int processedGames = 0;
    for (std::size_t gameIndex = 0; gameIndex < games.size(); ++gameIndex) {
        if (static_cast<int>(gameIndex) + 1 < startGame) {
            continue;
        }
        if (maxGames > 0 && processedGames >= maxGames) {
            break;
        }
        ++processedGames;
        const std::vector<std::string> &gameLines = games[gameIndex];
        std::string eventName = "game_" + std::to_string(gameIndex + 1);
        std::string startFen;
        std::ostringstream movetext;
        bool inTags = true;

        for (const std::string &rawLine : gameLines) {
            std::string trimmed = trim(rawLine);
            if (trimmed.rfind("[Event ", 0) == 0) {
                std::size_t firstQuote = trimmed.find('"');
                std::size_t lastQuote = trimmed.rfind('"');
                if (firstQuote != std::string::npos && lastQuote != std::string::npos && firstQuote != lastQuote) {
                    eventName = trimmed.substr(firstQuote + 1, lastQuote - firstQuote - 1);
                }
            } else if (trimmed.rfind("[FEN ", 0) == 0) {
                std::size_t firstQuote = trimmed.find('"');
                std::size_t lastQuote = trimmed.rfind('"');
                if (firstQuote != std::string::npos && lastQuote != std::string::npos && firstQuote != lastQuote) {
                    startFen = trimmed.substr(firstQuote + 1, lastQuote - firstQuote - 1);
                }
            } else if (trimmed.empty() && inTags) {
                inTags = false;
            } else if (!trimmed.empty() && !inTags) {
                movetext << trimmed << '\n';
            }
        }

        std::string replayError;
        if (!replayGameToPositions(extractMoveTokens(movetext.str()), startFen, eventName, 0, true,
                                   positions, replayError)) {
            errorMessage = replayError;
            return false;
        }
    }

    if (positions.empty()) {
        errorMessage = "No positions were produced from PGN file: " + path;
        return false;
    }

    return true;
}

bool loadPgnAsSampledPositions(const std::string &path, int everyNPly,
                               std::vector<InputPosition> &positions, std::string &errorMessage,
                               int maxGames, int startGame) {
    if (everyNPly <= 0) {
        errorMessage = "PGN sampling interval must be a positive integer.";
        return false;
    }
    if (startGame <= 0) {
        errorMessage = "PGN start game must be a positive integer.";
        return false;
    }

    std::ifstream input(path);
    if (!input) {
        errorMessage = "Could not open PGN file: " + path;
        return false;
    }

    std::vector<std::string> currentGameLines;
    std::string line;
    while (std::getline(input, line)) {
        currentGameLines.push_back(line);
    }

    std::vector<std::vector<std::string>> games;
    std::vector<std::string> currentGame;
    for (const std::string &gameLine : currentGameLines) {
        if (!currentGame.empty() && gameLine.rfind("[Event ", 0) == 0) {
            games.push_back(currentGame);
            currentGame.clear();
        }
        if (!gameLine.empty() || !currentGame.empty()) {
            currentGame.push_back(gameLine);
        }
    }
    if (!currentGame.empty()) {
        games.push_back(currentGame);
    }

    if (games.empty()) {
        errorMessage = "No PGN games found in file: " + path;
        return false;
    }

    int processedGames = 0;
    for (std::size_t gameIndex = 0; gameIndex < games.size(); ++gameIndex) {
        if (static_cast<int>(gameIndex) + 1 < startGame) {
            continue;
        }
        if (maxGames > 0 && processedGames >= maxGames) {
            break;
        }
        ++processedGames;
        const std::vector<std::string> &gameLines = games[gameIndex];
        std::string eventName = "game_" + std::to_string(gameIndex + 1);
        std::string startFen;
        std::ostringstream movetext;
        bool inTags = true;

        for (const std::string &rawLine : gameLines) {
            std::string trimmed = trim(rawLine);
            if (trimmed.rfind("[Event ", 0) == 0) {
                std::size_t firstQuote = trimmed.find('"');
                std::size_t lastQuote = trimmed.rfind('"');
                if (firstQuote != std::string::npos && lastQuote != std::string::npos && firstQuote != lastQuote) {
                    eventName = trimmed.substr(firstQuote + 1, lastQuote - firstQuote - 1);
                }
            } else if (trimmed.rfind("[FEN ", 0) == 0) {
                std::size_t firstQuote = trimmed.find('"');
                std::size_t lastQuote = trimmed.rfind('"');
                if (firstQuote != std::string::npos && lastQuote != std::string::npos && firstQuote != lastQuote) {
                    startFen = trimmed.substr(firstQuote + 1, lastQuote - firstQuote - 1);
                }
            } else if (trimmed.empty() && inTags) {
                inTags = false;
            } else if (!trimmed.empty() && !inTags) {
                movetext << trimmed << '\n';
            }
        }

        std::string replayError;
        if (!replayGameToPositions(extractMoveTokens(movetext.str()), startFen, eventName, everyNPly, false,
                                   positions, replayError)) {
            errorMessage = replayError;
            return false;
        }
    }

    if (positions.empty()) {
        errorMessage = "No sampled positions were produced from PGN file: " + path;
        return false;
    }

    return true;
}
