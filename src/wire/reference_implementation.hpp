#ifndef WIRE_REFERENCE_IMPLEMENTATION_HPP
#define WIRE_REFERENCE_IMPLEMENTATION_HPP

#include <vector>
#include <cstddef>
#include "./algorithm.hpp"

namespace wire::reference {

struct runner {
    void init(int* grid, std::size_t x_size, std::size_t y_size) {
        _x_size = x_size;
        _y_size = y_size;
        _current_grid.resize(x_size * y_size);
        
        // Copy input grid
        for (std::size_t i = 0; i < x_size * y_size; ++i) {
            _current_grid[i] = static_cast<wire_cell_state>(grid[i]);
        }
    }

    void run(int steps) {
        std::vector<wire_cell_state> next_grid(_x_size * _y_size);
        
        for (int step = 0; step < steps; ++step) {
            // Process each cell
            for (std::size_t y = 1; y < _y_size - 1; ++y) {
                for (std::size_t x = 1; x < _x_size - 1; ++x) {
                    // WireWorld rules
                    wire_cell_state current = _current_grid[y * _x_size + x];
                    wire_cell_state next = current;
                    
                    switch (current) {
                        case wire_cell_state::empty:
                            // Empty remains empty
                            next = wire_cell_state::empty;
                            break;
                            
                        case wire_cell_state::electron_head:
                            // Electron head becomes electron tail
                            next = wire_cell_state::electron_tail;
                            break;
                            
                        case wire_cell_state::electron_tail:
                            // Electron tail becomes conductor
                            next = wire_cell_state::conductor;
                            break;
                            
                        case wire_cell_state::conductor:
                            // Conductor becomes electron head if exactly 1 or 2 neighboring cells 
                            // are electron heads
                            int electron_head_count = 0;
                            for (int dy = -1; dy <= 1; ++dy) {
                                for (int dx = -1; dx <= 1; ++dx) {
                                    if (dx == 0 && dy == 0) continue; // Skip self
                                    
                                    std::size_t nx = x + dx;
                                    std::size_t ny = y + dy;
                                    if (_current_grid[ny * _x_size + nx] == wire_cell_state::electron_head) {
                                        electron_head_count++;
                                    }
                                }
                            }
                            
                            if (electron_head_count == 1 || electron_head_count == 2) {
                                next = wire_cell_state::electron_head;
                            } else {
                                next = wire_cell_state::conductor;
                            }
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
    std::vector<wire_cell_state> _current_grid;
};

} // namespace wire::reference

#endif // WIRE_REFERENCE_IMPLEMENTATION_HPP