#ifndef INDEXING_CUH
#define INDEXING_CUH

#ifdef __CUDACC__
#define CUDA_CALLABLE __host__ __device__
#else
#define CUDA_CALLABLE
#endif

#include <cstddef>

namespace reference::indexing {

struct indexer {
    static constexpr std::size_t x_margin = 0;
    static constexpr std::size_t y_margin = 0;
    
    CUDA_CALLABLE indexer(std::size_t x_size, std::size_t y_size)
        : _x_size(x_size), _y_size(y_size) {};

    CUDA_CALLABLE std::size_t at(std::size_t x, std::size_t y) {
        auto x_real = (x + _x_size) % _x_size;
        auto y_real = (y + _y_size) % _y_size;
        return y_real * _x_size + x_real;
    }

private:
    std::size_t _x_size, _y_size;
};

}


#endif // INDEXING_CUH