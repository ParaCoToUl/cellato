#!/bin/bash

set -euo pipefail

USE_SLURM=${USE_SLURM:-0}

INSTALL_DEPS=${INSTALL_DEPS:-1}
ENABLE_KOKKOS=${ENABLE_KOKKOS:-1}
ENABLE_HALIDE=${ENABLE_HALIDE:-1}
ENABLE_GRIDTOOLS=${ENABLE_GRIDTOOLS:-0}

HALIDE_SYSTEMWIDE=${HALIDE_SYSTEMWIDE:-0}

script_dir=$(dirname "$0")

should_remove="${1:-}"

SRUN="srun -p gpu-short -A kdss --cpus-per-task=32 --mem=64GB --time=2:00:00 --gres=gpu:L40"

size_and_stuff="--precision 32 --x_size 1024 --y_size 1024 --steps 100 --rounds 7 --warmup_rounds 4 --cuda_block_size_y 4"

args=(
    "--automaton brian --seed 42 --device CUDA --traverser simple --evaluator standard --layout standard $size_and_stuff"
    "--automaton brian --seed 42 --device CUDA --reference_impl kokkos $size_and_stuff"
    "--automaton brian --seed 42 --device CUDA --reference_impl halide $size_and_stuff"
    # "--automaton brian --seed 42 --device CUDA --reference_impl gridtools $size_and_stuff"

    "--automaton brian --seed 42 --device CPU --traverser simple --evaluator standard --layout standard $size_and_stuff"
    "--automaton brian --seed 42 --device CPU --reference_impl kokkos $size_and_stuff"
    "--automaton brian --seed 42 --device CPU --reference_impl halide $size_and_stuff"
    # "--automaton brian --seed 42 --device CPU --reference_impl gridtools $size_and_stuff"

    "--automaton critters --seed 42 --device CUDA --traverser simple --evaluator standard --layout standard $size_and_stuff"
    "--automaton critters --seed 42 --device CUDA --reference_impl kokkos $size_and_stuff"
    "--automaton critters --seed 42 --device CUDA --reference_impl halide $size_and_stuff"
    # "--automaton critters --seed 42 --device CUDA --reference_impl gridtools $size_and_stuff"

    "--automaton critters --seed 42 --device CPU --traverser simple --evaluator standard --layout standard $size_and_stuff"
    "--automaton critters --seed 42 --device CPU --reference_impl kokkos $size_and_stuff"
    "--automaton critters --seed 42 --device CPU --reference_impl halide $size_and_stuff"
    # "--automaton critters --seed 42 --device CPU --reference_impl gridtools $size_and_stuff"

    "--automaton cyclic --seed 42 --device CUDA --traverser simple --evaluator standard --layout standard $size_and_stuff"
    "--automaton cyclic --seed 42 --device CUDA --reference_impl kokkos $size_and_stuff"
    "--automaton cyclic --seed 42 --device CUDA --reference_impl halide $size_and_stuff"
    # "--automaton cyclic --seed 42 --device CUDA --reference_impl gridtools $size_and_stuff"

    "--automaton cyclic --seed 42 --device CPU --traverser simple --evaluator standard --layout standard $size_and_stuff"
    "--automaton cyclic --seed 42 --device CPU --reference_impl kokkos $size_and_stuff"
    "--automaton cyclic --seed 42 --device CPU --reference_impl halide $size_and_stuff"
    # "--automaton cyclic --seed 42 --device CPU --reference_impl gridtools $size_and_stuff"

    "--automaton fire --seed 42 --device CUDA --traverser simple --evaluator standard --layout standard $size_and_stuff"
    "--automaton fire --seed 42 --device CUDA --reference_impl kokkos $size_and_stuff"
    "--automaton fire --seed 42 --device CUDA --reference_impl halide $size_and_stuff"
    # "--automaton fire --seed 42 --device CUDA --reference_impl gridtools $size_and_stuff"

    "--automaton fire --seed 42 --device CPU --traverser simple --evaluator standard --layout standard $size_and_stuff"
    "--automaton fire --seed 42 --device CPU --reference_impl kokkos $size_and_stuff"
    "--automaton fire --seed 42 --device CPU --reference_impl halide $size_and_stuff"
    # "--automaton fire --seed 42 --device CPU --reference_impl gridtools $size_and_stuff"

    "--automaton game-of-life --seed 42 --device CUDA --traverser simple --evaluator standard --layout standard $size_and_stuff"
    "--automaton game-of-life --seed 42 --device CUDA --reference_impl kokkos $size_and_stuff"
    "--automaton game-of-life --seed 42 --device CUDA --reference_impl halide $size_and_stuff"
    # "--automaton game-of-life --seed 42 --device CUDA --reference_impl gridtools $size_and_stuff"

    "--automaton game-of-life --seed 42 --device CPU --traverser simple --evaluator standard --layout standard $size_and_stuff"
    "--automaton game-of-life --seed 42 --device CPU --reference_impl kokkos $size_and_stuff"
    "--automaton game-of-life --seed 42 --device CPU --reference_impl halide $size_and_stuff"
    # "--automaton game-of-life --seed 42 --device CPU --reference_impl gridtools $size_and_stuff"

    "--automaton greenberg --seed 42 --device CUDA --traverser simple --evaluator standard --layout standard $size_and_stuff"
    "--automaton greenberg --seed 42 --device CUDA --reference_impl kokkos $size_and_stuff"
    "--automaton greenberg --seed 42 --device CUDA --reference_impl halide $size_and_stuff"
    # "--automaton greenberg --seed 42 --device CUDA --reference_impl gridtools $size_and_stuff"

    "--automaton greenberg --seed 42 --device CPU --traverser simple --evaluator standard --layout standard $size_and_stuff"
    "--automaton greenberg --seed 42 --device CPU --reference_impl kokkos $size_and_stuff"
    "--automaton greenberg --seed 42 --device CPU --reference_impl halide $size_and_stuff"
    # "--automaton greenberg --seed 42 --device CPU --reference_impl gridtools $size_and_stuff"

    "--automaton hpp --seed 42 --device CUDA --traverser simple --evaluator standard --layout standard $size_and_stuff"
    "--automaton hpp --seed 42 --device CUDA --reference_impl kokkos $size_and_stuff"
    "--automaton hpp --seed 42 --device CUDA --reference_impl halide $size_and_stuff"
    # "--automaton hpp --seed 42 --device CUDA --reference_impl gridtools $size_and_stuff"

    "--automaton hpp --seed 42 --device CPU --traverser simple --evaluator standard --layout standard $size_and_stuff"
    "--automaton hpp --seed 42 --device CPU --reference_impl kokkos $size_and_stuff"
    "--automaton hpp --seed 42 --device CPU --reference_impl halide $size_and_stuff"
    # "--automaton hpp --seed 42 --device CPU --reference_impl gridtools $size_and_stuff"

    "--automaton maze --seed 42 --device CUDA --traverser simple --evaluator standard --layout standard $size_and_stuff"
    "--automaton maze --seed 42 --device CUDA --reference_impl kokkos $size_and_stuff"
    "--automaton maze --seed 42 --device CUDA --reference_impl halide $size_and_stuff"
    # "--automaton maze --seed 42 --device CUDA --reference_impl gridtools $size_and_stuff"

    "--automaton maze --seed 42 --device CPU --traverser simple --evaluator standard --layout standard $size_and_stuff"
    "--automaton maze --seed 42 --device CPU --reference_impl kokkos $size_and_stuff"
    "--automaton maze --seed 42 --device CPU --reference_impl halide $size_and_stuff"
    # "--automaton maze --seed 42 --device CPU --reference_impl gridtools $size_and_stuff"

    "--automaton traffic --seed 42 --device CUDA --traverser simple --evaluator standard --layout standard $size_and_stuff"
    "--automaton traffic --seed 42 --device CUDA --reference_impl kokkos $size_and_stuff"
    "--automaton traffic --seed 42 --device CUDA --reference_impl halide $size_and_stuff"
    # "--automaton traffic --seed 42 --device CUDA --reference_impl gridtools $size_and_stuff"

    "--automaton traffic --seed 42 --device CPU --traverser simple --evaluator standard --layout standard $size_and_stuff"
    "--automaton traffic --seed 42 --device CPU --reference_impl kokkos $size_and_stuff"
    "--automaton traffic --seed 42 --device CPU --reference_impl halide $size_and_stuff"
    # "--automaton traffic --seed 42 --device CPU --reference_impl gridtools $size_and_stuff"

    "--automaton wire --seed 42 --device CUDA --traverser simple --evaluator standard --layout standard $size_and_stuff"
    "--automaton wire --seed 42 --device CUDA --reference_impl kokkos $size_and_stuff"
    "--automaton wire --seed 42 --device CUDA --reference_impl halide $size_and_stuff"
    # "--automaton wire --seed 42 --device CUDA --reference_impl gridtools $size_and_stuff"

    "--automaton wire --seed 42 --device CPU --traverser simple --evaluator standard --layout standard $size_and_stuff"
    "--automaton wire --seed 42 --device CPU --reference_impl kokkos $size_and_stuff"
    "--automaton wire --seed 42 --device CPU --reference_impl halide $size_and_stuff"
    # "--automaton wire --seed 42 --device CPU --reference_impl gridtools $size_and_stuff"
)

if [ "$should_remove" == "clean-deps" ]; then
    echo "Removing old build and dependencies..."
    rm -rf "$script_dir/../bin"
    rm -rf "$script_dir/../deps"
elif [ "$should_remove" == "clean" ]; then
    echo "Removing old build..."
    rm -rf "$script_dir/../bin" 
fi

cd "$script_dir"

ENABLES=()

if [ "$ENABLE_KOKKOS" -eq 1 ]; then
    ENABLES+=("ENABLE_KOKKOS=ON")
fi

if [ "$ENABLE_HALIDE" -eq 1 ]; then
    ENABLES+=("ENABLE_HALIDE=ON")
fi

if [ "$HALIDE_SYSTEMWIDE" -eq 1 ]; then
    ENABLES+=("HALIDE_SYSTEMWIDE=ON")
fi

if [ "$ENABLE_GRIDTOOLS" -eq 1 ]; then
    ENABLES+=("ENABLE_GRIDTOOLS=ON")
fi

if [ "$INSTALL_DEPS" -eq 1 ]; then
    if [ "$USE_SLURM" -eq 1 ]; then
        $SRUN make -j4 install_deps "${ENABLES[@]}"
    else
        make -j4 install_deps "${ENABLES[@]}"
    fi
fi

export LD_LIBRARY_PATH="$script_dir/../_deps/halide-install/lib:$script_dir/../_deps/kokkos-install/lib:$LD_LIBRARY_PATH"


for arg in "${args[@]}"; do
    if [ "$USE_SLURM" -eq 1 ]; then
        $SRUN make -j4 run ARGS="$arg" "${ENABLES[@]}"
    else
        make -j4 run ARGS="$arg" "${ENABLES[@]}"
    fi
done
