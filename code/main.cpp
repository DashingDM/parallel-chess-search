#include "board.h"
#include "fen.h"
#include "movegen.h"
#include "search.h"
#include "parallel.h"
#include "pgn.h"
#include "utils.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <thread>
#include <vector>

namespace {

bool parsePositiveInt(const std::string &value, int &parsedValue);

std::vector<int> getThreadCounts() {
    unsigned int hwThreads = std::thread::hardware_concurrency();
    int maxThreads = hwThreads > 0 ? static_cast<int>(hwThreads) : 1;

    if (const char *ompThreads = std::getenv("OMP_NUM_THREADS")) {
        int envThreads = 0;
        if (parsePositiveInt(ompThreads, envThreads)) {
            maxThreads = envThreads;
        }
    }

    std::vector<int> threadCounts;
    for (int threads = 1; threads <= maxThreads; threads *= 2) {
        threadCounts.push_back(threads);
        if (threads > maxThreads / 2) {
            break;
        }
    }
    if (threadCounts.empty()) {
        threadCounts.push_back(1);
    }
    if (threadCounts.back() != maxThreads) {
        threadCounts.push_back(maxThreads);
    }
    return threadCounts;
}

const std::vector<InputPosition> TEST_POSITIONS = {
    {"start", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w"},
    {"center_opening", "r1bqkbnr/pppp1ppp/2n5/4p3/3PP3/5N2/PPP2PPP/RNBQKB1R b"},
    {"rook_endgame", "4k3/8/3K4/8/8/8/6R1/8 w"}
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

std::string toLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

bool parsePositiveInt(const std::string &value, int &parsedValue) {
    if (value.empty()) {
        return false;
    }

    for (char ch : value) {
        if (!std::isdigit(static_cast<unsigned char>(ch))) {
            return false;
        }
    }

    parsedValue = std::stoi(value);
    return parsedValue > 0;
}

bool loadFenFile(const std::string &path, std::vector<InputPosition> &positions, std::string &errorMessage) {
    std::ifstream input(path);
    if (!input) {
        errorMessage = "Could not open FEN file: " + path;
        return false;
    }

    std::string line;
    int lineNumber = 0;
    int positionIndex = 0;
    while (std::getline(input, line)) {
        ++lineNumber;
        std::string trimmed = trim(line);
        if (trimmed.empty() || trimmed[0] == '#') {
            continue;
        }

        std::string name;
        std::string fen;
        std::size_t separator = trimmed.find('|');
        if (separator != std::string::npos) {
            name = trim(trimmed.substr(0, separator));
            fen = trim(trimmed.substr(separator + 1));
        } else {
            ++positionIndex;
            name = "position_" + std::to_string(positionIndex);
            fen = trimmed;
        }

        if (fen.empty()) {
            errorMessage = "Line " + std::to_string(lineNumber) + " does not contain a FEN string.";
            return false;
        }
        if (name.empty()) {
            name = "position_" + std::to_string(positionIndex + 1);
        }

        positions.push_back({name, fen});
        ++positionIndex;
    }

    if (positions.empty()) {
        errorMessage = "No positions found in FEN file: " + path;
        return false;
    }

    return true;
}

bool loadPositionsFromInput(const std::string &path, int pgnEveryNPly, int maxGames, int startGame,
                            std::vector<InputPosition> &positions, std::string &errorMessage) {
    std::filesystem::path inputPath(path);
    std::string extension = toLower(inputPath.extension().string());
    if (extension == ".fen" || extension == ".txt") {
        return loadFenFile(path, positions, errorMessage);
    }
    if (extension == ".pgn") {
        if (pgnEveryNPly > 0) {
            return loadPgnAsSampledPositions(path, pgnEveryNPly, positions, errorMessage, maxGames, startGame);
        }
        return loadPgnAsFinalPositions(path, positions, errorMessage, maxGames, startGame);
    }

    errorMessage = "Unsupported input file extension: " + extension +
                   ". Use a .fen/.txt file or a .pgn file.";
    return false;
}

std::string defaultOutputPath(const std::string &inputPath, int pgnEveryNPly) {
    if (inputPath.empty()) {
        return "results.csv";
    }

    std::filesystem::path path(inputPath);
    std::string stem = path.stem().string();
    if (stem.empty()) {
        stem = "results";
    }

    if (toLower(path.extension().string()) == ".pgn") {
        if (pgnEveryNPly > 0) {
            return stem + "_sampled_results.csv";
        }
        return stem + "_final_results.csv";
    }

    return stem + "_results.csv";
}

void printUsage(const char *programName) {
    std::cout << "Usage:\n"
              << "  " << programName << "\n"
              << "  " << programName << " <positions.fen>\n"
              << "  " << programName << " <games.pgn>\n"
              << "  " << programName << " --every <N> <games.pgn>\n"
              << "  " << programName << " --every-move <N> <games.pgn>\n"
              << "  " << programName << " --max-games <N> <games.pgn>\n"
              << "  " << programName << " --start-game <N> --max-games <M> <games.pgn>\n"
              << "  " << programName << " --output <file.csv> <positions.fen|games.pgn>\n"
              << "  " << programName << " --output <file.csv> --every <N> <games.pgn>\n"
              << "  " << programName << " --output <file.csv> --every-move <N> <games.pgn>\n"
              << "  " << programName << " --output <file.csv> --max-games <N> <games.pgn>\n\n"
              << "FEN file format:\n"
              << "  One FEN per line, or name|FEN on each line.\n"
              << "PGN support:\n"
              << "  Replays PGN movetext and benchmarks the final position from each game.\n"
              << "  Use --every N to benchmark every Nth ply instead.\n"
              << "  Use --every-move N to benchmark every Nth full move instead.\n"
              << "  Use --start-game N to skip directly to the Nth PGN game.\n"
              << "  Use --max-games N to stop after the first N PGN games.\n"
              << "Output:\n"
              << "  Defaults to results.csv with no input, <name>_final_results.csv for PGN final runs,\n"
              << "  and <name>_sampled_results.csv for sampled PGN runs.\n";
}

void writeCsvHeader(std::ofstream &csv) {
    csv << "position,depth,mode,threads,best_move,best_score,nodes,time_seconds,speedup,efficiency\n";
}

void writeCsvRow(std::ofstream &csv, const std::string &positionName, int depth, const std::string &mode,
                 int threads, const SearchResult &result, double elapsed, double speedup, double efficiency) {
    csv << positionName << ','
        << depth << ','
        << mode << ','
        << threads << ','
        << '"' << moveToString(result.bestMove) << '"' << ','
        << result.bestScore << ','
        << result.nodesSearched << ','
        << elapsed << ','
        << speedup << ','
        << efficiency << '\n';
}

} // namespace

int main(int argc, char *argv[]) {
    const std::vector<int> depths = {4, 5, 6};
    const std::vector<int> threadCounts = getThreadCounts();
    std::vector<InputPosition> positions = TEST_POSITIONS;
    int pgnEveryNPly = 0;
    int maxGames = 0;
    int startGame = 1;
    std::string inputPath;
    std::string outputPath;
    bool quiet = false;
    int argIndex = 1;

    if (argc >= 2) {
        std::string firstArgument = argv[argIndex];
        if (firstArgument == "--help" || firstArgument == "-h") {
            printUsage(argv[0]);
            return 0;
        }
    }

    while (argIndex < argc) {
        std::string argument = argv[argIndex];
        if (argument == "--output") {
            if (argIndex + 1 >= argc) {
                std::cerr << "--output expects a file path.\n";
                return 1;
            }
            outputPath = argv[argIndex + 1];
            argIndex += 2;
            continue;
        }
        if (argument == "--max-games") {
            if (argIndex + 1 >= argc) {
                std::cerr << "--max-games expects a positive integer.\n";
                return 1;
            }
            if (!parsePositiveInt(argv[argIndex + 1], maxGames)) {
                std::cerr << "--max-games expects a positive integer.\n";
                return 1;
            }
            argIndex += 2;
            continue;
        }
        if (argument == "--start-game") {
            if (argIndex + 1 >= argc) {
                std::cerr << "--start-game expects a positive integer.\n";
                return 1;
            }
            if (!parsePositiveInt(argv[argIndex + 1], startGame)) {
                std::cerr << "--start-game expects a positive integer.\n";
                return 1;
            }
            argIndex += 2;
            continue;
        }
        if (argument == "--quiet") {
            quiet = true;
            ++argIndex;
            continue;
        }

        if (argument == "--every" || argument == "--every-move") {
            if (argIndex + 1 >= argc) {
                std::cerr << argument << " expects a positive integer.\n";
                return 1;
            }
            if (!parsePositiveInt(argv[argIndex + 1], pgnEveryNPly)) {
                std::cerr << argument << " expects a positive integer.\n";
                return 1;
            }
            if (argument == "--every-move") {
                pgnEveryNPly *= 2;
            }
            argIndex += 2;
            continue;
        }

        if (!argument.empty() && argument[0] == '-') {
            std::cerr << "Unknown option: " << argument << "\n";
            return 1;
        }
        if (!inputPath.empty()) {
            printUsage(argv[0]);
            return 1;
        }

        inputPath = argument;
        ++argIndex;
    }

    if (inputPath.empty() && argc > 1 && outputPath.empty() && pgnEveryNPly == 0 && maxGames == 0) {
        printUsage(argv[0]);
        return 1;
    }

    if (!inputPath.empty()) {
        positions.clear();
        std::string errorMessage;
        if (!loadPositionsFromInput(inputPath, pgnEveryNPly, maxGames, startGame, positions, errorMessage)) {
            std::cerr << errorMessage << "\n";
            return 1;
        }
    } else if (argc > 1 && (pgnEveryNPly > 0 || maxGames > 0 || startGame > 1)) {
        std::cerr << "An input file is required when using PGN options.\n";
        return 1;
    }

    if (outputPath.empty()) {
        outputPath = defaultOutputPath(inputPath, pgnEveryNPly);
    }

    std::ofstream csv(outputPath);
    if (!csv) {
        std::cerr << "Could not open " << outputPath << " for writing.\n";
        return 1;
    }
    writeCsvHeader(csv);

    for (const InputPosition &position : positions) {
        Board board;
        std::string errorMessage;
        if (!loadFEN(board, position.fen, errorMessage)) {
            std::cerr << "Skipping " << position.name << ": " << errorMessage << "\n";
            continue;
        }

        if (!quiet) {
            std::cout << "\nPosition: " << position.name << "\n";
            printBoard(board);
        }

        for (int depth : depths) {
            Board sequentialBoard = copyBoard(board);
            if (!quiet) {
                std::cout << "\nRunning sequential alpha-beta at depth " << depth << "...\n";
            }
            auto startSeq = std::chrono::high_resolution_clock::now();
            SearchResult seqResult = findBestMoveAlphaBeta(sequentialBoard, depth);
            auto endSeq = std::chrono::high_resolution_clock::now();
            double seqTime = std::chrono::duration<double>(endSeq - startSeq).count();

            if (!quiet) {
                std::cout << "Sequential best move: " << moveToString(seqResult.bestMove) << "\n";
                std::cout << "Sequential best score: " << seqResult.bestScore << "\n";
                std::cout << "Sequential nodes searched: " << seqResult.nodesSearched << "\n";
                std::cout << "Sequential time: " << seqTime << " s\n";
            }
            writeCsvRow(csv, position.name, depth, "sequential", 1, seqResult, seqTime, 1.0, 1.0);

            for (int threads : threadCounts) {
                Board parallelBoard = copyBoard(board);
                if (!quiet) {
                    std::cout << "\nRunning parallel alpha-beta with " << threads
                              << " threads at depth " << depth << "...\n";
                }

                auto startPar = std::chrono::high_resolution_clock::now();
                SearchResult parResult = findBestMoveParallel(parallelBoard, depth, threads);
                auto endPar = std::chrono::high_resolution_clock::now();

                double parTime = std::chrono::duration<double>(endPar - startPar).count();
                double speedup = parTime > 0.0 ? seqTime / parTime : 0.0;
                double efficiency = threads > 0 ? speedup / threads : 0.0;

                if (!quiet) {
                    std::cout << "Parallel best move: " << moveToString(parResult.bestMove) << "\n";
                    std::cout << "Parallel best score: " << parResult.bestScore << "\n";
                    std::cout << "Parallel nodes searched: " << parResult.nodesSearched << "\n";
                    std::cout << "Parallel time: " << parTime << " s\n";
                    std::cout << "Speedup: " << speedup << "\n";
                    std::cout << "Efficiency: " << efficiency << "\n";
                }

                writeCsvRow(csv, position.name, depth, "parallel", threads, parResult, parTime, speedup, efficiency);
            }
        }
    }

    return 0;
}
