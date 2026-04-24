# Results Folder Guide

This folder contains the curated outputs kept in the submission package after cleanup.

## Kept Files

- `Tal_final_results.csv`  
  Small representative benchmark output for quick inspection.

- `tal_full_results.csv`  
  Main full-dataset benchmark result file used for the project analysis.

- `sample_standard_game_final_results.csv`  
  Small example output useful for sanity checking and demonstration.

- `vtune_hotspots_summary.csv`  
  Summary-level profiling metrics from VTune hotspots analysis.

- `vtune_threading_summary.csv`  
  Summary-level profiling metrics from VTune threading analysis.

- `weak_scaling_runs/`  
  Weak-scaling experiment outputs and summary CSVs for thread counts `1, 2, 4, 8, 16`.

## Removed During Cleanup

The following were intentionally removed from the shareable package to reduce size and noise:

- Slurm `.out` logs
- shard-level intermediate CSV outputs
- redundant merged result files
- ad hoc local CSV exports
- debug-duplicate VTune CSVs
- bulky raw VTune per-function exports already summarized by figures and summary files

The goal of this cleanup is to keep the package focused on final, review-worthy artifacts rather than every intermediate file produced during experimentation.

