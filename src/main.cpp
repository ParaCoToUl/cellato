#include <iostream>
#include <vector>
#include <random>

#include "experiments/run_params.hpp"
#include "experiments/test_suites.hpp"
#include "experiments/experiment_manager.hpp"
#include "memory/grid_utils.hpp"

#include "game_of_life/algorithm.hpp"

int main() {
    // Define experiment parameters
    cellib::run::run_params params{
        .x_size = 40,
        .y_size = 20,
        .steps = 100
    };
    
    // Create the initial state with random data
    std::vector<game_of_life::cell_state> initial_state(params.x_size * params.y_size);
    
    // Generate random grid with 20% alive cells, 80% dead cells
    cellib::memory::grids::utils::generate_random_grid(
        initial_state,
        params.y_size, params.x_size,
        game_of_life::cell_state::Alive, 0.2,
        game_of_life::cell_state::Dead
    );
    
    // Create and run the experiment
    using baseline = cellib::run::test_suites::cpu_standard<game_of_life::cell_state, game_of_life::algorithm>;
    cellib::run::experiment_manager<baseline> manager;

    manager.run_experiment(
        params, initial_state
    );
    
    return 0;
}
