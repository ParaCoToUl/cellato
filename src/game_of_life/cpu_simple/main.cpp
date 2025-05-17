#include <iostream>
#include <string>
#include <map>
#include <thread>
#include <chrono>

#include "memory/standard_grid.hpp"
#include "memory/interface.hpp"
#include "core/ast.hpp"
#include "evaluators/standard.hpp"
#include "traversers/cpu/simple.hpp"

#include "../algorithm.hpp"

int main() {
    std::cout << "=== Game of Life Simulation ===\n" << std::endl;
    
    constexpr std::size_t WIDTH = 20;
    constexpr std::size_t HEIGHT = 20;
    constexpr int STEPS = 10;
    constexpr bool PRINT_STEPS = true;
    
    // Create grid and initialize with all dead cells
    cellib::memory::grids::standard::grid<game_of_life::cell_state> grid(WIDTH, HEIGHT);
    std::fill(grid.data(), grid.data() + WIDTH * HEIGHT, game_of_life::cell_state::Dead);

    // Add a glider pattern
    auto* data = grid.data();
    data[1 + WIDTH * 0] = game_of_life::cell_state::Alive;
    data[2 + WIDTH * 1] = game_of_life::cell_state::Alive;
    data[0 + WIDTH * 2] = game_of_life::cell_state::Alive;
    data[1 + WIDTH * 2] = game_of_life::cell_state::Alive;
    data[2 + WIDTH * 2] = game_of_life::cell_state::Alive;

    // Set up print configuration
    auto print_config = cellib::memory::grids::standard::print_config<game_of_life::cell_state>()
        .with(game_of_life::cell_state::Dead, ".")
        .with(game_of_life::cell_state::Alive, "O");

    // Create a grid with margins for boundary conditions
    auto padded_grid = grid.with_empty_margins<1, 1>();

    using eval_t = cellib::evaluators::standard::evaluator<game_of_life::cell_state, game_of_life::algorithm>;
    using grid_t = cellib::memory::grids::standard::grid<game_of_life::cell_state>;
    
    
    // Create the traverser
    cellib::traversers::cpu::simple::traverser<eval_t, grid_t> traverser;
    traverser.set_print_config(print_config);
    traverser.init(padded_grid);
    
    // Print initial state
    std::cout << "Initial grid with glider pattern:" << std::endl;
    padded_grid.print(std::cout, print_config);
    std::cout << std::endl;
    
    // Run the simulation
    if (PRINT_STEPS) {
        traverser.run<true>(STEPS);
    } else {
        traverser.run<false>(STEPS);
    }
    
    // Get the final result
    auto result_grid = traverser.fetch_result();
    
    // Print the final state
    std::cout << "\nFinal grid after " << STEPS << " steps:" << std::endl;
    result_grid.print(std::cout, print_config);
    
    return 0;
}
