#ifndef HALIDE_COMMON_RUNNER_BASE_HPP
#define HALIDE_COMMON_RUNNER_BASE_HPP

#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "Halide.h"

#include "runner_interface.hpp"

namespace halide::common {

template <typename Enum>
constexpr int to_int(Enum value) {
    return static_cast<int>(value);
}

class runner_base : public real_runner_interface {
public:
    using value_type = real_runner_interface::value_type;

    void init(int* grid,
              const cellato::run::run_params& params) override {
        params_ = params;
        if (params_.x_size <= 0 || params_.y_size <= 0) {
            throw std::runtime_error("Halide runner requires positive grid dimensions.");
        }

        width_ = params_.x_size;
        height_ = params_.y_size;

        current_grid_ = Halide::Buffer<int>(width_, height_);
        next_grid_ = Halide::Buffer<int>(width_, height_);

        if (!grid) {
            throw std::runtime_error("Halide runner received null grid pointer.");
        }

        for (int y = 0; y < height_; ++y) {
            for (int x = 0; x < width_; ++x) {
                int idx = y * width_ + x;
                current_grid_(x, y) = grid[idx];
                next_grid_(x, y) = current_grid_(x, y);
            }
        }

        try {
            build_pipeline(params_);

            target_ = Halide::get_host_target();
            if (params_.device == "CPU") {
                schedule_cpu(params_);
            } else if (params_.device == "CUDA") {
                target_.set_feature(Halide::Target::CUDA);
                if (!target_.has_gpu_feature()) {
                    throw std::runtime_error("CUDA feature unavailable for Halide target.");
                }
                schedule_gpu(params_);
            } else {
                throw std::runtime_error("Unsupported device: " + params_.device);
            }

            result_.compile_jit(target_);
        } catch (const Halide::CompileError& e) {
            throw std::runtime_error("Halide compile error: " + std::string(e.what()));
        }
    }

    void run(int steps) override {
        for (int step = 0; step < steps; ++step) {
            try {
                grid_param_.set(current_grid_);
                update_step_state(step);
                result_.realize(next_grid_, target_);
                std::swap(current_grid_, next_grid_);
            } catch (const Halide::RuntimeError& e) {
                throw std::runtime_error("Halide runtime error: " + std::string(e.what()));
            }
        }
    }

    std::vector<int> fetch_result() override {
        std::vector<int> out(static_cast<std::size_t>(width_ * height_));
        current_grid_.copy_to_host();
        for (int y = 0; y < height_; ++y) {
            for (int x = 0; x < width_; ++x) {
                out[y * width_ + x] = current_grid_(x, y);
            }
        }
        return out;
    }

protected:
    Halide::Var x{"x"}, y{"y"};
    Halide::Var xi{"xi"}, yi{"yi"};
    Halide::Var xo{"xo"}, yo{"yo"};

    Halide::ImageParam grid_param_{Halide::Int(32), 2, "grid"};
    Halide::Func result_{"result"};

    Halide::Buffer<int> current_grid_;
    Halide::Buffer<int> next_grid_;

    int width_{0};
    int height_{0};

    const cellato::run::run_params& params() const { return params_; }
    Halide::Target& target() { return target_; }
    Halide::ImageParam& grid_param() { return grid_param_; }
    Halide::Func& result() { return result_; }

    Halide::Expr grid_width() const { return grid_param_.dim(0).extent(); }
    Halide::Expr grid_height() const { return grid_param_.dim(1).extent(); }

    Halide::Func clamped_grid() {
        return Halide::BoundaryConditions::repeat_edge(
            grid_param_, {{0, grid_width()}, {0, grid_height()}});
    }

    virtual void build_pipeline(const cellato::run::run_params& params) = 0;

    virtual void schedule_cpu(const cellato::run::run_params&) {
        result_.compute_root();
        result_.vectorize(x, 8, Halide::TailStrategy::GuardWithIf);
    }

    virtual void schedule_gpu(const cellato::run::run_params& params) {
        result_.compute_root();
        int tile_x = params.cuda_block_size_x <= 0 ? 16 : params.cuda_block_size_x;
        int tile_y = params.cuda_block_size_y <= 0 ? 16 : params.cuda_block_size_y;
        result_.gpu_tile(x, y, xi, yi, tile_x, tile_y);
    }

    virtual void update_step_state(int /*step*/) {}

    Halide::Expr is_border() const {
        auto gw = grid_width();
        auto gh = grid_height();
        return (x == 0) || (y == 0) || (x == gw - 1) || (y == gh - 1);
    }

private:
    cellato::run::run_params params_{};
    Halide::Target target_{};
};

} // namespace halide::common

#endif // HALIDE_COMMON_RUNNER_BASE_HPP
