#!/bin/bash

# Define the input files and the output file
input_files=($(find test/benchmark/report -type f -name '*benchmark.json'))
output_file="test/benchmark/report/combined_benchmark.json"

# Start the output file with an opening square bracket
rm -f "$output_file"
touch "$output_file"
echo '[' > "$output_file"

# Loop through each input file
for i in "${!input_files[@]}"; do
  # Read the file, remove the first and last line (the square brackets), and append to the output file
  cat "${input_files[$i]}" | sed '1d;$d' >> "$output_file"
  
  # If this is not the last file, append a comma to separate the arrays
  if [ $i -lt $((${#input_files[@]} - 1)) ]; then
    echo ',' >> "$output_file"
  fi
done

# Finish the output file with a closing square bracket
echo ']' >> "$output_file"