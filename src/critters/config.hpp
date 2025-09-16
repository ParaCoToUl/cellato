#ifndef CRITTERS_CONFIG_HPP
#define CRITTERS_CONFIG_HPP

#include "./algorithm.hpp"
#include "./pretty_print.hpp"
#include "./data_init.hpp"
#include "./reference_implementation.hpp"
#include "memory/state_dictionary.hpp"

namespace critters {

struct config {

    static constexpr auto name = "critters";

    using algorithm = critters_algorithm;
    
    using cell_state = critters_cell_state;
    using state_dictionary = cellato::memory::grids::state_dictionary<
        cell_state::empty, cell_state::tree,
        cell_state::critters, cell_state::ash>;

    using pretty_print = critters_pretty_print;

    using reference_implementation = reference::runner;

    struct input {
        using random = critters_random_init;
    };
};

}

#endif // CRITTERS_CONFIG_HPP
