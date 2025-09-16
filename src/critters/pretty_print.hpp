#ifndef CRITTERS_PRETTY_PRINT_HPP
#define CRITTERS_PRETTY_PRINT_HPP

#include "memory/standard_grid.hpp"
#include "./algorithm.hpp"

namespace critters {

using print_config = cellato::memory::grids::standard::print_config<critters_cell_state>;

struct critters_pretty_print {
    static print_config get_config() {
        return print_config()
            .with(critters_cell_state::empty, "\033[90m.\033[0m")  // Dark grey for empty ground (almost invisible)
            .with(critters_cell_state::tree, "\033[1;32m#\033[0m") // Bright green for trees
            .with(critters_cell_state::ash, "\033[1;37m*\033[0m")  // Light gray for ash
            .with(critters_cell_state::critters, "\033[1;31m@\033[0m"); // Bright red for critters
    }
};

}

#endif // CRITTERS_PRETTY_PRINT_HPP
