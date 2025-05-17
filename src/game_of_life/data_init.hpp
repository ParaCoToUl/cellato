#ifndef GAME_OF_LIFE_INIT_HPP
#define GAME_OF_LIFE_INIT_HPP

#include <vector>
#include <tuple>

namespace game_of_life {

    struct random_init {
        static std::vector<game_of_life::cell_state> init(cellib::run::run_params& params) {

            std::vector<game_of_life::cell_state> initial_state(params.x_size * params.y_size);

            cellib::memory::grids::utils::generate_random_grid(
                initial_state,
                params.y_size, params.x_size,
                game_of_life::cell_state::alive, 0.2,
                game_of_life::cell_state::dead, 0.8
            );

            return initial_state;
        }
    };
}

#endif // GAME_OF_LIFE_INIT_HPP