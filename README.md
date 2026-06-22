# Cellato

[![license](https://img.shields.io/badge/license-MIT-blue.svg)](./LICENSE)

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

## Make your own cellular automaton

The [`examples/your_own_ca`](examples/your_own_ca) directory contains a simple application using the Cellato DSL to implement a cellular automaton.

```sh
cmake --preset release -S examples/your_own_ca
cmake --build examples/your_own_ca/build/release --parallel 4
examples/your_own_ca/build/release/your_own_ca
```
