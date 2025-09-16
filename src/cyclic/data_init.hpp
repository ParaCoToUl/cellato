#ifndef CYCLIC_DATA_INIT_HPP
#define CYCLIC_DATA_INIT_HPP

#include <vector>
#include <random>

#include "experiments/run_params.hpp"

namespace cyclic {

struct cyclic_random_init {
    static std::vector<cyclic_cell_state> init(cellato::run::run_params& params) {
        std::vector<cyclic_cell_state> initial_state(params.x_size * params.y_size);
        
        // Probabilities for each cell state
        std::vector<std::tuple<cyclic_cell_state, double>> probabilities = {
            {cyclic_cell_state::empty, 0.20},   // 20% empty cells
            {cyclic_cell_state::tree, 0.79},    // 79% trees
            {cyclic_cell_state::cyclic, 0.01},    // 1% cyclic (ignition points)
            {cyclic_cell_state::ash, 0.00}      // 0% ash initially
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

#endif // CYCLIC_DATA_INIT_HPP
