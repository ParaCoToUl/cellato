#ifndef CELLIB_TRAVERSERS_CPU_SIMPLE_HPP
#define CELLIB_TRAVERSERS_CPU_SIMPLE_HPP

#include <iostream>
#include <thread>
#include <chrono>
#include <utility>

#include "../../memory/interface.hpp"

namespace cellib::traversers::cpu::simple {

template <
    typename evaluator_type,
    typename grid_type >
class traverser {
    using evaluator_t = evaluator_type;
    using grid_t = grid_type;

    using cell_t = typename grid_t::cell_t;
    using state_t = cellib::memory::grids::point_in_grid<cell_t>;

  public:

    void init(grid_t grid) {
        _input_grid = std::move(grid);
        _intermediate_grid = _input_grid;
        _final_grid = &_intermediate_grid;
    }

    template <bool print = false>
    void run(int steps) {

        auto current = _input_grid.data();
        auto next = _intermediate_grid.data();

        state_t state;
        state.properties.x_size = _input_grid.x_size_physical();
        state.properties.y_size = _input_grid.y_size_physical();

        for (int step = 0; step < steps; ++step) {

            state.grid = current;

            // Process cells (skip border)
            for (std::size_t y = 1; y < state.properties.y_size - 1; ++y) {
                for (std::size_t x = 1; x < state.properties.x_size - 1; ++x) {

                    state.position.x = x;
                    state.position.y = y;

                    next[state.idx()] =
                        evaluator_t::evaluate(state);
                }
            }

            if constexpr (print) {
                std::cout << "Step " << step + 1 << ":\n";
                print_to_stdout(step % 2 == 0 ? _input_grid : _intermediate_grid);
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }

            std::swap(current, next);
        }

        if (steps % 2 == 1) {
            _final_grid = &_intermediate_grid;
        } else {
            _final_grid = &_input_grid;
        }
    }

    grid_t fetch_result() const {
        return std::move(*_final_grid);
    }

    void set_print_config(cellib::memory::grids::standard::print_config<cell_t> config) {
        _print_config = std::move(config);
    }

    private:

    grid_t _input_grid, _intermediate_grid;
    grid_t* _final_grid;

    cellib::memory::grids::standard::print_config<cell_t> _print_config;

    void print_to_stdout(const grid_t& grid) const {
        if constexpr (grid_t::HAS_OWN_PRINT) {
            grid.print(std::cout, _print_config);
        }
        else {
            auto standard_grid = grid.to_standard();
            standard_grid.print(std::cout, _print_config);
        }
    }
};

}

#endif // CELLIB_TRAVERSERS_CPU_SIMPLE_HPP