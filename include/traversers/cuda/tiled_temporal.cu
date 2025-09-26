#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <stdexcept>

#include "./tiled_temporal.hpp"
#include "../../memory/standard_grid.hpp"
#include "../../memory/interface.hpp"
#include "../../evaluators/standard.hpp"
#include "../../core/ast.hpp"
#include "../traverser_utils.hpp"
#include "../cuda_utils.cuh"

namespace cellato::traversers::cuda::tiled_temporal {

template <typename evaluator_t, typename grid_data_t, typename output_data_t>
__global__ void process_grid_kernel_tiled_temporal(
    grid_data_t input_data,
    output_data_t output_data,
    size_t width,
    size_t height,
    int time_step,
    int temporal_block_size
) {
    // Implement the temporal blocking kernel here
    // This is where the key differences from the simple traverser will be
    
    // Basic structure similar to simple.cu:
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    
    cellato::memory::grids::point_in_grid state(input_data);

    state.properties.x_size = width;
    state.properties.y_size = height;
    state.position.x = x;
    state.position.y = y;
    state.time_step = time_step;

    // TODO: Implement temporal blocking logic
    // This will involve shared memory for the tile and processing multiple time steps
    
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

    // Toroidal wrapping - same width and height
    size_t width_threads = width;
    size_t height_threads = height;

    dim3 blockDim(_block_size_x, _block_size_y);
    dim3 gridDim(
        width_threads / blockDim.x,
        height_threads / blockDim.y
    );

    if constexpr (mode == _run_mode::VERBOSE) {
        call_callback(0, current);
    }

    // TODO: Implement temporal blocking execution strategy
    // This will differ from the simple traverser - you might process multiple timesteps
    // in a single kernel launch or use a different loop structure
    for (int step = 0; step < steps; step += _temporal_block_size) {
        int actual_steps = std::min(_temporal_block_size, steps - step);
        
        auto input_data = current->data();
        auto output_data = next->data();
        
        process_grid_kernel_tiled_temporal<evaluator_t><<<gridDim, blockDim>>>(
            input_data,
            output_data,
            width,
            height,
            step,
            actual_steps
        );
        
        if constexpr (mode == _run_mode::VERBOSE) {
            call_callback(step + actual_steps, next);
        }

        std::swap(current, next);

        CUCH(cudaGetLastError());
    }
    
    CUCH(cudaDeviceSynchronize());

    _final_grid = current;
}

template <typename evaluator_type, typename grid_type>
auto traverser<evaluator_type, grid_type>::fetch_result() -> grid_t {
    grid_t cpu_grid = _final_grid->to_cpu();

    _input_grid_cuda.free_cuda_memory();
    _intermediate_grid_cuda.free_cuda_memory();

    return cpu_grid;
}

} // namespace cellato::traversers::cuda::tiled_temporal

#define TILED_TEMPORAL_CUDA_TRAVERSER_INSTANTIATIONS

#include "../../../src/game_of_life/cuda_instantiations.cuh"
#include "../../../src/fire/cuda_instantiations.cuh"
#include "../../../src/wire/cuda_instantiations.cuh"
#include "../../../src/greenberg/cuda_instantiations.cuh"
#include "../../../src/brian/cuda_instantiations.cuh"
#include "../../../src/maze/cuda_instantiations.cuh"
#include "../../../src/hpp/cuda_instantiations.cuh"
#include "../../../src/critters/cuda_instantiations.cuh"
#include "../../../src/traffic/cuda_instantiations.cuh"
#include "../../../src/cyclic/cuda_instantiations.cuh"

#undef TILED_TEMPORAL_CUDA_TRAVERSER_INSTANTIATIONS
