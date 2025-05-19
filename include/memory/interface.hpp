#ifndef CELLIB_MEMORY_INTERFACE_HPP
#define CELLIB_MEMORY_INTERFACE_HPP

#ifndef CUDA_CALLABLE
#ifdef __CUDACC__
#define CUDA_CALLABLE __host__ __device__
#else
#define CUDA_CALLABLE
#endif
#endif

namespace cellib::memory::grids {

struct properties {
    std::size_t x_size;
    std::size_t y_size;

    CUDA_CALLABLE std::size_t idx(std::size_t x, std::size_t y) const {
        return y * x_size + x;
    }
};

struct point {
    std::size_t x;
    std::size_t y;
};

template <typename cell_type>
struct point_in_grid {
    using cell_t = cell_type;

    cell_t* grid;
    grids::properties properties;

    grids::point position;

    CUDA_CALLABLE std::size_t idx() const {
        return properties.idx(position.x, position.y);
    }
};

}

#endif // CELLIB_MEMORY_INTERFACE_HPP