#ifndef CELLIB_MEMORY_INTERFACE_HPP
#define CELLIB_MEMORY_INTERFACE_HPP

namespace cellib::memory::grids {

struct properties {
    std::size_t x_size;
    std::size_t y_size;

    std::size_t idx(std::size_t x, std::size_t y) const {
        return y * x_size + x;
    }
};

struct point {
    std::size_t x;
    std::size_t y;
};

template <typename cell_type>
struct point_at_grid {
    using cell_t = cell_type;

    cell_t* grid;
    grids::properties properties;

    grids::point position;
};

}

#endif // CELLIB_MEMORY_INTERFACE_HPP