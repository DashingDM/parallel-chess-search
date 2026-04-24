#!/bin/bash
#SBATCH --job-name=chess-hpc-array
#SBATCH --cpus-per-task=16
#SBATCH --time=04:00:00
#SBATCH --array=0-9
#SBATCH --output=chess-array-%A_%a.out

set -euo pipefail

PROJECT_DIR="${SLURM_SUBMIT_DIR:-$PWD}"
cd "$PROJECT_DIR"

export OMP_NUM_THREADS="${SLURM_CPUS_PER_TASK:-16}"

CXX_COMPILER="${CXX_COMPILER:-g++}"
INPUT_FILE="${INPUT_FILE:-Data/Tal.pgn}"
OUTPUT_PREFIX="${OUTPUT_PREFIX:-tal_shard}"
GAMES_PER_TASK="${GAMES_PER_TASK:-250}"
START_INDEX="${SLURM_ARRAY_TASK_ID:-0}"
START_GAME="$((START_INDEX * GAMES_PER_TASK + 1))"
OUTPUT_FILE="${OUTPUT_PREFIX}_${START_GAME}.csv"

echo "Project dir: $PROJECT_DIR"
echo "Compiler: $CXX_COMPILER"
echo "Input: $INPUT_FILE"
echo "Games per task: $GAMES_PER_TASK"
echo "Array index: $START_INDEX"
echo "Start game: $START_GAME"
echo "Output: $OUTPUT_FILE"
echo "OMP_NUM_THREADS: ${OMP_NUM_THREADS:-unset}"

make clean
make CXX="$CXX_COMPILER" OPENMP=1

./chess_hpc \
    --quiet \
    --start-game "$START_GAME" \
    --max-games "$GAMES_PER_TASK" \
    --output "$OUTPUT_FILE" \
    "$INPUT_FILE"
