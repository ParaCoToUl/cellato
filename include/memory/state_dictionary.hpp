#ifndef CELLIB_MEMORY_STATE_DICTIONARY_HPP
#define CELLIB_MEMORY_STATE_DICTIONARY_HPP

#include <stdexcept>

namespace cellib::memory::grids {

template <typename states_enum, states_enum... states>
class state_dictionary {
private:
    constexpr static int log_2(int n) {
        return (n < 2) ? 0 : 1 + log_2(n / 2);
    }

public:
    using index_t = int;
    using state_t = states_enum;

    static constexpr index_t number_of_values = sizeof...(states);
    static constexpr index_t needed_bits = state_dictionary::log_2(number_of_values - 1) + 1;

    static constexpr index_t state_to_index(state_t state) {
        return state_to_index_impl(state, states...);
    }

    static constexpr state_t index_to_state(index_t index) {
        constexpr state_t state_array[] = {states...};
        if (index >= 0 && index < (index_t)sizeof...(states)) {
            return state_array[index];
        }
        throw std::out_of_range("Index out of range");
    }

private:
    template <typename... Rest>
    static constexpr index_t state_to_index_impl(states_enum target, states_enum head) {
        return (target == head) ? 0 : throw std::out_of_range("State not found in dictionary");
    }

    template <typename... Rest>
    static constexpr index_t state_to_index_impl(states_enum target, states_enum head, Rest... tail) {
        return (target == head) ? 0 : 1 + state_to_index_impl(target, tail...);
    }
};

}

#endif // CELLIB_MEMORY_STATE_DICTIONARY_HPP
