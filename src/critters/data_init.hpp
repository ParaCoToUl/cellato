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
            {critters_cell_state::dead, 0.50},
            {critters_cell_state::alive, 0.50}
        };
        
        // Generate random grid using utility
        cellato::memory::grids::utils::generate_random_grid(
            initial_state,
            params.y_size, params.x_size,
            probabilities
        );
        
        // auto idx = [=](int x, int y) {
        //     return y * params.x_size + x;
        // };

        // initial_state[idx(0,0)] = critters_cell_state::alive;
        // initial_state[idx(1,0)] = critters_cell_state::alive;
        // initial_state[idx(0,1)] = critters_cell_state::alive;
        // initial_state[idx(1,1)] = critters_cell_state::alive;

        return initial_state;
    }
};

}

#endif // CRITTERS_DATA_INIT_HPP
