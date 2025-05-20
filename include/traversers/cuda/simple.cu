#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <stdexcept>

#include "./simple.hpp"
#include "../../memory/standard_grid.hpp"
#include "../../memory/interface.hpp"
#include "../../evaluators/standard.hpp"
#include "../../core/ast.hpp"
#include "../traverser_utils.hpp"
#include "../cuda_utils.cuh"

namespace cellib::traversers::cuda::simple {

template <typename evaluator_t, typename grid_data_t, typename output_data_t>
__global__ void process_grid_kernel(
    grid_data_t input_data,
    output_data_t output_data,
    size_t width,
    size_t height
) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    
    // Skip if outside grid bounds or on border
    if (x <= 0 || x >= width - 1 || y <= 0 || y >= height - 1) return;
    
    cellib::memory::grids::point_in_grid state(input_data);

    state.properties.x_size = width;
    state.properties.y_size = height;
    state.position.x = x;
    state.position.y = y;
    
    auto result = evaluator_t::evaluate(state);
    save_to(output_data, state.idx(), result);
}

template <typename evaluator_type, typename grid_type>
template <_run_mode mode>
void traverser<evaluator_type, grid_type>::run_kernel(int steps) {

    auto current = &_input_grid_cuda;
    auto next = &_intermediate_grid_cuda;
    
    size_t width = current->x_size_physical();
    size_t height = current->y_size_physical();
    
    dim3 blockDim(16, 16);
    dim3 gridDim(
        (width + blockDim.x - 1) / blockDim.x,
        (height + blockDim.y - 1) / blockDim.y
    );

    for (int step = 0; step < steps; ++step) {
        auto input_data = current->data();
        auto output_data = next->data();
        
        process_grid_kernel<evaluator_t><<<gridDim, blockDim>>>(
            input_data,
            output_data,
            width,
            height
        );
        
        if constexpr (mode == _run_mode::VERBOSE) {
            call_callback(step, current);
        }

        std::swap(current, next);
    }
    
    CUCH(cudaDeviceSynchronize());

    if (steps % 2 == 1) {
        _final_grid = next;
    } else {
        _final_grid = current;
    }
}

template <typename evaluator_type, typename grid_type>
typename traverser<evaluator_type, grid_type>::grid_t 
traverser<evaluator_type, grid_type>::fetch_result() const {
    return _final_grid->to_cpu();
}

} // namespace cellib::traversers::cuda::simple


#include "../../../src/game_of_life/cuda_instantiations.cuh"
#include "../../../src/fire/cuda_instantiations.cuh"
#include "../../../src/wire/cuda_instantiations.cuh"
#include "../../../src/greenberg/cuda_instantiations.cuh"
