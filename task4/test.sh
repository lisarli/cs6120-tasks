#!/bin/bash

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 {defined|live}"
    exit 1
fi

ARG=$1

if [ "$ARG" != "defined" ] && [ "$ARG" != "live" ]; then
    echo "Argument must be either 'defined' or 'live'"
    exit 1
fi

output_file="test.$ARG.out"
> "$output_file"

for file in df_tests/*.bril; do
    output=$(bril2json < "$file" | ./df "$ARG")
    expected_output_file="${file%.bril}.$ARG.out"
    
    if [ -f "$expected_output_file" ]; then
        expected_output=$(cat "$expected_output_file")
        if [ "$output" != "$expected_output" ]; then
            echo "Differences in file: $file" >> "$output_file"
            echo "Output:" >> "$output_file"
            echo "$output" >> "$output_file"
            echo "Expected Output:" >> "$output_file"
            echo "$expected_output" >> "$output_file"
        else
            echo "File $file has correct output" >> "$output_file"
        fi
    else
        echo "Expected output file $expected_output_file not found" >> "$output_file"
    fi
    echo "------------------------" >> "$output_file"
done