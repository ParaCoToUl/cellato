import sys
import subprocess
import os
import itertools
import re
import math
import statistics
import shutil


file_contents_lines = []
with open('__slurm__/job-191185.out', 'r') as file:
    file_contents_lines = file.readlines()


# read csv to fix from 'results/cuda_test_20250928-153828.csv'

csv_lines = []
with open('results/cuda_test_20250928-153828.csv', 'r') as file:
    csv_lines = file.readlines()

def is_successful_run(index, lines):
    return lines[index].startswith('Running test case') and not lines[index+1].startswith('Error')

successful_runs = [i for i in range(len(file_contents_lines)) if is_successful_run(i, file_contents_lines)]
lines = [file_contents_lines[i].strip() for i in successful_runs]

for line in lines[:30]:
    print(line)

content=''

position_of_insert = 17  # after precision

for log, csv_line in zip(lines, csv_lines[1:]):
    if 'temporal_tile_size_y' in log:
        temporal_tile_size_y = int(re.search(r'temporal_tile_size_y (\d+)', log).group(1))
        temporal_steps = int(re.search(r'temporal_steps (\d+)', log).group(1))
    else:
        temporal_tile_size_y = 0
        temporal_steps = 0

    csv_parts = csv_line.strip().split(',')
    new_csv_parts = csv_parts[:position_of_insert] + [str(temporal_steps), str(temporal_tile_size_y)] + csv_parts[position_of_insert:]

    content += ','.join(new_csv_parts) + '\n'

new_header = csv_lines[0].strip().split(',')
new_header = new_header[:position_of_insert] + ['temporal_steps', 'temporal_tile_size_y'] + new_header[position_of_insert:]
new_header_line = ','.join(new_header) + '\n'

with open('results/cuda_test_20250928-153828_fixed.csv', 'w') as file:
    file.write(new_header_line)  # write header
    file.write(content)          # write fixed content

