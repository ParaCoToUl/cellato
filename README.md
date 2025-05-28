# Cellular Beauty: A DSL for Cellular Automata 🧬🔍

[![license](https://img.shields.io/badge/license-MIT-blue.svg)](./LICENSE) [![doi](https://img.shields.io/badge/DOI-TBD-blue)](https://doi.org/TBD)

This repository is associated with the following paper:  

```
@article{brabec2023cellato,
  title={Cellato: a DSL for Cellular Automata based on C++ Template Meta-programming},
  author={Brabec, Maty\'{a}\v{s} and Klepl, Ji\v{r}\'{\i} and Kruli\v{s}, Martin},
  journal={TBD},
  year={2023},
  publisher={TBD}
}
```

![Cellular Automata Examples](./ca-examples.gif)

## About  

Cellular automata (CA) are powerful modeling tools used across scientific disciplines, but their implementations often tightly couple algorithm logic, evaluation strategies, and memory layouts. **Cellato** addresses this limitation with a flexible, embedded C++ DSL that leverages template metaprogramming to express CA rules concisely. By cleanly separating four orthogonal concerns—Algorithm (rule), Evaluator (cell update), Layout (memory organization), and Traversor (iteration strategy)—Cellato enables users to experiment with different optimizations without modifying their rule logic.

Cellato provides:
- A type-level expression language for defining CA rules
- Support for diverse memory layouts (standard arrays, bit-packed arrays, bit-plates)
- Platform independence (CPU, CUDA) with the same rule definition
- Efficient evaluators specialized for each layout
- Zero overhead abstractions that match handwritten kernel performance

## Index  

The repository includes implementations of four canonical cellular automata and various optimizations:

| Component | Description | Link |  
|-----------|-------------|------|  
| **Game of Life** | Conway's Game of Life implementation using Cellato DSL | [Link](./src/game_of_life/algorithm.hpp) |  
| **Forest Fire** | Forest Fire model with von Neumann neighborhood | [Link](./src/fire/config.hpp) |  
| **Wireworld** | Digital circuit simulator with four states | [Link](./src/wire/config.hpp) |  
| **Greenberg-Hastings** | Excitable medium with multiple refractory states | [Link](./src/greenberg/config.hpp) |
| **Standard Layout** | Simple contiguous array layout | [Link](./src/memory/standard_grid.hpp) |
| **Bit Array Layout** | Bit-packed representation for efficient storage | [Link](./src/memory/bit_array_grid.hpp) |
| **Bit Plates Layout** | SOA-style bit planes for SIMD optimization | [Link](./src/memory/bit_plates_grid.hpp) |
| **CPU Traverser** | Sequential and parallel CPU traversal | [Link](./src/traversers/cpu_traverser.hpp) |
| **CUDA Traverser** | GPU execution with flexible tiling | [Link](./src/traversers/cuda_traverser.hpp) |
| **Framework Comparison** | Benchmarks against Kokkos, GridTools, Halide, AN5D | [Link](./src/_relwork/) |

## Tutorial  

### Prerequisites

- C++17 compatible compiler (GCC 9+ recommended)
- CUDA toolkit 11.0+ (for GPU support)
- CMake 3.15+

### Compilation

```bash
# Clone the repository
git clone https://github.com/matyas-brabec/cellato
cd cellato

# Build
make

# Run tests (optional)
make test
```

### Running Examples

The main executable provides a command-line interface for running cellular automata simulations:

```bash
# Run Game of Life with standard layout on CPU
./bin/cellib --automaton game-of-life --device CPU --traverser simple --evaluator standard --layout standard --x_size 256 --y_size 256 --steps 100

# Run Game of Life with bit array optimization on CUDA
./bin/cellib --automaton game-of-life --device CUDA --traverser spacial_blocking --evaluator bit_array --layout bit_array --x_size 1024 --y_size 1024 --steps 1000 --cuda_block_size_x 32 --cuda_block_size_y 8

# Compare with reference implementations
./bin/cellib --automaton game-of-life --reference_impl kokkos --x_size 2048 --y_size 2048 --steps 1000
```

For analyzing and visualizing results, use the Python scripts in the `src/_scripts` directory:

```bash
# Generate CSV data comparing implementations
./bin/cellib --automaton game-of-life --print_csv_header > results.csv
./bin/cellib --automaton game-of-life --device CPU --traverser simple --evaluator standard --layout standard --x_size 1024 --y_size 1024 --steps 100 >> results.csv
./bin/cellib --automaton game-of-life --device CPU --traverser simple --evaluator bit_array --layout bit_array --x_size 1024 --y_size 1024 --steps 100 >> results.csv

# Create visualization from CSV
cd src/_scripts
python plot_comparison.py ../../results.csv
```

## Defining Your Own Cellular Automaton

Cellato makes it easy to define new cellular automata using type-level expressions:

```cpp
// Define states
enum class my_cell_state { state_a, state_b, state_c };

using state_a = state_constant<my_cell_state::state_a>;
using state_b = state_constant<my_cell_state::state_b>;
using state_c = state_constant<my_cell_state::state_c>;

// Define predicates
using is_state_a = p<current_state, equals, state_a>;
using is_state_b = p<current_state, equals, state_b>;
using is_state_c = p<current_state, equals, state_c>;

// Count neighbors in state_a using Moore neighborhood
using state_a_cnt = count_neighbors<state_a, moore_8_neighbors>;
using has_two_a_neighbors = p<state_a_cnt, equals, constant<2>>;

// Define transition rule
using my_rule =
  if_<is_state_a>::then_<
    if_<has_two_a_neighbors>::then_<state_b>::else_<state_a>
  >::elif_<is_state_b>::then_<
    state_c
  >::else_<
    state_a
  >;

// Create configuration struct
struct my_automaton_config {
  using cell_state = my_cell_state;
  using algorithm = my_rule;
  using reference_implementation = my_reference_impl;
  // ...other settings
};
```

## Results  

Our evaluations compared Cellato against four prominent stencil and DSL frameworks:

| Framework     | Platform Independence | Memory Layout Flexibility | Explicit Vectorization | Optimization |
|---------------|----------------------|---------------------------|------------------------|--------------|
| **Cellato**   | ✅ CPU, CUDA         | ✅ Standard, bit-packed, bit-plates | ✅ Bit-level | ✅ User-defined |
| **Kokkos**    | ✅ CPU, CUDA         | ⚠️ Major order only       | ❌ No                  | ⚠️ Only tiling |
| **GridTools** | ✅ CPU, CUDA         | ⚠️ Major order only       | ❌ No                  | ⚠️ Only caching |
| **Halide**    | ✅ CPU, CUDA         | ⚠️ Major order only       | ⚠️ SIMD only           | ✅ User-defined |
| **AN5D**      | ✅ CPU, CUDA         | ❌ Fixed array layout      | ❌ No                  | ✅ User-defined |

Performance measurements on both CPU and GPU back-ends confirm that Cellato provides zero-overhead abstractions, matching handwritten kernels in throughput while offering significantly more flexibility in memory layout and evaluation strategies.

Detailed benchmark results are available in the `results/` directory.

## Future Work

We are actively working on:
- Support for higher-dimensional grids (3D+)
- Non-rectangular grid topologies (hexagonal, triangular)
- Nondeterministic cellular automata with random transitions
- Advanced stencil optimizations (temporal blocking, cache-aware scheduling)
- MPI integration for distributed execution
- Python frontend for improved usability

## Contact us

If you have any questions regarding the framework, our implementation, or if you have any suggestions, please feel free to contact us by raising [an issue](https://github.com/matyas-brabec/cellular-beauty/issues)!

## License

This source code is licensed under the MIT license.
