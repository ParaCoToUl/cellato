#ifndef CYCLIC_CONFIG_HPP
#define CYCLIC_CONFIG_HPP

#include "./algorithm.hpp"
#include "./pretty_print.hpp"
#include "./data_init.hpp"
#include "./reference_implementation.hpp"
#include "memory/state_dictionary.hpp"

namespace cyclic {

struct config {

    static constexpr auto name = "cyclic";

    using algorithm = cyclic_algorithm;
    
    using cell_state = cyclic_cell_state;
    using state_dictionary = cellato::memory::grids::state_dictionary<
        cell_state::empty, cell_state::tree,
        cell_state::cyclic, cell_state::ash>;

    using pretty_print = cyclic_pretty_print;

    using reference_implementation = reference::runner;

    struct input {
        using random = cyclic_random_init;
    };
};

}

#endif // CYCLIC_CONFIG_HPP
