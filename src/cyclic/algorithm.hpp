#ifndef CYCLIC_ALGORITHM_HPP
#define CYCLIC_ALGORITHM_HPP

#include "core/ast.hpp"

namespace cyclic {
using namespace cellato::ast;

enum class cyclic_cell_state {
    empty,
    tree,
    ash,
    cyclic
};

using cyclic_algorithm = current_state; // Placeholder for actual cyclic algorithm

}

#endif // CYCLIC_ALGORITHM_HPP
