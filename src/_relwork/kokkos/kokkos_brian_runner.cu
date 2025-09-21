#include "brian/runner.hpp"

#include "Kokkos_Core.hpp"

#include "../../brian/algorithm.hpp"
#include "detail/view_runner_base.hpp"

#include <cstdint>

namespace kokkos::brian {

namespace {

struct brian_runner_impl : public detail::view_runner_base<real_runner, brian_runner_impl, std::uint8_t> {
    using runner_base = detail::view_runner_base<real_runner, brian_runner_impl, std::uint8_t>;
    using value_type = typename runner_base::value_type;
    using cell_state = ::brian::brian_cell_state;

    static constexpr const char* cpu_label() { return "BrianStepCPU"; }
    static constexpr const char* cuda_label() { return "BrianStepCUDA"; }

    template <typename ViewType>
    KOKKOS_INLINE_FUNCTION static int alive_neighbors(const ViewType& grid, int i, int j) {
        int count = 0;
        for (int di = -1; di <= 1; ++di) {
            for (int dj = -1; dj <= 1; ++dj) {
                if (di == 0 && dj == 0) {
                    continue;
                }
                if (static_cast<cell_state>(grid(i + di, j + dj)) == cell_state::alive) {
                    ++count;
                }
            }
        }
        return count;
    }

    template <typename ViewType>
    KOKKOS_INLINE_FUNCTION static value_type apply_rule(const ViewType& grid, int i, int j, int /*step*/) {
        const auto current = static_cast<cell_state>(grid(i, j));
        auto next = current;
        const int neighbors = alive_neighbors(grid, i, j);

        if (current == cell_state::dead) {
            next = (neighbors == 2) ? cell_state::alive : cell_state::dead;
        } else if (current == cell_state::alive) {
            next = cell_state::dying;
        } else {
            next = cell_state::dead;
        }

        return static_cast<value_type>(next);
    }
};

} // namespace

std::unique_ptr<real_runner> create_runner() {
    return std::make_unique<brian_runner_impl>();
}

} // namespace kokkos::brian
