#include <iostream>
#include <vector>
#include <random>
#include <algorithm>
#include <unordered_map>
#include <functional>
#include <string>

#include "experiments/run_params.hpp"
#include "experiments/test_suites.hpp"
#include "experiments/experiment_manager.hpp"
#include "memory/grid_utils.hpp"

#include "game_of_life/algorithm.hpp"
#include "game_of_life/pretty_print.hpp"
#include "game_of_life/config.hpp"
#include "fire/config.hpp"
#include "greenberg/config.hpp"
#include "wire/config.hpp"

#include "args-parser.hpp"


#define LOG std::cerr
#define REPORT std::cout

template <typename... all_test_suites>
struct switch_ {
    static void run(cellib::run::run_params& params) {
        bool any_executed = (call<all_test_suites>(params) || ...);
        if (!any_executed) {
            std::cerr << "No suitable test suite found for the given parameters." << std::endl;
        }
    }

private:
    template <typename test_suite>
    static bool call(cellib::run::run_params& params) {
        if (!test_suite::is_for(params)) {
            return false;
        }

        using cellular_automaton = typename test_suite::automaton;

        auto initial_state = cellular_automaton::input::random::init(params);

        cellib::run::experiment_manager<test_suite> manager;
        manager.set_print_config(cellular_automaton::pretty_print::get_config());

        auto report = manager.run_experiment(
            params, initial_state
        );

        REPORT << report.csv_line() << std::endl;
        
        report.pretty_print(LOG);

        return true;
    }
};


template <typename test_suite>
void run(cellib::run::run_params& params) {

}

cellib::run::run_params get_params(int argc, char* argv[]) {
    input::parser parser {argc, argv};

    if (parser.exists("help")) {
        return cellib::run::run_params{.help = true};
    }

    if (parser.exists("print_csv_header")) {
        return cellib::run::run_params{.print_csv_header = true};
    }

    std::vector<std::string> required {
        "automaton",
        "device", "traverser", "evaluator", "layout",
        "x_size", "y_size", "steps",
    };

    std::vector<std::string> optional {
        "print", "precision", "x_tile_size", "y_tile_size",
        "seed", "rounds", "warmup_rounds", "print_csv_header",
    };

    for (const auto& opt : required) {
        if (!parser.exists(opt)) {
            std::cerr << "Missing required option: " << opt << std::endl;
            exit(1);
        }
    }

    cellib::run::run_params params {
        .automaton = parser.get("automaton"),

        .device = parser.get("device"),
        .traverser = parser.get("traverser"),
        .evaluator = parser.get("evaluator"),
        .layout = parser.get("layout"),

        .x_size = std::stoi(parser.get("x_size")),
        .y_size = std::stoi(parser.get("y_size")),
        .steps = std::stoi(parser.get("steps")),

        .precision = parser.exists("precision") ? std::stoi(parser.get("precision")) : 0,
        
        .x_tile_size = parser.exists("x_tile_size") ? std::stoi(parser.get("x_tile_size")) : 0,
        .y_tile_size = parser.exists("y_tile_size") ? std::stoi(parser.get("y_tile_size")) : 0,
        
        .rounds = parser.exists("rounds") ? std::stoi(parser.get("rounds")) : 1,
        .warmup_rounds = parser.exists("warmup_rounds") ? std::stoi(parser.get("warmup_rounds")) : 0,

        .seed = parser.exists("seed") ? std::stoi(parser.get("seed")) : 42,

        .print = parser.exists("print"),
        .help = parser.exists("help"),
        .print_csv_header = parser.exists("print_csv_header"),
    };

    return params;
}

void print_usage() {
    std::cout << "Usage: ./cellib [options]\n";
    std::cout << "Options:\n";
    std::cout << "  --automaton <name>       Name of the cellular automaton\n";
    std::cout << "  --device <name>          Device to run on (CPU, CUDA)\n";
    std::cout << "  --traverser <name>       Traverser type (simple, spacial_blocking)\n";
    std::cout << "  --evaluator <name>       Evaluator type (standard, bit_plates)\n";
    std::cout << "  --layout <name>          Layout type (standard, bit_array, bit_plates)\n";
    std::cout << "  --x_size <number>        X size of the grid\n";
    std::cout << "  --y_size <number>        Y size of the grid\n";
    std::cout << "  --x_tile_size <number>   X tile size for CUDA\n";
    std::cout << "  --y_tile_size <number>   Y tile size for CUDA\n";
    std::cout << "  --rounds <number>        Number of rounds to run\n";
    std::cout << "  --warmup_rounds <number> Number of warmup rounds to run\n";
    std::cout << "  --steps <number>         Number of steps to run\n";
    std::cout << "  --precision <number>     Precision for floating-point calculations (32, 64)\n";
    std::cout << "  --seed <number>          Random seed for initialization\n";
    std::cout << "  --print                  Print the grid after each step\n";
    std::cout << "  --print_csv_header       Print CSV header\n";
    std::cout << "  --help                   Show this help message\n";
}


int main(int argc, char* argv[]) {
    
    auto params = get_params(argc, argv);

    if (params.help) {
        print_usage();
        return 0;
    }

    if (params.print_csv_header) {
        std::cout << cellib::run::experiment_report::csv_header() << std::endl;
        return 0;
    }

    if (params.print) {
        params.print_std();
    }

    namespace test = cellib::run::test_suites;

    using _game_of_life_ = game_of_life::config;
    using _fire_ = fire::config;
    using _wire_ = wire::config;
    using _greenberg_ = greenberg::config;

    #define cases_for(automaton) \
        test::on_cpu::standard<automaton>, \
        test::on_cpu::using_<std::uint32_t>::bit_array<automaton>, \
        test::on_cpu::using_<std::uint32_t>::bit_plates<automaton>, \
        test::on_cuda::standard<automaton>, \
        test::on_cuda::standard<automaton>::with_spacial_blocking<1, 1>, \
        test::on_cuda::standard<automaton>::with_spacial_blocking<2, 1>, \
        test::on_cuda::standard<automaton>::with_spacial_blocking<4, 1>, \
        test::on_cuda::using_<std::uint32_t>::bit_array<automaton>, \
        test::on_cuda::using_<std::uint64_t>::bit_array<automaton>, \
        test::on_cuda::using_<std::uint32_t>::bit_plates<automaton>, \
        test::on_cuda::using_<std::uint64_t>::bit_plates<automaton>

    switch_<
        cases_for(_game_of_life_),
        cases_for(_fire_),
        cases_for(_wire_),
        cases_for(_greenberg_)
    >::run(params);

    return 0;
}
