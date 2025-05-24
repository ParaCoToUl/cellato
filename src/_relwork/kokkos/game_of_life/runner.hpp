#ifndef KOKKOS_GAME_OF_LIFE_RUNNER_HPP
#define KOKKOS_GAME_OF_LIFE_RUNNER_HPP

#include <cstddef>
#include <cstdint>

#include <vector>

#include "Kokkos_Core.hpp"

#ifndef ENABLE_KOKKOS
#error "Kokkos is not enabled, this source file should not be compiled."
#endif // ENABLE_KOKKOS

namespace kokkos::game_of_life {

struct runner {
    using value_type = std::uint8_t;

    void init(int* grid, std::size_t x_size, std::size_t y_size) {
        grid_ = Kokkos::View<value_type**>("grid", x_size, y_size);
        next_grid_ = Kokkos::View<value_type**>("next_grid", x_size, y_size);
        // Initialize the Kokkos view with the provided grid data
        for (std::size_t i = 0; i < x_size; ++i) {
            for (std::size_t j = 0; j < y_size; ++j) {
                grid_(i, j) = grid[i * y_size + j];
            }
        }
    }

    void run(int steps) {
        for (int step = 0; step < steps; ++step) {
            run_step();
        }
    }

    std::vector<int> fetch_result() const {
        // ...

        std::vector<int> result;
        result.reserve(grid_.extent(0) * grid_.extent(1));

        for (std::size_t i = 0; i < grid_.extent(0); ++i) {
            for (std::size_t j = 0; j < grid_.extent(1); ++j) {
                result.push_back(static_cast<int>(grid_(i, j)));
            }
        }

        return result;
    }

private:
    void run_step() {
        Kokkos::parallel_for("GoLStep", Kokkos::MDRangePolicy<Kokkos::Rank<2>>({1, 1}, {grid_.extent(0) - 1, grid_.extent(1) - 1}), KOKKOS_LAMBDA(const int i, const int j) {
            // game_of_life rules
            next_grid_(i, j) = /* apply rules */ grid_(i, j); // Placeholder for actual game_of_life rules logic
        });

        using std::swap;
        swap(grid_, next_grid_);
    }

    Kokkos::View<value_type**> grid_;
    Kokkos::View<value_type**> next_grid_;
};

} // namespace kokkos::game_of_life

#endif // KOKKOS_GAME_OF_LIFE_RUNNER_HPP
