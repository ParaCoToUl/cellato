# Performance

Cellato's performance model is shaped by cellular automata being stencil-like computations with small state sets. A cell update usually reads a local neighborhood, performs a modest amount of logic, and writes one new state. That makes memory traffic, layout, and reuse as important as raw arithmetic throughput.

## Why Encoding Matters

Many cellular automata store far fewer bits of information than a scalar `int` or enum occupies in memory.

| State count | Bits needed | Examples |
| ---: | ---: | --- |
| 2 | 1 | Game of Life, Maze, Critters |
| 3-4 | 2 | Brian's Brain, Forest Fire, Wireworld, Traffic |
| 8 | 3 | Excitable |
| 16 | 4 | Fluid |
| 32 | 5 | Cyclic |

Using full scalar cells is simple, but it can waste bandwidth. Packed representations reduce traffic, but the cost shifts into extraction, insertion, and rule evaluation. The best layout depends on whether the saved memory traffic outweighs the additional bit manipulation.

## Bit Array Tradeoff

Bit arrays pack complete encoded states into machine words. This can be useful when memory traffic dominates and the rule is simple.

The downside is that a logical cell update still tends to work with individual encoded states. Each access needs masks and shifts, and packed writes need insertion logic.

## Bit-Plane Vectorization

Bit planes store one plane per state bit. Instead of unpacking every state, the evaluator can keep data in bit-sliced form and apply logical operations to whole words.

For a 64-bit word, a bitwise operation updates information for 64 cells in parallel. This is SIMD-style vectorization using ordinary integer bitwise instructions. It is especially effective when the DSL rule can be lowered to logical and bitwise operations over planes.

Rules with many state bits, complex arithmetic, or heavy neighborhood counting still cost more. As the number of state bits grows, the number of planes and intermediate expressions grows as well.

## Linear Versus Tiled Bit Planes

Linear bit planes store consecutive row-major cells in each word. Neighbor access is comparatively simple. This layout is usually strongest for single-step bit-plane execution.

Tiled bit planes store small 2D tiles in each word. This reduces halo overhead for temporal blocking because each word covers a compact region of the grid. The cost is more complex neighbor access, especially for diagonal reads.

Practical guidance:

- Prefer linear bit planes as the first optimized CUDA layout.
- Compare tiled bit planes for temporal traversal.
- Expect Moore-neighborhood rules to pay more tiled neighbor-access overhead.
- Expect von Neumann and directional rules to be more favorable to tiled layouts because they avoid diagonal access.

## Temporal Blocking

Temporal blocking computes multiple time steps while data is local to a CUDA block. It reduces global memory traffic by reusing loaded cells, but it needs halo cells around the output region.

The effective output region shrinks as `temporal_steps` grows:

```text
effective region = loaded region - halo on each side
```

The halo size depends on the automaton's average halo radius and the number of aggregated steps. More temporal steps can reduce memory traffic, but also increase redundant computation and shared-memory pressure.

There are two useful tendencies:

- Linear temporal layouts often peak at a small number of temporal steps because the required halo grows quickly.
- Tiled temporal layouts can have sharp changes at tile boundaries, such as 8 steps for an 8 by 8 tile, because adding one more step may require an extra tile of halo.

## Compile-Time Temporal Options

Temporal kernel options are compiled into the binary. The current header [`include/cellato/traversers/cuda/temporal.cuh`](../include/cellato/traversers/cuda/temporal.cuh) selects option sets with compile definitions:

| Build mode | Compile definition | Purpose |
| --- | --- | --- |
| release | none | Small default option set for normal builds. |
| verification | `VERIFICATION_COMPILE` | Options needed by verification runs. |
| benchmark | `BENCHMARK_COMPILE` | Broader benchmark option set. |

If a runtime temporal value was not compiled, dispatch will not find a matching instantiation path.

## Benchmarking Guidance

Use this sequence when comparing variants:

1. Run a small CPU `standard` case.
2. Run `--reference_impl baseline` for the same automaton when available.
3. Compare checksums for CUDA `simple` before measuring temporal variants.
4. Add warmup rounds for CUDA timings.
5. Keep automaton, seed, grid size, and step count fixed while changing one variant dimension.
6. Check divisibility constraints before interpreting failures as performance issues.

Example benchmark-oriented build:

```sh
cmake --preset benchmark
cmake --build --preset benchmark --parallel 4
```

Example release comparison commands:

```sh
build/release/cellato \
  --automaton game-of-life \
  --device CPU \
  --traverser simple \
  --evaluator standard \
  --layout standard \
  --x_size 1024 \
  --y_size 1024 \
  --steps 100 \
  --rounds 3 \
  --warmup_rounds 1
```

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
  --steps 1000 \
  --rounds 5 \
  --warmup_rounds 2
```

Treat results as automaton-specific. State count, rule complexity, and neighborhood shape all affect which layout and traversal combination wins.
