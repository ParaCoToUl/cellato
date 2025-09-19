#ifndef CRITTERS_ALGORITHM_HPP
#define CRITTERS_ALGORITHM_HPP

#include "core/ast.hpp"

namespace critters {
using namespace cellato::ast;

enum class critters_cell_state {
    dead, alive
};

using dead = state_constant<critters_cell_state::dead>;
using alive = state_constant<critters_cell_state::alive>;

using toggled = if_< p<current_state, equals, alive> >::then_< dead >::else_< alive >;

using alive_count = count_neighbors<alive, margolus_alternating_neighborhood>;

using has_2_alive_in_block = p<alive_count, equals, constant<2>>;

using critters_algorithm = 
    if_< has_2_alive_in_block >::then_<
        current_state 
    >::else_<
        toggled
    >;
}

#endif // CRITTERS_ALGORITHM_HPP
