#include "critters/runner.hpp"

#include "Kokkos_Core.hpp"

#include "../../critters/algorithm.hpp"
#include "detail/view_runner_base.hpp"

#include <cstdint>

namespace kokkos::critters {

namespace {

struct critters_runner_impl : public detail::view_runner_base<real_runner, critters_runner_impl, std::uint8_t> {
    using runner_base = detail::view_runner_base<real_runner, critters_runner_impl, std::uint8_t>;
    using value_type = typename runner_base::value_type;
    using cell_state = ::critters::critters_cell_state;

    static constexpr const char* cpu_label() { return "CrittersStepCPU"; }
    static constexpr const char* cuda_label() { return "CrittersStepCUDA"; }

    KOKKOS_INLINE_FUNCTION static void block_offsets(int step_parity, int coord_parity, int (&offsets)[2]) {
        if (step_parity == 0) {
            if (coord_parity == 0) {
                offsets[0] = 0;
                offsets[1] = 1;
            } else {
                offsets[0] = -1;
                offsets[1] = 0;
            }
        } else {
            if (coord_parity == 0) {
                offsets[0] = -1;
                offsets[1] = 0;
            } else {
                offsets[0] = 0;
                offsets[1] = 1;
            }
        }
    }

    template <typename ViewType>
    KOKKOS_INLINE_FUNCTION static value_type apply_rule(const ViewType& grid, int i, int j, int step) {
        const auto current = static_cast<cell_state>(grid(i, j));

        const int step_parity = step & 1;
        const int x_parity = i & 1;
        const int y_parity = j & 1;

        int x_offsets[2];
        int y_offsets[2];
        block_offsets(step_parity, x_parity, x_offsets);
        block_offsets(step_parity, y_parity, y_offsets);

        int alive_count = 0;
        for (int dx = 0; dx < 2; ++dx) {
            for (int dy = 0; dy < 2; ++dy) {
                const int neighbor_x = i + x_offsets[dx];
                const int neighbor_y = j + y_offsets[dy];
                if (static_cast<cell_state>(grid(neighbor_x, neighbor_y)) == cell_state::alive) {
                    ++alive_count;
                }
            }
        }

        cell_state next = current;
        if (alive_count == 2) {
            next = current;
        } else {
            next = (current == cell_state::alive) ? cell_state::dead : cell_state::alive;
        }

        return static_cast<value_type>(next);
    }
};

} // namespace

std::unique_ptr<real_runner> create_runner() {
    return std::make_unique<critters_runner_impl>();
}

} // namespace kokkos::critters
