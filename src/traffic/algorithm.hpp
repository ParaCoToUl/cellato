#ifndef TRAFFIC_ALGORITHM_HPP
#define TRAFFIC_ALGORITHM_HPP

#include "core/ast.hpp"

namespace traffic {
using namespace cellato::ast;

enum class traffic_cell_state {
    empty,
    tree,
    ash,
    traffic
};

using traffic_algorithm = current_state; // Placeholder for actual traffic algorithm

}

#endif // TRAFFIC_ALGORITHM_HPP
