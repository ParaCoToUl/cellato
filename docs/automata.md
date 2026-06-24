# Bundled Automata

Cellato includes 10 automata used for examples, verification, and performance comparisons. This set matches the current [`src/automata/registry.hpp`](../src/automata/registry.hpp).

| Automaton | CLI name | Registry short name | States | Neighborhood | Notes |
| --- | --- | --- | ---: | --- | --- |
| Conway's Game of Life | `game-of-life` | `game_of_life` | 2 | Moore | Classic birth/survival rule over live-neighbor counts. |
| Forest Fire | `forest-fire` | `fire` | 4 | Von Neumann | Deterministic tree, fire, ash, and empty transitions. |
| Wireworld | `wire` | `wire` | 4 | Moore | Circuit-like electron head, tail, conductor, and empty states. |
| Excitable / Greenberg-Hastings | `excitable` | `excitable` | 8 | Moore | One quiescent state, one excited state, and six refractory states. |
| Brian's Brain | `brian` | `brian` | 3 | Moore | Dead cells become alive with exactly two alive neighbors; alive cells decay through dying. |
| Maze | `maze` | `maze` | 2 | Moore | Wall/empty automaton that tends toward maze-like stable structures. |
| Fluid / HPP | `fluid` | `fluid` | 16 | Von Neumann | Integer state encodes particle-direction bits. |
| Critters | `critters` | `critters` | 2 | Margolus block | Reversible block automaton using alternating Margolus neighborhoods. |
| Cyclic | `cyclic` | `cyclic` | 32 | Moore | State advances modulo 32 when a successor state appears nearby. |
| Traffic / BML-style | `traffic` | `traffic` | 3 | Custom directional | Alternates red and blue car movement directions. |

## Source Layout

Each automaton directory may contain:

- `algorithm.hpp`: state type and DSL rule.
- `config.hpp`: public config consumed by suites and the registry.
- `data_init.hpp`: random or structured initialization for CLI runs.
- `pretty_print.hpp`: standard-grid print mapping.
- `reference_implementation.hpp`: reference implementation type.
- `reference_impl.*.cu`: CUDA reference implementation source where needed.

The required minimum for direct library use is smaller: a config with `algorithm`, `cell_state`, `state_dictionary`, `name`, and `average_halo_radius`, plus an initial state supplied by the caller.

## Registry

The registry uses a structured macro:

```cpp
#define CELLATO_AUTOMATA(APPLY) \
    APPLY(game_of_life, game_of_life::config) \
    APPLY(fire, fire::config) \
    ...
```

The same list derives:

- `cellato::automata::all`, the C++ type list used by runtime suite dispatch;
- generated CUDA instantiation inputs;
- the effective list of automata supported by the CLI.

To add a bundled automaton, add the automaton implementation and one registry entry. If it should support CUDA runtime dispatch, the generated instantiation files will include it automatically on the next CMake configure.

## Notes On Names

The CLI matches `config::name`, not the registry short name. For example, the registry short name is `fire`, but the CLI automaton name is `forest-fire`.

Reference implementations use `--reference_impl baseline` and are not part of the layout/evaluator/traverser matrix.
