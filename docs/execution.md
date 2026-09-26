# Execution

Execution combines an automaton config, a layout/evaluator pair, and a traverser. Cellato does not construct arbitrary combinations at runtime. Instead, the CLI dispatches into a catalog of precompiled suites from [`include/cellato/experiments/test_suites.hpp`](../include/cellato/experiments/test_suites.hpp).

This keeps runtime dispatch simple and keeps CUDA explicit instantiations aligned with the variants the CLI can actually run.

## Runtime Selection

These CLI options identify a suite:

- `--automaton`
- `--device`
- `--traverser`
- `--evaluator`
- `--layout`
- `--word_size` for packed layouts
- `--x_tile_size` and `--y_tile_size` for CUDA spatial blocking
- `--temporal_steps` and `--temporal_tile_size_y` for CUDA temporal traversal

The suite matcher compares those values with static names in the suite catalog. If a packed layout is selected, `--word_size` must match the compiled word type. If spatial blocking is selected, tile sizes must match one of the compiled tile variants. See [Layouts](layouts.md) for the matching evaluator/layout pairs.

## Invalid CLI Options

Invalid runs print a specific error to standard error and exit with status `1`. The CLI distinguishes unknown names from supported names used in an unsupported combination. Available names and combinations come from the compiled suite catalog, so the suggestions match the current binary.

Examples:

```text
Error: Unknown --traverser 'standard'. Available values: simple, spatial_blocking, temporal.
Error: Unsupported combination: --automaton game-of-life --device CPU --traverser temporal. Supported --traverser values for --automaton game-of-life --device CPU: simple.
Error: Unsupported --word_size 16 for --evaluator bit_planes --layout bit_planes. Available values: 32, 64.
```

Diagnostics also identify missing suite-specific options, unsupported spatial tile pairs, temporal values absent from the current build, and invalid numeric ranges or grid divisibility. Grid-size errors report the required multiple in logical cells, accounting for packed words, CUDA blocks, and temporal halos.

Temporal option sets are shared between validation and kernel dispatch in [`temporal_options.hpp`](../include/cellato/traversers/cuda/temporal_options.hpp). Use `--help` to list the recognized names.

## CPU Simple Traversal

CLI:

```text
--device CPU --traverser simple
```

CPU simple traversal supports:

| Evaluator | Layout | Word sizes |
| --- | --- | --- |
| `standard` | `standard` | none |
| `bit_array` | `bit_array` | `32`, `64` |
| `bit_planes` | `bit_planes` | `32`, `64` |
| `tiled_bit_planes` | `tiled_bit_planes` | `32`, `64` |

Use this path for small correctness checks, baseline measurements, and direct-library examples.

When `--print` is enabled, `experiment_manager` converts the current grid to the standard representation and prints it after initialization and after each callback step. This is intended for small grids.

## CUDA Simple Traversal

CLI:

```text
--device CUDA --traverser simple
```

CUDA simple traversal supports:

| Evaluator | Layout | Word sizes |
| --- | --- | --- |
| `standard` | `standard` | none |
| `bit_array` | `bit_array` | `32`, `64` |
| `bit_planes` | `bit_planes` | `32`, `64` |
| `tiled_bit_planes` | `tiled_bit_planes` | `32`, `64` |

This traversal evaluates one time step at a time and does not aggregate temporal work. It is the main CUDA correctness and comparison path.

Example:

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
  --steps 100
```

## CUDA Spatial Blocking

CLI:

```text
--device CUDA --traverser spatial_blocking --evaluator standard --layout standard
```

Runtime-visible spatial-blocking tile variants:

| `--y_tile_size` | `--x_tile_size` |
| ---: | ---: |
| `1` | `1` |
| `2` | `1` |
| `4` | `1` |

Spatial blocking currently applies to the CUDA standard layout suite. It changes how work is grouped while preserving the same logical update semantics as simple traversal.

Example:

```sh
build/release/cellato \
  --automaton game-of-life \
  --device CUDA \
  --traverser spatial_blocking \
  --evaluator standard \
  --layout standard \
  --x_tile_size 1 \
  --y_tile_size 2 \
  --x_size 1024 \
  --y_size 1024 \
  --steps 100
```

## CUDA Temporal Traversal

CLI:

```text
--device CUDA --traverser temporal
```

CUDA temporal traversal supports bit-plane layouts:

| Evaluator | Layout | Word sizes |
| --- | --- | --- |
| `bit_planes` | `bit_planes` | `32`, `64` |
| `tiled_bit_planes` | `tiled_bit_planes` | `32`, `64` |

Temporal traversal computes multiple time steps inside a CUDA block. It loads a region with halo cells, computes several steps locally, and writes the correct interior region. The halo must be large enough for `average_halo_radius * temporal_steps`.

Required options:

- `--temporal_steps`
- `--temporal_tile_size_y`
- `--cuda_block_size_x`
- `--cuda_block_size_y`

Current validation also requires:

- `--steps` divisible by `--temporal_steps`;
- `--temporal_tile_size_y` divisible by `--cuda_block_size_y`;
- effective temporal tile dimensions that divide the physical grid;
- `--cuda_block_size_x` consistent with the compiled temporal kernel options.

Default release builds compile a small temporal option set. Verification and benchmark presets compile broader sets; see [Build Reference](build.md). For tuning guidance, see [Performance](performance.md).

## Generated CUDA Instantiations

CUDA traversers are templates. Runtime dispatch needs concrete compiled types, so CMake runs [`tools/generate_cuda_instantiations.py`](../tools/generate_cuda_instantiations.py) during configuration.

The generator reads [`src/automata/registry.hpp`](../src/automata/registry.hpp), then emits generated sources under the active build directory:

```text
build/<preset>/generated/cuda_instantiations/
```

Generated groups:

- `simple_all.cu`: all CUDA simple suites for all registered automata.
- `spatial_blocking_all.cu`: all CUDA spatial-blocking suites for all registered automata.
- `temporal_<automaton>.cu`: temporal suites split per automaton to keep translation units smaller.
- `cuda_instantiations.cmake`: generated source list included by the root CMake project.

CMake marks generated instantiation sources as generated build artifacts. The build tool schedules their objects normally according to `--parallel`; there is no hard sequential temporal chain.

## Public Suite Aliases

Existing direct-library aliases remain available:

```cpp
cellato::run::test_suites::on_cpu::standard<A>
cellato::run::test_suites::on_cpu::using_<std::uint64_t>::bit_planes<A>
cellato::run::test_suites::on_cuda::standard<A>
cellato::run::test_suites::on_cuda::using_<std::uint64_t>::temporal_linear_bit_planes<A>
```

Use aliases when writing a small program. Use CLI options when running the built executable.
