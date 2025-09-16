#ifndef CRITTERS_ALGORITHM_HPP
#define CRITTERS_ALGORITHM_HPP

#include "core/ast.hpp"

namespace critters {
using namespace cellato::ast;

enum class critters_cell_state {
    empty,
    tree,
    ash,
    critters
};

using critters_algorithm = current_state; // Placeholder for actual critters algorithm

}

#endif // CRITTERS_ALGORITHM_HPP
