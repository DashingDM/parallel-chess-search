# Chess HPC Project

This folder is the cleaned project bundle for the parallel search used in chess engine. It is organized to be easy to share, review, and upload as a standalone package.

## Package Structure

- `code/`  
  C++ source files, headers, Makefile, and usage notes for the benchmark implementation.

- `data/`  
  Input dataset used for the project, including the Tal PGN benchmark source.

- `results/`  
  Benchmark outputs, merged CSV files, representative run logs, and analysis-ready experiment results.

- `plots/`  
  Final report plots and supporting figures used to summarize scaling behavior and profiling findings.

- `slurm_scripts/`  
  Slurm job scripts for full runs, array sharding, CSV merge, and weak-scaling experiments.

## What This Project Demonstrates

This project studies the scalability of sequential and OpenMP-parallel Alpha-Beta chess search on shared-memory CPU systems.

Key outcomes:

- Benchmarked `2,431` chess positions from the Tal PGN dataset
- Evaluated depths `4`, `5`, and `6`
- Generated `43,758` benchmark measurements in the final study
- Added search optimizations including:
  - transposition-table caching
  - root alpha-beta window reuse
  - improved parallel bound sharing
  - dynamic thread-count handling
- Observed best overall efficiency-performance behavior around `8` CPU threads

## Recommended Review Order

If someone is reviewing the project for the first time, the best order is:

1. `code/README.md`
2. `results/Tal_final_results.csv`
3. `results/tal_full_results.csv`
4. `plots/report_plots/strong_scaling_runtime.svg`
5. `plots/report_plots/strong_scaling_speedup.svg`
6. `plots/report_plots/strong_scaling_efficiency.svg`
7. `plots/report_plots/weak_scaling_runtime.svg`
8. `plots/report_plots/weak_scaling_efficiency.svg`

