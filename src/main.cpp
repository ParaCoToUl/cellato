#include <iostream>
#include <vector>
#include <random>

#include "experiments/run_params.hpp"
#include "experiments/test_suites.hpp"
#include "experiments/experiment_manager.hpp"
#include "memory/grid_utils.hpp"

#include "game_of_life/algorithm.hpp"
#include "game_of_life/pretty_print.hpp"
#include "game_of_life/config.hpp"
#include "fire/config.hpp"
#include "greenberg/config.hpp"

template <
    typename cellular_automaton,
    template <typename, typename> typename test_suite
>
void run(cellib::run::run_params& params) {

    auto initial_state = cellular_automaton::input::random::init(params);

    using algorithm = typename cellular_automaton::algorithm;
    using cell_state = typename cellular_automaton::cell_state;

    using test_suite_for_alg = test_suite<cell_state, algorithm>;

    cellib::run::experiment_manager<test_suite_for_alg> manager;
    manager.set_print_config(cellular_automaton::pretty_print::get_config());

    manager.run_experiment(
        params, initial_state
    );
}

int main() {
    // Define experiment parameters
    cellib::run::run_params params{
        .x_size = 150,  // Larger grid for better patterns
        .y_size = 200,
        .steps = 100,
        .print = true
    };
    
    // 0 = Game of Life, 1 = Forest Fire, 2 = Greenberg-Hastings
    int simulation_type = 2;
    
    if (simulation_type == 0) {
        // Run Game of Life simulation
        run<game_of_life::config, cellib::run::test_suites::cpu_standard>(params);
    } else if (simulation_type == 1) {
        // Run Forest Fire simulation
        run<fire::config, cellib::run::test_suites::cpu_standard>(params);
    } else {
        // Run Greenberg-Hastings Model simulation
        run<greenberg::config, cellib::run::test_suites::cpu_standard>(params);
    }
    
    return 0;
}
