#include "greenberg/runner.hpp"

#include "Kokkos_Core.hpp"

#include "../../greenberg/algorithm.hpp"

#include <cstdint>
#include <stdexcept>
#include <utility>
#include <vector>

namespace kokkos::greenberg {

namespace {

struct greenberg_runner_impl : public real_runner {
    using value_type = std::uint8_t;
    using cell_state = ::greenberg::ghm_cell_state;

    enum class space {
        cpu,
        cuda
    };

    void init(int* grid, const cellato::run::run_params& params) override {
        kokkos_initialize(params);

        const std::size_t x_size = params.x_size;
        const std::size_t y_size = params.y_size;

        grid_ = view_type("greenberg_grid", x_size, y_size);
        next_grid_ = view_type("greenberg_next_grid", x_size, y_size);

        for (std::size_t j = 0; j < y_size; ++j) {
            for (std::size_t i = 0; i < x_size; ++i) {
                grid_(i, j) = static_cast<value_type>(grid[j * x_size + i]);
            }
        }
    }

    void run(int steps) override {
        for (int step = 0; step < steps; ++step) {
            if (execution_space_ == space::cpu) {
                run_step_cpu();
            } else {
                run_step_cuda();
            }
        }
    }

    std::vector<int> fetch_result() override {
        std::vector<int> result;
        result.reserve(grid_.extent(0) * grid_.extent(1));

        for (std::size_t j = 0; j < grid_.extent(1); ++j) {
            for (std::size_t i = 0; i < grid_.extent(0); ++i) {
                result.push_back(static_cast<int>(grid_(i, j)));
            }
        }

        return result;
    }

public:
    using view_type = Kokkos::View<value_type**, Kokkos::SharedSpace>;

    void run_step_cpu() {
        if (grid_.extent(0) < 3 || grid_.extent(1) < 3) {
            return;
        }

        Kokkos::parallel_for(
            "GreenbergStepCPU",
            Kokkos::MDRangePolicy<Kokkos::Serial, Kokkos::Rank<2>>({1, 1}, {grid_.extent_int(0) - 1, grid_.extent_int(1) - 1}),
            KOKKOS_CLASS_LAMBDA(const int i, const int j) {
                const auto current = static_cast<cell_state>(grid_(i, j));
                auto next = current;

                if (current == cell_state::quiescent) {
                    next = count_excited_neighbors(grid_, i, j) > 0 ? cell_state::excited : cell_state::quiescent;
                } else {
                    next = advance_state(current);
                }

                next_grid_(i, j) = static_cast<value_type>(next);
            });

        using std::swap;
        swap(grid_, next_grid_);
    }

    void run_step_cuda() {
        if (grid_.extent(0) < 3 || grid_.extent(1) < 3) {
            return;
        }

        Kokkos::parallel_for(
            "GreenbergStepCUDA",
            Kokkos::MDRangePolicy<Kokkos::Cuda, Kokkos::Rank<2>>({1, 1}, {grid_.extent_int(0) - 1, grid_.extent_int(1) - 1}, {tile_dim_, tile_dim_}),
            KOKKOS_CLASS_LAMBDA(const int i, const int j) {
                const auto current = static_cast<cell_state>(grid_(i, j));
                auto next = current;

                if (current == cell_state::quiescent) {
                    next = count_excited_neighbors(grid_, i, j) > 0 ? cell_state::excited : cell_state::quiescent;
                } else {
                    next = advance_state(current);
                }

                next_grid_(i, j) = static_cast<value_type>(next);
            });

        Kokkos::fence();

        using std::swap;
        swap(grid_, next_grid_);
    }

private:
    KOKKOS_INLINE_FUNCTION
    static int count_excited_neighbors(const view_type& grid, int i, int j) {
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

    KOKKOS_INLINE_FUNCTION
    static cell_state advance_state(cell_state current) {
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

    void kokkos_initialize(const cellato::run::run_params& params) {
        static struct kokkos_init_guard {
            kokkos_init_guard(const cellato::run::run_params& params, space* execution_space, int* tile_dim) {
                Kokkos::InitializationSettings settings;

                if (params.device == "CPU") {
                    *execution_space = space::cpu;
                    settings.set_num_threads(1);
                } else if (params.device == "CUDA") {
                    *execution_space = space::cuda;
                    settings.set_device_id(0);
                    if (params.cuda_block_size_x > 0) {
                        *tile_dim = params.cuda_block_size_x;
                    }
                } else {
                    throw std::runtime_error("Unsupported device: " + params.device);
                }

                Kokkos::initialize(settings);
            }

            ~kokkos_init_guard() {
                Kokkos::finalize();
            }
        } guard(params, &execution_space_, &tile_dim_);
    }

    view_type grid_;
    view_type next_grid_;
    static inline space execution_space_ = space::cpu;
    static inline int tile_dim_ = 16;
};

} // namespace

std::unique_ptr<real_runner> create_runner() {
    return std::make_unique<greenberg_runner_impl>();
}

} // namespace kokkos::greenberg
