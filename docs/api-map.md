# API Map

This page maps major source locations to responsibilities. It is not a generated API reference; it is a navigation aid for users and contributors.

## Core DSL

| Path | Responsibility |
| --- | --- |
| [`include/cellato/core/ast.hpp`](../include/cellato/core/ast.hpp) | Type-level AST nodes, neighborhood tags, conditional aliases, binary predicate alias `p`, and `current_state`. |
| [`include/cellato/core/probability.hpp`](../include/cellato/core/probability.hpp) | Exact rational sampling with random bit-plane words and reproducible streams across layouts. |
| [`include/cellato/core/vector_int.hpp`](../include/cellato/core/vector_int.hpp) | Integer helpers used by bit-level evaluator implementations. |
| [`include/cellato/utils/type_list.hpp`](../include/cellato/utils/type_list.hpp) | Compile-time type-list utilities used by automata and suite catalogs. |
| [`include/cellato/utils/static_dispatcher.hpp`](../include/cellato/utils/static_dispatcher.hpp) | Static dispatch support for runtime values that must map to compile-time options. |

Public users normally include `ast.hpp`, `experiment_manager.hpp`, and `test_suites.hpp`. The utility headers are mostly implementation support.

## Memory Layouts

| Path | Responsibility |
| --- | --- |
| [`include/cellato/memory/standard_grid.hpp`](../include/cellato/memory/standard_grid.hpp) | Standard one-cell-per-value grid and printing support. |
| [`include/cellato/memory/bit_array_grid.hpp`](../include/cellato/memory/bit_array_grid.hpp) | Packed whole-state grid. |
| [`include/cellato/memory/bit_planes_grid.hpp`](../include/cellato/memory/bit_planes_grid.hpp) | Linear bit-plane grid. |
| [`include/cellato/memory/tiled_bit_planes_grid.hpp`](../include/cellato/memory/tiled_bit_planes_grid.hpp) | Tiled bit-plane grid. |
| [`include/cellato/memory/state_dictionary.hpp`](../include/cellato/memory/state_dictionary.hpp) | Mapping between user states and compact integer indexes. |
| [`include/cellato/memory/interface.hpp`](../include/cellato/memory/interface.hpp) | Device tags and shared grid interface concepts. |

Most users should not manipulate packed grids directly. Use `experiment_manager` with a suite unless you are adding a layout or debugging an evaluator.

## Evaluators

| Path | Responsibility |
| --- | --- |
| [`include/cellato/evaluators/standard.hpp`](../include/cellato/evaluators/standard.hpp) | Evaluates DSL nodes over scalar cell values. |
| [`include/cellato/evaluators/bit_array.hpp`](../include/cellato/evaluators/bit_array.hpp) | Evaluates DSL nodes over packed whole-state words. |
| [`include/cellato/evaluators/bit_planes.hpp`](../include/cellato/evaluators/bit_planes.hpp) | Evaluates DSL nodes over linear bit planes. |
| [`include/cellato/evaluators/tiled_bit_planes.hpp`](../include/cellato/evaluators/tiled_bit_planes.hpp) | Evaluates DSL nodes over tiled bit planes. |

Evaluator support is node-specific. Adding a new AST node means adding evaluator behavior for each layout that should support it.

## Traversers

| Path | Responsibility |
| --- | --- |
| [`include/cellato/traversers/cpu/simple.hpp`](../include/cellato/traversers/cpu/simple.hpp) | CPU traversal interface and implementation. |
| [`include/cellato/traversers/cuda/simple.hpp`](../include/cellato/traversers/cuda/simple.hpp) | CUDA simple traverser declaration. |
| [`include/cellato/traversers/cuda/simple.cuh`](../include/cellato/traversers/cuda/simple.cuh) | CUDA simple kernel/template implementation. |
| [`include/cellato/traversers/cuda/spatial_blocking.hpp`](../include/cellato/traversers/cuda/spatial_blocking.hpp) | CUDA spatial-blocking traverser declaration. |
| [`include/cellato/traversers/cuda/spatial_blocking.cuh`](../include/cellato/traversers/cuda/spatial_blocking.cuh) | CUDA spatial-blocking kernel/template implementation. |
| [`include/cellato/traversers/cuda/temporal.hpp`](../include/cellato/traversers/cuda/temporal.hpp) | CUDA temporal traverser declaration and runtime validation. |
| [`include/cellato/traversers/cuda/temporal.cuh`](../include/cellato/traversers/cuda/temporal.cuh) | CUDA temporal kernel/template implementation and compile-time option sets. |

The `.hpp` files define the traverser types used by suites. The `.cuh` files provide template definitions needed for explicit CUDA instantiation.

## Experiments And CLI

| Path | Responsibility |
| --- | --- |
| [`include/cellato/experiments/test_suites.hpp`](../include/cellato/experiments/test_suites.hpp) | Runtime-visible suite catalog and public suite aliases. |
| [`include/cellato/experiments/experiment_manager.hpp`](../include/cellato/experiments/experiment_manager.hpp) | Builds grids, runs warmup/timed rounds, prints grids, and returns reports. |
| [`include/cellato/experiments/run_params.hpp`](../include/cellato/experiments/run_params.hpp) | Runtime parameter structure and CSV fields. |
| [`include/cellato/experiments/experiment_report.hpp`](../include/cellato/experiments/experiment_report.hpp) | Timing/checksum report shape. |
| [`include/cellato/experiments/reference_impl_manager.hpp`](../include/cellato/experiments/reference_impl_manager.hpp) | Wrapper for reference implementations. |
| [`src/app/main.cpp`](../src/app/main.cpp) | CLI parsing, reference dispatch, and suite dispatch. |

For external programs, `experiment_manager` plus a public suite alias is the most stable entry point.

## Automata

| Path | Responsibility |
| --- | --- |
| [`src/automata/registry.hpp`](../src/automata/registry.hpp) | Single registered automata list via `CELLATO_AUTOMATA(APPLY)`. |
| `src/automata/<name>/config.hpp` | Automaton config with `name`, `algorithm`, `cell_state`, and `state_dictionary`. |
| `src/automata/<name>/algorithm.hpp` | DSL rule definition and state type. |
| `src/automata/<name>/data_init.hpp` | Initial-state generation used by the CLI. |
| `src/automata/<name>/pretty_print.hpp` | Printable state names for small grids. |
| `src/automata/<name>/reference_implementation.hpp` | Baseline/reference implementation hook when available. |

Adding an automaton for the CLI requires adding its config to the registry. Generated CUDA instantiations follow from the registry.

## CUDA Instantiation

| Path | Responsibility |
| --- | --- |
| [`src/cuda_instantiation/template.cuh`](../src/cuda_instantiation/template.cuh) | Explicit-instantiation helper macros. |
| [`tools/generate_cuda_instantiations.py`](../tools/generate_cuda_instantiations.py) | Parses the registry and emits generated `.cu` sources and a CMake fragment. |
| `build/<preset>/generated/cuda_instantiations/` | Generated CUDA instantiation sources for the active build tree. |

The checked-in source tree owns templates and generation logic. The build tree owns generated instantiation translation units.
