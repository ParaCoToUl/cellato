# Build Reference

Cellato uses CMake presets. The main build configures C++ and CUDA, generates CUDA explicit-instantiation files, builds the `cellato` CLI, and builds the CPU and CUDA test executables.

## Presets

Release:

```sh
cmake --preset release
cmake --build --preset release --parallel 4
ctest --preset release
```

Artifacts:

- `build/release/cellato`
- `build/release/cellato_tests`
- `build/release/cellato_probability_cuda_tests`
- `build/release/generated/cuda_instantiations/`

Verification:

```sh
cmake --preset verification
cmake --build --preset verification --parallel 4
ctest --preset verification
```

Artifacts are under `build/verification/`. This preset defines `CELLATO_COMPILE_MODE=VERIFICATION`, which enables the temporal options used by verification-oriented runs.

Benchmark:

```sh
cmake --preset benchmark
cmake --build --preset benchmark --parallel 4
```

Artifacts are under `build/benchmark/`. This preset defines `CELLATO_COMPILE_MODE=BENCHMARK`, which compiles a broader temporal option set and can take substantially longer.

## Compile Modes

The cache variable `CELLATO_COMPILE_MODE` controls optional compile definitions:

| Mode | Compile definition | Temporal options |
| --- | --- | --- |
| empty | none | `temporal_steps = {4}`, `temporal_tile_size_y = {32}`, `cuda_block_size_y = {8}` |
| `VERIFICATION` | `VERIFICATION_COMPILE` | `temporal_steps = {4, 8, 12, 20}`, `temporal_tile_size_y = {8, 32}`, `cuda_block_size_y = {2, 4}` |
| `BENCHMARK` | `BENCHMARK_COMPILE` | `temporal_steps = {2, 3, 4, 5, 6, 7, 8, 9, 10, 12, 14, 16, 17, 18, 20, 22, 24}`, `temporal_tile_size_y = {8, 16, 32, 64, 128}`, `cuda_block_size_y = {2, 4, 8, 16}` |

`cuda_block_size_x` is fixed to `32` for temporal kernels.

## CUDA Architectures

The root project defaults:

```cmake
CMAKE_CUDA_ARCHITECTURES=native
```

Override it when configuring:

```sh
cmake --preset release -DCMAKE_CUDA_ARCHITECTURES=90
```

Use this on CI, clusters, cross-compilation environments, or machines where the configure host does not expose the target GPU.

## Generated CUDA Instantiations

During CMake configuration, the root [`CMakeLists.txt`](../CMakeLists.txt) runs:

```text
tools/generate_cuda_instantiations.py
```

Inputs:

- [`src/automata/registry.hpp`](../src/automata/registry.hpp)
- generator templates embedded in [`tools/generate_cuda_instantiations.py`](../tools/generate_cuda_instantiations.py)
- instantiation helpers in [`src/cuda_instantiation/template.cuh`](../src/cuda_instantiation/template.cuh)

Generated outputs:

```text
build/<preset>/generated/cuda_instantiations/simple_all.cu
build/<preset>/generated/cuda_instantiations/spatial_blocking_all.cu
build/<preset>/generated/cuda_instantiations/temporal_<automaton>.cu
build/<preset>/generated/cuda_instantiations/cuda_instantiations.cmake
```

The generated CMake fragment lists generated source paths. CMake includes those paths in the `cellato` executable and marks them as generated. Object build ordering is left to the selected build tool and the `--parallel` value.

To inspect what will be compiled:

```sh
cmake --preset release
sed -n '1,200p' build/release/generated/cuda_instantiations/cuda_instantiations.cmake
```

## Example Project Build

The standalone example has its own presets:

```sh
cmake --preset release -S examples/your_own_ca
cmake --build examples/your_own_ca/build/release --parallel 4
examples/your_own_ca/build/release/your_own_ca
```

The example uses `on_cpu::standard`, so it is the fastest path for learning the direct library API. Its CMake target still finds and links the CUDA runtime because it includes shared Cellato headers.

## Troubleshooting

- If CMake cannot find CUDA, install a CUDA toolkit. The current standalone example also finds and links the CUDA runtime.
- If `native` CUDA architecture detection fails, pass `-DCMAKE_CUDA_ARCHITECTURES=<arch>`.
- If a temporal CLI run rejects an option value, use the preset that compiles that value or add it to the option set in `temporal.cuh`.
- If an automaton is missing from CUDA dispatch, check `src/automata/registry.hpp`, reconfigure CMake, and inspect the generated instantiation sources.
