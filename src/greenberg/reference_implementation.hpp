#ifndef GREENBERG_REFERENCE_IMPLEMENTATION_HPP
#define GREENBERG_REFERENCE_IMPLEMENTATION_HPP

#include <vector>
#include <cstddef>
#include "./algorithm.hpp"
#include "experiments/run_params.hpp"
#include "traversers/cuda_utils.cuh"

namespace greenberg::reference {

struct runner {
    void init(const ghm_cell_state* grid, std::size_t x_size, std::size_t y_size,
              const cellib::run::run_params& params = cellib::run::run_params()) {
        _x_size = x_size;
        _y_size = y_size;
        _block_size_x = params.cuda_block_size_x;
        _block_size_y = params.cuda_block_size_y;
        _current_grid.resize(x_size * y_size);
        _next_grid.resize(x_size * y_size);  // Pre-allocate next_grid
        
        // Copy input grid
        if (grid) {
            for (std::size_t i = 0; i < x_size * y_size; ++i) {
                _current_grid[i] = grid[i];
            }
        }
    }

    void init_cuda() {
        const size_t grid_size = _x_size * _y_size * sizeof(ghm_cell_state);
        
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
                    ghm_cell_state current = _current_grid[y * _x_size + x];
                    ghm_cell_state next = current;
                    
                    switch (current) {
                        case ghm_cell_state::quiescent:
                            // Quiescent cell becomes excited if it has at least one excited neighbor
                            for (int dy = -1; dy <= 1; ++dy) {
                                for (int dx = -1; dx <= 1; ++dx) {
                                    if (dx == 0 && dy == 0) continue; // Skip self
                                    
                                    std::size_t nx = x + dx;
                                    std::size_t ny = y + dy;
                                    if (_current_grid[ny * _x_size + nx] == ghm_cell_state::excited) {
                                        next = ghm_cell_state::excited;
                                        break;
                                    }
                                }
                                if (next == ghm_cell_state::excited) break;
                            }
                            break;
                            
                        case ghm_cell_state::excited:
                            // Excited cell becomes refractory_1
                            next = ghm_cell_state::refractory_1;
                            break;
                            
                        case ghm_cell_state::refractory_1:
                            // Refractory cells progress through refractory states
                            next = ghm_cell_state::refractory_2;
                            break;
                            
                        case ghm_cell_state::refractory_2:
                            next = ghm_cell_state::refractory_3;
                            break;
                            
                        case ghm_cell_state::refractory_3:
                            next = ghm_cell_state::refractory_4;
                            break;
                            
                        case ghm_cell_state::refractory_4:
                            next = ghm_cell_state::refractory_5;
                            break;
                            
                        case ghm_cell_state::refractory_5:
                            next = ghm_cell_state::refractory_6;
                            break;
                            
                        case ghm_cell_state::refractory_6:
                            // Last refractory state returns to quiescent
                            next = ghm_cell_state::quiescent;
                            break;
                    }
                    
                    _next_grid[y * _x_size + x] = next;
                }
            }
            
            // Copy border cells unchanged
            for (std::size_t y = 0; y < _y_size; ++y) {
                for (std::size_t x = 0; x < _x_size; ++x) {
                    if (x == 0 || x == _x_size - 1 || y == 0 || y == _y_size - 1) {
                        _next_grid[y * _x_size + x] = _current_grid[y * _x_size + x];
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

    std::vector<ghm_cell_state> fetch_result() {
        if (d_current) {
            // Copy result back from device to host
            const size_t grid_size = _x_size * _y_size * sizeof(ghm_cell_state);
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
    std::vector<ghm_cell_state> _current_grid;
    std::vector<ghm_cell_state> _next_grid;
    
    // Device pointers
    ghm_cell_state* d_current = nullptr;
    ghm_cell_state* d_next = nullptr;

    void run_kernel(int steps);
};

} // namespace greenberg::reference

#endif // GREENBERG_REFERENCE_IMPLEMENTATION_HPP