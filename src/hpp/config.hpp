#ifndef HPP_CONFIG_HPP
#define HPP_CONFIG_HPP

#include "./algorithm.hpp"
#include "./pretty_print.hpp"
#include "./data_init.hpp"
#include "./reference_implementation.hpp"
#include "memory/state_dictionary.hpp"

namespace hpp {

struct config {

    static constexpr auto name = "hpp";

    using algorithm = hpp_algorithm;
    
    using cell_state = hpp_cell_state;
    using state_dictionary = cellato::memory::grids::int_based_state_dictionary<4>; // 4 bit int

    using pretty_print = hpp_pretty_print;

    using reference_implementation = reference::runner;

    struct input {
        using random = hpp_random_init;
    };
};

}

#endif // HPP_CONFIG_HPP
