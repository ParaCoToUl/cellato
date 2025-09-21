#include "critters/runner.hpp"

#include "Halide.h"

#include "common/runner_base.hpp"

#include "../../critters/algorithm.hpp"

namespace halide::critters {

namespace {

class critters_runner final : public common::runner_base {
public:
    void build_pipeline(const cellato::run::run_params&) override {
        using Halide::Expr;
        auto clamped = clamped_grid();
        auto& out = result();

        const Expr alive_value = Halide::cast<int>(common::to_int(::critters::critters_cell_state::alive));
        const Expr dead_value = Halide::cast<int>(common::to_int(::critters::critters_cell_state::dead));

        Expr parity_x = x & 1;
        Expr parity_y = y & 1;
        Expr step_parity_expr = step_parity_ & 1;

        Expr dx0 = Halide::select(parity_x == step_parity_expr, 0, -1);
        Expr dy0 = Halide::select(parity_y == step_parity_expr, 0, -1);
        Expr dx1 = dx0 + 1;
        Expr dy1 = dy0 + 1;

        auto is_alive = [&](const Expr& value) {
            return Halide::cast<int>(value == alive_value);
        };

        Expr alive_count =
            is_alive(clamped(x + dx0, y + dy0)) + is_alive(clamped(x + dx0, y + dy1)) +
            is_alive(clamped(x + dx1, y + dy0)) + is_alive(clamped(x + dx1, y + dy1));

        Expr current = clamped(x, y);
        Expr toggled = Halide::select(current == alive_value, dead_value, alive_value);
        Expr next_state = Halide::select(alive_count == 2, current, toggled);

        out(x, y) = Halide::select(is_border(), clamped(x, y), next_state);
    }

    void update_step_state(int step) override {
        step_parity_.set(step & 1);
    }

private:
    Halide::Param<int> step_parity_{"step_parity"};
};

} // namespace

std::unique_ptr<real_runner> create_runner() {
    return std::make_unique<critters_runner>();
}

} // namespace halide::critters
