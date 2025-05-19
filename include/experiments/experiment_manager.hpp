#ifndef EXPERIMENT_MANAGER_HPP
#define EXPERIMENT_MANAGER_HPP

#include <vector>
#include <thread>

namespace cellib::run {

using namespace cellib::memory::grids;

template <typename cell_t>
using print_config = cellib::memory::grids::standard::print_config<cell_t>;

template <typename test_suite>
class experiment_manager {

public:
    using original_cell_t = typename test_suite::original_cell_t;
    using grid_store_word_t = typename test_suite::grid_store_word_t;

    using grid_t = typename test_suite::grid_t;
    using traverser_t = typename test_suite::traverser_t;
    using run_params = cellib::run::run_params;

    using standard_grid_t = cellib::memory::grids::standard::grid<original_cell_t>;

    experiment_manager() = default;

    void run_experiment(const run_params& params, const std::vector<original_cell_t>& initial_state) {
        grid_t grid = get_padded_grid(params, initial_state);
        traverser_t traverser = get_initialized_traverser(grid);

        run_traverser(traverser, params);

        grid_t result = traverser.fetch_result();
        auto result_as_standard = result.to_standard();
    }

    void set_print_config(print_config<original_cell_t> config) {
        _print_config = config;
    }

    private:
    print_config<original_cell_t> _print_config;

    grid_t get_padded_grid(const run_params& params, const std::vector<original_cell_t>& initial_state) {
        standard_grid_t initial_grid(params.x_size, params.y_size);
        std::copy(initial_state.begin(), initial_state.end(), initial_grid.data());

        auto grid_padded = initial_grid.template with_empty_margins<test_suite::x_margin, test_suite::y_margin>();

        grid_t grid{grid_padded};
        return grid;
    }

    void run_traverser(traverser_t& traverser, const run_params& params) {
        traverser.run(params.steps, 
            [&](int iter, const auto& grid) {
                auto standard_grid = grid
                    .to_standard()
                    .template with_removed_margins<test_suite::x_margin, test_suite::y_margin>();

                std::cout << "\nIteration: " << iter << "\n";
                standard_grid.print(std::cout, _print_config);

                std::this_thread::sleep_for(std::chrono::milliseconds(400));
                std::cout << "\n";
            }
        );
        // if (params.print) {
        //     traverser.template run<true>(params.steps);
        // }
        // else {
        //     traverser.template run<false>(params.steps);
        // }
    }

    traverser_t get_initialized_traverser(grid_t& grid) {
        traverser_t traverser;
        traverser.init(grid);
        // traverser.set_print_config(_print_config);
        return traverser;
    }
};

}

#endif // EXPERIMENT_MANAGER_HPP