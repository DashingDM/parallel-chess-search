#!/bin/bash
#SBATCH --job-name=chess-hpc-full
#SBATCH --cpus-per-task=16
#SBATCH --time=12:00:00
#SBATCH --output=chess-full-%j.out

set -euo pipefail

PROJECT_DIR="${SLURM_SUBMIT_DIR:-$PWD}"
cd "$PROJECT_DIR"

export OMP_NUM_THREADS="${SLURM_CPUS_PER_TASK:-16}"

make clean
make CXX="${CXX_COMPILER:-g++}" OPENMP=1

./chess_hpc --quiet --output tal_full_results.csv Data/Tal.pgn
