#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <stdexcept>
#include <array>
#include <type_traits>
#include <cstddef> 

#include "./tiled_temporal.hpp"
#include "../../memory/standard_grid.hpp"
#include "../../memory/interface.hpp"
#include "../../evaluators/standard.hpp"
#include "../../core/ast.hpp"
#include "../traverser_utils.hpp"
#include "../cuda_utils.cuh"

namespace cellato::traversers::cuda::tiled_temporal {

template <typename TArray>
struct props {};

template <typename TPointer, std::size_t Size>
struct props<std::array<TPointer, Size>> {
    static constexpr std::size_t size = Size;
    using store_type = TPointer;
    using no_pointer_type = typename std::remove_pointer<TPointer>::type;
    using grid_type = std::array<TPointer, Size>;

    static constexpr std::size_t compute_buffer_size(std::size_t total_elements) {
        constexpr std::size_t bits_count = size;
        return (total_elements * bits_count);
    }

    static __device__ grid_type create_from_contiguous(TPointer data, std::size_t total_elements) {
        grid_type arr;
        for_each_bit([&]<std::size_t bit_idx>() {
            std::get<bit_idx>(arr) = data + bit_idx * total_elements;
        });
        return arr;
    }

    static __device__ void assign_to_from(
        grid_type to, std::size_t to_x_size,
        std::size_t to_x, std::size_t to_y,

        grid_type from, std::size_t from_x_size,
        std::size_t from_x, std::size_t from_y
    ) {

        for_each_bit([&]<std::size_t bit_idx>() {
            auto from_ptr = std::get<bit_idx>(from);
            auto to_ptr = std::get<bit_idx>(to);

            to_ptr[to_y * to_x_size + to_x] = from_ptr[from_y * from_x_size + from_x];
        });
    } 
private:
    template <typename Callback, std::size_t... Is>
    static __device__ void for_each_bit_impl(Callback&& cb, std::index_sequence<Is...>) {
        (cb.template operator()<Is>(), ...);
    }

    template <typename Callback>
    static __device__ void for_each_bit(Callback&& cb) {
        for_each_bit_impl(std::forward<Callback>(cb), std::make_index_sequence<Size>{});
    }
};

template <
    typename evaluator_t,
    
    std::size_t temporal_steps, std::size_t temporal_tile_size_y,
    std::size_t block_size_x, std::size_t block_size_y,

    typename grid_data_t, typename output_data_t>

__global__ void process_grid_kernel_tiled_temporal(
    grid_data_t input_data,
    output_data_t output_data,
    std::size_t width,
    std::size_t height,
    std::size_t time_step
) {
    constexpr std::size_t cells_per_thread_y = temporal_tile_size_y / block_size_y;
    constexpr std::size_t temporal_tile_size_x = block_size_x;

    std::size_t global_x_non_wrapped = static_cast<std::size_t>(blockIdx.x) * (temporal_tile_size_x - 2) + threadIdx.x - 1;
    std::size_t global_y_start_non_wrapped = static_cast<std::size_t>(blockIdx.y) * (temporal_tile_size_y - 2) + 
                                             static_cast<std::size_t>(threadIdx.y) * cells_per_thread_y - 1;
    
    std::size_t global_x_for_load = (global_x_non_wrapped + width) % width;
    std::size_t global_x_for_save = global_x_non_wrapped;

    std::size_t local_x = threadIdx.x;
    std::size_t local_y_start = threadIdx.y * cells_per_thread_y;

    constexpr std::size_t cells_in_temporal_blocks = temporal_tile_size_y * temporal_tile_size_x;

    __shared__ typename props<grid_data_t>::no_pointer_type buffer1[props<grid_data_t>::compute_buffer_size(cells_in_temporal_blocks)];
    __shared__ typename props<grid_data_t>::no_pointer_type buffer2[props<grid_data_t>::compute_buffer_size(cells_in_temporal_blocks)];

    grid_data_t grid_buffer1 = props<grid_data_t>::create_from_contiguous(buffer1, cells_in_temporal_blocks);
    grid_data_t grid_buffer2 = props<grid_data_t>::create_from_contiguous(buffer2, cells_in_temporal_blocks);

    for (std::size_t y_offset = 0; y_offset < cells_per_thread_y; ++y_offset) {
        std::size_t global_y_for_load = (global_y_start_non_wrapped + y_offset + height) % height;
        
        props<grid_data_t>::assign_to_from(
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

    if (threadIdx.x == 0 || threadIdx.x == block_size_x - 1)
        return;

    auto y_start_for_saving = 0;
    auto count_to_save = cells_per_thread_y;

    if (threadIdx.y == 0) {
        y_start_for_saving += 1;
    }

    if (threadIdx.y == block_size_y - 1) {
        count_to_save -= 1;
    }

    for (std::size_t y_offset = y_start_for_saving; y_offset < count_to_save; ++y_offset) {
        std::size_t global_y_for_save = global_y_start_non_wrapped + y_offset;
        
        props<grid_data_t>::assign_to_from(
            output_data, width,
            global_x_for_save, global_y_for_save,

            current, temporal_tile_size_x,
            local_x, local_y_start + y_offset
        );
    }
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
        width_threads / _effective_temporal_tile_size_x,
        height_threads / _effective_temporal_tile_size_y
    );

    if constexpr (mode == _run_mode::VERBOSE) {
        call_callback(0, current);
    }

    for (int step = 0; step < steps; step += _temporal_steps) {
        auto input_data = current->data();
        auto output_data = next->data();

        process_grid_kernel_tiled_temporal<evaluator_t, 4, 4, 32, 2><<<gridDim, blockDim>>>(
            input_data,
            output_data,
            width,
            height,
            step
        );
        
        if constexpr (mode == _run_mode::VERBOSE) {
            call_callback(step + _temporal_steps, next);
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
