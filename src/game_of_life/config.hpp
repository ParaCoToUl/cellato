#ifndef GAME_OF_LIFE_CONFIG_HPP
#define GAME_OF_LIFE_CONFIG_HPP

#include "./algorithm.hpp"
#include "./pretty_print.hpp"
#include "./data_init.hpp"

namespace game_of_life {

struct config {

    using cell_state = gol_cell_state;
    using algorithm = gol_algorithm;
    using pretty_print = gol_pretty_print;

    struct input {
        using random = gol_random_init;
    };
};

} // namespace game_of_life

#endif // GAME_OF_LIFE_CONFIG_HPP