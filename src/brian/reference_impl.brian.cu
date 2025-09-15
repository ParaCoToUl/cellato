#include "./reference_implementation.hpp"
#include <cuda_runtime.h>
#include "traversers/cuda_utils.cuh"

namespace brian::reference {

// CUDA kernel for Brian's Brain (single step)
__global__ void brian_kernel(const brian_cell_state* current, brian_cell_state* next, 
                             int width, int height) {
    int x = blockIdx.x * blockDim.x + threadIdx.x + 1;
    int y = blockIdx.y * blockDim.y + threadIdx.y + 1;
        
    const int idx = y * width + x;
    
    // Count alive neighbors using explicit indexing (Moore neighborhood)
    int alive_neighbors = 
        (current[(y - 1) * width + (x - 1)] == brian_cell_state::alive) + // Top-left
        (current[(y - 1) * width +  x     ] == brian_cell_state::alive) + // Top
        (current[(y - 1) * width + (x + 1)] == brian_cell_state::alive) + // Top-right
        (current[ y      * width + (x - 1)] == brian_cell_state::alive) + // Left
        (current[ y      * width + (x + 1)] == brian_cell_state::alive) + // Right
        (current[(y + 1) * width + (x - 1)] == brian_cell_state::alive) + // Bottom-left
        (current[(y + 1) * width +  x     ] == brian_cell_state::alive) + // Bottom
        (current[(y + 1) * width + (x + 1)] == brian_cell_state::alive);  // Bottom-right
    
    // Apply Brian's Brain rules
    brian_cell_state cell_state = current[idx];
    
    if (cell_state == brian_cell_state::dead) {
        // Dead cell with exactly 2 alive neighbors becomes alive
        if (alive_neighbors == 2) {
            next[idx] = brian_cell_state::alive;
        } else {
            next[idx] = brian_cell_state::dead;
        }
    } else if (cell_state == brian_cell_state::alive) {
        // Alive cell always becomes dying
        next[idx] = brian_cell_state::dying;
    } else { // cell_state == brian_cell_state::dying
        // Dying cell always becomes dead
        next[idx] = brian_cell_state::dead;
    }
}

void runner::run_kernel(int steps) {
    if (!d_current || !d_next) {
        init_cuda();
    }
    
    // Set up grid and block dimensions
    dim3 block_size(_block_size_x, _block_size_y);

    auto _x_size_threads = _x_size - 2; // Exclude borders
    auto _y_size_threads = _y_size - 2; // Exclude borders

    dim3 grid_dim((_x_size_threads + block_size.x - 1) / block_size.x, 
                 (_y_size_threads + block_size.y - 1) / block_size.y);
    
    // Run steps iterations
    for (int i = 0; i < steps; i++) {
        // Launch kernel for one step
        brian_kernel<<<grid_dim, block_size>>>(d_current, d_next, _x_size, _y_size);

        // Swap pointers for next iteration
        brian_cell_state* temp = d_current;
        d_current = d_next;
        d_next = temp;
    }
    
    CUCH(cudaDeviceSynchronize());
}

} // namespace game_of_life::reference
