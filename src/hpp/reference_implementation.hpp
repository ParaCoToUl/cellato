#ifndef HPP_REFERENCE_IMPLEMENTATION_HPP
#define HPP_REFERENCE_IMPLEMENTATION_HPP

#include <vector>
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include "./algorithm.hpp"
#include "experiments/run_params.hpp"
#include "traversers/cuda_utils.cuh"

namespace hpp::reference {

struct runner {
    void init(const hpp_cell_state* grid,
              const cellato::run::run_params& params = cellato::run::run_params()) {

        _x_size = params.x_size;
        _y_size = params.y_size;
        _block_size_x = params.cuda_block_size_x;
        _block_size_y = params.cuda_block_size_y;
        _current_grid.resize(_x_size * _y_size);
        _next_grid.resize(_x_size * _y_size);  // Pre-allocate next_grid

        if (params.device == "CUDA") {
            if ((_x_size - 2) % _block_size_x != 0 || (_y_size - 2) % _block_size_y != 0) {
                std::cerr << "Grid size must be divisible by block size.\n";
                throw std::runtime_error("Invalid grid size for CUDA traverser.");
            }
        }

        // Copy input grid
        if (grid) {
            for (std::size_t i = 0; i < _x_size * _y_size; ++i) {
                _current_grid[i] = grid[i];
                _next_grid[i] = grid[i];
            }
        }
    }

    void init_cuda() {
        const size_t grid_size = _x_size * _y_size * sizeof(hpp_cell_state);
        
        // Allocate device memory
        CUCH(cudaMalloc(&d_current, grid_size));
        CUCH(cudaMalloc(&d_next, grid_size));
        
        // Copy data to device
        CUCH(cudaMemcpy(d_current, _current_grid.data(), grid_size, cudaMemcpyHostToDevice));
    }
    
    void run(int steps) {
        for (int step = 0; step < steps; ++step) {
            // Process each cell
            for (std::size_t y = 1; y < _y_size - 1; ++y) {
                for (std::size_t x = 1; x < _x_size - 1; ++x) {

                    std::size_t idx = y * _x_size + x;

                    auto top_neighbor = _current_grid[(y - 1) * _x_size + x];
                    auto bottom_neighbor = _current_grid[(y + 1) * _x_size + x];
                    auto left_neighbor = _current_grid[y * _x_size + (x - 1)];
                    auto right_neighbor = _current_grid[y * _x_size + (x + 1)];

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

                    _next_grid[idx] = result;
                }
            }
            
            // Swap grids
            _current_grid.swap(_next_grid);
        }
    }

    void run_on_cuda(int steps) {
        if (!d_current || !d_next) {
            init_cuda();
        }
        run_kernel(steps);
    }

    std::vector<hpp_cell_state> fetch_result() {
        if (d_current) {
            // Copy result back from device to host
            const size_t grid_size = _x_size * _y_size * sizeof(hpp_cell_state);
            CUCH(cudaMemcpy(_current_grid.data(), d_current, grid_size, cudaMemcpyDeviceToHost));
            
            // Free CUDA memory
            CUCH(cudaFree(d_current));
            CUCH(cudaFree(d_next));
            d_current = nullptr;
            d_next = nullptr;
        }
        return _current_grid;
    }

    ~runner() {
        if (d_current) {
            cudaFree(d_current);
            d_current = nullptr;
        }
        if (d_next) {
            cudaFree(d_next);
            d_next = nullptr;
        }
    }

private:
    std::size_t _x_size, _y_size;
    int _block_size_x = 16;
    int _block_size_y = 16;
    std::vector<hpp_cell_state> _current_grid;
    std::vector<hpp_cell_state> _next_grid;
    
    // Device pointers
    hpp_cell_state* d_current = nullptr;
    hpp_cell_state* d_next = nullptr;

    void run_kernel(int steps);
};

} // namespace hpp::reference

#endif // HPP_REFERENCE_IMPLEMENTATION_HPP