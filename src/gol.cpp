#include <iostream>
#include <vector>
#include <thread>

#include "constructs.hpp"
#include "bit_mode.hpp"

using namespace expr_tree;
using namespace simple_evaluator;


enum class cell_state { alive, dead };

template <cell_state val>
using c = constant<cell_state, val>;

using c_2 = constant<int, 2>;
using c_3 = constant<int, 3>;

using alive = c<cell_state::alive>;
using dead = c<cell_state::dead>;

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
    std::size_t width = 10;
    std::size_t temporal_steps = 10;

    std::vector<cell_state> grid(height * width, cell_state::dead);

    // for (std::size_t i = 0; i < height; ++i) {
    //     for (std::size_t j = 0; j < width; ++j) {
    //         grid[i * width + j] = rand() % 2;
    //     }
    // }
    
    place_glider(grid.data(), height, width);
    
    std::cout << "Initial grid:\n";
    print_grid(grid.data(), height, width);
    
    
    std::vector<cell_state> other_grid(height * width, cell_state::dead);

    cell_state* input_grid = grid.data();
    cell_state* output_grid = other_grid.data();

    for (std::size_t t = 0; t < temporal_steps; ++t) {
        run_algorithm<game_of_life_algorithm>(
            height, width, 
            input_grid, output_grid);

        // run_GoL_reference_algorithm(
        //     height, width, 
        //     input_grid, output_grid);

        std::cout << "Grid at time step " << t + 1 << ":\n";
        print_grid(output_grid, height, width);
        std::cout << std::endl;

        // wait for 500ms
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        std::swap(input_grid, output_grid);
    }

    return 0;
}