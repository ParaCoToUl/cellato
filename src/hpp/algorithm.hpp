#ifndef HPP_ALGORITHM_HPP
#define HPP_ALGORITHM_HPP

#include "core/ast.hpp"

namespace hpp {
using namespace cellato::ast;

enum class hpp_cell_state {
    empty,
    tree,
    ash,
    hpp
};

using hpp_algorithm = current_state; // Placeholder for actual HPP algorithm

}

#endif // HPP_ALGORITHM_HPP
