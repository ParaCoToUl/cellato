#include "./reference_implementation.hpp"
#include <cuda_runtime.h>
#include "traversers/cuda_utils.cuh"

namespace traffic::reference {

template <traffic_cell_state movable, traffic_cell_state stationary>
__device__ traffic_cell_state rule(traffic_cell_state incoming_neighbor, traffic_cell_state current_state, traffic_cell_state outgoing_neighbor) {
    if (current_state == movable) {
        if (outgoing_neighbor == traffic_cell_state::empty) {
            return traffic_cell_state::empty;
        } else {
            return movable; // Car stays if it can't move out
        }
    } 
    else if (current_state == traffic_cell_state::empty) {
        if (incoming_neighbor == movable) {
            return movable;
        } else {
            return traffic_cell_state::empty; // Stays empty if no car moves in
        }
    } 
    // Must be stationary car
    else {
        return stationary; // Stationary cars don't move
    }
};

// CUDA kernel for Forest traffic (single step)
__global__ void traffic_kernel(const traffic_cell_state* current, traffic_cell_state* next, 
                            int width, int height, int step) {
    int x = blockIdx.x * blockDim.x + threadIdx.x + 1;
    int y = blockIdx.y * blockDim.y + threadIdx.y + 1;
    
    const int idx = y * width + x;
    
    traffic_cell_state cell_state = current[idx];
    traffic_cell_state left_neighbor = current[y * width + (x - 1)];
    traffic_cell_state right_neighbor = current[y * width + (x + 1)];
    traffic_cell_state up_neighbor = current[(y - 1) * width + x];
    traffic_cell_state down_neighbor = current[(y + 1) * width + x];

    if (step % 2 == 0) {
        next[idx] = rule<traffic_cell_state::red_car, traffic_cell_state::blue_car>(left_neighbor, cell_state, right_neighbor);
    } else {
        next[idx] = rule<traffic_cell_state::blue_car, traffic_cell_state::red_car>(up_neighbor, cell_state, down_neighbor);
    }
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
        traffic_kernel<<<grid_dim, block_size>>>(d_current, d_next, _x_size, _y_size, i);
        
        // Swap pointers for next iteration
        traffic_cell_state* temp = d_current;
        d_current = d_next;
        d_next = temp;
    }
    
    CUCH(cudaDeviceSynchronize());
}

} // namespace traffic::reference
