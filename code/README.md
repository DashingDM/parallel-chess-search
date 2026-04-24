# Chess HPC Code 

This folder contains the core implementation for the Chess HPC project.

## Included Components

- Board representation
- Move generation
- Material evaluation
- Sequential minimax / Alpha-Beta search
- OpenMP parallel root search
- Makefile for local builds

## Build

Run from inside the `code/` directory:

```bash
make
```

## Example Runs

```bash
./chess_hpc
./chess_hpc positions.fen
./chess_hpc games.pgn
./chess_hpc --max-games 5 games.pgn
./chess_hpc --every 20 games.pgn
./chess_hpc --every-move 10 games.pgn
./chess_hpc --start-game 101 --max-games 50 games.pgn
./chess_hpc --output custom_results.csv games.pgn
./chess_hpc --quiet --output shard.csv --start-game 101 --max-games 50 games.pgn
```

## Input Formats

- Default run with no arguments uses built-in test positions.
- FEN files: one FEN per line, or `name|FEN` per line.
- PGN files: replay SAN movetext and benchmark the final position from each game.
- `--max-games N` stops after the first `N` games.
- `--start-game N` skips to the `N`th PGN game before benchmarking.
- `--every N` benchmarks every `N`th ply instead of only the final position.
- `--every-move N` benchmarks every `N`th full move instead of thinking in plies.
- `--quiet` suppresses verbose board dumps and timing chatter while still writing CSV output.
- `--output file.csv` writes results to a custom CSV path.

## Default Output Names

- No input file: `results.csv`
- `games.pgn`: `games_final_results.csv`
- `--every N games.pgn` or `--every-move N games.pgn`: `games_sampled_results.csv`
- `positions.fen`: `positions_results.csv`

## Slurm / Cluster Workflows

These scripts live in `../slurm_scripts/`:

- `run_slurm.sh` for smaller CPU submissions
- `run_full_slurm.sh` for a single full-dataset run
- `run_array_slurm.sh` for sharded Slurm array execution
- `merge_csv.sh` to merge shard outputs
- `run_weak_scaling.sh` for weak-scaling experiments

Example commands:

```bash
sbatch ../slurm_scripts/run_slurm.sh
sbatch --cpus-per-task=8 --time=02:00:00 --export=ALL,MAX_GAMES=5,OUTPUT_FILE=tal_quick.csv ../slurm_scripts/run_slurm.sh
sbatch --array=0-9 --cpus-per-task=16 --export=ALL,GAMES_PER_TASK=250 ../slurm_scripts/run_array_slurm.sh
BASE_GAMES_PER_THREAD=25 ../slurm_scripts/run_weak_scaling.sh
```

You can override `MAX_GAMES`, `START_GAME`, `EVERY_MOVE`, `OUTPUT_FILE`, `INPUT_FILE`, `GAMES_PER_TASK`, and `CXX_COMPILER` with `sbatch --export=ALL,...`.

## Notes

- This package is structured for project review and reproducibility.
- Advanced rules like castling, en passant, and promotion handling are not fully implemented.
- Attack detection is basic and may need refinement if the engine is extended.
- PGN support handles common SAN moves from normal Lichess exports, including castling and en passant during replay.
- The engine is benchmark-focused and not intended to be a full tournament-grade chess engine.
