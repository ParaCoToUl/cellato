#ifndef GAME_OF_LIFE_INIT_HPP
#define GAME_OF_LIFE_INIT_HPP

#include "./algorithm.hpp"
#include "cellato/experiments/run_params.hpp"
#include "cellato/memory/grid_utils.hpp"
#include <tuple>
#include <vector>

namespace game_of_life {

struct gol_random_init {
    static std::vector<gol_cell_state> init(cellato::run::run_params& params) {

        std::vector<gol_cell_state> initial_state(params.x_size * params.y_size);

        cellato::memory::grids::utils::generate_random_grid(
            initial_state, params.y_size, params.x_size, gol_cell_state::alive, 0.2, gol_cell_state::dead, params.seed);

        return initial_state;
    }
};

} // namespace game_of_life

#endif // GAME_OF_LIFE_INIT_HPP