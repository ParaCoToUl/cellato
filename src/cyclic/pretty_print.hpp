#ifndef CYCLIC_PRETTY_PRINT_HPP
#define CYCLIC_PRETTY_PRINT_HPP

#include "memory/standard_grid.hpp"
#include "./algorithm.hpp"

namespace cyclic {

using print_config = cellato::memory::grids::standard::print_config<cyclic_cell_state>;

struct cyclic_pretty_print {
    static print_config get_config() {
        return print_config()
            .with(cyclic_cell_state::empty, "\033[90m.\033[0m")  // Dark grey for empty ground (almost invisible)
            .with(cyclic_cell_state::tree, "\033[1;32m#\033[0m") // Bright green for trees
            .with(cyclic_cell_state::ash, "\033[1;37m*\033[0m")  // Light gray for ash
            .with(cyclic_cell_state::cyclic, "\033[1;31m@\033[0m"); // Bright red for cyclic
    }
};

}

#endif // CYCLIC_PRETTY_PRINT_HPP
