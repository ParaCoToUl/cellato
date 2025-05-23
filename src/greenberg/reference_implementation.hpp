#ifndef GREENBERG_REFERENCE_IMPLEMENTATION_HPP
#define GREENBERG_REFERENCE_IMPLEMENTATION_HPP

#include <vector>
#include <cstddef>
#include "./algorithm.hpp"

namespace greenberg::reference {

struct runner {
    void init(int* grid, std::size_t x_size, std::size_t y_size) {
        _x_size = x_size;
        _y_size = y_size;
        _current_grid.resize(x_size * y_size);
        
        // Copy input grid
        for (std::size_t i = 0; i < x_size * y_size; ++i) {
            _current_grid[i] = static_cast<ghm_cell_state>(grid[i]);
        }
    }

    void run(int steps) {
        std::vector<ghm_cell_state> next_grid(_x_size * _y_size);
        
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
                    
                    next_grid[y * _x_size + x] = next;
                }
            }
            
            // Copy border cells unchanged
            for (std::size_t y = 0; y < _y_size; ++y) {
                for (std::size_t x = 0; x < _x_size; ++x) {
                    if (x == 0 || x == _x_size - 1 || y == 0 || y == _y_size - 1) {
                        next_grid[y * _x_size + x] = _current_grid[y * _x_size + x];
                    }
                }
            }
            
            // Swap grids
            _current_grid.swap(next_grid);
        }
    }

    std::vector<int> fetch_result() {
        std::vector<int> result(_x_size * _y_size);
        for (std::size_t i = 0; i < _current_grid.size(); ++i) {
            result[i] = static_cast<int>(_current_grid[i]);
        }
        return result;
    }

private:
    std::size_t _x_size, _y_size;
    std::vector<ghm_cell_state> _current_grid;
};

} // namespace greenberg::reference

#endif // GREENBERG_REFERENCE_IMPLEMENTATION_HPP