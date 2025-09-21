#include "wire/runner.hpp"

#include "Kokkos_Core.hpp"

#include "../../wire/algorithm.hpp"
#include "detail/view_runner_base.hpp"

#include <cstdint>

namespace kokkos::wire {

namespace {

struct wire_runner_impl : public detail::view_runner_base<real_runner, wire_runner_impl, std::uint8_t> {
    using runner_base = detail::view_runner_base<real_runner, wire_runner_impl, std::uint8_t>;
    using value_type = typename runner_base::value_type;
    using cell_state = ::wire::wire_cell_state;

    static constexpr const char* cpu_label() { return "WireStepCPU"; }
    static constexpr const char* cuda_label() { return "WireStepCUDA"; }

    template <typename ViewType>
    KOKKOS_INLINE_FUNCTION static int electron_head_count(const ViewType& grid, int i, int j) {
        int count = 0;
        for (int di = -1; di <= 1; ++di) {
            for (int dj = -1; dj <= 1; ++dj) {
                if (di == 0 && dj == 0) {
                    continue;
                }
                if (static_cast<cell_state>(grid(i + di, j + dj)) == cell_state::electron_head) {
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

        switch (current) {
        case cell_state::empty:
            next = cell_state::empty;
            break;
        case cell_state::electron_head:
            next = cell_state::electron_tail;
            break;
        case cell_state::electron_tail:
            next = cell_state::conductor;
            break;
        case cell_state::conductor: {
            const int heads = electron_head_count(grid, i, j);
            next = (heads == 1 || heads == 2) ? cell_state::electron_head : cell_state::conductor;
            break;
        }
        }

        return static_cast<value_type>(next);
    }
};

} // namespace

std::unique_ptr<real_runner> create_runner() {
    return std::make_unique<wire_runner_impl>();
}

} // namespace kokkos::wire
