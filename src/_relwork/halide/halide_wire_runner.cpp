#include "wire/runner.hpp"

#include "Halide.h"

#include "common/runner_base.hpp"

#include "../../wire/algorithm.hpp"

namespace halide::wire {

namespace {

class wire_runner final : public common::runner_base {
public:
    void build_pipeline(const cellato::run::run_params&) override {
        using Halide::Expr;
        auto clamped = clamped_grid();
        auto& out = result();

        const Expr empty_value = Halide::cast<int>(common::to_int(::wire::wire_cell_state::empty));
        const Expr head_value = Halide::cast<int>(common::to_int(::wire::wire_cell_state::electron_head));
        const Expr tail_value = Halide::cast<int>(common::to_int(::wire::wire_cell_state::electron_tail));
        const Expr conductor_value = Halide::cast<int>(common::to_int(::wire::wire_cell_state::conductor));

        auto is_head = [&](const Expr& value) {
            return Halide::cast<int>(value == head_value);
        };

        Expr head_neighbors =
            is_head(clamped(x - 1, y - 1)) + is_head(clamped(x, y - 1)) + is_head(clamped(x + 1, y - 1)) +
            is_head(clamped(x - 1, y)) + is_head(clamped(x + 1, y)) +
            is_head(clamped(x - 1, y + 1)) + is_head(clamped(x, y + 1)) + is_head(clamped(x + 1, y + 1));

        Expr current = clamped(x, y);

        Expr conductor_update = Halide::select((head_neighbors == 1) || (head_neighbors == 2), head_value, conductor_value);

        Expr next_state =
            Halide::select(current == head_value,
                           tail_value,
                           Halide::select(current == tail_value,
                                          conductor_value,
                                          Halide::select(current == conductor_value,
                                                         conductor_update,
                                                         empty_value)));

        out(x, y) = Halide::select(is_border(), clamped(x, y), next_state);
    }
};

} // namespace

std::unique_ptr<real_runner> create_runner() {
    return std::make_unique<wire_runner>();
}

} // namespace halide::wire
