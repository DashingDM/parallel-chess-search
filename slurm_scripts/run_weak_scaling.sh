#!/bin/bash

set -euo pipefail

BASE_GAMES_PER_THREAD="${BASE_GAMES_PER_THREAD:-25}"
THREADS_LIST="${THREADS_LIST:-1 2 4 8 16}"
INPUT_FILE="${INPUT_FILE:-Data/Tal.pgn}"
OUTPUT_DIR="${OUTPUT_DIR:-weak_scaling_runs}"
mkdir -p "$OUTPUT_DIR"

echo "Building chess_hpc with OpenMP..."
make clean
if [ -n "${CXX_COMPILER:-}" ]; then
    make CXX="$CXX_COMPILER" OPENMP=1
else
    make OPENMP=1
fi

SUMMARY_CSV="$OUTPUT_DIR/weak_scaling_summary.csv"
echo "threads,games_per_thread,total_games,parallel_rows,total_parallel_time,avg_parallel_time,avg_speedup,avg_efficiency,weak_scaling_efficiency" > "$SUMMARY_CSV"

BASELINE_TOTAL_TIME=""

for threads in $THREADS_LIST; do
    total_games=$((BASE_GAMES_PER_THREAD * threads))
    output_csv="$OUTPUT_DIR/weak_scaling_t${threads}.csv"

    echo
    echo "Running weak-scaling case: threads=$threads total_games=$total_games"

    OMP_NUM_THREADS="$threads" ./chess_hpc \
        --quiet \
        --max-games "$total_games" \
        --output "$output_csv" \
        "$INPUT_FILE"

    metrics="$(
        python3 - <<'PY' "$output_csv" "$threads"
import csv
import sys

path = sys.argv[1]
threads = sys.argv[2]

rows = list(csv.DictReader(open(path, newline='')))
parallel_rows = [r for r in rows if r["mode"] == "parallel" and r["threads"] == threads]

total_parallel_time = sum(float(r["time_seconds"]) for r in parallel_rows)
avg_parallel_time = total_parallel_time / len(parallel_rows)
avg_speedup = sum(float(r["speedup"]) for r in parallel_rows) / len(parallel_rows)
avg_efficiency = sum(float(r["efficiency"]) for r in parallel_rows) / len(parallel_rows)

print(len(parallel_rows))
print(total_parallel_time)
print(avg_parallel_time)
print(avg_speedup)
print(avg_efficiency)
PY
    )"

    parallel_rows="$(printf '%s\n' "$metrics" | sed -n '1p')"
    total_parallel_time="$(printf '%s\n' "$metrics" | sed -n '2p')"
    avg_parallel_time="$(printf '%s\n' "$metrics" | sed -n '3p')"
    avg_speedup="$(printf '%s\n' "$metrics" | sed -n '4p')"
    avg_efficiency="$(printf '%s\n' "$metrics" | sed -n '5p')"

    if [ -z "$BASELINE_TOTAL_TIME" ]; then
        BASELINE_TOTAL_TIME="$total_parallel_time"
    fi

    weak_scaling_efficiency="$(
        python3 - <<'PY' "$BASELINE_TOTAL_TIME" "$total_parallel_time"
import sys
baseline = float(sys.argv[1])
current = float(sys.argv[2])
print(baseline / current if current else 0.0)
PY
    )"

    echo "$threads,$BASE_GAMES_PER_THREAD,$total_games,$parallel_rows,$total_parallel_time,$avg_parallel_time,$avg_speedup,$avg_efficiency,$weak_scaling_efficiency" >> "$SUMMARY_CSV"
done

echo
echo "Weak-scaling runs complete."
echo "Per-run CSVs are in: $OUTPUT_DIR"
echo "Summary CSV: $SUMMARY_CSV"
