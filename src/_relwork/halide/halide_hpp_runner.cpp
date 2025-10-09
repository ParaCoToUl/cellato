#include "fluid/runner.hpp"

#include "Halide.h"

#include "common/runner_base.hpp"

#include "../../fluid/algorithm.hpp"

namespace halide::fluid {

namespace {

class hpp_runner final : public common::runner_base {
public:
    void build_pipeline(const cellato::run::run_params&) override {
        using Halide::Expr;
        auto clamped = clamped_grid();
        auto& out = result();

        const Expr TOP = Halide::cast<int>(::fluid::TOP);
        const Expr BOTTOM = Halide::cast<int>(::fluid::BOTTOM);
        const Expr LEFT = Halide::cast<int>(::fluid::LEFT);
        const Expr RIGHT = Halide::cast<int>(::fluid::RIGHT);

        Expr top_neighbor = clamped(x, y - 1);
        Expr bottom_neighbor = clamped(x, y + 1);
        Expr left_neighbor = clamped(x - 1, y);
        Expr right_neighbor = clamped(x + 1, y);

        Expr incoming_from_top = top_neighbor & TOP;
        Expr incoming_from_bottom = bottom_neighbor & BOTTOM;
        Expr incoming_from_left = left_neighbor & LEFT;
        Expr incoming_from_right = right_neighbor & RIGHT;

        Expr has_top = incoming_from_top != 0;
        Expr has_bottom = incoming_from_bottom != 0;
        Expr has_left = incoming_from_left != 0;
        Expr has_right = incoming_from_right != 0;

        Expr vertical_collision = has_top && has_bottom;
        Expr horizontal_collision = has_left && has_right;

        Expr just_vertical_collision = vertical_collision && !(has_left || has_right);
        Expr just_horizontal_collision = horizontal_collision && !(has_top || has_bottom);

        Expr pass_vertical = incoming_from_top | incoming_from_bottom;
        Expr pass_horizontal = incoming_from_left | incoming_from_right;

        Expr vertical_result = Halide::select(just_vertical_collision,
                                              LEFT | RIGHT,
                                              pass_vertical);

        Expr horizontal_result = Halide::select(just_horizontal_collision,
                                                TOP | BOTTOM,
                                                pass_horizontal);

        Expr next_state = vertical_result | horizontal_result;

        out(x, y) = Halide::select(is_border(), clamped(x, y), next_state);
    }
};

} // namespace

std::unique_ptr<real_runner> create_runner() {
    return std::make_unique<hpp_runner>();
}

} // namespace halide::fluid
