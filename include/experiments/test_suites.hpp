#ifndef CELLIB_TEST_SUITES_HPP
#define CELLIB_TEST_SUITES_HPP

#include "../evaluators/standard.hpp"
#include "../memory/standard_grid.hpp"
#include "../traversers/cpu/simple.hpp"

namespace cellib::run::test_suites {

template < typename cell_type, typename algorithm>
struct cpu_standard {
    using cell_t = cell_type;

    using algorithm_t = algorithm;

    using grid_t = cellib::memory::grids::standard::grid<cell_t>;
    using evaluator_t = cellib::evaluators::standard::evaluator<cell_t, algorithm_t>;

    using traverser_t = cellib::traversers::cpu::simple::traverser<evaluator_t, grid_t>;

    constexpr static int x_margin = 1;
    constexpr static int y_margin = 1;
};

}

#endif // CELLIB_TEST_SUITES_HPP