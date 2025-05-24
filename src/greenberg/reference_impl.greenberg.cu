#include "./reference_implementation.hpp"
#include <cuda_runtime.h>
#include "traversers/cuda_utils.cuh"

namespace greenberg::reference {

// CUDA kernel for Greenberg-Hastings Model (single step)
__global__ void greenberg_kernel(const ghm_cell_state* current, ghm_cell_state* next, 
                                int width, int height) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    
    if (x == 0 || y == 0 || x >= width - 1 || y >= height - 1) return;
    
    const int idx = y * width + x;
    
    ghm_cell_state cell_state = current[idx];
    ghm_cell_state next_state = cell_state;
    
    switch (cell_state) {
        case ghm_cell_state::quiescent:
            // Quiescent becomes excited if it has at least one excited neighbor
            next_state = ghm_cell_state::quiescent;
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    if (dx == 0 && dy == 0) continue; // Skip self
                    
                    int nx = x + dx;
                    int ny = y + dy;
                    int nidx = ny * width + nx;
                    if (current[nidx] == ghm_cell_state::excited) {
                        next_state = ghm_cell_state::excited;
                        break;
                    }
                }
                if (next_state == ghm_cell_state::excited) break;
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
    dim3 grid_dim((_x_size + block_size.x - 1) / block_size.x, 
                 (_y_size + block_size.y - 1) / block_size.y);
    
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
