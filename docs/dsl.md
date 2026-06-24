# DSL

Cellato's DSL is embedded directly in C++. A rule is a C++ type expression, not a function body and not an external language. The compiler sees the whole expression at compile time and can specialize the evaluator for the selected layout and backend.

This follows the zero-overhead direction: encode the rule, neighborhood, and constants as types; use template specialization to generate evaluator code; and change the memory layout or traverser without editing the rule.

## Header

The public AST nodes live in [`include/cellato/core/ast.hpp`](../include/cellato/core/ast.hpp):

```cpp
#include "cellato/core/ast.hpp"
```

Most automata use:

```cpp
using namespace cellato::ast;
```

inside the automaton namespace.

## Constants And State Values

Use `constant<Value>` for numeric or boolean constants and `state_constant<Value>` for automaton state values.

```cpp
enum class cell_state {
    dead,
    alive
};

using alive = state_constant<cell_state::alive>;
using dead = state_constant<cell_state::dead>;
using c_2 = constant<2>;
using c_3 = constant<3>;
```

`state_constant` is important for packed layouts because the state dictionary maps these logical states to compact integer encodings.

## Cell And Neighbor Access

```cpp
using left = neighbor_at<-1, 0>;
using right = neighbor_at<1, 0>;
using up = neighbor_at<0, -1>;
using down = neighbor_at<0, 1>;

using evaluated_cell = current_state;
```

`current_state` is an alias for `neighbor_at<0, 0>`.

Coordinates are relative to the cell currently being evaluated.

## Neighborhoods

Built-in neighborhood tags:

- `moore_8_neighbors`: all eight adjacent cells around the current cell.
- `von_neumann_4_neighbors`: four side-adjacent cells.
- `margolus_alternating_neighborhood`: alternating block neighborhood used by the Critters automaton.
- `margolus_180_neighbor`: paired block cell used by the Critters rule.

Use `count_neighbors<State, Neighborhood>` to count cells matching a state-like expression:

```cpp
using alive_count = count_neighbors<alive, moore_8_neighbors>;
using has_three_alive_neighbors = p<alive_count, equals, c_3>;
```

## Operators

Binary operators are represented as AST nodes:

- logical: `and_`, `or_`
- bitwise: `bit_and_`, `bit_or_`
- arithmetic: `plus`, `modulo`
- comparisons: `equals`, `not_equals`, `greater_than`, `less_than`

Unary nodes:

- `not_`
- `has_bit_set<Value, bit>`

The helper alias `p<Left, Operator, Right>` writes a binary predicate or expression in a compact form:

```cpp
using is_alive = p<current_state, equals, alive>;
using has_two_or_three = p<has_two_alive_neighbors, or_, has_three_alive_neighbors>;
```

## Conditionals

Rules use `if_`, `then_`, `elif_`, and `else_` aliases to build a nested `if_then_else` type.

```cpp
using gol_algorithm =
    if_<is_alive>::then_<
        if_<has_two_or_three>::then_<alive>::else_<dead>
    >::else_<
        if_<has_three_alive_neighbors>::then_<alive>::else_<dead>
    >;
```

The syntax resembles ordinary control flow, but all branches are type expressions. Evaluators decide how to lower those expressions for their layout.

## Alternate Algorithms

`alternate_algorithms<...>` represents automata whose update alternates between multiple rules. The bundled Traffic automaton uses it to alternate red-car and blue-car movement directions:

```cpp
using traffic_algorithm = alternate_algorithms<
    one_direction<red_car, blue_car, left, right>::algorithm,
    one_direction<blue_car, red_car, up, down>::algorithm>;
```

Use this only when the automaton semantics require time-step-dependent rule selection.

## Complete Minimal Config

A direct-library automaton needs a config type. The example in [`examples/your_own_ca/main.cpp`](../examples/your_own_ca/main.cpp) has the minimal shape:

```cpp
struct config {
    static constexpr char name[] = "your-own-ca";
    static constexpr double average_halo_radius = 1.0;

    using algorithm = rule;
    using cell_state = your_own_ca::cell_state;
    using state_dictionary =
        cellato::memory::grids::state_dictionary<
            cell_state::state_a,
            cell_state::state_b,
            cell_state::state_c>;
};
```

Fields:

- `name`: CLI/catalog identity for bundled automata and reports.
- `average_halo_radius`: used by temporal traversal to size halos.
- `algorithm`: the DSL rule type.
- `cell_state`: original state type.
- `state_dictionary`: compact-state mapping used by packed layouts.

Bundled CLI automata also provide initialization, pretty-printing, and reference-implementation hooks. A standalone program can supply its own input vector and print config directly through `experiment_manager`. See [Bundled Automata](automata.md) for the current config layout used by the built-in models.

## When Adding New DSL Nodes

Adding a node type is not enough by itself. Each evaluator that should support the node needs a specialization or implementation path. This is deliberate: a node such as `count_neighbors` can be implemented differently for standard cells, bit arrays, linear bit planes, and tiled bit planes.

If a rule uses a node unsupported by the chosen evaluator, compilation fails for that suite. This keeps unsupported runtime combinations from silently producing incorrect code.
