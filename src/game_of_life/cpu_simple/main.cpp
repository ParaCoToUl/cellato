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

// Define the Game of Life algorithm using the AST
namespace game_of_life {
    using namespace cellib::ast;
    
    enum class cell_state {
        Dead,
        Alive
    };
    
    // Define constants for cell states
    using alive = state_constant<cell_state, cell_state::Alive>;
    using dead = state_constant<cell_state, cell_state::Dead>;

    // Define integer constants
    using c_2 = constant<int, 2>;
    using c_3 = constant<int, 3>;
    
    // Define predicates for cell state checks
    using cell_is_alive = p<current_state, equals, alive>;
    using cell_is_dead = p<current_state, equals, dead>;
    
    // Count neighbors in Moore neighborhood
    using alive_count = count_neighbors<alive, moore_8_neighbors>;
    
    // Define predicates for neighbor count checks
    using has_two_alive_neighbors = p<alive_count, equals, c_2>;
    using has_three_alive_neighbors = p<alive_count, equals, c_3>;
    using has_two_or_three_alive_neighbors = p<has_two_alive_neighbors, or_, has_three_alive_neighbors>;
    
    // Define the Game of Life algorithm
    using algorithm = 
        if_<cell_is_alive>::
        then_<
            if_<has_two_or_three_alive_neighbors>::
                then_<alive>::
                else_<dead>
        >::
        else_< // cell_is_dead
            if_<has_three_alive_neighbors>::
                then_<alive>::
                else_<dead>
        >;
}

// Custom stream operator for cell_state (for debugging)
std::ostream& operator<<(std::ostream& os, const game_of_life::cell_state& state) {
    os << (state == game_of_life::cell_state::Alive ? "Alive" : "Dead");
    return os;
}

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
    using traverser_config_t = cellib::traversers::cpu::simple::traverser_config<
        eval_t,
        grid_t
    >;
    
    // Create the traverser
    cellib::traversers::cpu::simple::traverser<traverser_config_t> traverser;
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
