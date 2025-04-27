#include <iostream>
#include <vector>
#include <thread>
#include <string_view>

#include "constructs.hpp"
#include "bit-mode.hpp"
#include "bit-evaluator.hpp"
#include "bitwise-iterator.hpp"

using namespace expr_tree;
using namespace bitwise;
using namespace bitwise_no_cache;
using namespace iterators;

// Define cell state enum
enum class cell_state { dead, alive };

// Define string literals for symbols and colors
// Need to declare them at global scope for use in templates
constexpr char DEAD_SYMBOL[] = ".";
constexpr char ALIVE_SYMBOL[] = "#";
constexpr char RED_COLOR[] = "\033[1;31m";
constexpr char GREEN_COLOR[] = "\033[1;32m";

// Define print pairs for each cell state
using dead_print = print_pair<DEAD_SYMBOL, RED_COLOR>;
using alive_print = print_pair<ALIVE_SYMBOL, GREEN_COLOR>;

// Define print configuration
using gol_print_config = print_config<dead_print, alive_print>;

// Define state dictionary for mapping between cell states and bits
using cell_state_dictionary = state_dictionary<cell_state, cell_state::dead, cell_state::alive>;

// Define constants
using c_2 = constant<int, 2>;
using c_3 = constant<int, 3>;

// Define state constants
using alive = state_constant<cell_state, cell_state::alive>;
using dead = state_constant<cell_state, cell_state::dead>;

// Define rule components
using cell_is_alive = p<current_state, equals, alive>;
using cell_is_dead = p<current_state, equals, dead>;

using alive_count = count_neighbors<alive, moore_8_neighbors>;

using has_two_alive_neighbors = p<alive_count, equals, c_2>;
using has_three_alive_neighbors = p<alive_count, equals, c_3>;

using has_two_or_three_alive_neighbors = 
    p<has_two_alive_neighbors, or_, has_three_alive_neighbors>;

// Define Game of Life algorithm
using game_of_life_algorithm = 
    if_< cell_is_alive >::
    then_<
        if_< has_two_or_three_alive_neighbors > ::
            then_< alive > ::
            else_< dead >
    > ::
    else_< // cell_is_dead
        if_< has_three_alive_neighbors > ::
            then_< alive > ::
            else_< dead >
    >;

// Define configuration structure for the stencil iterator
struct gol_config {
    using algorithm_t = game_of_life_algorithm;
    using state_dictionary_t = cell_state_dictionary;
    using cell_row_t = uint8_t;
    using print_config_t = gol_print_config;
};

// Function to place a glider pattern
void place_glider(std::vector<cell_state>& grid, std::size_t height, std::size_t width) {
    if (height < 5 || width < 5) return;

    grid[1 * width + 2] = cell_state::alive;
    grid[2 * width + 3] = cell_state::alive;
    grid[3 * width + 1] = cell_state::alive;
    grid[3 * width + 2] = cell_state::alive;
    grid[3 * width + 3] = cell_state::alive;
}

int main(int argc, char* argv[]) {
    std::size_t height = 10;
    std::size_t width = 32;
    std::size_t temporal_steps = 10;

    // Make sure width is a multiple of the word size in bits (8 for uint8_t)
    if (width % 8 != 0) {
        width = ((width / 8) + 1) * 8;
    }

    // Create initial grid
    std::vector<cell_state> grid(height * width, cell_state::dead);
    
    // Place a glider pattern
    place_glider(grid, height, width);
    
    // Create and initialize the iterator
    bit_grid_simple_iterator<gol_config> iterator;
    iterator.init(grid, height, width);
    
    // Print initial grid
    std::cout << "Initial grid:\n";
    iterator.print_current_grid();
    std::cout << std::endl;
    
    // Run the simulation
    for (std::size_t t = 0; t < temporal_steps; ++t) {
        // Run a single step
        iterator.template run<true>(1);
        
        // Get the result
        auto result = iterator.get_result();
        
        // Print the result
        std::cout << "Grid at time step " << t + 1 << ":\n";
        iterator.print_current_grid();
        std::cout << std::endl;
        
        // Wait for 500ms
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    return 0;
}
