#include "excitable/runner.hpp"

#include "Halide.h"

#include "common/runner_base.hpp"

#include "../../excitable/algorithm.hpp"

namespace halide::excitable {

namespace {

class excitable_runner final : public common::runner_base {
public:
    void build_pipeline(const cellato::run::run_params&) override {
        using Halide::Expr;
        auto clamped = clamped_grid();
        auto& out = result();

        const Expr quiescent = Halide::cast<int>(common::to_int(::excitable::ghm_cell_state::quiescent));
        const Expr excited = Halide::cast<int>(common::to_int(::excitable::ghm_cell_state::excited));
        const Expr refractory_1 = Halide::cast<int>(common::to_int(::excitable::ghm_cell_state::refractory_1));
        const Expr refractory_2 = Halide::cast<int>(common::to_int(::excitable::ghm_cell_state::refractory_2));
        const Expr refractory_3 = Halide::cast<int>(common::to_int(::excitable::ghm_cell_state::refractory_3));
        const Expr refractory_4 = Halide::cast<int>(common::to_int(::excitable::ghm_cell_state::refractory_4));
        const Expr refractory_5 = Halide::cast<int>(common::to_int(::excitable::ghm_cell_state::refractory_5));
        const Expr refractory_6 = Halide::cast<int>(common::to_int(::excitable::ghm_cell_state::refractory_6));

        auto is_excited = [&](const Expr& value) {
            return Halide::cast<int>(value == excited);
        };

        Expr excited_neighbors =
            is_excited(clamped(x - 1, y - 1)) + is_excited(clamped(x, y - 1)) + is_excited(clamped(x + 1, y - 1)) +
            is_excited(clamped(x - 1, y)) + is_excited(clamped(x + 1, y)) +
            is_excited(clamped(x - 1, y + 1)) + is_excited(clamped(x, y + 1)) + is_excited(clamped(x + 1, y + 1));

        Expr current = clamped(x, y);

        Expr next = quiescent;
        next = Halide::select(current == refractory_5, refractory_6, next);
        next = Halide::select(current == refractory_4, refractory_5, next);
        next = Halide::select(current == refractory_3, refractory_4, next);
        next = Halide::select(current == refractory_2, refractory_3, next);
        next = Halide::select(current == refractory_1, refractory_2, next);
        next = Halide::select(current == excited, refractory_1, next);

        Expr quiescent_update = Halide::select(excited_neighbors > 0, excited, quiescent);
        next = Halide::select(current == quiescent, quiescent_update, next);

        out(x, y) = Halide::select(is_border(), clamped(x, y), next);
    }
};

} // namespace

std::unique_ptr<real_runner> create_runner() {
    return std::make_unique<excitable_runner>();
}

} // namespace halide::excitable
