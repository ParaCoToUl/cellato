#include "cyclic/runner.hpp"

#include "Kokkos_Core.hpp"

#include "../../cyclic/algorithm.hpp"
#include "detail/view_runner_base.hpp"

namespace kokkos::cyclic {

namespace {

struct cyclic_runner_impl : public detail::view_runner_base<real_runner, cyclic_runner_impl, int> {
    using runner_base = detail::view_runner_base<real_runner, cyclic_runner_impl, int>;
    using value_type = typename runner_base::value_type;

    static constexpr const char* cpu_label() { return "CyclicStepCPU"; }
    static constexpr const char* cuda_label() { return "CyclicStepCUDA"; }

    template <typename ViewType>
    KOKKOS_INLINE_FUNCTION static value_type apply_rule(const ViewType& grid, int i, int j, int /*step*/) {
        constexpr int states = ::cyclic::STATES;
        const int current = grid(i, j);
        const int target = (current + 1) % states;

        bool has_target_neighbor = false;
        for (int di = -1; di <= 1 && !has_target_neighbor; ++di) {
            for (int dj = -1; dj <= 1; ++dj) {
                if (di == 0 && dj == 0) {
                    continue;
                }
                if (grid(i + di, j + dj) == target) {
                    has_target_neighbor = true;
                    break;
                }
            }
        }

        return has_target_neighbor ? target : current;
    }
};

} // namespace

std::unique_ptr<real_runner> create_runner() {
    return std::make_unique<cyclic_runner_impl>();
}

} // namespace kokkos::cyclic
