#ifndef HALIDE_COMMON_RUNNER_INTERFACE_HPP
#define HALIDE_COMMON_RUNNER_INTERFACE_HPP

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <vector>

#include "experiments/run_params.hpp"

namespace halide::common {

struct real_runner_interface {
    using value_type = std::int32_t;

    virtual ~real_runner_interface() = default;

    virtual void init(int* grid,
                      const cellato::run::run_params& params) = 0;
    virtual void run(int steps) = 0;
    virtual std::vector<int> fetch_result() = 0;
};

} // namespace halide::common

#endif // HALIDE_COMMON_RUNNER_INTERFACE_HPP
