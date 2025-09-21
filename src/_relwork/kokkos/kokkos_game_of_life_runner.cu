#include "game_of_life/runner.hpp"

#include "Kokkos_Core.hpp"

#include "detail/view_runner_base.hpp"

#include <cstdint>
#include <stdexcept>
#include <utility>
#include <vector>

namespace kokkos::game_of_life {

namespace {

struct game_of_life_runner : public detail::view_runner_base<real_runner, game_of_life_runner, std::uint8_t> {
    using runner_base = detail::view_runner_base<real_runner, game_of_life_runner, std::uint8_t>;
    using value_type = typename runner_base::value_type;

    static constexpr const char* cpu_label() { return "GoLStepCPU"; }
    static constexpr const char* cuda_label() { return "GoLStepCUDA"; }

    template <typename ViewType>
    KOKKOS_INLINE_FUNCTION static value_type apply_rule(const ViewType& grid, int i, int j, int /*step*/) {
        const auto neighbors = grid(i - 1, j - 1) + grid(i - 1, j) + grid(i - 1, j + 1) +
                               grid(i, j - 1) + grid(i, j + 1) +
                               grid(i + 1, j - 1) + grid(i + 1, j) + grid(i + 1, j + 1);

        const bool alive = grid(i, j);
        const bool survives = neighbors == 2 || neighbors == 3;
        return alive ? (survives ? 1 : 0) : (neighbors == 3 ? 1 : 0);
    }
};

} // namespace

std::unique_ptr<real_runner> create_runner() {
    return std::make_unique<game_of_life_runner>();
}

} // namespace kokkos::game_of_life
