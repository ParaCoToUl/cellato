# Cellato

[![license](https://img.shields.io/badge/license-MIT-blue.svg)](./LICENSE)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://isocpp.org/)
[![CUDA](https://img.shields.io/badge/CUDA-required-76B900.svg)](https://developer.nvidia.com/cuda)

Cellato is a C++20 library and embedded DSL for cellular automata. It lets you describe an automaton rule as a type-level C++ expression, then run that same rule through different evaluators, memory layouts, and traversers.

The library is designed around separation of concerns:

| Component | Role |
| --- | --- |
| Algorithm | Cellular automaton rule written with the DSL. |
| Evaluator | Per-cell or per-word rule evaluation. |
| Layout | Memory representation such as standard cells, bit arrays, or bit planes. |
| Traverser | CPU or CUDA execution strategy. |

This keeps rule definitions independent from performance-oriented choices like packed state storage, bit-plane vectorization, CUDA traversal, and temporal blocking.

## Documentation

Start with [`docs/README.md`](docs/README.md). The documentation covers:

- getting started with the CMake build and CLI;
- defining a custom automaton in [`examples/your_own_ca`](examples/your_own_ca/README.md);
- the DSL nodes and bundled automata;
- layout, evaluator, traverser, and CUDA instantiation details;
- performance guidance for bit arrays, bit planes, tiled bit planes, and temporal blocking.

## Build

Configure and build the default release profile:

```sh
cmake --preset release
cmake --build --preset release --parallel 4
```

Run unit tests:

```sh
ctest --preset release
```

Verification and benchmark builds use separate build directories:

```sh
cmake --preset verification
cmake --build --preset verification --parallel 4
ctest --preset verification
```

```sh
cmake --preset benchmark
cmake --build --preset benchmark --parallel 4
```

## CLI Example

```sh
build/release/cellato \
  --automaton game-of-life \
  --device CPU \
  --traverser simple \
  --evaluator standard \
  --layout standard \
  --x_size 8 \
  --y_size 8 \
  --rounds 1 \
  --warmup_rounds 0 \
  --steps 2 \
  --seed 1
```

This runs the Game of Life automaton on a CPU with a simple traverser, standard evaluator, and standard layout (equivalent to a standard 2D array of cells). The program should end quickly and print a report of the run, producing checksum values `0-0-0-0-0-0-1-0-0-3-1-1-2-0-0-0`.

## Make Your Own Automaton

The [`examples/your_own_ca`](examples/your_own_ca/README.md) directory contains a small application using the Cellato DSL to implement and run a custom cellular automaton.

```sh
cmake --preset release -S examples/your_own_ca
cmake --build examples/your_own_ca/build/release --parallel 4
examples/your_own_ca/build/release/your_own_ca
```

## Publications

Cellato is developed from the ideas in the Cellato DSL paper and the follow-up bit-plane/vectorization work:

- **Cellato: a DSL for Cellular Automata based on C++ Template Meta-programming**
  - Journal of Object Technology, 2026.
  - DOI: [10.5381/jot.2026.25.1.a13](https://doi.org/10.5381/jot.2026.25.1.a13)
  - Artifact: [matyas-brabec/2025-icooolps-cellato](https://github.com/matyas-brabec/2025-icooolps-cellato)

  Cite as: Matyáš Brabec, Jiří Klepl, Martin Kruliš, “Cellato: a DSL for Cellular Automata based on C++ Template Meta-programming”, Journal of Object Technology, Volume 25, no. 1 ( 2026), pp. 1:1-13, doi:10.5381/jot.2026.25.1.a13.

  BibTeX entry for citation:

  ```bibtex
  @article{
    title = {Cellato: a DSL for Cellular Automata based on C++ Template Meta-programming},
    author = {Matyáš Brabec and Jiří Klepl and Martin Kruliš},
    journal = {Journal of Object Technology},
    volume = {25},
    number = {1},
    issn = {1660-1769},
    year = {2026},
    month = mar,
    pages = {1:1-13},
    doi = {10.5381/jot.2026.25.1.a13},
    url = {https://www.jot.fm/contents/issue_2026_01/a13.html},
    note = {ECOOP 2025 Workshops}
  }
  ```

- **Improving Cellular Automata Performance with Bit-Planes Encoding and Bitwise Vectorization**
  - In peer-review for the Parallel Computing journal
  - Artifact: [matyas-brabec/2026-cellato-journal](https://github.com/matyas-brabec/2026-cellato-journal)

If you use Cellato in research, please cite the most recent published paper.

The linked artifacts preserve the paper-specific replication packages with past versions of Cellato.

## License

Cellato is released under the [MIT License](LICENSE).
