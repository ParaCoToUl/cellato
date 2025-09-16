#ifndef CRITTERS_DATA_INIT_HPP
#define CRITTERS_DATA_INIT_HPP

#include <vector>
#include <random>

#include "experiments/run_params.hpp"

namespace critters {

struct critters_random_init {
    static std::vector<critters_cell_state> init(cellato::run::run_params& params) {
        std::vector<critters_cell_state> initial_state(params.x_size * params.y_size);
        
        // Probabilities for each cell state
        std::vector<std::tuple<critters_cell_state, double>> probabilities = {
            {critters_cell_state::empty, 0.20},   // 20% empty cells
            {critters_cell_state::tree, 0.79},    // 79% trees
            {critters_cell_state::critters, 0.01},    // 1% critters (ignition points)
            {critters_cell_state::ash, 0.00}      // 0% ash initially
        };
        
        // Generate random grid using utility
        cellato::memory::grids::utils::generate_random_grid(
            initial_state,
            params.y_size, params.x_size,
            probabilities
        );
        
        return initial_state;
    }
};

}

#endif // CRITTERS_DATA_INIT_HPP
