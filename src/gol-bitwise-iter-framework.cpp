#include <iostream>
#include <vector>
#include <thread>
#include <string>
#include <chrono>
#include <iomanip>

#include "constructs.hpp"
#include "bit-mode.hpp"
#include "bit-evaluator.hpp"
#include "bitwise-iterator.hpp"
#include "grid-utils.hpp"

// Control flags for execution modes
namespace config {
    // Print grids during execution
    constexpr bool PRINT_GRIDS = true;
    
    // Use a random grid instead of glider pattern
    constexpr bool USE_RANDOM_GRID = true;
    
    // Random seed value for reproducible tests
    constexpr unsigned int RANDOM_SEED = 12345;
    
    // Compare results between simple and bitwise evaluators
    constexpr bool COMPARE_RESULTS = true;

    // Measure performance
    constexpr bool MEASURE_PERFORMANCE = true;
}

using namespace expr_tree;
using namespace bitwise;
using namespace bitwise_no_cache;
using namespace iterators;
using namespace grid_utils;

// Define cell state enum
enum class cell_state { dead, alive };

// Define string literals for symbols and colors
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

// Define configuration structures for both iterators
struct gol_config {
    using algorithm_t = game_of_life_algorithm;
    using state_dictionary_t = cell_state_dictionary;
    using cell_row_t = uint8_t;
    using print_config_t = gol_print_config;
};

struct gol_simple_config {
    using algorithm_t = game_of_life_algorithm;
    using cell_state_t = cell_state;
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
    std::size_t width = 32;  // Multiple of 8 for bitwise compatibility
    std::size_t temporal_steps = 10;

    // Make sure width is a multiple of the word size in bits
    width = adjust_width_for_word_size<uint8_t>(width);

    // Create initial grid
    std::vector<cell_state> initial_grid(height * width, cell_state::dead);
    
    // Initialize grid based on configuration
    if (config::USE_RANDOM_GRID) {
        // Define probabilities for cell states
        std::vector<std::tuple<cell_state, double>> probabilities = {
            {cell_state::dead, 0.7},   // 70% probability of dead cells
            {cell_state::alive, 0.3}   // 30% probability of alive cells
        };
        
        generate_random_grid(initial_grid, height, width, probabilities, config::RANDOM_SEED);
        std::cout << "Generated random grid with seed: " << config::RANDOM_SEED << std::endl;
    } else {
        place_glider(initial_grid, height, width);
        std::cout << "Placed glider pattern on grid" << std::endl;
    }
    
    // Create both iterators
    simple_grid_iterator<gol_simple_config> simple_iterator;
    bit_grid_simple_iterator<gol_config> bitwise_iterator;
    
    // Initialize both iterators with the same initial grid
    simple_iterator.init(initial_grid, height, width);
    bitwise_iterator.init(initial_grid, height, width);
    
    // Print initial grid
    if (config::PRINT_GRIDS) {
        std::cout << "Initial grid:\n";
        simple_iterator.print_current_grid();
        std::cout << std::endl;
    }
    
    // Run the simulation for each step
    for (std::size_t t = 0; t < temporal_steps; ++t) {
        // Run a single step on both iterators with timing
        auto simple_start = std::chrono::high_resolution_clock::now();
        
        simple_iterator.template run<config::PRINT_GRIDS>(1);
        
        auto simple_end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> simple_elapsed = simple_end - simple_start;
        
        auto bitwise_start = std::chrono::high_resolution_clock::now();
        
        bitwise_iterator.template run<config::PRINT_GRIDS>(1);
        
        auto bitwise_end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> bitwise_elapsed = bitwise_end - bitwise_start;
        
        // Get results from both iterators
        auto simple_result = simple_iterator.get_result();
        auto bitwise_result = bitwise_iterator.get_result();
        
        // Print step header
        std::cout << "===== Step " << t + 1 << " =====\n";
        
        // Compare results if enabled
        if (config::COMPARE_RESULTS) {
            bool equal = compare_grids(simple_result, bitwise_result, height, width, true);
            
            std::cout << "Simple vs Bitwise: " 
                      << (equal ? "\033[1;32mMATCH\033[0m" : "\033[1;31mMISMATCH\033[0m") 
                      << std::endl;
        }
        
        // Print performance results if enabled
        if (config::MEASURE_PERFORMANCE) {
            std::cout << "Performance: " 
                      << "Simple: " << std::fixed << std::setprecision(3) << simple_elapsed.count() << " ms, "
                      << "Bitwise: " << std::fixed << std::setprecision(3) << bitwise_elapsed.count() << " ms, "
                      << "Ratio: " << std::fixed << std::setprecision(2) 
                      << (simple_elapsed.count() / bitwise_elapsed.count())
                      << "x" << std::endl;
        }
        
        std::cout << std::endl;
        
        // Wait for 500ms between steps (optional)
        if (config::PRINT_GRIDS) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }

    return 0;
}
