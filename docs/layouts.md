# Layouts

Layouts control how cell states are encoded in memory. In Cellato, a layout is paired with a matching evaluator because evaluation over ordinary cells, packed states, and bit planes requires different code for efficient execution.

## Standard Layout

Source: [`include/cellato/memory/standard_grid.hpp`](../include/cellato/memory/standard_grid.hpp)

CLI:

```text
--evaluator standard --layout standard
```

The standard grid stores one original state value per cell. It is the easiest representation to inspect and is the preferred baseline for debugging new rules.

Use it when:

- writing a new automaton;
- comparing against a reference implementation;
- printing small grids;
- measuring the abstraction overhead of Cellato without packed encodings.

Tradeoff: for small state sets, memory traffic is much higher than the logical information content of the grid.

## Bit Array Layout

Source: [`include/cellato/memory/bit_array_grid.hpp`](../include/cellato/memory/bit_array_grid.hpp)

CLI:

```text
--evaluator bit_array --layout bit_array --word_size 32
--evaluator bit_array --layout bit_array --word_size 64
```

The bit-array grid packs multiple encoded cell states into one machine word. A state dictionary maps enum values to integer indexes, and the layout stores the minimum number of bits needed for those indexes.

For example:

- 2 states need 1 bit per cell.
- 3 or 4 states need 2 bits per cell.
- 8 states need 3 bits per cell.
- 16 states need 4 bits per cell.
- 32 states need 5 bits per cell.

This reduces memory traffic, but extracting or writing one cell requires masks and shifts. This overhead can outweigh the traffic savings for some automata, especially on optimized CUDA runs.

## Linear Bit Planes

Source: [`include/cellato/memory/bit_planes_grid.hpp`](../include/cellato/memory/bit_planes_grid.hpp)

CLI:

```text
--evaluator bit_planes --layout bit_planes --word_size 32
--evaluator bit_planes --layout bit_planes --word_size 64
```

Bit planes split the encoded state into one dense bit array per state bit. A 64-bit word in one plane contains the same bit position for 64 consecutive cells.

This representation is the key to Cellato's bitwise vectorization. Instead of extracting one state at a time, the evaluator can apply logical operations to whole words. A single bitwise operation therefore acts like a SIMD operation over 32 or 64 cells.

Use linear bit planes when:

- the rule maps well to logical and bitwise operations;
- the automaton has a small or moderate state count;
- you want the strongest default CUDA performance before tuning temporal blocking.

Linear bit planes are usually simpler and faster than tiled bit planes for single-step execution.

## Tiled Bit Planes

Source: [`include/cellato/memory/tiled_bit_planes_grid.hpp`](../include/cellato/memory/tiled_bit_planes_grid.hpp)

CLI:

```text
--evaluator tiled_bit_planes --layout tiled_bit_planes --word_size 32
--evaluator tiled_bit_planes --layout tiled_bit_planes --word_size 64
```

Tiled bit planes also split state bits into planes, but each word stores a small two-dimensional tile:

- 32-bit word: `8 x 4` cells.
- 64-bit word: `8 x 8` cells.

Tiling makes the word shape closer to the neighborhood shape. This can reduce halo overhead when temporal blocking computes multiple time steps inside a CUDA block.

Tradeoff: neighbor access is more complex than in the linear layout. This extra access cost is most visible for diagonal-heavy Moore-neighborhood automata. Tiled layouts are more competitive when the automaton avoids diagonal access, such as von Neumann or directional traffic-like rules, especially with temporal blocking.

## Layout And Evaluator Matrix

The runtime catalog currently exposes matching layout/evaluator pairs only:

| CLI evaluator | CLI layout | Word size | Notes |
| --- | --- | --- | --- |
| `standard` | `standard` | none | Original cell values. |
| `bit_array` | `bit_array` | `32`, `64` | Whole encoded states packed into words. |
| `bit_planes` | `bit_planes` | `32`, `64` | Linear bit planes. |
| `tiled_bit_planes` | `tiled_bit_planes` | `32`, `64` | 2D word tiles in each bit plane. |

Mismatched pairs such as `--evaluator bit_planes --layout bit_array` are not runtime-visible suites.

## Choosing A Layout

Start with this order:

1. Use `standard` while developing a rule.
2. Use `bit_planes` for optimized CUDA runs when the rule is mostly logical.
3. Compare `tiled_bit_planes` when using CUDA temporal traversal or when the neighborhood avoids diagonal access.
4. Use `bit_array` as a compact-storage comparison point, not as the default performance answer.

Always compare checksums against a standard CPU or baseline run before relying on performance measurements. See [Execution](execution.md) for the runtime-visible suite matrix.
