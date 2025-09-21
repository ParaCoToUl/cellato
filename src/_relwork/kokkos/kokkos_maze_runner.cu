#include "maze/runner.hpp"

#include "Kokkos_Core.hpp"

#include "../../maze/algorithm.hpp"
#include "detail/view_runner_base.hpp"

#include <cstdint>

namespace kokkos::maze {

namespace {

struct maze_runner_impl : public detail::view_runner_base<real_runner, maze_runner_impl, std::uint8_t> {
    using runner_base = detail::view_runner_base<real_runner, maze_runner_impl, std::uint8_t>;
    using value_type = typename runner_base::value_type;
    using cell_state = ::maze::maze_cell_state;

    static constexpr const char* cpu_label() { return "MazeStepCPU"; }
    static constexpr const char* cuda_label() { return "MazeStepCUDA"; }

    template <typename ViewType>
    KOKKOS_INLINE_FUNCTION static int wall_neighbors(const ViewType& grid, int i, int j) {
        int count = 0;
        for (int di = -1; di <= 1; ++di) {
            for (int dj = -1; dj <= 1; ++dj) {
                if (di == 0 && dj == 0) {
                    continue;
                }
                if (static_cast<cell_state>(grid(i + di, j + dj)) == cell_state::wall) {
                    ++count;
                }
            }
        }
        return count;
    }

    template <typename ViewType>
    KOKKOS_INLINE_FUNCTION static value_type apply_rule(const ViewType& grid, int i, int j, int /*step*/) {
        const auto current = static_cast<cell_state>(grid(i, j));
        const int walls = wall_neighbors(grid, i, j);

        cell_state next = cell_state::empty;
        if (walls == 3) {
            next = cell_state::wall;
        } else if (current == cell_state::wall && walls < 6) {
            next = cell_state::wall;
        } else {
            next = cell_state::empty;
        }

        return static_cast<value_type>(next);
    }
};

} // namespace

std::unique_ptr<real_runner> create_runner() {
    return std::make_unique<maze_runner_impl>();
}

} // namespace kokkos::maze
