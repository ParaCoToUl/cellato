#ifndef WIRE_OF_LIFE_CUDA_INSTANTIATIONS_CUH
#define WIRE_OF_LIFE_CUDA_INSTANTIATIONS_CUH

#include "./config.hpp"
#include "traversers/cuda/simple.hpp"
#include "evaluators/standard.hpp"
#include "memory/standard_grid.hpp"
#include "memory/interface.hpp"

#define WIRE_TRAVERSER_TYPE \
    cellib::traversers::cuda::simple::traverser< \
        cellib::evaluators::standard::evaluator<wire::wire_cell_state, wire::config::algorithm>, \
        cellib::memory::grids::standard::grid<wire::wire_cell_state, cellib::memory::grids::device::CPU> \
    >

template class WIRE_TRAVERSER_TYPE;
template void WIRE_TRAVERSER_TYPE::run_kernel<cellib::traversers::cuda::simple::_run_mode::QUIET>(int);
template void WIRE_TRAVERSER_TYPE::run_kernel<cellib::traversers::cuda::simple::_run_mode::VERBOSE>(int);

#undef WIRE_TRAVERSER_TYPE

#endif // WIRE_OF_LIFE_CUDA_INSTANTIATIONS_CUH