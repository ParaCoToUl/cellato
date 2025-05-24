#ifndef __FRAMEWORK_OF_CHOICE__FIRE_RUNNER_HPP
#define __FRAMEWORK_OF_CHOICE__FIRE_RUNNER_HPP

#include <cstddef>

#include <vector>

namespace __framework_of_choice__::fire {

struct runner {
    void init(int* grid, std::size_t x_size, std::size_t y_size) {
        // ...
    }

    void run(int steps) {
        // ...
    }

    std::vector<int> fetch_result() const {
        // ...

        return {};
    }
};

}

#endif // __FRAMEWORK_OF_CHOICE__FIRE_RUNNER_HPP