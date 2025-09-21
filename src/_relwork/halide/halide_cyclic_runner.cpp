#include "cyclic/runner.hpp"

#include "Halide.h"

#include "common/runner_base.hpp"

#include "../../cyclic/algorithm.hpp"

namespace halide::cyclic {

namespace {

class cyclic_runner final : public common::runner_base {
public:
    void build_pipeline(const cellato::run::run_params&) override {
        using Halide::Expr;
        auto clamped = clamped_grid();
        auto& out = result();

        const int states = ::cyclic::STATES;

        auto is_target = [&](const Expr& value, const Expr& target) {
            return Halide::cast<int>(value == target);
        };

        Expr current = clamped(x, y);
        Expr target_state = (current + 1) % states;

        Expr neighbors =
            is_target(clamped(x - 1, y - 1), target_state) + is_target(clamped(x, y - 1), target_state) + is_target(clamped(x + 1, y - 1), target_state) +
            is_target(clamped(x - 1, y), target_state) + is_target(clamped(x + 1, y), target_state) +
            is_target(clamped(x - 1, y + 1), target_state) + is_target(clamped(x, y + 1), target_state) + is_target(clamped(x + 1, y + 1), target_state);

        Expr next_state = Halide::select(neighbors > 0, target_state, current);

        out(x, y) = Halide::select(is_border(), clamped(x, y), next_state);
    }
};

} // namespace

std::unique_ptr<real_runner> create_runner() {
    return std::make_unique<cyclic_runner>();
}

} // namespace halide::cyclic
