# Build Reference

Cellato uses CMake presets. The main build requires C++20 and Python 3 and builds the `cellato` CLI and CPU tests. When CUDA support is enabled, it also generates CUDA explicit-instantiation files and builds GPU traversers and CUDA tests.

## Optional CUDA Support

On the first configuration, `CELLATO_ENABLE_CUDA` defaults to `ON` if CMake detects a CUDA compiler and `OFF` otherwise. The choice is cached for subsequent configurations. Override it explicitly with:

```sh
cmake --preset release -DCELLATO_ENABLE_CUDA=OFF
cmake --build --preset release --parallel 4
ctest --preset release
```

A CPU-only build does not search for the CUDA toolkit, compile `.cu` files, or link the CUDA runtime. All CPU layouts and baseline reference implementations remain available. Requests for `--device CUDA` report that CUDA support is disabled. Set `-DCELLATO_ENABLE_CUDA=ON` to require CUDA; configuration fails if no usable CUDA compiler is found.

For direct header use, ordinary C++ compilation defaults to CPU-only. To use CUDA APIs, define `CELLATO_ENABLE_CUDA=1` consistently in all translation units, supply the CUDA include directories, and link the CUDA runtime. Compilation with `nvcc` defaults to CUDA enabled. Explicitly defining `CELLATO_ENABLE_CUDA=0` disables CUDA APIs even when the toolkit is installed.

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
- `build/release/cellato_probability_cuda_tests` (CUDA builds)
- `build/release/generated/cuda_instantiations/` (CUDA builds)

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

During CMake configuration with CUDA enabled, the root [`CMakeLists.txt`](../CMakeLists.txt) runs:

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

The example uses `on_cpu::standard` and builds with just CMake and a C++20 compiler. It does not require CUDA headers or runtime libraries.

## Troubleshooting

- If CUDA is explicitly enabled and CMake cannot find it, install a CUDA toolkit or configure with `-DCELLATO_ENABLE_CUDA=OFF`. After installing a toolkit, use a fresh build directory or set `CMAKE_CUDA_COMPILER` to avoid a cached failed detection.
- If `native` CUDA architecture detection fails, pass `-DCMAKE_CUDA_ARCHITECTURES=<arch>`.
- If a temporal CLI run rejects an option value, use the preset that compiles that value or add it to the option set in `include/cellato/traversers/cuda/temporal_options.hpp`.
- If an automaton is missing from CUDA dispatch, check `src/automata/registry.hpp`, reconfigure CMake, and inspect the generated instantiation sources.
