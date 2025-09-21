#include "game_of_life/runner.hpp"

#include "Halide.h"

#include "common/runner_base.hpp"

#include "../../game_of_life/algorithm.hpp"

namespace halide::game_of_life {

namespace {

class game_of_life_runner final : public common::runner_base {
public:
    void build_pipeline(const cellato::run::run_params&) override {
        using Halide::Expr;
        auto clamped = clamped_grid();
        auto& out = result();

        const Expr alive_value = Halide::cast<int>(common::to_int(::game_of_life::gol_cell_state::alive));
        const Expr dead_value = Halide::cast<int>(common::to_int(::game_of_life::gol_cell_state::dead));

        auto is_alive = [&](const Expr& value) {
            return Halide::cast<int>(value == alive_value);
        };

        Expr live_neighbors =
            is_alive(clamped(x - 1, y - 1)) + is_alive(clamped(x, y - 1)) + is_alive(clamped(x + 1, y - 1)) +
            is_alive(clamped(x - 1, y)) + is_alive(clamped(x + 1, y)) +
            is_alive(clamped(x - 1, y + 1)) + is_alive(clamped(x, y + 1)) + is_alive(clamped(x + 1, y + 1));

        Expr current = clamped(x, y);
        Expr stays_alive = live_neighbors == 2 || live_neighbors == 3;
        Expr becomes_alive = live_neighbors == 3;

        Expr next_state = Halide::select(current == alive_value,
                                         Halide::select(stays_alive, alive_value, dead_value),
                                         Halide::select(becomes_alive, alive_value, dead_value));

        out(x, y) = Halide::select(is_border(), clamped(x, y), next_state);
    }
};

} // namespace

std::unique_ptr<real_runner> create_runner() {
    return std::make_unique<game_of_life_runner>();
}

} // namespace halide::game_of_life
