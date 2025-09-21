#include "traffic/runner.hpp"

#include "Halide.h"

#include "common/runner_base.hpp"

#include "../../traffic/algorithm.hpp"

namespace halide::traffic {

namespace {

class traffic_runner final : public common::runner_base {
public:
    void build_pipeline(const cellato::run::run_params&) override {
        using Halide::Expr;
        auto clamped = clamped_grid();
        auto& out = result();

        const Expr empty_value = Halide::cast<int>(common::to_int(::traffic::traffic_cell_state::empty));
        const Expr red_value = Halide::cast<int>(common::to_int(::traffic::traffic_cell_state::red_car));
        const Expr blue_value = Halide::cast<int>(common::to_int(::traffic::traffic_cell_state::blue_car));

        auto rule = [&](const Expr& incoming, const Expr& current, const Expr& outgoing,
                        const Expr& movable, const Expr& stationary) {
            Expr is_movable = current == movable;
            Expr is_empty = current == empty_value;
            Expr outgoing_empty = outgoing == empty_value;
            Expr incoming_movable = incoming == movable;

            Expr move_out = Halide::select(outgoing_empty, empty_value, movable);
            Expr move_in = Halide::select(incoming_movable, movable, empty_value);

            return Halide::select(is_movable,
                                  move_out,
                                  Halide::select(is_empty, move_in, stationary));
        };

        Expr current = clamped(x, y);

        Expr horizontal_update = rule(clamped(x - 1, y), current, clamped(x + 1, y), red_value, blue_value);
        Expr vertical_update = rule(clamped(x, y - 1), current, clamped(x, y + 1), blue_value, red_value);

        Expr next_state = Halide::select((step_phase_ & 1) == 0, horizontal_update, vertical_update);

        out(x, y) = Halide::select(is_border(), clamped(x, y), next_state);
    }

    void update_step_state(int step) override {
        step_phase_.set(step & 1);
    }

private:
    Halide::Param<int> step_phase_{"step_phase"};
};

} // namespace

std::unique_ptr<real_runner> create_runner() {
    return std::make_unique<traffic_runner>();
}

} // namespace halide::traffic
