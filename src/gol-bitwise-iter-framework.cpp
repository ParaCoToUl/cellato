#include <iostream>
#include <vector>
#include <thread>
#include <string>
#include <chrono>
#include <iomanip>
#include <functional> // For std::function

#include "constructs.hpp"
#include "bit-mode.hpp"
#include "bit-evaluator.hpp"
#include "bitwise-iterator.hpp"
#include "grid-utils.hpp"
#include "fujita.hpp"

// Control flags for execution modes
namespace config {
    // Print mode (silence, minimal, differences, verbose)
    constexpr grid_utils::PrintMode PRINT_MODE = grid_utils::PrintMode::Minimal;
    // constexpr grid_utils::PrintMode PRINT_MODE = grid_utils::PrintMode::Verbose;
    
    // Use a random grid instead of glider pattern
    constexpr bool USE_RANDOM_GRID = true;
    
    // Random seed value for reproducible tests
    constexpr unsigned int RANDOM_SEED = 12345;
    
    // Run comparisons between simple and bitwise evaluators
    constexpr bool RUN_COMPARISONS = false;
    
    // Run performance tests
    constexpr bool RUN_PERFORMANCE = true;
    
    // Number of iterations for performance testing
    constexpr int PERFORMANCE_ITERATIONS = 100;
    
    // Number of steps for regular simulation run
    constexpr int SIMULATION_STEPS = 10;
    
    // Grid dimensions
    constexpr std::size_t GRID_HEIGHT = 8 * 40 * 10;
    constexpr std::size_t GRID_WIDTH = 32 * 20 * 10;  // Will be adjusted to word size
    
    // Probability of alive cells in random grid
    constexpr double ALIVE_PROBABILITY = 0.3;
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
    // using cell_row_t = uint32_t;
    using cell_row_t = uint64_t;
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

// Custom formatter for side-by-side grid comparison
std::string format_cell(const cell_state& state) {
    if (state == cell_state::alive) {
        return GREEN_COLOR + std::string(ALIVE_SYMBOL) + "\033[0m ";
    } else {
        return RED_COLOR + std::string(DEAD_SYMBOL) + "\033[0m ";
    }
}

int main(int argc, char* argv[]) {
    // Set up grid dimensions
    std::size_t height = config::GRID_HEIGHT;
    std::size_t width = config::GRID_WIDTH;
    
    // Make sure width is a multiple of the word size in bits
    width = adjust_width_for_word_size<uint8_t>(width);
    
    std::cout << "=== Game of Life Simulation ===" << std::endl;
    std::cout << "Grid size: " << height << "x" << width << std::endl;
    std::cout << "============================" << std::endl << std::endl;

    // Create initial grid
    std::vector<cell_state> initial_grid(height * width, cell_state::dead);
    
    // Initialize grid based on configuration
    if (config::USE_RANDOM_GRID) {
        // Define probabilities for cell states
        std::vector<std::tuple<cell_state, double>> probabilities = {
            {cell_state::dead, 1.0 - config::ALIVE_PROBABILITY},
            {cell_state::alive, config::ALIVE_PROBABILITY}
        };
        
        generate_random_grid(initial_grid, height, width, probabilities, config::RANDOM_SEED);
        std::cout << "Generated random grid with seed: " << config::RANDOM_SEED << std::endl;
    } else {
        place_glider(initial_grid, height, width);
        std::cout << "Placed glider pattern on grid" << std::endl;
    }
    
    // Run comparisons if enabled
    if (config::RUN_COMPARISONS) {
        std::cout << "\n=== Comparing Simple and Bitwise Implementations ===" << std::endl;
        
        // Create copies of the initial grid for comparison runs
        std::vector<cell_state> grid_for_simple(initial_grid);
        std::vector<cell_state> grid_for_bitwise(initial_grid);
        
        // Create both iterators
        simple_grid_iterator<gol_simple_config> simple_iterator;
        bit_grid_simple_iterator<gol_config> bitwise_iterator;
        
        // Initialize both iterators with the same initial grid
        simple_iterator.init(grid_for_simple, height, width);
        bitwise_iterator.init(grid_for_bitwise, height, width);
        
        // Set up print options based on config
        bool print_grids_on_match = config::PRINT_MODE == grid_utils::PrintMode::Verbose;
        bool print_grids_on_mismatch = config::PRINT_MODE == grid_utils::PrintMode::Verbose || 
                                       config::PRINT_MODE == grid_utils::PrintMode::Differences;
        bool print_diff_details = config::PRINT_MODE != grid_utils::PrintMode::Silent;
        
        // Run the comparison
        compare_iterators_step_by_step(
            simple_iterator, bitwise_iterator,
            "Simple", "Bitwise",
            height, width,
            config::SIMULATION_STEPS,
            print_grids_on_match,
            print_grids_on_mismatch,
            print_diff_details
        );
    }
    
    // Run performance tests if enabled
    if (config::RUN_PERFORMANCE) {
        std::cout << "\n=== Performance Testing ===" << std::endl;
        
        // Create copies of the initial grid for performance runs
        std::vector<cell_state> grid_for_simple_perf(initial_grid);
        std::vector<cell_state> grid_for_bitwise_perf(initial_grid);
        
        // Create fresh iterators for performance testing
        simple_grid_iterator<gol_simple_config> simple_perf_iterator;
        bit_grid_simple_iterator<gol_config> bitwise_perf_iterator;
        
        // Initialize both iterators with the same initial grid
        simple_perf_iterator.init(grid_for_simple_perf, height, width);
        bitwise_perf_iterator.init(grid_for_bitwise_perf, height, width);
        
        // Run the performance measurement
        measure_performance(
            simple_perf_iterator, bitwise_perf_iterator,
            "Simple", "Bitwise",
            config::PERFORMANCE_ITERATIONS
        );

        // measure fujita performance
        // TODO maybe fill with actual data ¯\_(ツ)_/¯

        auto adjusted_width = width + 2 * sizeof(gol_config::cell_row_t) * 8;
        adjusted_width /= sizeof(gol_config::cell_row_t) * 8;
        auto adjusted_height = height + 2;
        std::vector<gol_config::cell_row_t> fujita_grid_in(adjusted_height * adjusted_width, 0);
        std::vector<gol_config::cell_row_t> fujita_grid_out(adjusted_height * adjusted_width, 0);

        std::cout << "Fujita performance test... Warm up..." << std::endl;

        baselines::compute_using_fujita(
            fujita_grid_in.data(), fujita_grid_out.data(),
            adjusted_height, adjusted_width,
            config::PERFORMANCE_ITERATIONS
        );

        std::cout << "Fujita performance test... Running hot..." << std::endl;

        auto start = std::chrono::high_resolution_clock::now();
        baselines::compute_using_fujita(
            fujita_grid_in.data(), fujita_grid_out.data(),
            adjusted_height, adjusted_width,
            config::PERFORMANCE_ITERATIONS
        );
        auto end = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double, std::milli> elapsed = end - start;
        std::cout << "Fujita performance test completed in " 
                  << std::fixed << std::setprecision(3) 
                  << elapsed.count() << " ms" << std::endl;
    }
    
    std::cout << "Simulation completed." << std::endl;
    return 0;
}
