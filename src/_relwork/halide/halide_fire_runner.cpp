#include "fire/runner.hpp"

#include "Halide.h"

#include "common/runner_base.hpp"

#include "../../fire/algorithm.hpp"

namespace halide::fire {

namespace {

class fire_runner final : public common::runner_base {
public:
    void build_pipeline(const cellato::run::run_params&) override {
        using Halide::Expr;
        auto clamped = clamped_grid();
        auto& out = result();

        const Expr empty_value = Halide::cast<int>(common::to_int(::fire::fire_cell_state::empty));
        const Expr tree_value = Halide::cast<int>(common::to_int(::fire::fire_cell_state::tree));
        const Expr fire_value = Halide::cast<int>(common::to_int(::fire::fire_cell_state::fire));
        const Expr ash_value = Halide::cast<int>(common::to_int(::fire::fire_cell_state::ash));

        auto is_fire = [&](const Expr& value) {
            return Halide::cast<int>(value == fire_value);
        };

        Expr fire_neighbors =
            is_fire(clamped(x, y - 1)) + is_fire(clamped(x + 1, y)) +
            is_fire(clamped(x, y + 1)) + is_fire(clamped(x - 1, y));

        Expr has_fire_neighbor = fire_neighbors > 0;
        Expr current = clamped(x, y);

        Expr next_if_tree = Halide::select(has_fire_neighbor, fire_value, tree_value);
        Expr next_if_ash = Halide::select(has_fire_neighbor, ash_value, empty_value);

        Expr next_state =
            Halide::select(current == empty_value,
                           empty_value,
                           Halide::select(current == tree_value,
                                          next_if_tree,
                                          Halide::select(current == fire_value, ash_value, next_if_ash)));

        out(x, y) = Halide::select(is_border(), clamped(x, y), next_state);
    }
};

} // namespace

std::unique_ptr<real_runner> create_runner() {
    return std::make_unique<fire_runner>();
}

} // namespace halide::fire
