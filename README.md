# Cellato: A DSL for Cellular Automata 🧬🔍

[![license](https://img.shields.io/badge/license-MIT-blue.svg)](./LICENSE) [![doi](https://img.shields.io/badge/DOI-TODO-blue)](https://doi.org/TODO)

This repository accompanies our paper:

```
@article{
    TODO
}
```

## 🚀 Overview

Cellular automata (CA) are versatile models used across physics, biology, computer science, and environmental science. Unfortunately, most implementations conflate the **rule logic**, **evaluation strategy**, and **memory layout**, making it hard to experiment with new optimizations. **Cellato** solves this by offering an embedded C++ DSL that cleanly separates:

1. **Algorithm** (rule definition)
2. **Evaluator** (how each cell is updated)
3. **Layout** (memory representation)
4. **Traversor** (iteration strategy)

With zero-overhead abstractions powered by template metaprogramming, Cellato lets you swap in different layouts (standard arrays, bit-packed arrays, bit-plates) and execution back-ends (CPU, CUDA) without touching your rule code.

---

## 📂 Repository Structure

```
.
├── LICENSE
├── README.md
├── include/             ← Cellato library headers
├── src/
│   ├── game_of_life/    ← Game of Life example
│   ├── fire/            ← Forest Fire example
│   ├── wire/            ← Wireworld example
│   ├── greenberg/       ← Greenberg–Hastings example
│   ├── _relwork/        ← Reference implementations
│   └── _scripts/        ← Benchmark & plotting scripts
└── results/             ← Benchmark outputs (CSV, PNG, PDF)
```

### 🔍 Canonical Models

| Automaton              | Description                               | Cellato Rule                     |
| ---------------------- | ----------------------------------------- | -------------------------------- |
| **Game of Life**       | Conway’s binary grid (Moore neighborhood) | `src/game_of_life/algorithm.hpp` |
| **Forest Fire**        | Tree ↔ Fire ↔ Ash ↔ Empty (von Neumann)   | `src/fire/config.hpp`            |
| **Wireworld**          | Digital circuit simulator (4 states)      | `src/wire/config.hpp`            |
| **Greenberg–Hastings** | Excitable medium with refractory states   | `src/greenberg/config.hpp`       |

### 🔗 Related Work

We also implemented Game of Life in several other frameworks under `src/_relwork/`:

| Framework | Path                      |
| --------- | ------------------------- |
| Kokkos    | `src/_relwork/kokkos/`    |
| GridTools | `src/_relwork/gridtools/` |
| Halide    | `src/_relwork/halide/`    |
| AN5D      | `src/_relwork/an5d/`      |

> **Note:** All are integrated into our CLI test harness except AN5D, which you must build separately with its `Makefile`.

---

## 🛠️ Cellato Library

All core headers live in [`include/`](./include/). Key components:

| Component                  | Header                                                                                                     |
| -------------------------- | ---------------------------------------------------------------------------------------------------------- |
| **AST nodes**              | `include/core/ast.hpp`                                                                                     |
| **Evaluators**             | `include/evaluators/standard.hpp` • `bit_array.hpp` • `bit_plates.hpp`                                     |
| **Memory layouts**         | `include/memory/standard_grid.hpp` • `bit_array_grid.hpp` • `bit_plates_grid.hpp`                          |
| **Traversors (iteration)** | CPU: `traversers/cpu/simple.hpp`<br>CUDA: `traversers/cuda/simple.{hpp,cu}`, `…/spatial_blocking.{hpp,cu}` |


## 📖 Tutorial

### 🔧 Prerequisites

* **Compiler:** GCC 13.2.0
* **CUDA:** NVCC V12.8.61

### Compilation

```bash
# Clone & enter
git clone https://github.com/matyas-brabec/cellato.git
cd cellato

# Build everything
make
./bin/cellato <options>

# Or via Make:
make run ARGS="<options>"
```

To enable reference back-ends, toggle these `Makefile` flags (default OFF):

```make
ENABLE_KOKKOS   = OFF
ENABLE_HALIDE   = OFF
ENABLE_GRIDTOOLS= OFF
```
---

### ▶️ Running Examples

```bash
# Game of Life on CPU, standard layout
./bin/cellato \
  --automaton game-of-life \
  --device CPU \
  --traverser simple \
  --evaluator standard \
  --layout standard \
  --x_size 256 --y_size 256 \
  --steps 100

# Game of Life on CUDA with bit-plates
./bin/cellato \
  --automaton game-of-life \
  --device CUDA \
  --traverser simple \
  --evaluator bit_plates \
  --layout bit_plates \
  --precision 32
  --x_size 4096 --y_size 4096 \
  --steps 1000 \
  --cuda_block_size_x 32 --cuda_block_size_y 8

# Compare against Kokkos reference impl
./bin/cellato \
  --automaton game-of-life \
  --reference_impl kokkos \
  --x_size 2048 --y_size 2048 \
  --steps 1000
```
---

## ⚙️ CLI Options

```bash
Usage: ./cellato [options]
Options:
  --automaton <name>           Name of the automaton to run (game-of-life, forest-fire, wire, greenberg-hastings)
  --device <CPU|CUDA>          Execution device
  --traverser <name>           Traversal strategy (simple, spatial_blocking)
  --evaluator <name>           Evaluator type (standard, bit_array, bit_plates)
  --layout <name>              Memory layout (standard, bit_array, bit_plates)
  --reference_impl <name>      Run reference implementation (standard, kokkos, halide)
  --x_size <N>                 Grid width
  --y_size <N>                 Grid height
  --x_tile_size <N>            CUDA tile size in X (only with spatial_blocking)
  --y_tile_size <N>            CUDA tile size in Y
  --rounds <N>                 Number of benchmarking rounds
  --warmup_rounds <N>          Number of warmup rounds
  --steps <N>                  Number of CA time steps
  --precision <32|64>          Word precision used by the `bit array` and `bit plates`
  --seed <N>                   RNG seed for initialization
  --print                      Print grid state after each step
  --print_csv_header           Emit CSV header line
  --cuda_block_size_x <N>      CUDA block X dimension (default: 32)
  --cuda_block_size_y <N>      CUDA block Y dimension (default: 8)
  --help                       Show this help message
```

### ✨ Supported Evaluator / Layout / Traverser Combinations

We support three memory layouts, each with its matching evaluator. All can be run with the simple traverser. For the bit_array and bit_plates options, you must specify `--precision`.

```bash
# ▶️ Standard layout + evaluator
./bin/cellato \
  --evaluator standard \
  --layout standard \
  --traverser simple \
  [other options…]

# ▶️ Bit-array layout + evaluator (32-bit)
./bin/cellato \
  --evaluator bit_array \
  --layout bit_array \
  --precision 32 \
  --traverser simple \
  [other options…]

# ▶️ Bit-plates layout + evaluator (32-bit)
./bin/cellato \
  --evaluator bit_plates \
  --layout bit_plates \
  --precision 32 \
  --traverser simple \
  [other options…]

```

---

## 📊 Scripts & Benchmarks

Reproduce paper results via scripts in `src/_scripts/`:

```bash
# Generate raw CSV data
python src/_scripts/baseline_vs_standard.py > results.csv

# Plot comparison (requires pandas, matplotlib)
python src/_scripts/plot_baseline_vs_standard.py results.csv
```

Outputs: `results/baseline_vs_standard_comparison.{png,pdf}`

---

## 🧩 Defining Your Own CA

Cellato makes it easy to define new cellular automata using type-level expressions:

```cpp
// Define states
enum class my_cell_state { state_a, state_b, state_c };

using state_a = state_constant< my_cell_state::state_a >;
using state_b = state_constant< my_cell_state::state_b >;
using state_c = state_constant< my_cell_state::state_c >;

// Define predicates
using is_state_a = p< current_state, equals, state_a >;
using is_state_b = p< current_state, equals, state_b >;
using is_state_c = p< current_state, equals, state_c >;

// Count neighbors in state_a using Moore neighborhood
using state_a_cnt = count_neighbors< state_a, moore_8_neighbors >;
using has_two_a_neighbors = p< state_a_cnt, equals, constant<2> >;

// Define transition rule
using my_rule =
  if_< is_state_a >::then_<
    if_< has_two_a_neighbors >::then_< state_b >::else_< state_a >
  >::elif_< is_state_b >::then_<
    state_c
  >::else_<
    state_a
  >;
```

## 📈 Results

Our evaluations compared Cellato against four prominent stencil and DSL frameworks:

| Framework   | CPU & GPU | Bit-packed | Bit-plates | Vectorization | Native C++ |
| ----------- | :-------: | :--------: | :--------: | :-----------: | :--------: |
| **Cellato** |     ✅     |      ✅     |      ✅     |   Bit-level   |      ✅     |
| Kokkos      |     ✅     |      ❌     |      ❌     |       ❌       |      ✅     |
| GridTools   |     ✅     |      ❌     |      ❌     |       ❌       |      ✅     |
| Halide      |     ✅     |      ❌     |      ❌     |      SIMD     |      ❌     |
| AN5D        |     ❌     |      ❌     |      ❌     |       ❌       |      ❌     |

Performance on standard layouts matches handwritten kernels, demonstrating zero-overhead abstraction.

---

## 🔭 Future Work

* **Higher-dimensional grids:** 3D+ support
* **Non-rectangular topologies:** hexagonal, triangular
* **Probabilistic CAs:** introduce random-node AST types
* **Advanced traversors:** temporal blocking, NUMA-aware scheduling
* **Distributed execution:** MPI-based traversor with halo exchange
* **Framework integration:** embed Cellato evaluators into Kokkos/GridTools

---

## 📬 Contact

If you have any questions regarding the framework, our implementation, or if you have any suggestions, please feel free to contact us by raising [an issue](https://github.com/matyas-brabec/cellato/issues)!

## 📝 License

This project is released under the [MIT License](./LICENSE).
