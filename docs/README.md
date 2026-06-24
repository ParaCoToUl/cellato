# Cellato Documentation

Cellato is a C++20 library and embedded domain-specific language (DSL) for cellular automata (CA).
Users write an automaton rule as C++ types, then run that rule through different evaluators, memory layouts, and traversers. This allows users to explore the performance tradeoffs of different execution variants without changing the rule itself. To start with a small library program, see the [`examples/your_own_ca`](../examples/your_own_ca/README.md) project.

The central design goal is separation of concerns. A rule should describe the cellular automaton, not how a grid is stored, how a cell update is evaluated, or which CPU/CUDA traversal strategy drives the computation. Those concerns are handled by evaluators, layouts, and traversers provided by the library.

## Start Here

- [Getting Started](getting-started.md): configure, build, run the CLI, and build the custom automaton example.
- [Core Concepts](core-concepts.md): how Algorithm, Evaluator, Layout, Traverser, and Grid fit together.
- [DSL](dsl.md): the type-level rule language, AST nodes, neighborhoods, and custom automata.
- [Layouts](layouts.md): standard grids, bit arrays, bit planes, tiled bit planes, and word-size choices.
- [Execution](execution.md): CPU/CUDA traversers, runtime suite selection, and generated CUDA instantiations.
- [Performance](performance.md): memory traffic, bitwise vectorization, linear/tiled bit planes, and temporal blocking.

## Reference

- [API Map](api-map.md): where major public and internal concepts live in the source tree.
- [Bundled Automata](automata.md): built-in automata, CLI names, state counts, and neighborhoods.
- [Build Reference](build.md): CMake presets, CUDA generation, compile modes, and artifacts.

## Current Scope

These pages are plain Markdown. They are aimed first at library users who want to define automata, choose execution variants, and understand the performance tradeoffs. Contributor-facing internals are included where they explain user-visible behavior, especially generated CUDA instantiations and the suite catalog.
