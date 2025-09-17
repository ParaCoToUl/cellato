#include "./reference_implementation.hpp"
#include <cuda_runtime.h>
#include "traversers/cuda_utils.cuh"

namespace hpp::reference {

// CUDA kernel for Forest hpp (single step)
__global__ void hpp_kernel(const hpp_cell_state* current, hpp_cell_state* next, 
                            int width, int height) {
    int x = blockIdx.x * blockDim.x + threadIdx.x + 1;
    int y = blockIdx.y * blockDim.y + threadIdx.y + 1;
    
    const int idx = y * width + x;

    auto top_neighbor = current[(y - 1) * width + x];
    auto bottom_neighbor = current[(y + 1) * width + x];
    auto left_neighbor = current[y * width + (x - 1)];
    auto right_neighbor = current[y * width + (x + 1)];

    auto incoming_from_top = (top_neighbor & 0b0001);
    auto incoming_from_bottom = (bottom_neighbor & 0b0010);
    auto incoming_from_left = (left_neighbor & 0b0100);
    auto incoming_from_right = (right_neighbor & 0b1000);

    auto vertical_collision_appears = (incoming_from_top != 0) && (incoming_from_bottom != 0);
    auto horizontal_collision_appears = (incoming_from_left != 0) && (incoming_from_right != 0);

    auto combined_vertical_incoming = incoming_from_top | incoming_from_bottom;
    auto combined_horizontal_incoming = incoming_from_left | incoming_from_right;

    hpp_cell_state result = 0;

    if (vertical_collision_appears) {
        result |= 0b1100; // horizontal outgoing
    } else {
        result |= combined_vertical_incoming; // pass vertical incoming
    }

    if (horizontal_collision_appears) {
        result |= 0b0011; // vertical outgoing
    } else {
        result |= combined_horizontal_incoming; // pass horizontal incoming
    }

    next[idx] = result;
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
        hpp_kernel<<<grid_dim, block_size>>>(d_current, d_next, _x_size, _y_size);
        
        // Swap pointers for next iteration
        hpp_cell_state* temp = d_current;
        d_current = d_next;
        d_next = temp;
    }
    
    CUCH(cudaDeviceSynchronize());
}

} // namespace hpp::reference
