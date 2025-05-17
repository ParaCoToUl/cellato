#ifndef GREENBERG_HASTINGS_CONFIG_HPP
#define GREENBERG_HASTINGS_CONFIG_HPP

#include "./algorithm.hpp"
#include "./pretty_print.hpp"
#include "./data_init.hpp"

namespace greenberg {

struct config {
    using cell_state = ghm_cell_state;
    using algorithm = ghm_algorithm;
    using pretty_print = ghm_pretty_print;
    
    struct input {
        using random = ghm_random_init;
    };
};

}

#endif // GREENBERG_HASTINGS_CONFIG_HPP
