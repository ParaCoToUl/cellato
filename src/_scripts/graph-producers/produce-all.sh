#!/bin/bash

script_dir=$(dirname "$0")
csv=$script_dir/../results/_h100_res.csv

python ./0_total_perf.py $csv
python ./1_linear_vs_tiled.py $csv
python ./2_time_steps.py $csv
