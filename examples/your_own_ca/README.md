# Your Own Cellular Automaton Example

This example is a small standalone program that defines a custom cellular automaton with the Cellato DSL and runs it through `experiment_manager`.

It is the shortest path for learning the direct library API because it avoids the CLI registry and chooses one suite explicitly in C++:

```cpp
using suite = cellato::run::test_suites::on_cpu::standard<your_own_ca::config>;
```

The selected suite is CPU `simple` traversal with the `standard` evaluator and `standard` layout.

## Build and Run

From the repository root:

```sh
cmake --preset release -S examples/your_own_ca
cmake --build examples/your_own_ca/build/release --parallel 4
examples/your_own_ca/build/release/your_own_ca
```

The example CMake project is a C++ target, but it currently still finds and links the CUDA runtime because it includes shared Cellato headers.

## File Layout

| File | Role |
| --- | --- |
| [`main.cpp`](main.cpp) | Defines the automaton, creates run parameters, runs the experiment, and prints the report. |
| [`CMakeLists.txt`](CMakeLists.txt) | Builds the standalone `your_own_ca` executable. |
| [`CMakePresets.json`](CMakePresets.json) | Defines the example's `release` build directory. |

## Automaton Definition

The example automaton uses three states:

```cpp
enum class cell_state {
    state_a,
    state_b,
    state_c
};
```

Each state is lifted into the DSL with `state_constant`:

```cpp
using state_a = state_constant<cell_state::state_a>;
using state_b = state_constant<cell_state::state_b>;
using state_c = state_constant<cell_state::state_c>;
```

The rule uses the current state and the number of `state_a` neighbors in the Moore neighborhood:

```cpp
using is_state_a = p<current_state, equals, state_a>;
using is_state_b = p<current_state, equals, state_b>;

using state_a_count = count_neighbors<state_a, moore_8_neighbors>;
using has_two_a_neighbors = p<state_a_count, equals, constant<2>>;
```

The rule itself is:

```cpp
using rule =
    if_<is_state_a>::then_<
        if_<has_two_a_neighbors>::then_<state_b>::else_<state_a>
    >::elif_<is_state_b>::then_<
        state_c
    >::else_<
        state_a
    >;
```

In words:

- `state_a` becomes `state_b` when exactly two Moore-neighborhood cells are `state_a`; otherwise it stays `state_a`.
- `state_b` becomes `state_c`.
- every other state becomes `state_a`.

## Config Type

The `config` type connects the rule with Cellato:

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

The fields have these roles:

- `name`: report and parameter identity.
- `average_halo_radius`: neighborhood radius used by temporal traversal; `1.0` is correct for the Moore-neighborhood rule.
- `algorithm`: the DSL rule.
- `cell_state`: the original state type.
- `state_dictionary`: state ordering used by packed layouts, even though this example runs the standard layout.

## Running the Experiment

The example creates a fixed `3 x 3` initial state, chooses `3` steps, enables printing, and runs one measured round without warmup:

```cpp
cellato::run::run_params params{
    .automaton = your_own_ca::config::name,
    .device = "CPU",
    .traverser = "simple",
    .evaluator = "standard",
    .layout = "standard",
    .x_size = 3,
    .y_size = 3,
    .steps = 3,
    .rounds = 1,
    .warmup_rounds = 0,
    .print = true};
```

Then it runs:

```cpp
cellato::run::experiment_manager<suite> manager;
manager.set_print_config(your_own_ca::pretty_print::get_config());

const auto report = manager.run_experiment(params, your_own_ca::initial_state());
```

The selected `suite` determines the actual execution type. The string fields in `run_params` are still useful for reports and consistency, but they do not dynamically switch this standalone program to another suite.
