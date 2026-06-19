#include <algorithm>
#include <cstdint>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

#include "cellato/experiments/experiment_manager.hpp"
#include "cellato/experiments/reference_impl_manager.hpp"
#include "cellato/experiments/run_params.hpp"
#include "cellato/experiments/test_suites.hpp"
#include "cellato/memory/grid_utils.hpp"

#include "brian/config.hpp"
#include "critters/config.hpp"
#include "cyclic/config.hpp"
#include "excitable/config.hpp"
#include "fire/config.hpp"
#include "fluid/config.hpp"
#include "game_of_life/algorithm.hpp"
#include "game_of_life/config.hpp"
#include "game_of_life/pretty_print.hpp"
#include "maze/config.hpp"
#include "traffic/config.hpp"
#include "wire/config.hpp"

#include "args_parser.hpp"

namespace {

constexpr int default_rounds = 1;
constexpr int default_warmup_rounds = 0;
constexpr int default_seed = 42;
constexpr int default_cuda_block_size_x = 32;
constexpr int default_cuda_block_size_y = 8;

bool requires_word_size(const std::optional<std::string>& evaluator) {
    return evaluator && (*evaluator == "bit_planes" || *evaluator == "bit_array");
}

bool requires_temporal_options(const std::optional<std::string>& traverser) {
    return traverser && *traverser == "temporal";
}

int parse_int_option(const input::parser& parser, const std::string& option) {
    const auto raw_value = parser.require(option);

    try {
        std::size_t parsed_chars = 0;
        const int value = std::stoi(raw_value, &parsed_chars);
        if (parsed_chars != raw_value.size()) {
            throw std::invalid_argument("trailing characters");
        }

        return value;
    } catch (const std::invalid_argument&) {
        throw std::invalid_argument("Invalid integer for --" + option + ": " + raw_value);
    } catch (const std::out_of_range&) {
        throw std::out_of_range("Integer out of range for --" + option + ": " + raw_value);
    }
}

int parse_optional_int_option(const input::parser& parser, const std::string& option, int default_value) {
    if (!parser.exists(option)) {
        return default_value;
    }

    return parse_int_option(parser, option);
}

template <typename automaton_config>
bool automaton_matches(const std::string& automaton) {
    if (automaton == automaton_config::name) {
        return true;
    }

    if constexpr (std::is_same_v<automaton_config, fire::config>) {
        return automaton == "fire";
    }

    return false;
}

template <typename... all_test_suites>
struct switch_ {
    static void run(cellato::run::run_params& params) {
        // If reference implementation is requested, handle it separately
        if (params.reference_impl != "none") {
            bool ref_executed = run_reference_impl(params);
            if (!ref_executed) {
                std::cerr << "No suitable reference implementation found for the given parameters." << std::endl;
            }
            return;
        }

        bool any_executed = (call<all_test_suites>(params) || ...);
        if (!any_executed) {
            std::cerr << "No suitable test suite found for the given parameters." << std::endl;
        }
    }

private:
    static bool run_reference_impl(cellato::run::run_params& params) {
        if (params.reference_impl != "baseline") {
            return false;
        }

        return (run_reference_if_for<all_test_suites>(params) || ...);
    }

    template <typename test_suite>
    static bool run_reference_if_for(cellato::run::run_params& params) {
        using automaton_config = typename test_suite::automaton;
        if (!automaton_matches<automaton_config>(params.automaton)) {
            return false;
        }

        return run_reference_for_automaton<automaton_config>(params);
    }

    template <typename automaton_config, typename runner_t = typename automaton_config::reference_implementation>
    static bool run_reference_for_automaton(cellato::run::run_params& params) {
        using cell_state_t = typename automaton_config::cell_state;

        // Generate initial state using the automaton's random initializer
        auto initial_state = automaton_config::input::random::init(params);

        // Run the reference implementation
        cellato::run::reference_impl_manager<runner_t, cell_state_t> manager;
        auto report = manager.run_experiment(params, initial_state);

        std::cout << report.csv_line() << std::endl;
        report.pretty_print(std::cerr);

        return true;
    }

    template <typename test_suite>
    static bool call(cellato::run::run_params& params) {
        if (!test_suite::is_for(params)) {
            return false;
        }

        using cellular_automaton = typename test_suite::automaton;

        auto initial_state = cellular_automaton::input::random::init(params);

        cellato::run::experiment_manager<test_suite> manager;
        manager.set_print_config(cellular_automaton::pretty_print::get_config());

        auto report = manager.run_experiment(params, initial_state);

        std::cout << report.csv_line() << std::endl;

        report.pretty_print(std::cerr);

        return true;
    }
};

cellato::run::run_params get_params(int argc, char* argv[]) {
    input::parser parser{argc, argv};

    if (parser.exists("help")) {
        return cellato::run::run_params{.help = true};
    }

    if (parser.exists("print_csv_header")) {
        return cellato::run::run_params{.print_csv_header = true};
    }

    std::vector<std::string> required{
        "automaton",
        "device",
        "traverser",
        "evaluator",
        "layout",
        "x_size",
        "y_size",
        "steps",
    };

    if (requires_word_size(parser.get("evaluator"))) {
        required.push_back("word_size");
    }

    if (requires_temporal_options(parser.get("traverser"))) {
        required.push_back("temporal_steps");
        required.push_back("temporal_tile_size_y");
    }

    if (parser.exists("reference_impl")) {
        for (const auto& no_longer_required : {"device", "traverser", "evaluator", "layout"}) {
            required.erase(std::remove(required.begin(), required.end(), no_longer_required), required.end());
        }
    }

    for (const auto& opt : required) {
        if (!parser.exists(opt)) {
            throw std::invalid_argument("Missing required option: --" + opt);
        }
    }

    cellato::run::run_params params{
        .automaton = parser.require("automaton"),

        .device = parser.exists("device") ? parser.require("device") : "",
        .traverser = parser.exists("traverser") ? parser.require("traverser") : "",
        .evaluator = parser.exists("evaluator") ? parser.require("evaluator") : "",
        .layout = parser.exists("layout") ? parser.require("layout") : "",

        .reference_impl = parser.exists("reference_impl") ? parser.require("reference_impl") : "none",

        .x_size = parse_int_option(parser, "x_size"),
        .y_size = parse_int_option(parser, "y_size"),
        .steps = parse_int_option(parser, "steps"),

        .word_size = parse_optional_int_option(parser, "word_size", 0),

        .x_tile_size = parse_optional_int_option(parser, "x_tile_size", 0),
        .y_tile_size = parse_optional_int_option(parser, "y_tile_size", 0),

        .temporal_steps = parse_optional_int_option(parser, "temporal_steps", 0),
        .temporal_tile_size_y = parse_optional_int_option(parser, "temporal_tile_size_y", 0),

        .rounds = parse_optional_int_option(parser, "rounds", default_rounds),
        .warmup_rounds = parse_optional_int_option(parser, "warmup_rounds", default_warmup_rounds),

        .seed = parse_optional_int_option(parser, "seed", default_seed),

        .print = parser.exists("print"),
        .help = parser.exists("help"),
        .print_csv_header = parser.exists("print_csv_header"),

        .cuda_block_size_x = parse_optional_int_option(parser, "cuda_block_size_x", default_cuda_block_size_x),
        .cuda_block_size_y = parse_optional_int_option(parser, "cuda_block_size_y", default_cuda_block_size_y)};

    return params;
}

void print_usage() {
    std::cout << "Usage: ./cellato [options]\n";
    std::cout << "Options:\n";
    std::cout << "  --automaton <name>              Name of the cellular automaton\n";
    std::cout << "  --device <name>                 Device to run on (CPU, CUDA)\n";
    std::cout << "  --traverser <name>              Traverser type (simple, spatial_blocking)\n";
    std::cout << "  --evaluator <name>              Evaluator type (standard, bit_planes)\n";
    std::cout << "  --layout <name>                 Layout type (standard, bit_array, bit_planes)\n";
    std::cout << "  --reference_impl <name>         Reference implementation to use (baseline)\n";
    std::cout << "  --x_size <number>               X size of the grid\n";
    std::cout << "  --y_size <number>               Y size of the grid\n";
    std::cout << "  --x_tile_size <number>          X tile size for CUDA\n";
    std::cout << "  --y_tile_size <number>          Y tile size for CUDA\n";
    std::cout << "  --temporal_steps <number>       Temporal steps for CUDA (only for temporal_tiled_bit_planes)\n";
    std::cout << "  --temporal_tile_size_y <number> Temporal tile size Y for CUDA (only for temporal_tiled_bit_planes)\n";
    std::cout << "  --rounds <number>               Number of rounds to run\n";
    std::cout << "  --warmup_rounds <number>        Number of warmup rounds to run\n";
    std::cout << "  --steps <number>                Number of steps to run\n";
    std::cout << "  --word_size <number>            word_size for floating-point calculations (32, 64)\n";
    std::cout << "  --seed <number>                 Random seed for initialization\n";
    std::cout << "  --print                         Print the grid after each step\n";
    std::cout << "  --cuda_block_size_x <number>    CUDA block size X (default: 32)\n";
    std::cout << "  --cuda_block_size_y <number>    CUDA block size Y (default: 8)\n";
    std::cout << "  --print_csv_header              Print CSV header\n";
    std::cout << "  --help                          Show this help message\n";
}

} // namespace

int main(int argc, char* argv[]) {
    cellato::run::run_params params;
    try {
        params = get_params(argc, argv);
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << "\n\n";
        print_usage();
        return 1;
    }

    if (params.help) {
        print_usage();
        return 0;
    }

    if (params.print_csv_header) {
        std::cout << cellato::run::experiment_report::csv_header() << std::endl;
        return 0;
    }

    if (params.print) {
        params.print_std();
    }

    namespace test = cellato::run::test_suites;

    using _game_of_life_ = game_of_life::config;
    using _fire_ = fire::config;
    using _wire_ = wire::config;
    using _excitable_ = excitable::config;
    using _brian_ = brian::config;
    using _maze_ = maze::config;
    using _fluid_ = fluid::config;
    using _critters_ = critters::config;
    using _cyclic_ = cyclic::config;
    using _traffic_ = traffic::config;

    // clang-format off
#define cases_for(automaton) \
        test::on_cpu::standard<automaton>, \
        test::on_cpu::using_<std::uint32_t>::bit_array<automaton>, \
        test::on_cpu::using_<std::uint64_t>::bit_array<automaton>, \
        test::on_cpu::using_<std::uint32_t>::bit_planes<automaton>, \
        test::on_cpu::using_<std::uint64_t>::bit_planes<automaton>, \
        test::on_cpu::using_<std::uint32_t>::tiled_bit_planes<automaton>, \
        test::on_cpu::using_<std::uint64_t>::tiled_bit_planes<automaton>, \
        test::on_cuda::using_<std::uint32_t>::tiled_bit_planes<automaton>, \
        test::on_cuda::using_<std::uint64_t>::tiled_bit_planes<automaton>, \
        test::on_cuda::standard<automaton>, \
        test::on_cuda::standard<automaton>::with_spatial_blocking<1, 1>, \
        test::on_cuda::standard<automaton>::with_spatial_blocking<2, 1>, \
        test::on_cuda::standard<automaton>::with_spatial_blocking<4, 1>, \
        test::on_cuda::using_<std::uint32_t>::bit_array<automaton>, \
        test::on_cuda::using_<std::uint64_t>::bit_array<automaton>, \
        test::on_cuda::using_<std::uint32_t>::bit_planes<automaton>, \
        test::on_cuda::using_<std::uint64_t>::bit_planes<automaton>, \
        test::on_cuda::using_<std::uint32_t>::temporal_tiled_bit_planes<automaton>, \
        test::on_cuda::using_<std::uint64_t>::temporal_tiled_bit_planes<automaton>, \
        test::on_cuda::using_<std::uint32_t>::temporal_linear_bit_planes<automaton>, \
        test::on_cuda::using_<std::uint64_t>::temporal_linear_bit_planes<automaton>

    switch_<cases_for(_game_of_life_),
            cases_for(_fire_),
            cases_for(_wire_),
            cases_for(_excitable_),
            cases_for(_brian_),
            cases_for(_maze_),
            cases_for(_fluid_),
            cases_for(_critters_),
            cases_for(_cyclic_),
            cases_for(_traffic_)>::run(params);
    // clang-format on

    return 0;
}
