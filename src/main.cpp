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
#include "wire/config.hpp"

template <
    typename cellular_automaton,
    template <typename> typename test_suite
>
void run(cellib::run::run_params& params) {

    auto initial_state = cellular_automaton::input::random::init(params);

    using test_suite_for_alg = test_suite<cellular_automaton>;

    cellib::run::experiment_manager<test_suite_for_alg> manager;
    manager.set_print_config(cellular_automaton::pretty_print::get_config());

    manager.run_experiment(
        params, initial_state
    );
}

int main() {
    // Define experiment parameters
    cellib::run::run_params params{
        .x_size = 128,  // Larger grid for better patterns
        .y_size = 64,
        .steps = 100,
        .print = true
    };
    
    // 0 = Game of Life, 1 = Forest Fire, 2 = Greenberg-Hastings, 3 = Wireworld
    int simulation_type = 0; // Default to Wireworld

    if (simulation_type == 0) {
        // Run Game of Life simulation
        
        // run<game_of_life::config, cellib::run::test_suites::cpu_standard>(params);
        
        // run<game_of_life::config, cellib::run::test_suites::using_<std::uint32_t>::bit_plates_cpu>(params);
        
        // run<game_of_life::config, cellib::run::test_suites::cuda_standard>(params);

        run<game_of_life::config, cellib::run::test_suites::using_<std::uint32_t>::bit_plates_cuda>(params);
    } else if (simulation_type == 1) {
        // Run Forest Fire simulation
        
        // run<fire::config, cellib::run::test_suites::cpu_standard>(params);
        
        // run<fire::config, cellib::run::test_suites::using_<std::uint32_t>::bit_plates_cpu>(params);
        
        run<fire::config, cellib::run::test_suites::cuda_standard>(params);
    } else if (simulation_type == 2) {
        // Run Greenberg-Hastings Model simulation
        // run<game_of_life::config, cellib::run::test_suites::cpu_standard>(params);
        
        // run<greenberg::config, cellib::run::test_suites::using_<std::uint32_t>::bit_plates_cpu>(params);

        run<greenberg::config, cellib::run::test_suites::cuda_standard>(params);
    } else {
        // Run Wireworld simulation
        
        // run<wire::config, cellib::run::test_suites::cpu_standard>(params);
        
        // run<wire::config, cellib::run::test_suites::using_<std::uint32_t>::bit_plates_cpu>(params);

        run<wire::config, cellib::run::test_suites::cuda_standard>(params);
    }
    
    return 0;
}
