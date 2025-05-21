#ifndef AUTOMATON_NAMESPACE
static_assert(false, "AUTOMATON_NAMESPACE must be defined");
#endif

#include <cstdint>

#include "traversers/cuda/simple.hpp"
#include "evaluators/standard.hpp"
#include "evaluators/bit_plates.hpp"
#include "memory/standard_grid.hpp"
#include "memory/bit_plates_grid.hpp"
#include "memory/interface.hpp"

// Standard grid with standard evaluator
#define TRAVERSER_TYPE \
    cellib::traversers::cuda::simple::traverser< \
        cellib::evaluators::standard::evaluator<AUTOMATON_NAMESPACE::config::cell_state, AUTOMATON_NAMESPACE::config::algorithm>, \
        cellib::memory::grids::standard::grid<AUTOMATON_NAMESPACE::config::cell_state, cellib::memory::grids::device::CPU> \
    >

template class TRAVERSER_TYPE;
template void TRAVERSER_TYPE::run_kernel<cellib::traversers::cuda::simple::_run_mode::QUIET>(int);
template void TRAVERSER_TYPE::run_kernel<cellib::traversers::cuda::simple::_run_mode::VERBOSE>(int);

#undef TRAVERSER_TYPE

// Bit plates grid with bit plates evaluator (32-bit)
#define TRAVERSER_TYPE \
    cellib::traversers::cuda::simple::traverser< \
        cellib::evaluators::bit_plates::evaluator<std::uint32_t, AUTOMATON_NAMESPACE::config::state_dictionary, AUTOMATON_NAMESPACE::config::algorithm>, \
        cellib::memory::grids::bit_plates::grid<std::uint32_t, AUTOMATON_NAMESPACE::config::state_dictionary, cellib::memory::grids::device::CPU> \
    >

template class TRAVERSER_TYPE;
template void TRAVERSER_TYPE::run_kernel<cellib::traversers::cuda::simple::_run_mode::QUIET>(int);
template void TRAVERSER_TYPE::run_kernel<cellib::traversers::cuda::simple::_run_mode::VERBOSE>(int);

#undef TRAVERSER_TYPE

// Bit plates grid with bit plates evaluator (64-bit)
#define TRAVERSER_TYPE \
    cellib::traversers::cuda::simple::traverser< \
        cellib::evaluators::bit_plates::evaluator<std::uint64_t, AUTOMATON_NAMESPACE::config::state_dictionary, AUTOMATON_NAMESPACE::config::algorithm>, \
        cellib::memory::grids::bit_plates::grid<std::uint64_t, AUTOMATON_NAMESPACE::config::state_dictionary, cellib::memory::grids::device::CPU> \
    >

template class TRAVERSER_TYPE;
template void TRAVERSER_TYPE::run_kernel<cellib::traversers::cuda::simple::_run_mode::QUIET>(int);
template void TRAVERSER_TYPE::run_kernel<cellib::traversers::cuda::simple::_run_mode::VERBOSE>(int);

#undef TRAVERSER_TYPE