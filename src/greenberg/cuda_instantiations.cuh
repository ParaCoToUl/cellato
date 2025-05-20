#ifndef GREENBERG_CUDA_INSTANTIATIONS_CUH
#define GREENBERG_CUDA_INSTANTIATIONS_CUH

#include "./config.hpp"
#include "traversers/cuda/simple.hpp"
#include "evaluators/standard.hpp"
#include "memory/standard_grid.hpp"
#include "memory/interface.hpp"

#define GREENBERG_TRAVERSER_TYPE \
    cellib::traversers::cuda::simple::traverser< \
        cellib::evaluators::standard::evaluator<greenberg::ghm_cell_state, greenberg::config::algorithm>, \
        cellib::memory::grids::standard::grid<greenberg::ghm_cell_state, cellib::memory::grids::device::CPU> \
    >

template class GREENBERG_TRAVERSER_TYPE;
template void GREENBERG_TRAVERSER_TYPE::run_kernel<cellib::traversers::cuda::simple::_run_mode::QUIET>(int);
template void GREENBERG_TRAVERSER_TYPE::run_kernel<cellib::traversers::cuda::simple::_run_mode::VERBOSE>(int);

#undef GREENBERG_TRAVERSER_TYPE

#endif // GREENBERG_CUDA_INSTANTIATIONS_CUH