#ifndef TRAFFIC_REFERENCE_IMPLEMENTATION_HPP
#define TRAFFIC_REFERENCE_IMPLEMENTATION_HPP

#include "./algorithm.hpp"
#include "cellato/experiments/run_params.hpp"
#include "cellato/config.hpp"
#if CELLATO_ENABLE_CUDA
#include "cellato/traversers/cuda_utils.cuh"
#endif
#include "cuda_instantiation/indexing.hpp"
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace traffic::reference {
using namespace ::reference::indexing;

struct runner {
    static constexpr std::size_t x_margin = indexer::x_margin;
    static constexpr std::size_t y_margin = indexer::y_margin;

    void init(const traffic_cell_state* grid, const cellato::run::run_params& params = cellato::run::run_params()) {

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
        const size_t grid_size = _x_size * _y_size * sizeof(traffic_cell_state);

        // Allocate device memory
        CUCH(cudaMalloc(&d_current, grid_size));
        CUCH(cudaMalloc(&d_next, grid_size));

        // Copy data to device
        CUCH(cudaMemcpy(d_current, _current_grid.data(), grid_size, cudaMemcpyHostToDevice));
#else
        throw std::runtime_error("CUDA support is disabled. Rebuild with CELLATO_ENABLE_CUDA=ON.");
#endif
    }

    template <traffic_cell_state movable, traffic_cell_state stationary>
    traffic_cell_state
    rule(traffic_cell_state incoming_neighbor, traffic_cell_state current_state, traffic_cell_state outgoing_neighbor) {
        if (current_state == movable) {
            if (outgoing_neighbor == traffic_cell_state::empty) {
                return traffic_cell_state::empty;
            } else {
                return movable; // Car stays if it can't move out
            }
        } else if (current_state == traffic_cell_state::empty) {
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
    }

    void run(int steps) {
        indexer idx(_x_size, _y_size);

        for (int step = 0; step < steps; ++step) {
            // Process each cell, accounting for margins
            for (std::size_t y = y_margin; y < _y_size - y_margin; ++y) {
                for (std::size_t x = x_margin; x < _x_size - x_margin; ++x) {
                    // Forest traffic rules
                    const int center_idx = idx.at(x, y);

                    traffic_cell_state cell_state = _current_grid[center_idx];
                    traffic_cell_state left_neighbor = _current_grid[idx.at(x - 1, y)];
                    traffic_cell_state right_neighbor = _current_grid[idx.at(x + 1, y)];
                    traffic_cell_state up_neighbor = _current_grid[idx.at(x, y - 1)];
                    traffic_cell_state down_neighbor = _current_grid[idx.at(x, y + 1)];

                    if (step % 2 == 0) {
                        _next_grid[center_idx] = rule<traffic_cell_state::red_car, traffic_cell_state::blue_car>(
                            left_neighbor, cell_state, right_neighbor);
                    } else {
                        _next_grid[center_idx] = rule<traffic_cell_state::blue_car, traffic_cell_state::red_car>(
                            up_neighbor, cell_state, down_neighbor);
                    }
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

    std::vector<traffic_cell_state> fetch_result() {
#if CELLATO_ENABLE_CUDA
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
    std::vector<traffic_cell_state> _current_grid;
    std::vector<traffic_cell_state> _next_grid;

#if CELLATO_ENABLE_CUDA
    // Device pointers
    traffic_cell_state* d_current = nullptr;
    traffic_cell_state* d_next = nullptr;

    void run_kernel(int steps);
#endif
};

} // namespace traffic::reference

#endif // TRAFFIC_REFERENCE_IMPLEMENTATION_HPP
