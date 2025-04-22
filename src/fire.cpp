#include <iostream>
#include <vector>
#include <thread>
#include <random>

#include "constructs.hpp"

using namespace expr_tree;
using namespace simple_evaluator;

// --- States for the forest fire CA ---
enum class cell_state { empty, tree, ash, fire };

// --- Constants ---
template <cell_state val>
using c = constant<cell_state, val>;

using empty = c<cell_state::empty>;
using tree = c<cell_state::tree>;
using ash = c<cell_state::ash>;
using fire = c<cell_state::fire>;

// --- Predicates for cell state checks ---
using cell_is_empty = p<current_state, equals, empty>;
using cell_is_tree = p<current_state, equals, tree>;
using cell_is_ash = p<current_state, equals, ash>;
using cell_is_fire = p<current_state, equals, fire>;

// --- Count fire cells in the von Neumann neighborhood ---
using fire_count = count_neighbors<fire, moore_4_neighbors>;

// --- Constants for comparisons ---
using c_0 = constant<int, 0>;

using has_fire_neighbors = p<fire_count, greater_than, c_0>;
using no_fire_neighbors = p<fire_count, equals, c_0>;

// --- Forest Fire algorithm ---
// Rule 1: If cell is EMPTY, it stays EMPTY
// Rule 2: If cell is ASH and has fire neighbors, it stays ASH
// Rule 3: If cell is ASH and has no fire neighbors, it becomes EMPTY
// Rule 4: If cell is TREE and has fire neighbors, it becomes FIRE
// Rule 5: If cell is TREE and has no fire neighbors, it stays TREE
// Rule 6: If cell is FIRE, it becomes ASH
using forest_fire_algorithm = 
    if_< cell_is_empty >::
    then_< empty >::
    
    elif_< cell_is_ash >::
    then_<
        if_< has_fire_neighbors >::
        then_< ash >::
        else_< empty >
    >::
    
    elif_< cell_is_tree >::
    then_<
        if_< has_fire_neighbors >::
        then_< fire >::
        else_< tree >
    >::
    
    else_< ash >;

// --- Visualization functions ---
void print_grid(cell_state* grid, std::size_t height, std::size_t width) {
    for (std::size_t i = 0; i < height; ++i) {
        for (std::size_t j = 0; j < width; ++j) {
            auto state = grid[i * width + j];

            switch (state) {
                case cell_state::empty:
                    std::cout << "\033[1;33m.\033[0m "; // Brown/Yellow for empty
                    break;
                case cell_state::tree:
                    std::cout << "\033[1;32m#\033[0m "; // Green for tree
                    break;
                case cell_state::ash:
                    std::cout << "\033[1;37m*\033[0m "; // Gray/White for ash
                    break;
                case cell_state::fire:
                    std::cout << "\033[1;31m@\033[0m "; // Red for fire
                    break;
            }
        }
        std::cout << "\n";
    }
}

// --- Cellular automaton algorithm runner ---
template <typename Algorithm>
void run_algorithm(std::size_t height, std::size_t width,
    cell_state* grid_input, cell_state* grid_output) {
    
    for (std::size_t i = 1; i < height - 1; ++i) {
        for (std::size_t j = 1; j < width - 1; ++j) {
            grid_config<cell_state> state{ grid_input, height, width, j, i };
            
            grid_output[i * width + j] = evaluator<cell_state, Algorithm>::evaluate(state);
        }
    }
}

// --- Grid initialization ---
void initialize_forest(cell_state* grid, std::size_t height, std::size_t width,
                      double p_empty = 0.2, double p_tree = 0.8, double p_fire = 0.01) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);
    
    // Fill the entire grid with empty cells first
    for (std::size_t i = 0; i < height; ++i) {
        for (std::size_t j = 0; j < width; ++j) {
            grid[i * width + j] = cell_state::empty;
        }
    }
    
    // Fill the inner grid with trees, empty spaces, and fire
    for (std::size_t i = 1; i < height - 1; ++i) {
        for (std::size_t j = 1; j < width - 1; ++j) {
            double r = dis(gen);
            if (r < p_fire) {
                grid[i * width + j] = cell_state::fire;
            } else if (r < p_fire + p_empty) {
                grid[i * width + j] = cell_state::empty;
            } else {
                grid[i * width + j] = cell_state::tree;
            }
        }
    }
}

int main(int argc, char* argv[]) {
    std::size_t height = 20;
    std::size_t width = 40;
    std::size_t temporal_steps = 30;
    
    // Reduce the fire probability for more interesting progression
    double p_empty = 0.2;
    double p_tree = 0.79;
    double p_fire = 0.01;  // Increased slightly to see fire spots

    std::vector<cell_state> grid(height * width, cell_state::empty);
    
    initialize_forest(grid.data(), height, width, p_empty, p_tree, p_fire);
    
    std::cout << "Initial forest:\n";
    print_grid(grid.data(), height, width);
    
    
    std::vector<cell_state> other_grid(height * width, cell_state::empty);

    cell_state* input_grid = grid.data();
    cell_state* output_grid = other_grid.data();

    for (std::size_t t = 0; t < temporal_steps; ++t) {
        run_algorithm<forest_fire_algorithm>(
            height, width, 
            input_grid, output_grid);

        std::cout << "Forest at time step " << t + 1 << ":\n";
        print_grid(output_grid, height, width);
        std::cout << std::endl;

        // wait for 500ms
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        std::swap(input_grid, output_grid);
    }

    return 0;
}
