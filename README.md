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
