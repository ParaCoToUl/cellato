#ifndef TRAFFIC_REFERENCE_IMPLEMENTATION_HPP
#define TRAFFIC_REFERENCE_IMPLEMENTATION_HPP

#include <vector>
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include "./algorithm.hpp"
#include "experiments/run_params.hpp"
#include "traversers/cuda_utils.cuh"

namespace traffic::reference {

struct runner {
    void init(const traffic_cell_state* grid,
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
        const size_t grid_size = _x_size * _y_size * sizeof(traffic_cell_state);
        
        // Allocate device memory
        CUCH(cudaMalloc(&d_current, grid_size));
        CUCH(cudaMalloc(&d_next, grid_size));
        
        // Copy data to device
        CUCH(cudaMemcpy(d_current, _current_grid.data(), grid_size, cudaMemcpyHostToDevice));
    }
    
    template <traffic_cell_state movable, traffic_cell_state stationary>
    traffic_cell_state rule(traffic_cell_state incoming_neighbor, traffic_cell_state current_state, traffic_cell_state outgoing_neighbor) {
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

    void run(int steps) {
        for (int step = 0; step < steps; ++step) {
            // Process each cell
            for (std::size_t y = 1; y < _y_size - 1; ++y) {
                for (std::size_t x = 1; x < _x_size - 1; ++x) {
                    // Forest traffic rules

                    traffic_cell_state cell_state = _current_grid[y * _x_size + x];
                    traffic_cell_state left_neighbor = _current_grid[y * _x_size + (x - 1)];
                    traffic_cell_state right_neighbor = _current_grid[y * _x_size + (x + 1)];
                    traffic_cell_state up_neighbor = _current_grid[(y - 1) * _x_size + x];
                    traffic_cell_state down_neighbor = _current_grid[(y + 1) * _x_size + x];

                    if (step % 2 == 0) {
                        _next_grid[y * _x_size + x] = rule<traffic_cell_state::red_car, traffic_cell_state::blue_car>(left_neighbor, cell_state, right_neighbor);
                    } else {
                        _next_grid[y * _x_size + x] = rule<traffic_cell_state::blue_car, traffic_cell_state::red_car>(up_neighbor, cell_state, down_neighbor);
                    }
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

    std::vector<traffic_cell_state> fetch_result() {
        if (d_current) {
            // Copy result back from device to host
            const size_t grid_size = _x_size * _y_size * sizeof(traffic_cell_state);
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
    std::vector<traffic_cell_state> _current_grid;
    std::vector<traffic_cell_state> _next_grid;
    
    // Device pointers
    traffic_cell_state* d_current = nullptr;
    traffic_cell_state* d_next = nullptr;

    void run_kernel(int steps);
};

} // namespace traffic::reference

#endif // TRAFFIC_REFERENCE_IMPLEMENTATION_HPP