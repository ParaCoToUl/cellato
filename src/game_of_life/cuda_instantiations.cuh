#ifndef GAME_OF_LIFE_CUDA_INSTANTIATIONS_CUH
#define GAME_OF_LIFE_CUDA_INSTANTIATIONS_CUH

#include "./config.hpp"
#include "traversers/cuda/simple.hpp"
#include "evaluators/standard.hpp"
#include "memory/standard_grid.hpp"
#include "memory/interface.hpp"

#define GOL_TRAVERSER_TYPE \
    cellib::traversers::cuda::simple::traverser< \
        cellib::evaluators::standard::evaluator<game_of_life::gol_cell_state, game_of_life::config::algorithm>, \
        cellib::memory::grids::standard::grid<game_of_life::gol_cell_state, cellib::memory::grids::device::CPU> \
    >

template class GOL_TRAVERSER_TYPE;
template void GOL_TRAVERSER_TYPE::run_kernel<cellib::traversers::cuda::simple::_run_mode::QUIET>(int);
template void GOL_TRAVERSER_TYPE::run_kernel<cellib::traversers::cuda::simple::_run_mode::VERBOSE>(int);

#undef GOL_TRAVERSER_TYPE

#endif // GAME_OF_LIFE_CUDA_INSTANTIATIONS_CUH