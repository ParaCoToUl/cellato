#ifndef FIRE_REFERENCE_IMPLEMENTATION_HPP
#define FIRE_REFERENCE_IMPLEMENTATION_HPP

#include <vector>
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include "./algorithm.hpp"
#include "experiments/run_params.hpp"
#include "traversers/cuda_utils.cuh"

namespace fire::reference {

struct runner {
    void init(const fire_cell_state* grid,
              const cellib::run::run_params& params = cellib::run::run_params()) {

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
        const size_t grid_size = _x_size * _y_size * sizeof(fire_cell_state);
        
        // Allocate device memory
        CUCH(cudaMalloc(&d_current, grid_size));
        CUCH(cudaMalloc(&d_next, grid_size));
        
        // Copy data to device
        CUCH(cudaMemcpy(d_current, _current_grid.data(), grid_size, cudaMemcpyHostToDevice));
    }
    
    void run(int steps) {
        const int dx[4] = {0, 0, 1, -1};
        const int dy[4] = {1, -1, 0, 0};
        
        for (int step = 0; step < steps; ++step) {
            // Process each cell
            for (std::size_t y = 1; y < _y_size - 1; ++y) {
                for (std::size_t x = 1; x < _x_size - 1; ++x) {
                    // Forest fire rules
                    fire_cell_state current = _current_grid[y * _x_size + x];
                    fire_cell_state next = current;
                    
                    switch (current) {
                        case fire_cell_state::empty:
                            // Empty remains empty
                            next = fire_cell_state::empty;
                            break;
                            
                        case fire_cell_state::tree:
                            // Tree catches fire if any von Neumann neighbor (N,E,S,W) is on fire
                            next = fire_cell_state::tree;
                            // Check only the 4 adjacent neighbors (von Neumann neighborhood)
                            for (int i = 0; i < 4; ++i) {
                                std::size_t nx = x + dx[i];
                                std::size_t ny = y + dy[i];
                                if (_current_grid[ny * _x_size + nx] == fire_cell_state::fire) {
                                    next = fire_cell_state::fire;
                                    break;
                                }
                            }
                            break;
                            
                        case fire_cell_state::fire:
                            // Fire becomes ash
                            next = fire_cell_state::ash;
                            break;
                            
                        case fire_cell_state::ash:
                            // Ash remains ash if it has fire neighbors, otherwise becomes empty
                            bool has_fire_neighbor = false;
                            
                            // Check only the 4 adjacent neighbors (von Neumann neighborhood)
                            for (int i = 0; i < 4; ++i) {
                                std::size_t nx = x + dx[i];
                                std::size_t ny = y + dy[i];
                                if (_current_grid[ny * _x_size + nx] == fire_cell_state::fire) {
                                    has_fire_neighbor = true;
                                    break;
                                }
                            }
                            
                            if (has_fire_neighbor) {
                                next = fire_cell_state::ash;
                            } else {
                                next = fire_cell_state::empty;
                            }
                            break;
                    }
                    
                    _next_grid[y * _x_size + x] = next;
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

    std::vector<fire_cell_state> fetch_result() {
        if (d_current) {
            // Copy result back from device to host
            const size_t grid_size = _x_size * _y_size * sizeof(fire_cell_state);
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
    std::vector<fire_cell_state> _current_grid;
    std::vector<fire_cell_state> _next_grid;
    
    // Device pointers
    fire_cell_state* d_current = nullptr;
    fire_cell_state* d_next = nullptr;

    void run_kernel(int steps);
};

} // namespace fire::reference

#endif // FIRE_REFERENCE_IMPLEMENTATION_HPP