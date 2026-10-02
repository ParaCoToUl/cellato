#ifndef CYCLIC_REFERENCE_IMPLEMENTATION_HPP
#define CYCLIC_REFERENCE_IMPLEMENTATION_HPP

#include "./algorithm.hpp"
#include "cellato/config.hpp"
#include "cellato/experiments/run_params.hpp"
#if CELLATO_ENABLE_CUDA
#include "cellato/traversers/cuda_utils.cuh"
#endif
#include "cuda_instantiation/indexing.hpp"
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace cyclic::reference {
using namespace ::reference::indexing;

struct runner {
    static constexpr std::size_t x_margin = indexer::x_margin;
    static constexpr std::size_t y_margin = indexer::y_margin;

    void init(const cyclic_cell_state* grid, const cellato::run::run_params& params = cellato::run::run_params()) {

        _x_size = params.x_size;
        _y_size = params.y_size;
#if CELLATO_ENABLE_CUDA
        _block_size_x = params.cuda_block_size_x;
        _block_size_y = params.cuda_block_size_y;
#endif
        _current_grid.resize(_x_size * _y_size);
        _next_grid.resize(_x_size * _y_size); // Pre-allocate next_grid

        if (params.device == "CUDA") {
#if CELLATO_ENABLE_CUDA
            if ((_x_size - 2 * x_margin) % _block_size_x != 0 || (_y_size - 2 * y_margin) % _block_size_y != 0) {
                std::cerr << "Grid size must be divisible by block size.\n";
                throw std::runtime_error("Invalid grid size for CUDA traverser.");
            }
#else
            throw std::runtime_error("CUDA support is disabled. Rebuild with CELLATO_ENABLE_CUDA=ON.");
#endif
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
#if CELLATO_ENABLE_CUDA
        const size_t grid_size = _x_size * _y_size * sizeof(cyclic_cell_state);

        // Allocate device memory
        CUCH(cudaMalloc(&d_current, grid_size));
        CUCH(cudaMalloc(&d_next, grid_size));

        // Copy data to device
        CUCH(cudaMemcpy(d_current, _current_grid.data(), grid_size, cudaMemcpyHostToDevice));
#else
        throw std::runtime_error("CUDA support is disabled. Rebuild with CELLATO_ENABLE_CUDA=ON.");
#endif
    }

    void run(int steps) {
        indexer idx(_x_size, _y_size);

        for (int step = 0; step < steps; ++step) {
            // Process each cell, accounting for margins
            for (std::size_t y = y_margin; y < _y_size - y_margin; ++y) {
                for (std::size_t x = x_margin; x < _x_size - x_margin; ++x) {
                    // Forest cyclic rules
                    const int center_idx = idx.at(x, y);
                    cyclic_cell_state current = _current_grid[center_idx];
                    cyclic_cell_state next_state = current;

                    constexpr int states = cyclic::STATES;

                    int target_state = (current + 1) % states;
                    int count = 0;

                    for (int dy = -1; dy <= 1; dy++) {
                        for (int dx = -1; dx <= 1; dx++) {
                            if (dx == 0 && dy == 0) continue;

                            int neighbor_idx = idx.at(x + dx, y + dy);
                            if (_current_grid[neighbor_idx] == target_state) {
                                count++;
                            }
                        }
                    }

                    if (count >= 1) {
                        next_state = target_state;
                    } else {
                        next_state = current;
                    }

                    _next_grid[center_idx] = next_state;
                }
            }

            // Swap grids
            _current_grid.swap(_next_grid);
        }
    }

    void run_on_cuda(int steps) {
#if CELLATO_ENABLE_CUDA
        if (!d_current || !d_next) {
            init_cuda();
        }
        run_kernel(steps);
#else
        (void)steps;
        throw std::runtime_error("CUDA support is disabled. Rebuild with CELLATO_ENABLE_CUDA=ON.");
#endif
    }

    std::vector<cyclic_cell_state> fetch_result() {
#if CELLATO_ENABLE_CUDA
        if (d_current) {
            // Copy result back from device to host
            const size_t grid_size = _x_size * _y_size * sizeof(cyclic_cell_state);
            CUCH(cudaMemcpy(_current_grid.data(), d_current, grid_size, cudaMemcpyDeviceToHost));

            // Free CUDA memory
            CUCH(cudaFree(d_current));
            CUCH(cudaFree(d_next));
            d_current = nullptr;
            d_next = nullptr;
        }
#endif
        return _current_grid;
    }

#if CELLATO_ENABLE_CUDA
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
#endif

private:
    std::size_t _x_size, _y_size;
#if CELLATO_ENABLE_CUDA
    int _block_size_x = 16;
    int _block_size_y = 16;
#endif
    std::vector<cyclic_cell_state> _current_grid;
    std::vector<cyclic_cell_state> _next_grid;

#if CELLATO_ENABLE_CUDA
    // Device pointers
    cyclic_cell_state* d_current = nullptr;
    cyclic_cell_state* d_next = nullptr;

    void run_kernel(int steps);
#endif
};

} // namespace cyclic::reference

#endif // CYCLIC_REFERENCE_IMPLEMENTATION_HPP
