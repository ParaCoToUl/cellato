#include "./reference_implementation.hpp"
#include <cuda_runtime.h>
#include "traversers/cuda_utils.cuh"

namespace greenberg::reference {

// CUDA kernel for Greenberg-Hastings Model (single step)
__global__ void greenberg_kernel(const ghm_cell_state* current, ghm_cell_state* next, 
                                int width, int height) {
    int x = blockIdx.x * blockDim.x + threadIdx.x + 1;
    int y = blockIdx.y * blockDim.y + threadIdx.y + 1;
    
    const int idx = y * width + x;
    
    ghm_cell_state cell_state = current[idx];
    ghm_cell_state next_state = cell_state;
    
    switch (cell_state) {
        case ghm_cell_state::quiescent:
            // Quiescent becomes excited if it has at least one excited neighbor
            // Check all 8 neighbors using explicit indexing
            next_state = ghm_cell_state::quiescent;
            if (current[(y - 1) * width + (x - 1)] == ghm_cell_state::excited || // Top-left
                current[(y - 1) * width +  x     ] == ghm_cell_state::excited || // Top
                current[(y - 1) * width + (x + 1)] == ghm_cell_state::excited || // Top-right
                current[ y      * width + (x - 1)] == ghm_cell_state::excited || // Left
                current[ y      * width + (x + 1)] == ghm_cell_state::excited || // Right
                current[(y + 1) * width + (x - 1)] == ghm_cell_state::excited || // Bottom-left
                current[(y + 1) * width +  x     ] == ghm_cell_state::excited || // Bottom
                current[(y + 1) * width + (x + 1)] == ghm_cell_state::excited) { // Bottom-right
                next_state = ghm_cell_state::excited;
            }
            break;
            
        case ghm_cell_state::excited:
            // Excited cell becomes refractory_1
            next_state = ghm_cell_state::refractory_1;
            break;
            
        case ghm_cell_state::refractory_1:
            // Progress through refractory states
            next_state = ghm_cell_state::refractory_2;
            break;
            
        case ghm_cell_state::refractory_2:
            next_state = ghm_cell_state::refractory_3;
            break;
            
        case ghm_cell_state::refractory_3:
            next_state = ghm_cell_state::refractory_4;
            break;
            
        case ghm_cell_state::refractory_4:
            next_state = ghm_cell_state::refractory_5;
            break;
            
        case ghm_cell_state::refractory_5:
            next_state = ghm_cell_state::refractory_6;
            break;
            
        case ghm_cell_state::refractory_6:
            // Last refractory state returns to quiescent
            next_state = ghm_cell_state::quiescent;
            break;
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
        greenberg_kernel<<<grid_dim, block_size>>>(d_current, d_next, _x_size, _y_size);
        
        // Swap pointers for next iteration
        ghm_cell_state* temp = d_current;
        d_current = d_next;
        d_next = temp;
    }
    
    CUCH(cudaDeviceSynchronize());
}

} // namespace greenberg::reference
