#!/bin/bash
#SBATCH --job-name=chess-hpc
#SBATCH --cpus-per-task=8
#SBATCH --time=02:00:00
#SBATCH --output=slurm-%j.out

set -euo pipefail

PROJECT_DIR="${SLURM_SUBMIT_DIR:-$PWD}"
cd "$PROJECT_DIR"

if [ -n "${SLURM_CPUS_PER_TASK:-}" ]; then
    export OMP_NUM_THREADS="$SLURM_CPUS_PER_TASK"
fi

CXX_COMPILER="${CXX_COMPILER:-g++}"
INPUT_FILE="${INPUT_FILE:-Data/Tal.pgn}"
OUTPUT_FILE="${OUTPUT_FILE:-tal_server_results.csv}"
MAX_GAMES="${MAX_GAMES:-10}"
EVERY_MOVE="${EVERY_MOVE:-0}"
START_GAME="${START_GAME:-1}"
QUIET="${QUIET:-1}"
EXTRA_ARGS="${EXTRA_ARGS:-}"

echo "Project dir: $PROJECT_DIR"
echo "Compiler: $CXX_COMPILER"
echo "Input: $INPUT_FILE"
echo "Output: $OUTPUT_FILE"
echo "Max games: $MAX_GAMES"
echo "Every move: $EVERY_MOVE"
echo "Start game: $START_GAME"
echo "OMP_NUM_THREADS: ${OMP_NUM_THREADS:-unset}"

make clean
make CXX="$CXX_COMPILER" OPENMP=1

CMD=(./chess_hpc --output "$OUTPUT_FILE")

if [ "$MAX_GAMES" -gt 0 ]; then
    CMD+=(--max-games "$MAX_GAMES")
fi

if [ "$START_GAME" -gt 1 ]; then
    CMD+=(--start-game "$START_GAME")
fi

if [ "$EVERY_MOVE" -gt 0 ]; then
    CMD+=(--every-move "$EVERY_MOVE")
fi

if [ "$QUIET" -eq 1 ]; then
    CMD+=(--quiet)
fi

CMD+=("$INPUT_FILE")

if [ -n "$EXTRA_ARGS" ]; then
    # Optional extra CLI args for experimentation.
    # shellcheck disable=SC2206
    EXTRA_ARRAY=($EXTRA_ARGS)
    CMD=("${CMD[@]:0:1}" "${EXTRA_ARRAY[@]}" "${CMD[@]:1}")
fi

printf 'Running command:'
printf ' %q' "${CMD[@]}"
printf '\n'

"${CMD[@]}"

echo "Done. Results written to $OUTPUT_FILE"
