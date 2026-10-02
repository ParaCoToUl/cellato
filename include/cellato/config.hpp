#ifndef CELLATO_CONFIG_HPP
#define CELLATO_CONFIG_HPP

// CMake sets this explicitly. Standalone headers enable CUDA only when compiled
// with a CUDA compiler, so ordinary C++ consumers do not need the CUDA toolkit.
#ifndef CELLATO_ENABLE_CUDA
#ifdef __CUDACC__
#define CELLATO_ENABLE_CUDA 1
#else
#define CELLATO_ENABLE_CUDA 0
#endif
#endif

#endif // CELLATO_CONFIG_HPP
