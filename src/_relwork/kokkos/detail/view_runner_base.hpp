#ifndef KOKKOS_DETAIL_VIEW_RUNNER_BASE_HPP
#define KOKKOS_DETAIL_VIEW_RUNNER_BASE_HPP

#include "Kokkos_Core.hpp"

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <utility>
#include <vector>

namespace kokkos::detail {

template <typename RealRunnerBase, typename Derived, typename ValueType>
class view_runner_base : public RealRunnerBase {
public:
    using value_type = ValueType;
    using host_view_type = Kokkos::View<value_type**, Kokkos::LayoutRight, Kokkos::HostSpace>;
    using device_view_type = Kokkos::View<value_type**, Kokkos::LayoutRight, typename Kokkos::DefaultExecutionSpace::memory_space>;

    void init(int* grid,
              const cellato::run::run_params& params) override {
        kokkos_initialize(params);

        x_size_ = params.x_size;
        y_size_ = params.y_size;

        if (x_size_ <= 0 || y_size_ <= 0) {
            throw std::runtime_error("Invalid grid dimensions");
        }

        host_grid_ = host_view_type("kokkos_host_grid", x_size_, y_size_);
        host_next_grid_ = host_view_type("kokkos_host_next", x_size_, y_size_);

        auto host_grid = host_grid_;
        auto host_next = host_next_grid_;

        for (std::size_t j = 0; j < y_size_; ++j) {
            for (std::size_t i = 0; i < x_size_; ++i) {
                const value_type value = convert_input(static_cast<int>(grid[j * x_size_ + i]));
                host_grid(i, j) = value;
                host_next(i, j) = value;
            }
        }

        device_allocated_ = false;
        host_dirty_ = true;
        device_dirty_ = false;
        current_step_ = 0;
    }

    void run(int steps) override {
        for (int step = 0; step < steps; ++step) {
            if (execution_space_ == space::cpu) {
                run_step_cpu();
            } else {
                run_step_cuda();
            }
            ++current_step_;
        }
    }

    std::vector<int> fetch_result() override {
        if (!host_dirty_ && device_dirty_) {
            sync_host_from_device();
        }

        std::vector<int> result;
        result.reserve(host_grid_.extent(0) * host_grid_.extent(1));

        auto grid = host_grid_;
        for (std::size_t j = 0; j < host_grid_.extent(1); ++j) {
            for (std::size_t i = 0; i < host_grid_.extent(0); ++i) {
                result.push_back(static_cast<int>(grid(i, j)));
            }
        }

        return result;
    }

public:
    void run_step_cpu() {
        if (!host_dirty_ && device_dirty_) {
            sync_host_from_device();
        }

        auto grid = host_grid_;
        auto next_grid = host_next_grid_;
        const int step = current_step_;

        for (std::size_t j = 1; j < y_size_ - 1; ++j) {
            for (std::size_t i = 1; i < x_size_ - 1; ++i) {
                next_grid(static_cast<int>(i), static_cast<int>(j)) =
                    Derived::template apply_rule(grid, static_cast<int>(i), static_cast<int>(j), step);
            }
        }

        host_dirty_ = true;
        device_dirty_ = false;

        using std::swap;
        swap(host_grid_, host_next_grid_);
    }

    void run_step_cuda() {
#ifdef KOKKOS_ENABLE_CUDA
        ensure_device_views();

        if (host_dirty_) {
            sync_device_from_host();
        }

        auto grid = device_grid_;
        auto next_grid = device_next_grid_;
        const int step = current_step_;

        Kokkos::parallel_for(
            Derived::cuda_label(),
            Kokkos::MDRangePolicy<Kokkos::Cuda, Kokkos::Rank<2>>({1, 1}, {int(x_size_) - 1, int(y_size_) - 1}, {tile_dim_, tile_dim_}),
            KOKKOS_CLASS_LAMBDA(const int i, const int j) {
                next_grid(i, j) = Derived::template apply_rule(grid, i, j, step);
            });

        Kokkos::fence();

        host_dirty_ = false;
        device_dirty_ = true;

        using std::swap;
        swap(device_grid_, device_next_grid_);
#else
        throw std::runtime_error("CUDA execution requested but not enabled");
#endif
    }

protected:
    enum class space {
        cpu,
        cuda
    };

    static inline space execution_space_ = space::cpu;
    static inline int tile_dim_ = 16;

    host_view_type host_grid_;
    host_view_type host_next_grid_;
    device_view_type device_grid_;
    device_view_type device_next_grid_;
    std::size_t x_size_ = 0;
    std::size_t y_size_ = 0;
    bool device_allocated_ = false;
    bool host_dirty_ = true;
    bool device_dirty_ = false;
    int current_step_ = 0;

    void kokkos_initialize(const cellato::run::run_params& params) {
        static struct kokkos_init_guard {
            kokkos_init_guard(const cellato::run::run_params& params, space* execution_space, int* tile_dim) {
                Kokkos::InitializationSettings settings;

                if (params.device == "CPU") {
                    *execution_space = space::cpu;
                    settings.set_num_threads(1);
                } else if (params.device == "CUDA") {
#ifdef KOKKOS_ENABLE_CUDA
                    *execution_space = space::cuda;
                    settings.set_device_id(0);
                    if (params.cuda_block_size_x > 0) {
                        *tile_dim = params.cuda_block_size_x;
                    }
#else
                    throw std::runtime_error("CUDA execution requested but not enabled");
#endif
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

private:
    void ensure_device_views() {
        if (!device_allocated_) {
            device_grid_ = device_view_type("kokkos_device_grid", x_size_, y_size_);
            device_next_grid_ = device_view_type("kokkos_device_next", x_size_, y_size_);
            device_allocated_ = true;
            device_dirty_ = false;
        }
    }

    void sync_device_from_host() {
#ifdef KOKKOS_ENABLE_CUDA
        ensure_device_views();
        Kokkos::deep_copy(device_grid_, host_grid_);
        Kokkos::deep_copy(device_next_grid_, host_next_grid_);
        device_dirty_ = true;
        host_dirty_ = true;
#endif
    }

    void sync_host_from_device() {
#ifdef KOKKOS_ENABLE_CUDA
        if (!device_allocated_) {
            return;
        }
        Kokkos::deep_copy(host_grid_, device_grid_);
        Kokkos::deep_copy(host_next_grid_, device_next_grid_);
        host_dirty_ = true;
        device_dirty_ = true;
#endif
    }

    static value_type convert_input(int value) {
        return static_cast<value_type>(value);
    }
};

} // namespace kokkos::detail

#endif // KOKKOS_DETAIL_VIEW_RUNNER_BASE_HPP
