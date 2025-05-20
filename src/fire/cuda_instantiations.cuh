#ifndef FIRE_CUDA_INSTANTIATIONS_CUH
#define FIRE_CUDA_INSTANTIATIONS_CUH

#include "./config.hpp"
#include "traversers/cuda/simple.hpp"
#include "evaluators/standard.hpp"
#include "memory/standard_grid.hpp"
#include "memory/interface.hpp"

#define FIRE_TRAVERSER_TYPE \
    cellib::traversers::cuda::simple::traverser< \
        cellib::evaluators::standard::evaluator<fire::fire_cell_state, fire::config::algorithm>, \
        cellib::memory::grids::standard::grid<fire::fire_cell_state, cellib::memory::grids::device::CPU> \
    >

template class FIRE_TRAVERSER_TYPE;
template void FIRE_TRAVERSER_TYPE::run_kernel<cellib::traversers::cuda::simple::_run_mode::QUIET>(int);
template void FIRE_TRAVERSER_TYPE::run_kernel<cellib::traversers::cuda::simple::_run_mode::VERBOSE>(int);

#undef FIRE_TRAVERSER_TYPE

#endif // FIRE_CUDA_INSTANTIATIONS_CUH