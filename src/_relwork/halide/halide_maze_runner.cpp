#include "maze/runner.hpp"

#include "Halide.h"

#include "common/runner_base.hpp"

#include "../../maze/algorithm.hpp"

namespace halide::maze {

namespace {

class maze_runner final : public common::runner_base {
public:
    void build_pipeline(const cellato::run::run_params&) override {
        using Halide::Expr;
        auto clamped = clamped_grid();
        auto& out = result();

        const Expr empty_value = Halide::cast<int>(common::to_int(::maze::maze_cell_state::empty));
        const Expr wall_value = Halide::cast<int>(common::to_int(::maze::maze_cell_state::wall));

        auto is_wall = [&](const Expr& value) {
            return Halide::cast<int>(value == wall_value);
        };

        Expr wall_neighbors =
            is_wall(clamped(x - 1, y - 1)) + is_wall(clamped(x, y - 1)) + is_wall(clamped(x + 1, y - 1)) +
            is_wall(clamped(x - 1, y)) + is_wall(clamped(x + 1, y)) +
            is_wall(clamped(x - 1, y + 1)) + is_wall(clamped(x, y + 1)) + is_wall(clamped(x + 1, y + 1));

        Expr current = clamped(x, y);

        Expr next_state = Halide::select(wall_neighbors == 3,
                                         wall_value,
                                         Halide::select((current == wall_value) && (wall_neighbors < 6),
                                                        wall_value,
                                                        empty_value));

        out(x, y) = Halide::select(is_border(), clamped(x, y), next_state);
    }
};

} // namespace

std::unique_ptr<real_runner> create_runner() {
    return std::make_unique<maze_runner>();
}

} // namespace halide::maze
