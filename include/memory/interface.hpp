#ifndef CELLATO_MEMORY_INTERFACE_HPP
#define CELLATO_MEMORY_INTERFACE_HPP

#include <cstddef>
#ifndef CUDA_CALLABLE
#ifdef __CUDACC__
#define CUDA_CALLABLE __host__ __device__
#else
#define CUDA_CALLABLE
#endif
#endif

namespace cellato::memory::grids {

enum class device {
    CPU,
    CUDA
};

struct properties {
    int x_size;
    int y_size;

    CUDA_CALLABLE int idx(int x, int y) const {
        auto x_real = (x + x_size) % x_size;
        auto y_real = (y + y_size) % y_size;
        return y_real * x_size + x_real;
    }
};

struct point {
    int x;
    int y;
};

template <typename grid_data_type>
struct point_in_grid {
    using grid_t = grid_data_type;

    grid_data_type grid{};

    grids::properties properties{};

    grids::point position{};

    int time_step = 0;

    CUDA_CALLABLE int idx() const {
        return properties.idx(position.x, position.y);
    }

    CUDA_CALLABLE int idx(int x, int y) const {
        return properties.idx(x, y);
    }
};

}

#endif // CELLATO_MEMORY_INTERFACE_HPP