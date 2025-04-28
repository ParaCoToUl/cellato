#include <iostream>
#include <vector>
#include <thread>
#include <cstdint>
#include <bitset>

#include "constructs.hpp"
#include "bit-mode.hpp"
#include "bit-evaluator.hpp"

using namespace expr_tree;
using namespace bitwise;
using namespace bitwise_no_cache;

using cell_row_t = std::uint64_t;

enum class cell_state { dead, alive };

// Define the state dictionary for mapping between cell states and bits
using cell_state_dictionary = state_dictionary<cell_state, cell_state::dead, cell_state::alive>;

using c_2 = constant<int, 2>;
using c_3 = constant<int, 3>;

using alive = state_constant<cell_state, cell_state::alive>;
using dead = state_constant<cell_state, cell_state::dead>;

using cell_is_alive = p<current_state, equals, alive>;
using cell_is_dead = p<current_state, equals, dead>;

using alive_count = count_neighbors<alive, moore_8_neighbors>;

using has_two_alive_neighbors = p<alive_count, equals, c_2>;
using has_three_alive_neighbors = p<alive_count, equals, c_3>;

using has_two_or_three_alive_neighbors = 
    p<has_two_alive_neighbors, or_, has_three_alive_neighbors>;

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

void print_grid(cell_state* grid, std::size_t height, std::size_t width) {
    for (std::size_t i = 0; i < height; ++i) {
        for (std::size_t j = 0; j < width; ++j) {
            auto state = grid[i * width + j];

            if (state == cell_state::alive) {
                std::cout << "\033[1;32m#\033[0m ";
            } else {
                std::cout << "\033[1;31m.\033[0m ";
            }
        }
        std::cout << "\n";
    }
}

// Convert standard grid to bit-grid
bit_grid<cell_row_t, cell_state_dictionary> standard_to_bitgrid(
    const cell_state* grid, std::size_t height, std::size_t width) {

    std::size_t word_bits = sizeof(cell_row_t) * 8;
    
    if (width % word_bits != 0) {
        throw std::invalid_argument("Width must be a multiple of the word size in bits.");
    }

    std::size_t adjusted_width = width + 2 * word_bits;
    std::size_t adjusted_height = height + 2;

    std::vector<cell_state> padded_grid(adjusted_height * adjusted_width, cell_state::dead);

    for (std::size_t y = 0; y < height; y++) {
        for (std::size_t x = 0; x < width; x++) {
            padded_grid[(y + 1) * adjusted_width + (x + word_bits)] = grid[y * width + x];
        }
    }

    return bit_grid<cell_row_t, cell_state_dictionary>(adjusted_height, adjusted_width, padded_grid.data());
}

std::vector<cell_state> bitgrid_to_standard(
    const bit_grid<cell_row_t, cell_state_dictionary>& bit_grid) {
        
    std::size_t bits_per_word = sizeof(cell_row_t) * 8;

    std::size_t adjusted_width = bit_grid.x_size_original();
    std::size_t width = adjusted_width - 2 * bits_per_word;
    std::size_t height = bit_grid.y_size_original() - 2; // Subtract padding
    
    std::vector<cell_state> standard_grid(height * width, cell_state::dead);
    
    auto adjusted_grid = bit_grid.to_original_representation();
    
    // Copy from the padded grid back to the standard grid, skipping the padding
    for (std::size_t y = 0; y < height; ++y) {
        for (std::size_t x = 0; x < width; ++x) {
            standard_grid[y * width + x] = adjusted_grid[(y + 1) * adjusted_width + (x + bits_per_word)];
        }
    }

    return standard_grid;
}

// Run algorithm using bit-grid representation
template <typename Algorithm>
void run_algorithm_bitwise(std::size_t height, std::size_t width,
    const cell_state* input, cell_state* output) {
    
    // Convert input to bit-grid representation
    auto bit_input = standard_to_bitgrid(input, height, width);
    
    // Create a properly initialized output bit grid instead of using uninitialized memory
    std::size_t word_bits = sizeof(cell_row_t) * 8;
    std::size_t adjusted_width = width + 2 * word_bits;
    std::size_t adjusted_height = height + 2;
    
    // Initialize output grid with all dead cells
    std::vector<cell_state> output_init(adjusted_height * adjusted_width, cell_state::dead);
    bit_grid<cell_row_t, cell_state_dictionary> bit_output(adjusted_height, adjusted_width, output_init.data());

    // Create grid config for bitwise evaluation
    using grid_conf_t = grid_config<cell_row_t, cell_state_dictionary>;
    
    auto grid_data = bit_input.data();
    auto grid_data_out = bit_output.data();

    // For each position - note that the physical x coordinate needs to be offset
    // to account for the padding we added
    for (std::size_t y = 1; y < bit_input.y_size_physical() - 1; ++y) {
        // Loop through the original width plus an additional word on each side
        for (std::size_t x = 1; x < bit_input.x_size_physical() - 1; ++x) {
            // Create the state for this position
            grid_conf_t state;
            state.x = x;
            state.y = y;
            state.width_b = bit_input.x_size_physical();
            state.height_b = bit_input.y_size_physical(); // Use physical height with padding
            state.bit_grid = grid_data;
            
            // Evaluate algorithm and set result
            auto result = evaluator<cell_row_t, cell_state_dictionary, Algorithm>::evaluate(state);

            // Debug output

            auto offset = y * bit_input.x_size_physical() + x;
            result.save_to(grid_data_out, offset);
        }
    }

    auto final_grid = bitgrid_to_standard(bit_output);
    std::copy(final_grid.begin(), final_grid.end(), output);
}

void run_GoL_reference_algorithm(
    std::size_t height, std::size_t width,
    cell_state* grid_input, cell_state* grid_output) {
    for (std::size_t i = 1; i < height - 1; ++i) {
        for (std::size_t j = 1; j < width - 1; ++j) {
            int alive_count = 0;
            for (int dx = -1; dx <= 1; ++dx) {
                for (int dy = -1; dy <= 1; ++dy) {
                    if (dx == 0 && dy == 0) continue;
                    int ni = i + dx;
                    int nj = j + dy;
                    if (ni >= 0 && ni < height && nj >= 0 && nj < width) {
                        alive_count += 
                            grid_input[ni * width + nj] == cell_state::alive ? 1 : 0;
                    }
                }
            }

            if (grid_input[i * width + j] == cell_state::alive) {
                grid_output[i * width + j] = (alive_count == 2 || alive_count == 3) 
                    ? cell_state::alive : cell_state::dead;
            } else {
                grid_output[i * width + j] = (alive_count == 3) ?
                    cell_state::alive : cell_state::dead;
            }
        }
    }
}

void place_glider(cell_state* grid, std::size_t height, std::size_t width) {
    if (height < 5 || width < 5) return;

    auto x_start = 25;
    auto y_start = 0;

    grid[(y_start + 1) * width + (x_start + 2)] = cell_state::alive;
    grid[(y_start + 2) * width + (x_start + 3)] = cell_state::alive;
    grid[(y_start + 3) * width + (x_start + 1)] = cell_state::alive;
    grid[(y_start + 3) * width + (x_start + 2)] = cell_state::alive;
    grid[(y_start + 3) * width + (x_start + 3)] = cell_state::alive;
}

int main(int argc, char* argv[]) {
    std::size_t height = 10;
    std::size_t width = 64;
    std::size_t temporal_steps = 15;

    std::vector<cell_state> grid(height * width, cell_state::dead);
    
    place_glider(grid.data(), height, width);
    
    std::cout << "Initial grid:\n";
    print_grid(grid.data(), height, width);
    
    std::vector<cell_state> other_grid(height * width, cell_state::dead);

    cell_state* input_grid = grid.data();
    cell_state* output_grid = other_grid.data();

    for (std::size_t t = 0; t < temporal_steps; ++t) {
        // Use the bitwise version of the algorithm
        run_algorithm_bitwise<game_of_life_algorithm>(
            height, width, 
            input_grid, output_grid);

        std::cout << "Grid at time step " << t + 1 << ":\n";
        print_grid(output_grid, height, width);
        std::cout << std::endl;

        // wait for 500ms
        // std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // wait for enter
        std::cout << "Press Enter to continue...";
        std::cin.get();
        std::cout << std::endl;

        std::swap(input_grid, output_grid);
    }

    return 0;
}