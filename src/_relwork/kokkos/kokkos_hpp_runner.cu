#include "fluid/runner.hpp"

#include "Kokkos_Core.hpp"

#include "../../fluid/algorithm.hpp"
#include "detail/view_runner_base.hpp"

#include <cstdint>

namespace kokkos::fluid {

namespace {

struct hpp_runner_impl : public detail::view_runner_base<real_runner, hpp_runner_impl, std::uint8_t> {
    using runner_base = detail::view_runner_base<real_runner, hpp_runner_impl, std::uint8_t>;
    using value_type = typename runner_base::value_type;

    static constexpr const char* cpu_label() { return "HPPStepCPU"; }
    static constexpr const char* cuda_label() { return "HPPStepCUDA"; }

    template <typename ViewType>
    KOKKOS_INLINE_FUNCTION static value_type apply_rule(const ViewType& grid, int i, int j, int /*step*/) {
        const int north = grid(i, j - 1);
        const int south = grid(i, j + 1);
        const int west = grid(i - 1, j);
        const int east = grid(i + 1, j);

        const bool incoming_from_top = (north & ::fluid::TOP) != 0;
        const bool incoming_from_bottom = (south & ::fluid::BOTTOM) != 0;
        const bool incoming_from_left = (west & ::fluid::LEFT) != 0;
        const bool incoming_from_right = (east & ::fluid::RIGHT) != 0;

        const bool vertical_collision = incoming_from_top && incoming_from_bottom;
        const bool horizontal_collision = incoming_from_left && incoming_from_right;

        const bool just_vertical_collision = vertical_collision && !(incoming_from_left || incoming_from_right);
        const bool just_horizontal_collision = horizontal_collision && !(incoming_from_top || incoming_from_bottom);

        const int combined_vertical_incoming = (north & ::fluid::TOP) | (south & ::fluid::BOTTOM);
        const int combined_horizontal_incoming = (west & ::fluid::LEFT) | (east & ::fluid::RIGHT);

        const int vertical_result = just_vertical_collision ? (::fluid::LEFT | ::fluid::RIGHT) : combined_vertical_incoming;
        const int horizontal_result = just_horizontal_collision ? (::fluid::TOP | ::fluid::BOTTOM) : combined_horizontal_incoming;

        return static_cast<value_type>(vertical_result | horizontal_result);
    }
};

} // namespace

std::unique_ptr<real_runner> create_runner() {
    return std::make_unique<hpp_runner_impl>();
}

} // namespace kokkos::fluid
