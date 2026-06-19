#ifndef AUTOMATON_NAMESPACE
#error "AUTOMATON_NAMESPACE must be defined"
#endif

#include <cstdint>

#include "cellato/traversers/cuda/simple.hpp"
#include "cellato/traversers/cuda/temporal.hpp"
#include "cellato/evaluators/standard.hpp"
#include "cellato/evaluators/bit_planes.hpp"
#include "cellato/evaluators/tiled_bit_planes.hpp"
#include "cellato/evaluators/bit_array.hpp"
#include "cellato/memory/standard_grid.hpp"
#include "cellato/memory/bit_planes_grid.hpp"
#include "cellato/memory/tiled_bit_planes_grid.hpp"
#include "cellato/memory/bit_array_grid.hpp"
#include "cellato/memory/interface.hpp"

#ifdef SIMPLE_CUDA_TRAVERSER_INSTANTIATIONS

// Standard grid with standard evaluator
#define TRAVERSER_TYPE \
    cellato::traversers::cuda::simple::traverser< \
        cellato::evaluators::standard::evaluator<AUTOMATON_NAMESPACE::config::cell_state, AUTOMATON_NAMESPACE::config::algorithm>, \
        cellato::memory::grids::standard::grid<AUTOMATON_NAMESPACE::config::cell_state, cellato::memory::grids::device::CPU> \
    >

template class TRAVERSER_TYPE;
template void TRAVERSER_TYPE::run_kernel<cellato::traversers::cuda::simple::_run_mode::QUIET>(int);
template void TRAVERSER_TYPE::run_kernel<cellato::traversers::cuda::simple::_run_mode::VERBOSE>(int);

#undef TRAVERSER_TYPE

// Bit array grid with simple evaluator (32-bit)

#define GRID_TYPE \
    cellato::memory::grids::bit_array::grid< \
        AUTOMATON_NAMESPACE::config::state_dictionary, \
        std::uint32_t, \
        cellato::memory::grids::device::CPU \
    >

#define TRAVERSER_TYPE \
    cellato::traversers::cuda::simple::traverser< \
        cellato::evaluators::bit_array::evaluator<GRID_TYPE, AUTOMATON_NAMESPACE::config::algorithm>, \
        GRID_TYPE \
    >

template class TRAVERSER_TYPE;
template void TRAVERSER_TYPE::run_kernel<cellato::traversers::cuda::simple::_run_mode::QUIET>(int);
template void TRAVERSER_TYPE::run_kernel<cellato::traversers::cuda::simple::_run_mode::VERBOSE>(int);

#undef TRAVERSER_TYPE
#undef GRID_TYPE

// Bit array grid with simple evaluator (64-bit)
#define GRID_TYPE \
    cellato::memory::grids::bit_array::grid< \
        AUTOMATON_NAMESPACE::config::state_dictionary, \
        std::uint64_t, \
        cellato::memory::grids::device::CPU \
    >

#define TRAVERSER_TYPE \
    cellato::traversers::cuda::simple::traverser< \
        cellato::evaluators::bit_array::evaluator<GRID_TYPE, AUTOMATON_NAMESPACE::config::algorithm>, \
        GRID_TYPE \
    >

template class TRAVERSER_TYPE;
template void TRAVERSER_TYPE::run_kernel<cellato::traversers::cuda::simple::_run_mode::QUIET>(int);
template void TRAVERSER_TYPE::run_kernel<cellato::traversers::cuda::simple::_run_mode::VERBOSE>(int);

#undef TRAVERSER_TYPE
#undef GRID_TYPE

// Bit planes grid with bit planes evaluator (32-bit)
#define TRAVERSER_TYPE \
    cellato::traversers::cuda::simple::traverser< \
        cellato::evaluators::bit_planes::evaluator<std::uint32_t, AUTOMATON_NAMESPACE::config::state_dictionary, AUTOMATON_NAMESPACE::config::algorithm>, \
        cellato::memory::grids::bit_planes::grid<std::uint32_t, AUTOMATON_NAMESPACE::config::state_dictionary, cellato::memory::grids::device::CPU> \
    >

template class TRAVERSER_TYPE;
template void TRAVERSER_TYPE::run_kernel<cellato::traversers::cuda::simple::_run_mode::QUIET>(int);
template void TRAVERSER_TYPE::run_kernel<cellato::traversers::cuda::simple::_run_mode::VERBOSE>(int);

#undef TRAVERSER_TYPE

// Bit planes grid with bit planes evaluator (64-bit)
#define TRAVERSER_TYPE \
    cellato::traversers::cuda::simple::traverser< \
        cellato::evaluators::bit_planes::evaluator<std::uint64_t, AUTOMATON_NAMESPACE::config::state_dictionary, AUTOMATON_NAMESPACE::config::algorithm>, \
        cellato::memory::grids::bit_planes::grid<std::uint64_t, AUTOMATON_NAMESPACE::config::state_dictionary, cellato::memory::grids::device::CPU> \
    >

template class TRAVERSER_TYPE;
template void TRAVERSER_TYPE::run_kernel<cellato::traversers::cuda::simple::_run_mode::QUIET>(int);
template void TRAVERSER_TYPE::run_kernel<cellato::traversers::cuda::simple::_run_mode::VERBOSE>(int);

#undef TRAVERSER_TYPE

// Tiled bit planes grid with bit planes evaluator (32-bit)
#define TRAVERSER_TYPE \
    cellato::traversers::cuda::simple::traverser< \
        cellato::evaluators::tiled_bit_planes::evaluator<std::uint32_t, AUTOMATON_NAMESPACE::config::state_dictionary, AUTOMATON_NAMESPACE::config::algorithm>, \
        cellato::memory::grids::tiled_bit_planes::grid<std::uint32_t, AUTOMATON_NAMESPACE::config::state_dictionary, cellato::memory::grids::device::CPU> \
    >

template class TRAVERSER_TYPE;
template void TRAVERSER_TYPE::run_kernel<cellato::traversers::cuda::simple::_run_mode::QUIET>(int);
template void TRAVERSER_TYPE::run_kernel<cellato::traversers::cuda::simple::_run_mode::VERBOSE>(int);

#undef TRAVERSER_TYPE

// Tiled bit planes grid with bit planes evaluator (64-bit)
#define TRAVERSER_TYPE \
    cellato::traversers::cuda::simple::traverser< \
        cellato::evaluators::tiled_bit_planes::evaluator<std::uint64_t, AUTOMATON_NAMESPACE::config::state_dictionary, AUTOMATON_NAMESPACE::config::algorithm>, \
        cellato::memory::grids::tiled_bit_planes::grid<std::uint64_t, AUTOMATON_NAMESPACE::config::state_dictionary, cellato::memory::grids::device::CPU> \
    >

template class TRAVERSER_TYPE;
template void TRAVERSER_TYPE::run_kernel<cellato::traversers::cuda::simple::_run_mode::QUIET>(int);
template void TRAVERSER_TYPE::run_kernel<cellato::traversers::cuda::simple::_run_mode::VERBOSE>(int);

#undef TRAVERSER_TYPE

#endif // SIMPLE_CUDA_TRAVERSER_INSTANTIATIONS

#ifdef TILED_TEMPORAL_CUDA_TRAVERSER_INSTANTIATIONS

// Temporal tiled bit planes grid with bit planes evaluator (32-bit)
#define TRAVERSER_TYPE \
    cellato::traversers::cuda::temporal::traverser< \
        cellato::evaluators::tiled_bit_planes::evaluator<std::uint32_t, AUTOMATON_NAMESPACE::config::state_dictionary, AUTOMATON_NAMESPACE::config::algorithm>, \
        cellato::memory::grids::tiled_bit_planes::grid<std::uint32_t, AUTOMATON_NAMESPACE::config::state_dictionary, cellato::memory::grids::device::CPU>, \
        AUTOMATON_NAMESPACE::config::average_halo_radius \
    >

template class TRAVERSER_TYPE;
template void TRAVERSER_TYPE::run_kernel<cellato::traversers::cuda::temporal::_run_mode::QUIET>(int);
template void TRAVERSER_TYPE::run_kernel<cellato::traversers::cuda::temporal::_run_mode::VERBOSE>(int);

#undef TRAVERSER_TYPE

// Temporal tiled bit planes grid with bit planes evaluator (64-bit)
#define TRAVERSER_TYPE \
    cellato::traversers::cuda::temporal::traverser< \
        cellato::evaluators::tiled_bit_planes::evaluator<std::uint64_t, AUTOMATON_NAMESPACE::config::state_dictionary, AUTOMATON_NAMESPACE::config::algorithm>, \
        cellato::memory::grids::tiled_bit_planes::grid<std::uint64_t, AUTOMATON_NAMESPACE::config::state_dictionary, cellato::memory::grids::device::CPU>, \
        AUTOMATON_NAMESPACE::config::average_halo_radius \
    >

template class TRAVERSER_TYPE;
template void TRAVERSER_TYPE::run_kernel<cellato::traversers::cuda::temporal::_run_mode::QUIET>(int);
template void TRAVERSER_TYPE::run_kernel<cellato::traversers::cuda::temporal::_run_mode::VERBOSE>(int);

#undef TRAVERSER_TYPE

#endif // TILED_TEMPORAL_CUDA_TRAVERSER_INSTANTIATIONS

#ifdef LINEAR_TEMPORAL_CUDA_TRAVERSER_INSTANTIATIONS

// Temporal linear bit planes grid with bit planes evaluator (32-bit)
#define TRAVERSER_TYPE \
    cellato::traversers::cuda::temporal::traverser< \
        cellato::evaluators::bit_planes::evaluator<std::uint32_t, AUTOMATON_NAMESPACE::config::state_dictionary, AUTOMATON_NAMESPACE::config::algorithm>, \
        cellato::memory::grids::bit_planes::grid<std::uint32_t, AUTOMATON_NAMESPACE::config::state_dictionary, cellato::memory::grids::device::CPU>, \
        AUTOMATON_NAMESPACE::config::average_halo_radius \
    >

template class TRAVERSER_TYPE;
template void TRAVERSER_TYPE::run_kernel<cellato::traversers::cuda::temporal::_run_mode::QUIET>(int);
template void TRAVERSER_TYPE::run_kernel<cellato::traversers::cuda::temporal::_run_mode::VERBOSE>(int);

#undef TRAVERSER_TYPE

// Temporal linear bit planes grid with bit planes evaluator (64-bit)
#define TRAVERSER_TYPE \
    cellato::traversers::cuda::temporal::traverser< \
        cellato::evaluators::bit_planes::evaluator<std::uint64_t, AUTOMATON_NAMESPACE::config::state_dictionary, AUTOMATON_NAMESPACE::config::algorithm>, \
        cellato::memory::grids::bit_planes::grid<std::uint64_t, AUTOMATON_NAMESPACE::config::state_dictionary, cellato::memory::grids::device::CPU>, \
        AUTOMATON_NAMESPACE::config::average_halo_radius \
    >

template class TRAVERSER_TYPE;
template void TRAVERSER_TYPE::run_kernel<cellato::traversers::cuda::temporal::_run_mode::QUIET>(int);
template void TRAVERSER_TYPE::run_kernel<cellato::traversers::cuda::temporal::_run_mode::VERBOSE>(int);

#undef TRAVERSER_TYPE

#endif // LINEAR_TEMPORAL_CUDA_TRAVERSER_INSTANTIATIONS

#ifdef SPATIAL_BLOCKING_CUDA_TRAVERSER_INSTANTIATIONS


#define GRID_TYPE \
    cellato::memory::grids::bit_array::grid< \
        AUTOMATON_NAMESPACE::config::state_dictionary, \
        std::uint32_t, \
        cellato::memory::grids::device::CPU \
    >
#define TRAVERSER_TYPE \
    cellato::traversers::cuda::spatial_blocking::traverser< \
        cellato::evaluators::bit_array::evaluator< \
            GRID_TYPE, AUTOMATON_NAMESPACE::config::algorithm>, \
        GRID_TYPE, \
        1, GRID_TYPE::cells_per_word \
    >

template class TRAVERSER_TYPE;
template void TRAVERSER_TYPE::run_kernel<cellato::traversers::cuda::spatial_blocking::_run_mode::QUIET>(int);
template void TRAVERSER_TYPE::run_kernel<cellato::traversers::cuda::spatial_blocking::_run_mode::VERBOSE>(int);

#undef GRID_TYPE
#undef TRAVERSER_TYPE

#define GRID_TYPE \
    cellato::memory::grids::bit_array::grid< \
        AUTOMATON_NAMESPACE::config::state_dictionary, \
        std::uint64_t, \
        cellato::memory::grids::device::CPU \
    >
#define TRAVERSER_TYPE \
    cellato::traversers::cuda::spatial_blocking::traverser< \
        cellato::evaluators::bit_array::evaluator< \
            GRID_TYPE, AUTOMATON_NAMESPACE::config::algorithm>, \
        GRID_TYPE, \
        1, GRID_TYPE::cells_per_word \
    >

template class TRAVERSER_TYPE;
template void TRAVERSER_TYPE::run_kernel<cellato::traversers::cuda::spatial_blocking::_run_mode::QUIET>(int);
template void TRAVERSER_TYPE::run_kernel<cellato::traversers::cuda::spatial_blocking::_run_mode::VERBOSE>(int);

#undef GRID_TYPE
#undef TRAVERSER_TYPE


#define Y_TILE_SIZE 1

    #define X_TILE_SIZE 1
    #include "spatial_blocking.cuh"
    #undef X_TILE_SIZE

#undef Y_TILE_SIZE

#define Y_TILE_SIZE 2

    #define X_TILE_SIZE 1
    #include "spatial_blocking.cuh"
    #undef X_TILE_SIZE

#undef Y_TILE_SIZE

#define Y_TILE_SIZE 4

    #define X_TILE_SIZE 1
    #include "spatial_blocking.cuh"
    #undef X_TILE_SIZE

#undef Y_TILE_SIZE

#define Y_TILE_SIZE 8

    #define X_TILE_SIZE 1
    #include "spatial_blocking.cuh"
    #undef X_TILE_SIZE

#undef Y_TILE_SIZE

#endif // SPATIAL_BLOCKING_CUDA_TRAVERSER_INSTANTIATIONS
