#ifndef RUN_PARAMS_HPP
#define RUN_PARAMS_HPP

namespace cellib::run {

struct run_params {
    std::size_t x_size = 0;
    std::size_t y_size = 0;
    std::size_t steps = 0;

    bool print = false;
};

}

#endif // RUN_PARAMS_HPP