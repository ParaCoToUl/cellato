#ifndef HALIDE_FIRE_RUNNER_HPP
#define HALIDE_FIRE_RUNNER_HPP

#include <memory>
#include <stdexcept>
#include <vector>

#include "experiments/run_params.hpp"

#include "../common/runner_interface.hpp"

#ifndef ENABLE_HALIDE
#error "Halide is not enabled, this source file should not be compiled."
#endif // ENABLE_HALIDE

namespace halide::fire {

using real_runner = common::real_runner_interface;

std::unique_ptr<real_runner> create_runner();

struct runner {
    using value_type = real_runner::value_type;

    void init(int* grid,
              const cellato::run::run_params& params) {
        if (!real_runner_) {
            real_runner_ = create_runner();
        }

        if (!real_runner_) {
            throw std::runtime_error("Failed to create real_runner instance");
        }

        real_runner_->init(grid, params);
    }

    void run(int steps) {
        real_runner_->run(steps);
    }

    std::vector<int> fetch_result() {
        return real_runner_->fetch_result();
    }

private:
    std::unique_ptr<real_runner> real_runner_;
};

} // namespace halide::fire

#endif // HALIDE_FIRE_RUNNER_HPP
