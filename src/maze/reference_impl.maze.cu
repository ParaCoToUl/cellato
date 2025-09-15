#include "./reference_implementation.hpp"
#include <cuda_runtime.h>
#include "traversers/cuda_utils.cuh"

namespace maze::reference {

__global__ void maze_kernel(const maze_cell_state* current, maze_cell_state* next, 
                            int width, int height) {
    int x = blockIdx.x * blockDim.x + threadIdx.x + 1;
    int y = blockIdx.y * blockDim.y + threadIdx.y + 1;
    
    const int idx = y * width + x;
    
    maze_cell_state cell_state = current[idx];
    
    // Count wall neighbors in Moore neighborhood (8 surrounding cells)
    int wall_count = 0;
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            if (dx == 0 && dy == 0) continue; // Skip the center cell
            if (current[(y + dy) * width + (x + dx)] == maze_cell_state::wall) {
                wall_count++;
            }
        }
    }
    
    // Apply maze algorithm rules
    maze_cell_state next_state;
    if (wall_count == 3) {
        next_state = maze_cell_state::wall;
    } else if (cell_state == maze_cell_state::wall && wall_count < 6) {
        next_state = maze_cell_state::wall;
    } else {
        next_state = maze_cell_state::empty;
    }

    next[idx] = next_state;
}

void runner::run_kernel(int steps) {
    if (!d_current || !d_next) {
        init_cuda();
    }
    
    // Set up grid and block dimensions
    dim3 block_size(_block_size_x, _block_size_y);
    
    // Exclude borders from calculation
    auto _x_size_threads = _x_size - 2;
    auto _y_size_threads = _y_size - 2;
    
    dim3 grid_dim((_x_size_threads + block_size.x - 1) / block_size.x, 
                 (_y_size_threads + block_size.y - 1) / block_size.y);
    
    // Run steps iterations
    for (int i = 0; i < steps; i++) {
        // Launch kernel for one step
        maze_kernel<<<grid_dim, block_size>>>(d_current, d_next, _x_size, _y_size);
        
        // Swap pointers for next iteration
        maze_cell_state* temp = d_current;
        d_current = d_next;
        d_next = temp;
    }
    
    CUCH(cudaDeviceSynchronize());
}

} // namespace maze::reference
