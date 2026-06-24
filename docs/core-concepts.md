# Core Concepts

Cellato is built around five concepts: *Algorithm*, *Evaluator*, *Layout*, *Traverser*, and *Grid*.

The important rule for users is simple: the automaton rule should be independent of the memory representation and execution backend. That lets the same rule run on the CPU, on CUDA, with standard cells, or with packed bit-level layouts.

## Algorithm

An Algorithm is the type-level expression that defines one cell update. It is a type alias built from nodes in [`include/cellato/core/ast.hpp`](../include/cellato/core/ast.hpp).

Example shape:

```cpp
enum class cell_state { dead, alive };

using alive = cellato::ast::state_constant<cell_state::alive>;
using dead = cellato::ast::state_constant<cell_state::dead>;
using alive_count = cellato::ast::count_neighbors<alive, cellato::ast::moore_8_neighbors>;
using has_three_alive_neighbors = cellato::ast::p<alive_count, cellato::ast::equals, cellato::ast::constant<3>>;

using rule =
    cellato::ast::if_<has_three_alive_neighbors>::then_<alive>::else_<dead>;
```

Real rules usually use nested conditionals using the `cellato::ast::if_<condition>::then_<then_case>::else_<else_case>` node. The nested nodes (`condition`, `then_case`, and `else_case`) can contain arbitrarily complex expressions, including other conditionals.

## Evaluator

An Evaluator applies an Algorithm to one logical update position. The evaluator is where the type-level AST becomes executable code.

Current evaluator families:

- `standard`: evaluates rules over ordinary cell values.
- `bit_array`: evaluates rules over packed states in machine words.
- `bit_planes`: evaluates rules over linear bit-plane words.
- `tiled_bit_planes`: evaluates rules over two-dimensional bit-plane word tiles.

They follow a compile-time visitor pattern. Instead of walking an AST dynamically, the evaluator uses C++ template specialization and inlining. Unsupported nodes issue an error at compile time.

## Layout

A Layout defines the memory representation of the grid. It controls how logical cell coordinates map to stored data.

Current layout families:

- `standard`: stores original cell values.
- `bit_array`: packs whole encoded states into 32-bit or 64-bit words.
- `bit_planes`: stores each bit of the encoded state in a separate linear plane.
- `tiled_bit_planes`: stores each state bit in planes, but groups cells into small two-dimensional word tiles.

Compact layouts are motivated by the fact that many cellular automata have small finite state sets. For example, Game of Life needs one bit per cell; Forest Fire and Wireworld need two.

## Traverser

A Traverser drives execution over the grid. It determines the scheduling strategy: CPU loop, CUDA kernel launch, spatial blocking, or temporal blocking.

Current user-visible traversers:

- CPU `simple`: host traversal.
- CUDA `simple`: one-step CUDA traversal.
- CUDA `spatial_blocking`: CUDA traversal that groups small spatial tiles for the standard layout.
- CUDA `temporal`: CUDA traversal that aggregates multiple time steps for bit-plane layouts.

The traverser should not contain automaton-specific rule logic. It invokes the evaluator for the selected suite.

## Grid

A Grid is the concrete data container for a layout. It stores CPU data, CUDA data, or both conversion paths depending on the layout.

Grid responsibilities include:

- allocate storage for the selected layout;
- initialize from standard cell values;
- convert back to a standard representation for printing and checksums;
- expose layout-specific access used by evaluators and traversers;
- transfer CUDA-backed storage when needed.

The grid is usually not used directly by the user. Instead, an appropriate grid is usually constructed by the `experiment_manager` from a runtime suite (see the next section) and input vector.

## How The Pieces Compose

A *runtime suite* combines:

```text
automaton config + layout traits + evaluator + traverser
```

The automaton config supplies the Algorithm and state information. The layout traits choose the Grid and Evaluator. The traverser chooses the execution driver. Runtime CLI options are matched against this catalog; if no suite matches, the run is rejected instead of constructing arbitrary combinations at runtime.

Continue with [Layouts](layouts.md) for storage choices and [Execution](execution.md) for the catalog and CLI-facing combinations.
