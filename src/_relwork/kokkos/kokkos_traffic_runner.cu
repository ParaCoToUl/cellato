#include "traffic/runner.hpp"

#include "Kokkos_Core.hpp"

#include "../../traffic/algorithm.hpp"

#include <cstdint>
#include <stdexcept>
#include <utility>
#include <vector>

namespace kokkos::traffic {

namespace {

struct traffic_runner_impl : public real_runner {
    using value_type = std::uint8_t;
    using cell_state = ::traffic::traffic_cell_state;

    enum class space {
        cpu,
        cuda
    };

    void init(int* grid, const cellato::run::run_params& params) override {
        kokkos_initialize(params);

        const std::size_t x_size = params.x_size;
        const std::size_t y_size = params.y_size;

        grid_ = view_type("traffic_grid", x_size, y_size);
        next_grid_ = view_type("traffic_next_grid", x_size, y_size);

        for (std::size_t i = 0; i < x_size; ++i) {
            for (std::size_t j = 0; j < y_size; ++j) {
                grid_(i, j) = static_cast<value_type>(grid[i * y_size + j]);
            }
        }

        current_step_ = 0;
    }

    void run(int steps) override {
        for (int step = 0; step < steps; ++step) {
            if (execution_space_ == space::cpu) {
                run_step_cpu(current_step_);
            } else {
                run_step_cuda(current_step_);
            }
            ++current_step_;
        }
    }

    std::vector<int> fetch_result() override {
        std::vector<int> result;
        result.reserve(grid_.extent(0) * grid_.extent(1));

        for (std::size_t i = 0; i < grid_.extent(0); ++i) {
            for (std::size_t j = 0; j < grid_.extent(1); ++j) {
                result.push_back(static_cast<int>(grid_(i, j)));
            }
        }

        return result;
    }

public:
    using view_type = Kokkos::View<value_type**, Kokkos::SharedSpace>;

    template <typename ExecPolicy>
    void run_step_impl(ExecPolicy policy, int step) {
        if (grid_.extent(0) < 3 || grid_.extent(1) < 3) {
            return;
        }

        Kokkos::parallel_for(
            "TrafficStep",
            policy,
            KOKKOS_CLASS_LAMBDA(const int i, const int j) {
                const auto current = static_cast<cell_state>(grid_(i, j));
                auto next = current;

                const int step_parity = step & 1;

                if (step_parity == 0) {
                    // Move red cars to the right
                    const auto outgoing = static_cast<cell_state>(grid_(i + 1, j));
                    const auto incoming = static_cast<cell_state>(grid_(i - 1, j));

                    if (current == cell_state::red_car) {
                        next = (outgoing == cell_state::empty) ? cell_state::empty : cell_state::red_car;
                    } else if (current == cell_state::empty) {
                        next = (incoming == cell_state::red_car) ? cell_state::red_car : cell_state::empty;
                    } else {
                        next = current; // blue car remains
                    }
                } else {
                    // Move blue cars downward
                    const auto outgoing = static_cast<cell_state>(grid_(i, j + 1));
                    const auto incoming = static_cast<cell_state>(grid_(i, j - 1));

                    if (current == cell_state::blue_car) {
                        next = (outgoing == cell_state::empty) ? cell_state::empty : cell_state::blue_car;
                    } else if (current == cell_state::empty) {
                        next = (incoming == cell_state::blue_car) ? cell_state::blue_car : cell_state::empty;
                    } else {
                        next = current; // red car remains
                    }
                }

                next_grid_(i, j) = static_cast<value_type>(next);
            });

        using std::swap;
        swap(grid_, next_grid_);
    }

    void run_step_cpu(int step) {
        run_step_impl(Kokkos::MDRangePolicy<Kokkos::Serial, Kokkos::Rank<2>>({1, 1}, {grid_.extent_int(0) - 1, grid_.extent_int(1) - 1}), step);
    }

    void run_step_cuda(int step) {
        run_step_impl(Kokkos::MDRangePolicy<Kokkos::Cuda, Kokkos::Rank<2>>({1, 1}, {grid_.extent_int(0) - 1, grid_.extent_int(1) - 1}, {tile_dim_, tile_dim_}), step);
        Kokkos::fence();
    }

private:
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
    int current_step_ = 0;
};

} // namespace

std::unique_ptr<real_runner> create_runner() {
    return std::make_unique<traffic_runner_impl>();
}

} // namespace kokkos::traffic
