#include <iostream>
#include <vector>

#include "cellato/core/ast.hpp"
#include "cellato/experiments/experiment_manager.hpp"
#include "cellato/experiments/test_suites.hpp"
#include "cellato/memory/standard_grid.hpp"
#include "cellato/memory/state_dictionary.hpp"

namespace your_own_ca {
using namespace cellato::ast;

enum class cell_state {
    state_a,
    state_b,
    state_c
};

using state_a = state_constant<cell_state::state_a>;
using state_b = state_constant<cell_state::state_b>;
using state_c = state_constant<cell_state::state_c>;

using is_state_a = p<current_state, equals, state_a>;
using is_state_b = p<current_state, equals, state_b>;

using state_a_count = count_neighbors<state_a, moore_8_neighbors>;
using has_two_a_neighbors = p<state_a_count, equals, constant<2>>;

// clang-format off
using rule =
    if_<is_state_a>::then_<
        if_<has_two_a_neighbors>::then_<state_b>::else_<state_a>
    >::elif_<is_state_b>::then_<
        state_c
    >::else_<
        state_a
    >;
// clang-format on

struct pretty_print {
    static cellato::memory::grids::standard::print_config<cell_state> get_config() {
        return cellato::memory::grids::standard::print_config<cell_state>()
            .with(cell_state::state_a, "A")
            .with(cell_state::state_b, "B")
            .with(cell_state::state_c, "C");
    }
};

struct config {
    static constexpr char name[] = "your-own-ca";
    static constexpr double average_halo_radius = 1.0;

    using algorithm = rule;
    using cell_state = your_own_ca::cell_state;
    using state_dictionary =
        cellato::memory::grids::state_dictionary<cell_state::state_a, cell_state::state_b, cell_state::state_c>;
};

std::vector<cell_state> initial_state() {
    return {
        cell_state::state_a,
        cell_state::state_a,
        cell_state::state_c,
        cell_state::state_c,
        cell_state::state_b,
        cell_state::state_c,
        cell_state::state_c,
        cell_state::state_a,
        cell_state::state_a,
    };
}

} // namespace your_own_ca

int main() {
    using suite = cellato::run::test_suites::on_cpu::standard<your_own_ca::config>;

    cellato::run::run_params params{.automaton = your_own_ca::config::name,
                                    .device = "CPU",
                                    .traverser = "simple",
                                    .evaluator = "standard",
                                    .layout = "standard",
                                    .x_size = 3,
                                    .y_size = 3,
                                    .steps = 3,
                                    .rounds = 1,
                                    .warmup_rounds = 0,
                                    .print = true};

    cellato::run::experiment_manager<suite> manager;
    manager.set_print_config(your_own_ca::pretty_print::get_config());

    const auto report = manager.run_experiment(params, your_own_ca::initial_state());

    std::cout << "\nCSV header:\n" << cellato::run::experiment_report::csv_header() << "\n";
    std::cout << "\nCSV result:\n" << report.csv_line() << "\n";
}
