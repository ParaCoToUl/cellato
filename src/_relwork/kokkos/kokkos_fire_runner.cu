#include "fire/runner.hpp"

#include "Kokkos_Core.hpp"

#include "../../fire/algorithm.hpp"
#include "detail/view_runner_base.hpp"

#include <cstdint>

namespace kokkos::fire {

namespace {

struct fire_runner_impl : public detail::view_runner_base<real_runner, fire_runner_impl, std::uint8_t> {
    using runner_base = detail::view_runner_base<real_runner, fire_runner_impl, std::uint8_t>;
    using value_type = typename runner_base::value_type;
    using cell_state = ::fire::fire_cell_state;

    static constexpr const char* cpu_label() { return "FireStepCPU"; }
    static constexpr const char* cuda_label() { return "FireStepCUDA"; }

    template <typename ViewType>
    KOKKOS_INLINE_FUNCTION static bool has_fire_neighbor(const ViewType& grid, int i, int j) {
        const auto north = static_cast<cell_state>(grid(i - 1, j));
        const auto south = static_cast<cell_state>(grid(i + 1, j));
        const auto west = static_cast<cell_state>(grid(i, j - 1));
        const auto east = static_cast<cell_state>(grid(i, j + 1));

        return north == cell_state::fire || south == cell_state::fire || west == cell_state::fire || east == cell_state::fire;
    }

    template <typename ViewType>
    KOKKOS_INLINE_FUNCTION static value_type apply_rule(const ViewType& grid, int i, int j, int /*step*/) {
        const auto current = static_cast<cell_state>(grid(i, j));
        auto next = current;

        switch (current) {
        case cell_state::empty:
            next = cell_state::empty;
            break;
        case cell_state::tree:
            next = has_fire_neighbor(grid, i, j) ? cell_state::fire : cell_state::tree;
            break;
        case cell_state::fire:
            next = cell_state::ash;
            break;
        case cell_state::ash:
            next = has_fire_neighbor(grid, i, j) ? cell_state::ash : cell_state::empty;
            break;
        }

        return static_cast<value_type>(next);
    }
};

} // namespace

std::unique_ptr<real_runner> create_runner() {
    return std::make_unique<fire_runner_impl>();
}

} // namespace kokkos::fire
