# Getting Started

Cellato has two common entry points:

- Run the bundled CLI against one of the built-in automata.
- Use the library from a small C++ program and define your own automaton rule with the DSL.

The CLI is useful for checking correctness and comparing execution variants on different hardware. The example project is better when you want to understand the minimum pieces required for a new automaton.

## Requirements

Cellato is built with CMake and uses C++20 and CUDA. The top-level project enables both C++ and CUDA languages, so a CUDA toolkit must be available even when you only plan to run CPU examples from the main build. The standalone example is a C++ target, but it currently still finds and links the CUDA runtime because it includes the shared Cellato headers.

The default CUDA architecture is `native`. On machines where CMake cannot infer the local GPU or to compile for a specific architecture, pass an explicit option to the preset:

```sh
cmake --preset release -DCMAKE_CUDA_ARCHITECTURES=90
```

For clusters or CI images, use the architecture that matches the target GPU rather than the login node.

## Build The Main CLI

Configure and build the default release preset:

```sh
cmake --preset release
cmake --build --preset release --parallel 4
```

Run unit tests from the same preset:

```sh
ctest --preset release
```

The executable is created at:

```text
build/release/cellato
```

## Run A CPU Smoke Test

This command runs Conway's Game of Life on a small toroidal grid with the standard evaluator and standard layout:

```sh
build/release/cellato \
  --automaton game-of-life \
  --device CPU \
  --traverser simple \
  --evaluator standard \
  --layout standard \
  --x_size 8 \
  --y_size 8 \
  --rounds 1 \
  --warmup_rounds 0 \
  --steps 2 \
  --seed 1
```

The option triplet `--traverser`, `--evaluator`, and `--layout` selects a precompiled runtime suite. Packed layouts also require `--word_size 32` or `--word_size 64`.

## Run A CUDA Smoke Test

On a machine with a usable CUDA GPU, this command runs the same automaton with linear bit planes:

```sh
build/release/cellato \
  --automaton game-of-life \
  --device CUDA \
  --traverser simple \
  --evaluator bit_planes \
  --layout bit_planes \
  --word_size 64 \
  --x_size 4096 \
  --y_size 4096 \
  --rounds 1 \
  --warmup_rounds 1 \
  --steps 100 \
  --seed 1
```

Grid dimensions must satisfy the selected layout and traverser constraints. For example, a 64-bit linear bit-plane grid stores 64 adjacent cells per word in the X direction, so practical CUDA runs should choose dimensions divisible by the word and block geometry.

## Compare With The Baseline

Reference implementations are separate from the Cellato layout/evaluator/traverser matrix. To run the baseline implementation:

```sh
build/release/cellato \
  --automaton game-of-life \
  --reference_impl baseline \
  --x_size 8 \
  --y_size 8 \
  --rounds 1 \
  --warmup_rounds 0 \
  --steps 2 \
  --seed 1
```

Use baseline runs for smoke-level correctness checks before comparing optimized variants.

## Build The Custom Automaton Example

The standalone example in [`examples/your_own_ca`](../examples/your_own_ca/README.md) builds against Cellato and defines a tiny three-state automaton.

```sh
cmake --preset release -S examples/your_own_ca
cmake --build examples/your_own_ca/build/release --parallel 4
examples/your_own_ca/build/release/your_own_ca
```

The example shows the minimum direct-library path:

1. Define a `cell_state` type.
2. Bind states with `cellato::ast::state_constant`.
3. Build predicates and a rule type.
4. Define a `config` with `name`, `average_halo_radius`, `algorithm` (the rule), `cell_state` (state type), and `state_dictionary` (state values).
5. Choose a suite, such as `cellato::run::test_suites::on_cpu::standard<config>`.
6. Run it with `cellato::run::experiment_manager`.

For a line-by-line explanation of the example, see [`examples/your_own_ca/README.md`](../examples/your_own_ca/README.md). For more library detail, continue with [Core Concepts](core-concepts.md), [DSL](dsl.md), and [Execution](execution.md).
