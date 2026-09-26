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

## Probabilistic Predicates

`probability<Numerator, Denominator, Stream = 0>` is a boolean predicate with the
exact rational probability `Numerator / Denominator` of being true for each cell
at each time step. The denominator must be positive and the numerator must be
between zero and the denominator; invalid ratios fail at compile time.

```cpp
using ignite = probability<1, 100, 0>;
using recover = probability<3, 8, 1>;

using stochastic_rule =
    if_<p<current_state, equals, alive>>::then_<
        if_<recover>::then_<dead>::else_<alive>
    >::else_<
        if_<ignite>::then_<alive>::else_<dead>
    >;
```

Use a distinct `Stream` number for each independent random event in a rule. Reusing a stream reuses the same random binary fraction for that cell and time step, even across different ratios: `probability<1, 4, 0>` implies
`probability<1, 2, 0>`. In particular, combining two copies of `probability<1, 2>` with `and_` still has probability `1/2`. Use different streams to obtain independent trials with combined probability `1/4`.

Randomness is determined by `run_params::seed`, logical cell coordinates, time step, and stream. The same inputs reproduce the same events on CPU and CUDA, across all four layouts and supported word sizes. Temporal traversal uses global coordinates, so overlapping halo computations agree. Branch evaluation order does not affect the draws. Direct evaluator users can set `point_in_grid::random_seed` (default `42`); traversal supplies it automatically.
When evaluating a local tile directly, also supply its physical word offset in `random_origin` and the global physical dimensions in `random_domain`.

The bit-plane implementations generate whole words of random bits, one bit per cell, and compare the resulting random binary fractions with the requested ratio using bitwise logic. Finite binary ratios reduce to a fixed AND/OR
expression: `1/4` uses two random words and `3/8` uses three. Ratios such as `1/3` continue drawing words for undecided lanes until all lanes are resolved. This avoids fixed-precision rounding and modulo bias; non-dyadic ratios have variable work with no fixed worst-case draw count. Ratios `0/D` and `D/D` need no random
draws. Tiled bit planes assemble eight-bit row slices from the same random words used by the other layouts.

The ratio describes each cell's probability, not a guaranteed fraction of set bits in each word or generation. Sampling uses a deterministic integer PRNG; exactness refers to the rational sampling algorithm under uniform random bits, not to exact frequencies in a finite simulation.

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
