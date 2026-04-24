#ifndef PGN_H
#define PGN_H

#include <string>
#include <vector>

struct InputPosition {
    std::string name;
    std::string fen;
};

bool loadPgnAsFinalPositions(const std::string &path, std::vector<InputPosition> &positions,
                             std::string &errorMessage, int maxGames = 0, int startGame = 1);
bool loadPgnAsSampledPositions(const std::string &path, int everyNPly,
                               std::vector<InputPosition> &positions, std::string &errorMessage,
                               int maxGames = 0, int startGame = 1);

#endif
