#ifndef RUN_PARAMS_HPP
#define RUN_PARAMS_HPP

#include <string>
#include <cstddef>
#include <iostream>

namespace cellib::run {

struct run_params {
    std::string automaton = "game-of-life";

    std::string device = "CPU";
    std::string traverser = "standard";
    std::string evaluator = "standard";
    std::string layout = "standard";
    
    int x_size = 0;
    int y_size = 0;
    int steps = 0;

    int precision = 0;

    int x_tile_size = 0;
    int y_tile_size = 0;

    bool print = false;
    bool help = false;

    void print_to(std::ostream& os) {
        os << "Run Parameters:\n";
        os << "  Automaton: " << automaton << "\n";
        os << "  Device: " << device << "\n";
        os << "  Traverser: " << traverser << "\n";
        os << "  Evaluator: " << evaluator << "\n";
        os << "  Layout: " << layout << "\n";
        os << "  Grid Size: (" << x_size << ", " << y_size << ")\n";
        os << "  Steps: " << steps << "\n";
        os << "  Print: " << (print ? "true" : "false") << "\n";
    }

    void print_std() {
        print_to(std::cout);
    }
};

}

#endif // RUN_PARAMS_HPP