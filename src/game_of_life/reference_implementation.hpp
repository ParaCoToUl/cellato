#ifndef GAME_OF_LIFE_REFERENCE_IMPLEMENTATION_HPP
#define GAME_OF_LIFE_REFERENCE_IMPLEMENTATION_HPP

#include <vector>
#include <cstddef>
#include "./algorithm.hpp"

namespace game_of_life::reference {

struct runner {
    void init(int* grid, std::size_t x_size, std::size_t y_size) {
        _x_size = x_size;
        _y_size = y_size;
        _current_grid.resize(x_size * y_size);
        
        // Copy input grid
        for (std::size_t i = 0; i < x_size * y_size; ++i) {
            _current_grid[i] = static_cast<gol_cell_state>(grid[i]);
        }
    }

    void run(int steps) {
        std::vector<gol_cell_state> next_grid(_x_size * _y_size);
        
        for (int step = 0; step < steps; ++step) {
            // Process each cell
            for (std::size_t y = 1; y < _y_size - 1; ++y) {
                for (std::size_t x = 1; x < _x_size - 1; ++x) {
                    // Count live neighbors (Moore neighborhood)
                    int live_neighbors = 0;
                    for (int dy = -1; dy <= 1; ++dy) {
                        for (int dx = -1; dx <= 1; ++dx) {
                            // Skip self
                            if (dx == 0 && dy == 0) continue;
                            
                            std::size_t nx = x + dx;
                            std::size_t ny = y + dy;
                            if (_current_grid[ny * _x_size + nx] == gol_cell_state::alive) {
                                live_neighbors++;
                            }
                        }
                    }
                    
                    // Apply Game of Life rules
                    gol_cell_state current = _current_grid[y * _x_size + x];
                    gol_cell_state next;
                    
                    if (current == gol_cell_state::alive) {
                        // Live cell with fewer than 2 or more than 3 live neighbors dies
                        if (live_neighbors < 2 || live_neighbors > 3) {
                            next = gol_cell_state::dead;
                        } else {
                            // Live cell with 2 or 3 live neighbors stays alive
                            next = gol_cell_state::alive;
                        }
                    } else {
                        // Dead cell with exactly 3 live neighbors becomes alive
                        if (live_neighbors == 3) {
                            next = gol_cell_state::alive;
                        } else {
                            // Dead cell stays dead
                            next = gol_cell_state::dead;
                        }
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
    std::vector<gol_cell_state> _current_grid;
};

} // namespace game_of_life::reference

#endif // GAME_OF_LIFE_REFERENCE_IMPLEMENTATION_HPP