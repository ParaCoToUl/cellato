#ifndef CELLATO_CUDA_INSTANTIATION_TEMPLATE_CUH
#define CELLATO_CUDA_INSTANTIATION_TEMPLATE_CUH

#include <cstdint>

#include "cellato/experiments/test_suites.hpp"
#include "cellato/traversers/cuda/simple.hpp"
#include "cellato/traversers/cuda/spatial_blocking.hpp"
#include "cellato/traversers/cuda/temporal.hpp"

#define CELLATO_INSTANTIATE_CUDA_SIMPLE_SUITE(...)                                                                     \
    template auto __VA_ARGS__::traverser_t::fetch_result() -> __VA_ARGS__::grid_t;                                     \
    template void __VA_ARGS__::traverser_t::run_kernel<cellato::traversers::cuda::simple::_run_mode::QUIET>(int);      \
    template void __VA_ARGS__::traverser_t::run_kernel<cellato::traversers::cuda::simple::_run_mode::VERBOSE>(int);

#define CELLATO_INSTANTIATE_CUDA_TEMPORAL_SUITE(...)                                                                   \
    template auto __VA_ARGS__::traverser_t::fetch_result() -> __VA_ARGS__::grid_t;                                     \
    template void __VA_ARGS__::traverser_t::run_kernel<cellato::traversers::cuda::temporal::_run_mode::QUIET>(int);    \
    template void __VA_ARGS__::traverser_t::run_kernel<cellato::traversers::cuda::temporal::_run_mode::VERBOSE>(int);

#define CELLATO_INSTANTIATE_CUDA_SPATIAL_BLOCKING_SUITE(...)                                                           \
    template auto __VA_ARGS__::traverser_t::fetch_result() -> __VA_ARGS__::grid_t;                                     \
    template void __VA_ARGS__::traverser_t::run_kernel<cellato::traversers::cuda::spatial_blocking::_run_mode::QUIET>( \
        int);                                                                                                          \
    template void                                                                                                      \
    __VA_ARGS__::traverser_t::run_kernel<cellato::traversers::cuda::spatial_blocking::_run_mode::VERBOSE>(int);

#define CELLATO_INSTANTIATE_CUDA_SIMPLE_SUITES_FOR_AUTOMATON(AUTOMATON)                                                \
    CELLATO_INSTANTIATE_CUDA_SIMPLE_SUITE(cellato::run::test_suites::on_cuda::standard<AUTOMATON>)                     \
    CELLATO_INSTANTIATE_CUDA_SIMPLE_SUITE(                                                                             \
        cellato::run::test_suites::on_cuda::using_<std::uint32_t>::bit_array<AUTOMATON>)                               \
    CELLATO_INSTANTIATE_CUDA_SIMPLE_SUITE(                                                                             \
        cellato::run::test_suites::on_cuda::using_<std::uint64_t>::bit_array<AUTOMATON>)                               \
    CELLATO_INSTANTIATE_CUDA_SIMPLE_SUITE(                                                                             \
        cellato::run::test_suites::on_cuda::using_<std::uint32_t>::bit_planes<AUTOMATON>)                              \
    CELLATO_INSTANTIATE_CUDA_SIMPLE_SUITE(                                                                             \
        cellato::run::test_suites::on_cuda::using_<std::uint64_t>::bit_planes<AUTOMATON>)                              \
    CELLATO_INSTANTIATE_CUDA_SIMPLE_SUITE(                                                                             \
        cellato::run::test_suites::on_cuda::using_<std::uint32_t>::tiled_bit_planes<AUTOMATON>)                        \
    CELLATO_INSTANTIATE_CUDA_SIMPLE_SUITE(                                                                             \
        cellato::run::test_suites::on_cuda::using_<std::uint64_t>::tiled_bit_planes<AUTOMATON>)

#define CELLATO_INSTANTIATE_CUDA_TEMPORAL_SUITES_FOR_AUTOMATON(AUTOMATON)                                              \
    CELLATO_INSTANTIATE_CUDA_TEMPORAL_SUITE(                                                                           \
        cellato::run::test_suites::on_cuda::using_<std::uint32_t>::temporal_tiled_bit_planes<AUTOMATON>)               \
    CELLATO_INSTANTIATE_CUDA_TEMPORAL_SUITE(                                                                           \
        cellato::run::test_suites::on_cuda::using_<std::uint64_t>::temporal_tiled_bit_planes<AUTOMATON>)               \
    CELLATO_INSTANTIATE_CUDA_TEMPORAL_SUITE(                                                                           \
        cellato::run::test_suites::on_cuda::using_<std::uint32_t>::temporal_linear_bit_planes<AUTOMATON>)              \
    CELLATO_INSTANTIATE_CUDA_TEMPORAL_SUITE(                                                                           \
        cellato::run::test_suites::on_cuda::using_<std::uint64_t>::temporal_linear_bit_planes<AUTOMATON>)

#define CELLATO_INSTANTIATE_CUDA_SPATIAL_BLOCKING_SUITES_FOR_AUTOMATON(AUTOMATON)                                      \
    CELLATO_INSTANTIATE_CUDA_SPATIAL_BLOCKING_SUITE(                                                                   \
        cellato::run::test_suites::on_cuda::standard<AUTOMATON>::with_spatial_blocking<1, 1>)                          \
    CELLATO_INSTANTIATE_CUDA_SPATIAL_BLOCKING_SUITE(                                                                   \
        cellato::run::test_suites::on_cuda::standard<AUTOMATON>::with_spatial_blocking<2, 1>)                          \
    CELLATO_INSTANTIATE_CUDA_SPATIAL_BLOCKING_SUITE(                                                                   \
        cellato::run::test_suites::on_cuda::standard<AUTOMATON>::with_spatial_blocking<4, 1>)

#endif // CELLATO_CUDA_INSTANTIATION_TEMPLATE_CUH
