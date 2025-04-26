#include <iostream>
#include <vector>
#include <thread>

#include "constructs.hpp"
#include "bit-mode.hpp"
#include "bit-evaluator.hpp"

using namespace expr_tree;
using namespace bitwise;
using namespace bitwise_no_cache;

using cell_row_t = uint8_t; // use only 8 bits

enum class cell_state { dead, alive };  // Note: Updated order to match state_dictionary convention (typically starts with 0)

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
    
    std::vector<cell_state> padded_grid(height * adjusted_width, cell_state::dead);

    // Copy original grid with padding
    for (std::size_t y = 0; y < height; y++) {
        std::copy(grid + y * width, grid + (y + 1) * width, padded_grid.data() + y * adjusted_width);
    }

    return bit_grid<cell_row_t, cell_state_dictionary>(height, adjusted_width, padded_grid.data());
}

std::vector<cell_state> bitgrid_to_standard(
    const bit_grid<cell_row_t, cell_state_dictionary>& bit_grid) {
        
    std::size_t bits_per_word = sizeof(cell_row_t) * 8;

    std::size_t height = bit_grid.y_size_original();
    std::size_t adjusted_width = bit_grid.x_size_original();
    std::size_t width = adjusted_width - 2 * bits_per_word;
    
    std::vector<cell_state> standard_grid(height * width, cell_state::dead);
    
    auto adjusted_grid = bit_grid.to_original_representation();
    
    for (std::size_t y = 0; y < height; ++y) {
        for (std::size_t x = 0; x < width; ++x) {
            standard_grid[y * width + x] = adjusted_grid[y * adjusted_width + x + bits_per_word];
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
    auto bit_output = standard_to_bitgrid(output, height, width);

    // Get the adjusted width (may be padded)
    std::size_t adjusted_width = bit_input.x_size_original();
    std::size_t bits_per_word = sizeof(cell_row_t) * 8;
    std::size_t adjusted_width_in_bit_grid = adjusted_width / bits_per_word;

    // Create grid config for bitwise evaluation
    using grid_conf_t = grid_config<cell_row_t, cell_state_dictionary>;
    
    auto grid_data = bit_input.data();
    auto grid_data_out = bit_output.data();

    // For each position
    for (std::size_t y = 1; y < height - 1; ++y) {
        for (std::size_t x = 1; x < adjusted_width - 1; ++x) {
            // Create the state for this position
            grid_conf_t state;
            state.x = x;
            state.y = y;
            state.width_b = adjusted_width_in_bit_grid;
            state.height_b = height;
            state.bit_grid = grid_data;
            
            // Evaluate algorithm and set result
            auto result = evaluator<cell_row_t, cell_state_dictionary, Algorithm>::evaluate(state);

            auto offset = y * state.width_b + x;
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
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        std::swap(input_grid, output_grid);
    }

    return 0;
}