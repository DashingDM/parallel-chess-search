#!/bin/bash

set -euo pipefail

OUTPUT_FILE="${1:-tal_merged_results.csv}"
shift || true

if [ "$#" -eq 0 ]; then
    echo "Usage: $0 output.csv shard1.csv [shard2.csv ...]" >&2
    exit 1
fi

head -n 1 "$1" > "$OUTPUT_FILE"

for csv in "$@"; do
    tail -n +2 "$csv" >> "$OUTPUT_FILE"
done

echo "Merged $# shard files into $OUTPUT_FILE"
