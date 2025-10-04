#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <stdexcept>
#include <array>
#include <type_traits>
#include <cstddef> 

#include "./linear_temporal.hpp"
#include "../../memory/standard_grid.hpp"
#include "../../memory/interface.hpp"
#include "../../evaluators/standard.hpp"
#include "../../core/ast.hpp"
#include "../traverser_utils.hpp"
#include "../cuda_utils.cuh"
#include "../../utils/static_dispatcher.hpp"
#include "../../traversers/temporal_utils.cuh"

namespace cellato::traversers::cuda::linear_temporal {

using namespace cellato::traversers::temporal_utils;

template <
    typename evaluator_t,
    
    std::size_t temporal_steps, std::size_t temporal_tile_size_y,
    std::size_t word_tile_x, std::size_t word_tile_y, double average_halo_radius,

    std::size_t block_size_x, std::size_t block_size_y,

    typename grid_data_t, typename output_data_t>

__global__ void process_grid_kernel_linear_temporal(
    grid_data_t input_data,
    output_data_t output_data,
    std::size_t width,
    std::size_t height,
    std::size_t time_step
) {
    using grid_props = props<grid_data_t>;
    using store_t = typename grid_props::no_pointer_type;
    using prt_t = typename grid_props::ptr_type;

    constexpr std::size_t cells_per_thread_y = temporal_tile_size_y / block_size_y;
    constexpr std::size_t temporal_tile_size_x = block_size_x;

    constexpr std::size_t needed_halo_cells = static_cast<std::size_t>(std::ceil(average_halo_radius * temporal_steps * 0.999));
    constexpr std::size_t x_halo_words = (needed_halo_cells + word_tile_x - 1) / word_tile_x;
    constexpr std::size_t y_halo_words = (needed_halo_cells + word_tile_y - 1) / word_tile_y;

    constexpr std::size_t effective_temporal_tile_size_x = temporal_tile_size_x - (2 * x_halo_words);
    constexpr std::size_t effective_temporal_tile_size_y = temporal_tile_size_y - (2 * y_halo_words);

    std::size_t global_x_non_wrapped = static_cast<std::size_t>(blockIdx.x) * effective_temporal_tile_size_x + threadIdx.x - x_halo_words;
    std::size_t global_y_start_non_wrapped = static_cast<std::size_t>(blockIdx.y) * effective_temporal_tile_size_y + 
                                             static_cast<std::size_t>(threadIdx.y) * cells_per_thread_y - y_halo_words;

    std::size_t global_x_for_load = (global_x_non_wrapped + width) % width;
    std::size_t global_x_for_save = global_x_non_wrapped;

    std::size_t local_x = threadIdx.x;
    std::size_t local_y_start = threadIdx.y * cells_per_thread_y;

    constexpr std::size_t cells_in_temporal_blocks = temporal_tile_size_y * temporal_tile_size_x;

    extern __shared__ char s_buffer[];
    store_t* buffer_base = reinterpret_cast<store_t*>(s_buffer);

    store_t* buffer1 = buffer_base;
    store_t* buffer2 = buffer_base + grid_props::compute_buffer_size(cells_in_temporal_blocks);
    
    grid_data_t grid_buffer1 = grid_props::create_from_contiguous(buffer1, cells_in_temporal_blocks);
    grid_data_t grid_buffer2 = grid_props::create_from_contiguous(buffer2, cells_in_temporal_blocks);

    for (std::size_t y_offset = 0; y_offset < cells_per_thread_y; ++y_offset) {
        std::size_t global_y_for_load = (global_y_start_non_wrapped + y_offset + height) % height;
        
        grid_props::assign_to_from(
            grid_buffer1, temporal_tile_size_x,
            local_x, local_y_start + y_offset,

            input_data, width,
            global_x_for_load, global_y_for_load
        );
    }

    grid_data_t current = grid_buffer1;
    grid_data_t next = grid_buffer2;

    __syncthreads();

    for (int t = 0; t < temporal_steps; ++t) {
        for (std::size_t y_offset = 0; y_offset < cells_per_thread_y; ++y_offset) {
            cellato::memory::grids::point_in_grid state(current);

            state.properties.x_size = temporal_tile_size_x;
            state.properties.y_size = temporal_tile_size_y;

            state.position.x = local_x;
            state.position.y = local_y_start + y_offset;

            state.time_step = time_step + t;

            auto result = evaluator_t::evaluate(state);
            save_to(next, state.idx(), result);
        }

        auto temp = current;
        current = next;
        next = temp;

        __syncthreads();
    }

    if (threadIdx.x < x_halo_words || threadIdx.x >= (temporal_tile_size_x - x_halo_words))
        return;

    for (std::size_t y_offset = 0; y_offset < cells_per_thread_y; ++y_offset) {
        std::size_t global_y_for_save = global_y_start_non_wrapped + y_offset;
        std::size_t local_y = local_y_start + y_offset;

        if (local_y < y_halo_words || local_y >= (temporal_tile_size_y - y_halo_words))
            continue;

        grid_props::assign_to_from(
            output_data, width,
            global_x_for_save, global_y_for_save,

            current, temporal_tile_size_x,
            local_x, local_y
        );
    }
}

template <typename evaluator_type, typename grid_type, double average_halo_radius>
template <_run_mode mode>
void traverser<evaluator_type, grid_type, average_halo_radius>::run_kernel(int steps) {
    auto current = &_input_grid_cuda;
    auto next = &_intermediate_grid_cuda;

    size_t width = current->x_size_physical();
    size_t height = current->y_size_physical();

    size_t width_threads = width;
    size_t height_threads = height;

    dim3 blockDim(_block_size_x, _block_size_y);
    dim3 gridDim(
        width_threads / _effective_temporal_tile_size_x,
        height_threads / _effective_temporal_tile_size_y
    );

    if constexpr (mode == _run_mode::VERBOSE) {
        call_callback(0, current);
    }
    
    // Hot compilation
    // using temporal_steps_options = std::integer_sequence<std::size_t, 2, 4, 6, 8>;
    // using tile_y_options         = std::integer_sequence<std::size_t, 8, 16, 32, 64, 128>;
    // using block_x_options        = std::integer_sequence<std::size_t, 32>;
    // using block_y_options        = std::integer_sequence<std::size_t, 2, 4, 8, 16, 32>;
    
    // Fast compilation
    using temporal_steps_options = std::integer_sequence<std::size_t, 4>;
    using tile_y_options         = std::integer_sequence<std::size_t, 16>;
    using block_x_options        = std::integer_sequence<std::size_t, 32>;
    using block_y_options        = std::integer_sequence<std::size_t, 8>;

    cellato::generic_dispatcher::call<
        temporal_steps_options,
        tile_y_options,
        block_x_options,
        block_y_options
    >(
        [&]<
            std::size_t temporal_steps, std::size_t temporal_tile_size_y,
            std::size_t block_size_x, std::size_t block_size_y
        >() {
            constexpr std::size_t temporal_tile_size_x = block_size_x;
            constexpr std::size_t required_buffers_bytes = 2 * temporal_tile_size_y * temporal_tile_size_x * grid_type::needed_bits * sizeof(typename grid_type::cell_t);
        
            constexpr std::size_t needed_halo_cells = static_cast<std::size_t>(std::ceil(average_halo_radius * temporal_steps));
            constexpr std::size_t y_halo_words = (needed_halo_cells + word_tile_y - 1) / word_tile_y;
            constexpr std::size_t effective_y_tile_size = temporal_tile_size_y - (2 * y_halo_words);

            if constexpr (required_buffers_bytes > max_shm_size) {
                throw std::runtime_error("Configuration exceeds maximum shared memory size. The temporal tile is too large.");
                
            } else if constexpr (temporal_tile_size_y < block_size_y) {
                throw std::runtime_error("Invalid configuration: block_size_y must be less than or equal to temporal_tile_size_y");

            } else if constexpr (effective_y_tile_size <= 0) {
                throw std::runtime_error("Invalid configuration: effective_y_tile_size must be greater than 0.");

            } else {
                for (int step = 0; step < steps; step += temporal_steps) {
                    auto input_data = current->data();
                    auto output_data = next->data();

                    cudaFuncSetAttribute(
                        process_grid_kernel_linear_temporal<
                            evaluator_type,
                            temporal_steps, temporal_tile_size_y,
                            word_tile_x, word_tile_y, average_halo_radius,
                            block_size_x, block_size_y,
                            decltype(input_data), decltype(output_data)
                        >,
                        cudaFuncAttributePreferredSharedMemoryCarveout,
                        cudaSharedmemCarveoutMaxShared
                    );

                    process_grid_kernel_linear_temporal<
                        evaluator_type,
                        temporal_steps, temporal_tile_size_y,
                        word_tile_x, word_tile_y, average_halo_radius,
                        block_size_x, block_size_y
                    ><<<gridDim, blockDim, required_buffers_bytes>>>(
                        input_data,
                        output_data,
                        width,
                        height,
                        step
                    );

                    if constexpr (mode == _run_mode::VERBOSE) {
                        call_callback(step + temporal_steps, next);
                    }

                    std::swap(current, next);
                    CUCH(cudaGetLastError());
                }
            }
        },
        (std::size_t)_temporal_steps,
        (std::size_t)_temporal_tile_size_y,
        (std::size_t)_block_size_x,
        (std::size_t)_block_size_y
    );
    
    CUCH(cudaDeviceSynchronize());
    
    _final_grid = current;
}

template <typename evaluator_type, typename grid_type, double average_halo_radius>
auto traverser<evaluator_type, grid_type, average_halo_radius>::fetch_result() -> grid_t {
    grid_t cpu_grid = _final_grid->to_cpu();

    _input_grid_cuda.free_cuda_memory();
    _intermediate_grid_cuda.free_cuda_memory();

    return cpu_grid;
}

} // namespace cellato::traversers::cuda::linear_temporal

#define LINEAR_TEMPORAL_CUDA_TRAVERSER_INSTANTIATIONS

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

#undef LINEAR_TEMPORAL_CUDA_TRAVERSER_INSTANTIATIONS
