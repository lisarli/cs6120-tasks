#!/bin/bash

if [ $# -ne 1 ]; then
    echo "Usage: $0 <filename>"
    exit 1
fi

file=$1

baseline_sum=$(awk -F',' '$2 == "baseline" {sum += $3} END {print sum}' "$file")
lvn_sum=$(awk -F',' '$2 == "lvn" {sum += $3} END {print sum}' "$file")

percent_decrease=$(awk -v base="$baseline_sum" -v lvn="$lvn_sum" 'BEGIN {print ((base - lvn) / base) * 100}')

echo "Baseline total: $baseline_sum"
echo "LVN total: $lvn_sum"
echo "Percent decrease: $percent_decrease%"

