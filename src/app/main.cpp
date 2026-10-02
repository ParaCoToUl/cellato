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

#include "automata/registry.hpp"

#include "args_parser.hpp"
#include "validation.hpp"

namespace {

constexpr int default_rounds = 1;
constexpr int default_warmup_rounds = 0;
constexpr int default_seed = 42;
constexpr int default_cuda_block_size_x = 32;
constexpr int default_cuda_block_size_y = 8;

bool requires_word_size(const std::optional<std::string>& evaluator) {
    return evaluator && (*evaluator == "bit_planes" || *evaluator == "bit_array" || *evaluator == "tiled_bit_planes");
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
                throw std::logic_error("No reference implementation matches the validated parameters.");
            }
            return;
        }

        bool any_executed = (call<all_test_suites>(params) || ...);
        if (!any_executed) {
            throw std::logic_error("No compiled suite matches the validated parameters.");
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

template <typename test_suite_list>
struct switch_list;

template <typename... all_test_suites>
struct switch_list<cellato::utils::type_list<all_test_suites...>> : switch_<all_test_suites...> {};

cellato::run::run_params get_params(int argc, char* argv[], const input::suite_catalog& catalog) {
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

    const bool reference_requested = parser.exists("reference_impl") && parser.require("reference_impl") != "none";
    if (reference_requested) {
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

    const auto candidates = input::select_suites(params, catalog);
    if (!reference_requested) {
        std::vector<std::string> dependent_options;
        if (requires_word_size(parser.get("evaluator"))) dependent_options.push_back("word_size");
        if (requires_temporal_options(parser.get("traverser"))) {
            dependent_options.push_back("temporal_steps");
            dependent_options.push_back("temporal_tile_size_y");
        }
        if (params.traverser == "spatial_blocking") {
            dependent_options.push_back("x_tile_size");
            dependent_options.push_back("y_tile_size");
        }
        for (const auto& option : dependent_options) {
            if (!parser.exists(option)) {
                throw std::invalid_argument("Missing required option: --" + option + " for the selected suite.");
            }
        }
    }

    input::validate_parameters(params, candidates);
    return params;
}

void print_usage(const input::suite_catalog& catalog) {
    std::cout << "Usage: ./cellato [options]\n";
    std::cout << "Options:\n";
    std::cout << "  --automaton <name>              "
              << input::join(input::names(catalog, &input::suite_options::automaton)) << "\n";
    std::cout << "  --device <name>                 "
              << input::join(input::names(catalog, &input::suite_options::device)) << "\n";
    std::cout << "  --traverser <name>              "
              << input::join(input::names(catalog, &input::suite_options::traverser)) << "\n";
    std::cout << "  --evaluator <name>              "
              << input::join(input::names(catalog, &input::suite_options::evaluator)) << "\n";
    std::cout << "  --layout <name>                 "
              << input::join(input::names(catalog, &input::suite_options::layout)) << "\n";
    std::cout << "  --reference_impl <name>         Reference implementation to use (baseline)\n";
    std::cout << "  --x_size <number>               X size of the grid\n";
    std::cout << "  --y_size <number>               Y size of the grid\n";
#if CELLATO_ENABLE_CUDA
    std::cout << "  --x_tile_size <number>          X tile size for CUDA\n";
    std::cout << "  --y_tile_size <number>          Y tile size for CUDA\n";
    std::cout << "  --temporal_steps <number>       Time steps per temporal CUDA batch\n";
    std::cout << "  --temporal_tile_size_y <number> Temporal CUDA tile height in words\n";
#endif
    std::cout << "  --rounds <number>               Number of rounds to run\n";
    std::cout << "  --warmup_rounds <number>        Number of warmup rounds to run\n";
    std::cout << "  --steps <number>                Number of steps to run\n";
    std::cout << "  --word_size <number>            Packed storage word width (32, 64 bits)\n";
    std::cout << "  --seed <number>                 Random seed for initialization and probabilistic rules\n";
    std::cout << "  --print                         Print the grid after each step\n";
#if CELLATO_ENABLE_CUDA
    std::cout << "  --cuda_block_size_x <number>    CUDA block size X (default: 32)\n";
    std::cout << "  --cuda_block_size_y <number>    CUDA block size Y (default: 8)\n";
#endif
    std::cout << "  --print_csv_header              Print CSV header\n";
    std::cout << "  --help                          Show this help message\n";
}

} // namespace

int main(int argc, char* argv[]) {
    using all_suites = cellato::run::test_suites::suites_for_all_t<cellato::automata::all>;
    try {
        const auto catalog = input::make_suite_catalog(all_suites{});
        auto params = get_params(argc, argv, catalog);

        if (params.help) {
            print_usage(catalog);
            return 0;
        }
        if (params.print_csv_header) {
            std::cout << cellato::run::experiment_report::csv_header() << std::endl;
            return 0;
        }
        if (params.print) params.print_std();

        switch_list<all_suites>::run(params);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << "\nUse --help to see available options.\n";
        return 1;
    }
}
