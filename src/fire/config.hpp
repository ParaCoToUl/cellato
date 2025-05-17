#ifndef FOREST_FIRE_CONFIG_HPP
#define FOREST_FIRE_CONFIG_HPP

#include "./algorithm.hpp"
#include "./pretty_print.hpp"
#include "./data_init.hpp"

namespace fire {

struct config {

    using cell_state = fire_cell_state;
    using algorithm = fire_algorithm;
    using pretty_print = fire_pretty_print;

    struct input {
        using random = fire_random_init;
    };
};

}

#endif // FOREST_FIRE_CONFIG_HPP
