namespace cellib::run {

using namespace cellib::memory::grids;

template <typename test_suite>
class experiment_manager {

public:
    using cell_t = typename test_suite::grid_t::cell_t;
    using grid_t = typename test_suite::grid_t;
    using traverser_t = typename test_suite::traverser_t;
    using run_params = cellib::run::run_params;

    using standard_grid_t = cellib::memory::grids::standard::grid<cell_t>;

    experiment_manager() = default;

    void run_experiment(const run_params& params, const std::vector<cell_t>& initial_state) {

        standard_grid_t initial_grid(params.x_size, params.y_size);
        std::copy(initial_state.begin(), initial_state.end(), initial_grid.data());

        grid_t grid{initial_grid};
        traverser_t traverser;

        traverser.init(grid);

        traverser.template run<true>(params.steps);

        grid_t result = traverser.fetch_result();
        auto result_as_standard = result.to_standard();

        result_as_standard.print(std::cout);
    }
};

}