#include <iostream>
#include <vector>
#include <thread>

#include "constructs.hpp"

using namespace expr_tree;
using namespace simple_evaluator;

// --- States for the Wireworld CA ---
enum class cell_state { empty, electron_head, electron_tail, conductor };

// --- Constants ---
template <cell_state val>
using c = constant<cell_state, val>;

using empty = c<cell_state::empty>;
using electron_head = c<cell_state::electron_head>;
using electron_tail = c<cell_state::electron_tail>;
using conductor = c<cell_state::conductor>;

// --- Integer constants ---
using c_1 = constant<int, 1>;
using c_2 = constant<int, 2>;

// --- Predicates for cell state checks ---
using cell_is_empty = p<current_state, equals, empty>;
using cell_is_electron_head = p<current_state, equals, electron_head>;
using cell_is_electron_tail = p<current_state, equals, electron_tail>;
using cell_is_conductor = p<current_state, equals, conductor>;

// --- Count electron heads in the Moore neighborhood ---
using electron_head_count = count_neighbors<electron_head, moore_8_neighbors>;

// --- Check if exactly 1 or 2 neighboring cells are electron heads ---
using has_one_electron_head_neighbor = p<electron_head_count, equals, c_1>;
using has_two_electron_head_neighbors = p<electron_head_count, equals, c_2>;
using has_one_or_two_electron_head_neighbors = 
    p<has_one_electron_head_neighbor, or_, has_two_electron_head_neighbors>;

// --- Wireworld algorithm ---
// Rule 1: empty → empty
// Rule 2: electron head → electron tail
// Rule 3: electron tail → conductor
// Rule 4: conductor → electron head if exactly 1 or 2 neighboring cells 
//         are electron heads, otherwise remains conductor
using wireworld_algorithm = 
    if_< cell_is_empty >::
    then_< empty >::

    elif_< cell_is_electron_head >::
    then_< electron_tail >::
    
    elif_< cell_is_electron_tail >::
    then_< conductor >::
    
    elif_< has_one_or_two_electron_head_neighbors >::
    then_< electron_head >::
    
    else_< conductor >;

// --- Visualization functions ---
void print_grid(cell_state* grid, std::size_t height, std::size_t width) {
    for (std::size_t i = 0; i < height; ++i) {
        for (std::size_t j = 0; j < width; ++j) {
            auto state = grid[i * width + j];

            switch (state) {
                case cell_state::empty:
                    std::cout << "\033[1;30m.\033[0m "; // Black for empty
                    break;
                case cell_state::electron_head:
                    std::cout << "\033[1;34m@\033[0m "; // Blue for electron head
                    break;
                case cell_state::electron_tail:
                    std::cout << "\033[1;31m#\033[0m "; // Red for electron tail
                    break;
                case cell_state::conductor:
                    std::cout << "\033[1;33m-\033[0m "; // Yellow for conductor
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

// --- Initialize a straight wire with an electron ---
void initialize_straight_wire(cell_state* grid, std::size_t height, std::size_t width) {
    // Clear grid
    for (std::size_t i = 0; i < height; ++i) {
        for (std::size_t j = 0; j < width; ++j) {
            grid[i * width + j] = cell_state::empty;
        }
    }
    
    // Create a horizontal wire in the middle
    std::size_t middle_row = height / 2;
    for (std::size_t j = 1; j < width - 1; ++j) {
        grid[middle_row * width + j] = cell_state::conductor;
    }
    
    // Place an electron head and tail at the start of the wire
    grid[middle_row * width + 2] = cell_state::electron_head;
    grid[middle_row * width + 1] = cell_state::electron_tail;
}

// --- Initialize a simple clock circuit ---
void initialize_clock_circuit(cell_state* grid, std::size_t height, std::size_t width) {
    // Clear grid
    for (std::size_t i = 0; i < height; ++i) {
        for (std::size_t j = 0; j < width; ++j) {
            grid[i * width + j] = cell_state::empty;
        }
    }
    
    // Create a loop of conductor
    std::size_t center_x = width / 2;
    std::size_t center_y = height / 2;
    std::size_t radius = std::min(width, height) / 4;
    
    for (int i = -radius; i <= radius; ++i) {
        grid[(center_y - radius) * width + (center_x + i)] = cell_state::conductor; // Top
        grid[(center_y + radius) * width + (center_x + i)] = cell_state::conductor; // Bottom
        grid[(center_y + i) * width + (center_x - radius)] = cell_state::conductor; // Left
        grid[(center_y + i) * width + (center_x + radius)] = cell_state::conductor; // Right
    }
    
    // Add an electron (head and tail) to start the clock
    grid[(center_y - radius) * width + center_x] = cell_state::electron_head;
    grid[(center_y - radius) * width + (center_x - 1)] = cell_state::electron_tail;
}

// --- Initialize a simple diode circuit ---
void initialize_diode(cell_state* grid, std::size_t height, std::size_t width) {
    // Clear grid
    for (std::size_t i = 0; i < height; ++i) {
        for (std::size_t j = 0; j < width; ++j) {
            grid[i * width + j] = cell_state::empty;
        }
    }
    
    std::size_t start_x = 5;
    std::size_t middle_y = height / 2;
    
    // Create input wire
    for (std::size_t j = start_x; j < start_x + 15; ++j) {
        grid[middle_y * width + j] = cell_state::conductor;
    }
    
    // Create the diode structure
    grid[(middle_y-1) * width + (start_x+15)] = cell_state::conductor;
    grid[(middle_y+1) * width + (start_x+15)] = cell_state::conductor;
    grid[(middle_y-1) * width + (start_x+16)] = cell_state::conductor;
    grid[(middle_y+1) * width + (start_x+16)] = cell_state::conductor;
    
    // Create output wire
    for (std::size_t j = start_x + 17; j < width - 5; ++j) {
        grid[middle_y * width + j] = cell_state::conductor;
    }
    
    // Add an electron head and tail at the start
    grid[middle_y * width + (start_x+2)] = cell_state::electron_head;
    grid[middle_y * width + (start_x+1)] = cell_state::electron_tail;
}

// --- Initialize a grid with two diodes ---
void initialize_two_diodes(cell_state* grid, std::size_t height, std::size_t width) {
    // Clear grid
    for (std::size_t i = 0; i < height; ++i) {
        for (std::size_t j = 0; j < width; ++j) {
            grid[i * width + j] = cell_state::empty;
        }
    }
    
    // Create two horizontal rows for the diodes
    std::size_t row1 = height / 3;
    std::size_t row2 = 2 * height / 3;
    
    // First diode (left to right) on top row
    // Input wire
    for (std::size_t j = 5; j < width/2 - 5; ++j) {
        grid[row1 * width + j] = cell_state::conductor;
    }
    
    // Diode structure 1 (allows current to flow left to right)
    std::size_t diode1_x = width/2 - 5;
    grid[(row1-1) * width + diode1_x] = cell_state::conductor;
    grid[(row1+1) * width + diode1_x] = cell_state::conductor;
    grid[(row1-1) * width + diode1_x+1] = cell_state::conductor;
    grid[(row1+1) * width + diode1_x+1] = cell_state::conductor;
    
    // Output wire for diode 1
    for (std::size_t j = diode1_x + 2; j < width - 5; ++j) {
        grid[row1 * width + j] = cell_state::conductor;
    }
    
    // Connect with vertical wire on right
    for (std::size_t i = row1; i <= row2; ++i) {
        grid[i * width + (width - 6)] = cell_state::conductor;
    }
    
    // Second diode (right to left) on bottom row
    // Input wire for diode 2 on the right
    for (std::size_t j = width/2 + 5; j < width - 6; ++j) {
        grid[row2 * width + j] = cell_state::conductor;
    }
    
    // Diode structure 2 (allows current to flow right to left)
    std::size_t diode2_x = width/2 + 3;
    grid[(row2-1) * width + diode2_x] = cell_state::conductor;
    grid[(row2+1) * width + diode2_x] = cell_state::conductor;
    grid[(row2-1) * width + diode2_x+1] = cell_state::conductor;
    grid[(row2+1) * width + diode2_x+1] = cell_state::conductor;
    
    // Output wire for diode 2 on the left
    for (std::size_t j = 5; j <= diode2_x; ++j) {
        grid[row2 * width + j] = cell_state::conductor;
    }
    
    // Connect with vertical wire on left
    for (std::size_t i = row1; i <= row2; ++i) {
        grid[i * width + 5] = cell_state::conductor;
    }
    
    // Add electrons to start the circuit
    grid[row1 * width + 10] = cell_state::electron_head;
    grid[row1 * width + 9] = cell_state::electron_tail;
    
    // Another set of electrons going in the other direction
    grid[row2 * width + (width - 10)] = cell_state::electron_head;
    grid[row2 * width + (width - 9)] = cell_state::electron_tail;
}

int main(int argc, char* argv[]) {
    std::size_t height = 20;
    std::size_t width = 60; // Wider to fit the two diodes
    std::size_t temporal_steps = 150; // More steps to observe the circuit longer
    
    std::vector<cell_state> grid(height * width, cell_state::empty);
    
    // Choose one of the initialization methods
    // initialize_straight_wire(grid.data(), height, width);
    // initialize_clock_circuit(grid.data(), height, width);
    // initialize_diode(grid.data(), height, width);
    initialize_two_diodes(grid.data(), height, width);
    
    std::cout << "Initial Wireworld:\n";
    print_grid(grid.data(), height, width);
    
    
    std::vector<cell_state> other_grid(height * width, cell_state::empty);

    cell_state* input_grid = grid.data();
    cell_state* output_grid = other_grid.data();

    for (std::size_t t = 0; t < temporal_steps; ++t) {
        run_algorithm<wireworld_algorithm>(
            height, width, 
            input_grid, output_grid);

        std::cout << "Wireworld at time step " << t + 1 << ":\n";
        print_grid(output_grid, height, width);
        std::cout << std::endl;

        // wait for 150ms
        std::this_thread::sleep_for(std::chrono::milliseconds(150));

        std::swap(input_grid, output_grid);
    }

    return 0;
}
