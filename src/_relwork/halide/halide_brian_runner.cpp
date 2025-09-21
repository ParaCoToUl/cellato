#include "brian/runner.hpp"

#include "Halide.h"

#include "common/runner_base.hpp"

#include "../../brian/algorithm.hpp"

namespace halide::brian {

namespace {

class brian_runner final : public common::runner_base {
public:
    void build_pipeline(const cellato::run::run_params&) override {
        using Halide::Expr;
        auto clamped = clamped_grid();
        auto& out = result();

        const Expr dead_value = Halide::cast<int>(common::to_int(::brian::brian_cell_state::dead));
        const Expr dying_value = Halide::cast<int>(common::to_int(::brian::brian_cell_state::dying));
        const Expr alive_value = Halide::cast<int>(common::to_int(::brian::brian_cell_state::alive));

        auto is_alive = [&](const Expr& value) {
            return Halide::cast<int>(value == alive_value);
        };

        Expr alive_neighbors =
            is_alive(clamped(x - 1, y - 1)) + is_alive(clamped(x, y - 1)) + is_alive(clamped(x + 1, y - 1)) +
            is_alive(clamped(x - 1, y)) + is_alive(clamped(x + 1, y)) +
            is_alive(clamped(x - 1, y + 1)) + is_alive(clamped(x, y + 1)) + is_alive(clamped(x + 1, y + 1));

        Expr current = clamped(x, y);

        Expr next_state = Halide::select(current == dead_value,
                                         Halide::select(alive_neighbors == 2, alive_value, dead_value),
                                         Halide::select(current == alive_value, dying_value, dead_value));

        out(x, y) = Halide::select(is_border(), clamped(x, y), next_state);
    }
};

} // namespace

std::unique_ptr<real_runner> create_runner() {
    return std::make_unique<brian_runner>();
}

} // namespace halide::brian
