#include "./reference_implementation.hpp"
#include <cuda_runtime.h>
#include "traversers/cuda_utils.cuh"

namespace fire::reference {

// CUDA kernel for Forest Fire (single step)
__global__ void fire_kernel(const fire_cell_state* current, fire_cell_state* next, 
                            int width, int height) {
    int x = blockIdx.x * blockDim.x + threadIdx.x + 1;
    int y = blockIdx.y * blockDim.y + threadIdx.y + 1;
    
    const int idx = y * width + x;
    
    fire_cell_state cell_state = current[idx];
    fire_cell_state next_state = cell_state;
    
    const int dx[4] = {0, 0, 1, -1};
    const int dy[4] = {1, -1, 0, 0};

    switch (cell_state) {
        case fire_cell_state::empty:
            // Empty remains empty
            next_state = fire_cell_state::empty;
            break;
            
        case fire_cell_state::tree:
            // Tree catches fire if any von Neumann neighbor is on fire
            next_state = fire_cell_state::tree;
            // Check von Neumann neighbors (N, S, E, W)
            for (int i = 0; i < 4; i++) {
                int nx = x + dx[i];
                int ny = y + dy[i];
                int nidx = ny * width + nx;
                if (current[nidx] == fire_cell_state::fire) {
                    next_state = fire_cell_state::fire;
                    break;
                }
            }
            break;
            
        case fire_cell_state::fire:
            // Fire becomes ash
            next_state = fire_cell_state::ash;
            break;
            
        case fire_cell_state::ash:
            // Check if ash has fire neighbors
            bool has_fire_neighbor = false;
            // Check only von Neumann neighbors (N, S, E, W)
            for (int i = 0; i < 4; i++) {
                int nx = x + dx[i];
                int ny = y + dy[i];
                int nidx = ny * width + nx;
                if (current[nidx] == fire_cell_state::fire) {
                    has_fire_neighbor = true;
                    break;
                }
            }
            
            // Ash cell with fire neighbors remains ash, others become empty
            if (has_fire_neighbor) {
                next_state = fire_cell_state::ash;
            } else {
                next_state = fire_cell_state::empty;
            }
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
        fire_kernel<<<grid_dim, block_size>>>(d_current, d_next, _x_size, _y_size);
        
        // Swap pointers for next iteration
        fire_cell_state* temp = d_current;
        d_current = d_next;
        d_next = temp;
    }
    
    CUCH(cudaDeviceSynchronize());
}

} // namespace fire::reference
