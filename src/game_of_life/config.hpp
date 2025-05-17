#ifndef GAME_OF_LIFE_CONFIG_HPP
#define GAME_OF_LIFE_CONFIG_HPP

#include "./algorithm.hpp"
#include "./pretty_print.hpp"
#include "./data_init.hpp"

namespace game_of_life {

struct config {

    using cell_state = typename game_of_life::cell_state;
    using algorithm = typename game_of_life::algorithm;
    using pretty_print = typename game_of_life::pretty_print;
    
    struct input {
        using random = game_of_life::random_init;
    };
};

} // namespace game_of_life

#endif // GAME_OF_LIFE_CONFIG_HPP