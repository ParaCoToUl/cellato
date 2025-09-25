#include "./reference_implementation.hpp"
#include <cuda_runtime.h>
#include "traversers/cuda_utils.cuh"
#include "../_shared/indexing.hpp"

namespace hpp::reference {
using namespace ::reference::indexing;

// CUDA kernel for Forest hpp (single step)
__global__ void hpp_kernel(const hpp_cell_state* current, hpp_cell_state* next, 
                            int width, int height) {
    // Calculate thread indices, adjusting for margins
    int x = blockIdx.x * blockDim.x + threadIdx.x + indexer::x_margin;
    int y = blockIdx.y * blockDim.y + threadIdx.y + indexer::y_margin;
    
    indexer idx(width, height);
    const int center_idx = idx.at(x, y);

    auto top_neighbor = current[idx.at(x, y-1)];
    auto bottom_neighbor = current[idx.at(x, y+1)];
    auto left_neighbor = current[idx.at(x-1, y)];
    auto right_neighbor = current[idx.at(x+1, y)];

    constexpr hpp_cell_state TOP = 0b0001;
    constexpr hpp_cell_state BOTTOM = 0b0010;
    constexpr hpp_cell_state LEFT = 0b0100;
    constexpr hpp_cell_state RIGHT = 0b1000;

    auto incoming_from_top = (top_neighbor & TOP);
    auto incoming_from_bottom = (bottom_neighbor & BOTTOM);
    auto incoming_from_left = (left_neighbor & LEFT);
    auto incoming_from_right = (right_neighbor & RIGHT);

    auto vertical_collision_appears = (incoming_from_top != 0) && (incoming_from_bottom != 0);
    auto horizontal_collision_appears = (incoming_from_left != 0) && (incoming_from_right != 0);

    auto combined_vertical_incoming = incoming_from_top | incoming_from_bottom;
    auto combined_horizontal_incoming = incoming_from_left | incoming_from_right;

    hpp_cell_state result = 0;

    auto just_vertical_collision = vertical_collision_appears && !((incoming_from_left != 0) || (incoming_from_right != 0));
    auto just_horizontal_collision = horizontal_collision_appears && !((incoming_from_top != 0) || (incoming_from_bottom != 0));

    if (just_vertical_collision) {
        result |= (LEFT | RIGHT); // horizontal outgoing
    } else {
        result |= combined_vertical_incoming; // pass vertical incoming
    }

    if (just_horizontal_collision) {
        result |= (TOP | BOTTOM); // vertical outgoing
    } else {
        result |= combined_horizontal_incoming; // pass horizontal incoming
    }

    next[center_idx] = result;
}

void runner::run_kernel(int steps) {
    if (!d_current || !d_next) {
        init_cuda();
    }
    
    // Set up grid and block dimensions, accounting for margins
    dim3 block_size(_block_size_x, _block_size_y);
    
    constexpr std::size_t x_margin = indexer::x_margin;
    constexpr std::size_t y_margin = indexer::y_margin;
    
    auto _x_size_threads = _x_size - 2 * x_margin; // Adjust for margins
    auto _y_size_threads = _y_size - 2 * y_margin; // Adjust for margins
    
    dim3 grid_dim((_x_size_threads + block_size.x - 1) / block_size.x, 
                 (_y_size_threads + block_size.y - 1) / block_size.y);
    
    // Run steps iterations
    for (int i = 0; i < steps; i++) {
        // Launch kernel for one step
        hpp_kernel<<<grid_dim, block_size>>>(d_current, d_next, _x_size, _y_size);
        
        // Swap pointers for next iteration
        hpp_cell_state* temp = d_current;
        d_current = d_next;
        d_next = temp;
    }
    
    CUCH(cudaDeviceSynchronize());
}

} // namespace hpp::reference
