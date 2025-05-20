#ifndef WIRE_DATA_INIT_HPP
#define WIRE_DATA_INIT_HPP

#include <vector>
#include <random>
#include "experiments/run_params.hpp"

namespace wire {

struct wire_random_init {
    static std::vector<wire_cell_state> init(cellib::run::run_params& params) {
        std::vector<wire_cell_state> initial_state(params.x_size * params.y_size);
        
        // Simple initialization for now: create a horizontal wire with an electron at the start
        std::size_t middle_row = params.y_size / 2;
        
        // Fill with empty cells
        for (std::size_t i = 0; i < params.y_size; ++i) {
            for (std::size_t j = 0; j < params.x_size; ++j) {
                initial_state[i * params.x_size + j] = wire_cell_state::empty;
            }
        }
        
        // Create a horizontal wire in the middle
        for (std::size_t j = 1; j < params.x_size - 1; ++j) {
            initial_state[middle_row * params.x_size + j] = wire_cell_state::conductor;
        }
        
        // Place an electron head and tail at the start of the wire
        if (params.x_size > 3) {
            initial_state[middle_row * params.x_size + 2] = wire_cell_state::electron_head;
            initial_state[middle_row * params.x_size + 1] = wire_cell_state::electron_tail;
        }
        
        return initial_state;
    }
};

struct wire_clock_init {
    static std::vector<wire_cell_state> init(cellib::run::run_params& params) {
        std::vector<wire_cell_state> initial_state(params.x_size * params.y_size);
        
        // Fill with empty cells
        for (std::size_t i = 0; i < params.y_size; ++i) {
            for (std::size_t j = 0; j < params.x_size; ++j) {
                initial_state[i * params.x_size + j] = wire_cell_state::empty;
            }
        }
        
        // Create a loop of conductor
        std::size_t center_x = params.x_size / 2;
        std::size_t center_y = params.y_size / 2;
        std::size_t radius = std::min(params.x_size, params.y_size) / 4;
        
        for (int i = -static_cast<int>(radius); i <= static_cast<int>(radius); ++i) {
            // Ensure we're within bounds - remove redundant unsigned >= 0 checks
            if (center_y >= radius && center_x + i < params.x_size)
                initial_state[(center_y - radius) * params.x_size + (center_x + i)] = wire_cell_state::conductor; // Top
            
            if (center_y + radius < params.y_size && center_x + i < params.x_size)
                initial_state[(center_y + radius) * params.x_size + (center_x + i)] = wire_cell_state::conductor; // Bottom
            
            if (center_y + i < params.y_size && center_x >= radius)
                initial_state[(center_y + i) * params.x_size + (center_x - radius)] = wire_cell_state::conductor; // Left
            
            if (center_y + i < params.y_size && center_x + radius < params.x_size)
                initial_state[(center_y + i) * params.x_size + (center_x + radius)] = wire_cell_state::conductor; // Right
        }
        
        // Add an electron (head and tail) to start the clock
        if (center_y >= radius && center_x < params.x_size && center_x > 0) {
            initial_state[(center_y - radius) * params.x_size + center_x] = wire_cell_state::electron_head;
            initial_state[(center_y - radius) * params.x_size + (center_x - 1)] = wire_cell_state::electron_tail;
        }
        
        return initial_state;
    }
};

}

#endif // WIRE_DATA_INIT_HPP
