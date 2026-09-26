#ifndef CELLATO_CUDA_TEMPORAL_OPTIONS_HPP
#define CELLATO_CUDA_TEMPORAL_OPTIONS_HPP

#include "cellato/memory/idx_type.hpp"
#include <utility>

namespace cellato::traversers::cuda::temporal::options {

using idx_type = cellato::memory::idx_type;
using block_size_x = std::integer_sequence<idx_type, 32>;

// Shared by kernel instantiation and CLI validation so diagnostics always
// describe the options compiled into this binary.
#if defined(BENCHMARK_COMPILE)
using time_steps = std::integer_sequence<idx_type, 2, 3, 4, 5, 6, 7, 8, 9, 10, 12, 14, 16, 17, 18, 20, 22, 24>;
using tile_size_y = std::integer_sequence<idx_type, 8, 16, 32, 64, 128>;
using block_size_y = std::integer_sequence<idx_type, 2, 4, 8, 16>;
#elif defined(VERIFICATION_COMPILE)
using time_steps = std::integer_sequence<idx_type, 4, 8, 12, 20>;
using tile_size_y = std::integer_sequence<idx_type, 8, 32>;
using block_size_y = std::integer_sequence<idx_type, 2, 4>;
#else
// Default release options; edit these aliases to customize the compiled set.
using time_steps = std::integer_sequence<idx_type, 4>;
using tile_size_y = std::integer_sequence<idx_type, 32>;
using block_size_y = std::integer_sequence<idx_type, 8>;
#endif

} // namespace cellato::traversers::cuda::temporal::options

#endif // CELLATO_CUDA_TEMPORAL_OPTIONS_HPP
