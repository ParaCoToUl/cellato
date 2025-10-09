# Improving Cellular Automata Performance with Bit-Planes Encoding and Bitwise Vectorization 🔍

[![license](https://img.shields.io/badge/license-MIT-blue.svg)](./LICENSE) [![doi](https://img.shields.io/badge/DOI-TODO-blue)](https://doi.org/TODO)

This repository accompanies paper:

```bibtex
@article{
  TODO
}
```

## 🚀 Overview

Cellular automata (CA) are discrete computational models widely used to simulate complex systems through simple, localized rules. While CA are primarily valued for their ability to visualize complex phenomena, certain simulations, such as traffic models or electrical circuits, demand high-performance processing to be practical. Being a special case of stencil computations, CA are well-suited for data parallel models and can benefit from both vectorization and GPU offloading. However, the performance optimization of CA has not been thoroughly explored, leaving many open questions. In this paper, we propose a bit-plane data encoding of CA cell states that enables efficient bitwise vectorization and works well with well-known optimizations like temporal blocking. We implemented 10 different CA using this technique to demonstrate the versatility of our approach and performed extensive evaluation. Furthermore, our implementation took advantage of Cellato abstraction, which allows simple CA rule definitions using C++ templates while abstracting away the complex implementation details. Our approach offers speedup of two orders of magnitude (comparing baseline and optimized CUDA implementations) whilst maintaining code simplicity and ease of use for the end users.

---

## 📂 Repository Structure

```text
.
├── LICENSE
├── README.md
├── include/             ← Cellato library
├── src/
│   ├── game_of_life/    ← Game of Life example
│   ├── fire/            ← Forest Fire example
│   ├── wire/            ← Wireworld example
│   ├── greenberg/       ← Greenberg–Hastings example
│   ├── .../             ← ... remaining 6 automata
│   └── _scripts/        ← Benchmark & plotting scripts
└── results/             ← Benchmark outputs (CSV, PNG, PDF)
```

### 🔍 Implemented Automata

| Automaton              | Key in paper | Description                                             | Implementation Directory                 |
| ---------------------- | ------------ | ------------------------------------------------------- | ---------------------------------------- |
| **Game of Life**       | `GoL`        | Conway’s binary grid (Moore neighborhood)               | [`src/game_of_life`](./src/game_of_life) |
| **Forest Fire**        | `fire`       | Spread of forest fire simulation (von Neumann)          | [`src/fire`](./src/fire)                 |
| **WireWorld**          | `wire`       | Digital circuit simulator (4 states)                    | [`src/wire`](./src/wire)                 |
| **Greenberg–Hastings** | `excitable`  | Excitable medium with refractory states                 | [`src/greenberg`](./src/greenberg)       |
| **Maze**               | `GoL`        | Maze generating CA                                      | [`src/maze`](./src/maze)                 |
| **Brian's Brain**      | `brian`      | Gama of life cousin with 3 states                       | [`src/brian`](./src/brian)               |
| **Cyclic**             | `cyclic`     | Modeling of excitable medium (32 states)                | [`src/cyclic`](./src/cyclic)             |
| **Fluid Simulation**   | `fluid`      | The Hardy–Pomeau–Pazzis (HPP) model                     | [`src/hpp`](./src/hpp)                   |
| **Critters**           | `critters`   | Reversible automaton with a Margolus block neighborhood | [`src/greenberg`](./src/greenberg)       |
| **Traffic**            | `traffic`    | A traffic simulation using 2 different cars             | [`src/traffic`](./src/traffic)           |

## 🛠️ Core Cellato Components

All core headers live in [`include/`](./include/). Key components:

| Component| Header    |
| -------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **AST nodes**  | [`include/core/ast.hpp`](./include/core/ast.hpp)    |
| **Evaluators** | [`include/evaluators/standard.hpp`](./include/evaluators/standard.hpp) • [`bit_array.hpp`](./include/evaluators/bit_array.hpp) • [`bit_planes.hpp`](./include/evaluators/bit_planes.hpp) • [`tiled_bit_planes.hpp`](./include/evaluators/tiled_bit_planes.hpp)|
| **Memory layouts**   | [`include/memory/standard_grid.hpp`](./include/memory/standard_grid.hpp) • [`bit_array_grid.hpp`](include/memory/bit_array_grid.hpp) • [`bit_planes_grid.hpp`](./include/memory/bit_planes_grid.hpp) • [`tiled_bit_planes_grid.hpp`](./include/memory/tiled_bit_planes_grid.hpp)|
| **Traversers (iteration)** | CPU: [`traversers/cpu/simple.hpp`](./traversers/cpu/simple.hpp)<br>CUDA: `traversers/cuda/simple.{hpp,cu}` [.hpp](./include/traversers/cuda/simple.hpp) [.cu](./include/traversers/cuda/simple.cu), `…/temporal.{hpp,cu}` [.hpp](./include/traversers/cuda/temporal.hpp) [.cu](./include/traversers/cuda/temporal.cu) |

## 📖 Tutorial

### 🔧 Prerequisites

* **Compiler:** GCC 15.2.0
* **CUDA:** NVCC V13.0.88

### Compilation

```bash
# Clone & enter
git clone (REMOVED TO PRESERVE AUTHOR ANONYMITY REMOVED TO PRESERVE AUTHOR ANONYMITY DURING REVIEW)
cd cellato

# Build Cellato and the `baseline` reference implementation
(cd src && make)

# Run the CLI test harness
./bin/cellato <options>
```

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

# Game of Life on CUDA with bit-planes
./bin/cellato \
  --automaton game-of-life \
  --device CUDA \
  --traverser simple \
  --evaluator bit_planes \
  --layout bit_planes \
  --precision 32
  --x_size 4096 --y_size 4096 \
  --steps 1000 \
  --cuda_block_size_x 32 --cuda_block_size_y 8
```

---

## ⚙️ CLI Options Explained

```bash
Usage: ./cellato [options]
Options:
  --automaton <name>           Name of the automaton to run (game-of-life, forest-fire, wire, greenberg-hastings)
  --device <CPU|CUDA>          Execution device
  --traverser <name>           Traversal strategy (simple, temporal)
  --evaluator <name>           Evaluator type (standard, bit_array, bit_planes, tiled_bit_planes)
  --layout <name>              Memory layout (standard, bit_array, bit_planes, tiled_bit_planes)
  --reference_impl <name>      Run reference implementation (baseline)
  --x_size <N>                 Grid width
  --y_size <N>                 Grid height
  --rounds <N>                 Number of benchmarking rounds
  --warmup_rounds <N>          Number of warmup rounds
  --steps <N>                  Number of CA time steps
  --precision <32|64>          Word precision used by the `bit array`, `bit planes` and `tiled_bit_planes`
  --seed <N>                   RNG seed for initialization
  --print                      Print grid state after each step
  --print_csv_header           Emit CSV header line
  --cuda_block_size_x <N>      CUDA block X dimension (default: 32)
  --cuda_block_size_y <N>      CUDA block Y dimension (default: 8)
  --temporal_steps             Number of temporal steps for the temporal implementation
  --temporal_tile_size_y       Height of temporal block (Width is fixed to 32)
  --help                       Show this help message
```


### ✨ Supported Evaluator / Layout / Traverser Combinations

Note that that a specific implementation is uniquely identified by the triplet of options `--traverser`, `--evaluator` and `--layout`. However not arbitrary combination is allowed. For the CUDA (`--device CUDA`) and `32`/`64`-bit precision (`--precision (32|64)`) we have implemented the following.

| Implementation | Traverser | Evaluator | Layout | Notes |
| --- | --- | --- | --- | --- |
| Baseline | | | | As baseline does not uses a Cellato it has none of the mention options set. Instead it is invoked with the option `--reference_impl baseline`. |
| Cellato Standard | `simple` | `standard` | `standard` | The implementation uses a the standard encoding to represent states of an automaton (`enum type`) and the CUDA kernel evaluates one cell per CUDA thread. |
| Bit Packed Representation | `simple` | `bit_array` | `bit_array` | This implementation uses a well known "bit packing" technique i.e. many states are packed into one machine word. The number that fit depends both on the `--automaton` in question and the `--precision` of the machine word used. |
| Linear Bit Planes | `simple` | `bit_planes` | `bit_planes` | An integer representation of a cell state is split into independent "bit planes". These cells are then processed in vectorized manner (for details see the affiliated paper). |
| Tiled Bit Planes | `simple` | `tiled_bit_planes` | `tiled_bit_planes` | Similar to the Linear Bit Planes, but one machine word does not encode a row of consecutive cells - as in the previous case - but a small 8x8 (or 8x4) tile. |
| Temporal Bit Planes (Linear) | `temporal` | `bit_planes` | `bit_planes` | Implementation of temporal blocking. The number of steps is set using `--temporal_steps` option and the size of temporal block is set using `--temporal_tile_size_y` - the `x` dimension of the temporal block is fixed to `32`. |
| Temporal Bit Planes (Tiled) | `temporal` | `tiled_bit_planes` | `tiled_bit_planes` | Same as the above but uses the 8x8 (or 8x4) machine word tiles. |

#### ⚠️🚫 Limitations

| Implementation | Limitation Description |
| --- | --- |
| Baseline | The grid X (`--x_size`) and Y (`--y_size`) dimensions **must** be be divisible by the CUDA thread block `X` (`--cuda_block_size_x`) and `Y` (`--cuda_block_size_y`) respectively. |
| Cellato standard | Same limitations as for *Baseline* |
| Bit Packed Representation | Defining the `k` as a number of cells in a machine word (`k = precision // bits_per_automaton_state`).<br> The grid size `X` (`--x_size`) **must** be divisible by the thread block size `X` (`--cuda_block_size_x`) times `k`. <br> The grid size `Y` (`--y_size`) **must** be divisible by the thread block size `Y` (`--cuda_block_size_y`). |
| Linear Bit Planes | The grid size `X` (`--x_size`) **must** be divisible by the `--precision` times cuda thread block block size `X` (`--cuda_block_size_x`). <br> The grid size `Y` (`--y_size`) **must** be divisible by the thread block size `Y` (`--cuda_block_size_y`). |
| Tiled Bit Planes | The grid size `X` (`--x_size`) **must** be divisible by `8` times cuda thread block block size `X` (`--cuda_block_size_x`). <br> The grid size `Y` (`--y_size`) **must** be divisible by the thread block size `Y` (`--cuda_block_size_y`) times `4` or `8` for `--precision 32` and `--precision 64` respectively. |
| Temporal Bit Planes (Linear) | Defining the `effective_temporal_size_Y` as `--temporal_tile_size_y - 2 * --temporal_steps`. <br> The grid size `X` (`--x_size`) **must** be divisible by `30` (which is warp size `32` minus the halo of `2`) times `--precision`. <br> The grid size `Y` (`--y_size`) **must** be divisible by `effective_temporal_size_Y`. <br> The `--temporal_tile_size_y` **must** be divisible by the thread block size `Y` (`--cuda_block_size_y`). <br> The total simulation steps (`--steps`) must be divisible by the `--temporal_steps`. |
| Temporal Bit Planes (Tiled) | Defining the `effective_temporal_size_X` as `32 - 2 * word_halo` where `word_halo_x` is `ceil(8 / --time_steps)`. <br> Defining the `effective_temporal_size_Y` as `--temporal_tile_size_y - 2 * word_halo` where `word_halo_x` is `ceil(8 / --time_steps)` for `--precision 64` and `ceil(4 / --time_steps)` for `--precision 32`.  <br> The grid size `X` (`--x_size`) **must** be divisible by `effective_temporal_size_X` <br> The grid size `Y` (`--y_size`) **must** be divisible by `effective_temporal_size_Y` <br> The `--temporal_tile_size_y` **must** be divisible by the thread block size `Y` (`--cuda_block_size_y`). <br> The total simulation steps (`--steps`) must be divisible by the `--temporal_steps` |

#### 🧩 Examples for each combination

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

# ▶️ Bit-planes layout + evaluator (32-bit)
./bin/cellato \
  --evaluator bit_planes \
  --layout bit_planes \
  --precision 32 \
  --traverser simple \
  [other options…]

# ▶️ Tiled bit-planes layout + evaluator (32-bit)
./bin/cellato \
  --evaluator tiled_bit_planes \
  --layout tiled_bit_planes \
  --precision 32 \
  --traverser simple \
  [other options…]

# ▶️ Temporal bit-planes layout + evaluator (32-bit)
./bin/cellato \
  --evaluator bit_planes \
  --layout bit_planes \
  --precision 32 \
  --traverser temporal \
  [other options…]

# ▶️ Temporal tiled bit-planes layout + evaluator (32-bit)
./bin/cellato \
  --evaluator tiled_bit_planes \
  --layout tiled_bit_planes \
  --precision 32 \
  --traverser temporal \
  [other options…]

```

---

## 📊 Scripts & Benchmarks

Reproduce paper results via scripts in `src/_scripts/`.

```bash
# Generate raw CSV data
python ./src/_scripts/cluster_run/run_all.py > results.csv

# Detailed table in ASCI
python src/_scripts/table-producers/show_result_table.py results.csv

# HTML Detailed table - includes also the best hyper params information
python src/_scripts/table-producers/to_html_report.py results.csv > report.html

# Graphs from the paper (last argument is the name of the output file {.png | .pdf})
python src/_scripts/graph-producers/0_total_perf.py      results.csv   total_perf.pdf
python src/_scripts/graph-producers/1_linear_vs_tiled.py results.csv   linear_vs_tiled.pdf
python src/_scripts/graph-producers/2_time_steps.py      results.csv   time_steps.pdf
```

### ✅ Verification

Due to limitation discussed earlier the grid sizes cannot be precisely same in each of the benchmarks. Consequently the check sums produced by the `run_all.py` cannot be compared. For the test of the validity use a script [./src/_scripts/cluster_run/verify.py](./src/_scripts/cluster_run/verify.py):

``` bash
$> python ./src/_scripts/cluster_run/verify.py

Running all automata
Automata to test: ['critters', 'traffic', 'hpp', 'game-of-life', 'cyclic', 'brian', 'maze', 'forest-fire', 'wire', 'greenberg-hastings']
Starting correctness validation...
--------------------------------------------------------------------------------

--- Automaton: critters ---
[  OK   ] Standard                  | Automaton: critters, Eval: standard, Layout: standard
[  OK   ] BitArray                  | Automaton: critters, Eval: bit_array, Layout: bit_array, Prec: 32
[  OK   ] BitPlanes                 | Automaton: critters, Eval: bit_planes, Layout: bit_planes, Prec: 32
...
```

---

## 📈 Results

We have performed an extensive testing of out bit planes methods. The results are in the [./results](./results/) folder which contain two sub-folders for the H100 GPU and A100 GPU. The respective `.csv` contains full measurements ([_h100.csv](./results/H100/_h100.csv), [_a100.csv](./results/A100/_a100.csv)). Both subdirectories also contain graphs generated from the respective `.csv` files and the `.html` report tables. Lastly, the [./results/H100/grid-search-results.csv](./results/H100/grid-search-results.csv) contain data from the initial grid search.

---

<!-- ## 🔭 Future Work

* **Higher-dimensional grids:** 3D+ support
* **Non-rectangular topologies:** hexagonal, triangular
* **Probabilistic CAs:** introduce random-node AST types
* **Advanced traversers:** temporal blocking, NUMA-aware scheduling
* **Distributed execution:** MPI-based traverser with halo exchange
* **Framework integration:** embed Cellato evaluators into Kokkos/GridTools

--- -->

## 📝 License

This project is released under the [MIT License](./LICENSE).
