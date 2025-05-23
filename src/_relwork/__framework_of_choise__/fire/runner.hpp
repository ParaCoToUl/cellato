#ifndef __FRAMEWORK_OF_CHOICE__FIRE_RUNNER_HPP
#define __FRAMEWORK_OF_CHOICE__FIRE_RUNNER_HPP

#include <vector>
#include <cstddef>

namespace __framework_of_choice__::fire {

struct runner {
    void init(int* grid, std::size_t x_size, std::size_t y_size) {
        // ...
    }

    void run(int steps) {
        // ...
    }

    std::vector<int> fetch_result() {
        // ...
    }
};

}

#endif // __FRAMEWORK_OF_CHOICE__FIRE_RUNNER_HPP