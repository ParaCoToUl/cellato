#!/bin/bash

set -euo pipefail

script_dir=$(dirname "$0")

# Uncomment one of these test configurations:

# args="--automaton brian  --device CUDA --layout standard --evaluator standard --traverser simple --steps 100 --x_size 64 --y_size 64 --print"

# args="--automaton game-of-life --seed 42 --device CUDA --traverser simple --evaluator bit_planes --layout bit_planes --precision 32 --x_size 1024 --y_size 1024 --steps 2 --rounds 1 --warmup_rounds 0 --cuda_block_size_y 4"
# args="--automaton game-of-life --seed 42 --device CUDA --traverser simple --evaluator bit_planes --layout bit_planes --precision 64 --x_size 14336 --y_size 14336 --steps 100 --rounds 1 --warmup_rounds 0 --cuda_block_size_y 4"
# args="--automaton hpp --seed 42 --device CPU --traverser simple --evaluator bit_planes --layout bit_planes --precision 32 --x_size 64 --y_size 64 --steps 100 --rounds 1 --warmup_rounds 0 --cuda_block_size_y 4"
# args="--automaton critters --seed 42 --device CPU --traverser simple --evaluator bit_planes --layout bit_planes --precision 32 --x_size 64 --y_size 64 --steps 100 --rounds 1 --warmup_rounds 0 --cuda_block_size_y 4 --print"
# args="--automaton critters --seed 42 --device CPU --traverser simple --evaluator standard --layout standard --precision 32 --x_size 64 --y_size 64 --steps 1 --rounds 1 --warmup_rounds 0 --cuda_block_size_y 4 --print"

args="--automaton critters --seed 42 --device CUDA --traverser simple --evaluator standard --layout standard --precision 32 --x_size 1024 --y_size 4 --steps 5 --rounds 1 --warmup_rounds 0 --cuda_block_size_y 4 --print"
args="--automaton critters --seed 42 --device CUDA --traverser simple --evaluator bit_array --layout bit_array --precision 32 --x_size 1024 --y_size 4 --steps 5 --rounds 1 --warmup_rounds 0 --cuda_block_size_y 4 --print"

# args="--automaton critters --seed 42 --device CPU --traverser simple --evaluator standard --layout standard --precision 32 --x_size 64 --y_size 64 --steps 99 --rounds 1 --warmup_rounds 0 --cuda_block_size_y 4"
# args="--automaton hpp --seed 42 --device CPU --reference_impl baseline --precision 32 --x_size 64 --y_size 64 --steps 100 --rounds 1 --warmup_rounds 0 --cuda_block_size_y 4 --print"
# args="--automaton game-of-life --seed 42 --device CPU --traverser simple --evaluator bit_planes --layout bit_planes --precision 64 --x_size 2048 --y_size 2048 --steps 100 --rounds 1 --warmup_rounds 0"

# Game of Life with standard grid on CUDA
# args="--automaton wire --precision 64 \
# --device CUDA --layout tiled_bit_planes --traverser simple --evaluator tiled_bit_planes \
# --warmup_rounds 1 --rounds 3 \
# --steps 1000 --x_size 8192 --y_size 8192"

# Fire automaton with bit_array grid on CUDA
#args="--automaton fire --device CUDA --layout bit_array --steps 50 --x_size 2048 --y_size 2048"

# Wire automaton with bit_array grid and spatial blocking
#args="--automaton wire --device CUDA --layout bit_array --traverser spacial_blocking --x_tile_size 8 --y_tile_size 8 --steps 200 --x_size 4096 --y_size 4096"

# Greenberg automaton on CPU with bit_planes
#args="--automaton greenberg --device CPU --layout bit_planes --evaluator bit_planes --steps 150 --x_size 512 --y_size 512 --precision 64"

# Game of Life on CPU for comparison with CUDA
#args="--automaton game_of_life --device CPU --layout standard --steps 100 --x_size 1024 --y_size 1024"

# Visualize output with print option
#args="--automaton game_of_life --device CUDA --layout bit_array --steps 20 --x_size 32 --y_size 32 --print"

should_remove="${1:-}"

if [ "$should_remove" == "clean" ]; then
    echo "Removing old build..."
    rm -rf "$script_dir/../bin"
fi

cd "$script_dir"
