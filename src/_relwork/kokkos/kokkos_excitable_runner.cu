#include "excitable/runner.hpp"

#include "Kokkos_Core.hpp"

#include "../../excitable/algorithm.hpp"
#include "detail/view_runner_base.hpp"

#include <cstdint>

namespace kokkos::excitable {

namespace {

struct excitable_runner_impl : public detail::view_runner_base<real_runner, excitable_runner_impl, std::uint8_t> {
    using runner_base = detail::view_runner_base<real_runner, excitable_runner_impl, std::uint8_t>;
    using value_type = typename runner_base::value_type;
    using cell_state = ::excitable::ghm_cell_state;

    static constexpr const char* cpu_label() { return "GreenbergStepCPU"; }
    static constexpr const char* cuda_label() { return "GreenbergStepCUDA"; }

    template <typename ViewType>
    KOKKOS_INLINE_FUNCTION static int count_excited_neighbors(const ViewType& grid, int i, int j) {
        int count = 0;
        for (int di = -1; di <= 1; ++di) {
            for (int dj = -1; dj <= 1; ++dj) {
                if (di == 0 && dj == 0) {
                    continue;
                }
                if (static_cast<cell_state>(grid(i + di, j + dj)) == cell_state::excited) {
                    ++count;
                }
            }
        }
        return count;
    }

    KOKKOS_INLINE_FUNCTION static cell_state advance_state(cell_state current) {
        switch (current) {
        case cell_state::excited:
            return cell_state::refractory_1;
        case cell_state::refractory_1:
            return cell_state::refractory_2;
        case cell_state::refractory_2:
            return cell_state::refractory_3;
        case cell_state::refractory_3:
            return cell_state::refractory_4;
        case cell_state::refractory_4:
            return cell_state::refractory_5;
        case cell_state::refractory_5:
            return cell_state::refractory_6;
        case cell_state::refractory_6:
            return cell_state::quiescent;
        case cell_state::quiescent:
        default:
            return cell_state::quiescent;
        }
    }

    template <typename ViewType>
    KOKKOS_INLINE_FUNCTION static value_type apply_rule(const ViewType& grid, int i, int j, int /*step*/) {
        const auto current = static_cast<cell_state>(grid(i, j));
        auto next = current;

        if (current == cell_state::quiescent) {
            next = count_excited_neighbors(grid, i, j) > 0 ? cell_state::excited : cell_state::quiescent;
        } else {
            next = advance_state(current);
        }

        return static_cast<value_type>(next);
    }
};

} // namespace

std::unique_ptr<real_runner> create_runner() {
    return std::make_unique<excitable_runner_impl>();
}

} // namespace kokkos::excitable
