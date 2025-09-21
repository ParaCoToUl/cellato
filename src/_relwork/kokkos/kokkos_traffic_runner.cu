#include "traffic/runner.hpp"

#include "Kokkos_Core.hpp"

#include "../../traffic/algorithm.hpp"
#include "detail/view_runner_base.hpp"

#include <cstdint>

namespace kokkos::traffic {

namespace {

struct traffic_runner_impl : public detail::view_runner_base<real_runner, traffic_runner_impl, std::uint8_t> {
    using runner_base = detail::view_runner_base<real_runner, traffic_runner_impl, std::uint8_t>;
    using value_type = typename runner_base::value_type;
    using cell_state = ::traffic::traffic_cell_state;

    static constexpr const char* cpu_label() { return "TrafficStepCPU"; }
    static constexpr const char* cuda_label() { return "TrafficStepCUDA"; }

    template <typename ViewType>
    KOKKOS_INLINE_FUNCTION static value_type apply_rule(const ViewType& grid, int i, int j, int step) {
        const auto current = static_cast<cell_state>(grid(i, j));
        auto next = current;
        const int step_parity = step & 1;

        if (step_parity == 0) {
            const auto outgoing = static_cast<cell_state>(grid(i + 1, j));
            const auto incoming = static_cast<cell_state>(grid(i - 1, j));

            if (current == cell_state::red_car) {
                next = (outgoing == cell_state::empty) ? cell_state::empty : cell_state::red_car;
            } else if (current == cell_state::empty) {
                next = (incoming == cell_state::red_car) ? cell_state::red_car : cell_state::empty;
            }
        } else {
            const auto outgoing = static_cast<cell_state>(grid(i, j + 1));
            const auto incoming = static_cast<cell_state>(grid(i, j - 1));

            if (current == cell_state::blue_car) {
                next = (outgoing == cell_state::empty) ? cell_state::empty : cell_state::blue_car;
            } else if (current == cell_state::empty) {
                next = (incoming == cell_state::blue_car) ? cell_state::blue_car : cell_state::empty;
            }
        }

        return static_cast<value_type>(next);
    }
};

} // namespace

std::unique_ptr<real_runner> create_runner() {
    return std::make_unique<traffic_runner_impl>();
}

} // namespace kokkos::traffic
