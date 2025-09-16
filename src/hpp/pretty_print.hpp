#ifndef HPP_PRETTY_PRINT_HPP
#define HPP_PRETTY_PRINT_HPP

#include "memory/standard_grid.hpp"
#include "./algorithm.hpp"

namespace hpp {

using print_config = cellato::memory::grids::standard::print_config<hpp_cell_state>;

struct hpp_pretty_print {
    static print_config get_config() {
        return print_config()
            .with(hpp_cell_state::empty, "\033[90m.\033[0m")  // Dark grey for empty ground (almost invisible)
            .with(hpp_cell_state::tree, "\033[1;32m#\033[0m") // Bright green for trees
            .with(hpp_cell_state::ash, "\033[1;37m*\033[0m")  // Light gray for ash
            .with(hpp_cell_state::hpp, "\033[1;31m@\033[0m"); // Bright red for hpp
    }
};

}

#endif // HPP_PRETTY_PRINT_HPP
