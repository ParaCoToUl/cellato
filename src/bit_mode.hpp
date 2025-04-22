#ifndef CELLULAR_BIT_MODE_HPP
#define CELLULAR_BIT_MODE_HPP

#include <tuple>
#include <vector>
#include <cstddef>
#include <utility>
#include <iostream>

#include "constructs.hpp" 

namespace bitwise {
 
using namespace expr_tree;

template <typename states_enum, states_enum... states>
class state_dictionary {
public:
    using index_t = int;

    static constexpr index_t number_of_values = sizeof...(states);
    static constexpr index_t needed_bits = state_dictionary::log_2(number_of_values - 1) + 1;

    static constexpr index_t state_to_index(states_enum state) {
        return state_to_index_impl(state, states...);
    }

private:
    constexpr static int log_2(int n) {
        return (n < 2) ? 0 : 1 + log_2(n / 2);
    }

    template <typename... Rest>
    static constexpr index_t state_to_index_impl(states_enum target, states_enum head) {
        return (target == head) ? 0 : -1;
    }

    template <typename... Rest>
    static constexpr index_t state_to_index_impl(states_enum target, states_enum head, Rest... tail) {
        return (target == head) ? 0 : 1 + state_to_index_impl(target, tail...);
    }
};

// Helper type to expand the same vector type multiple times in a tuple
template <typename T, std::size_t I>
using always_vector_t = std::vector<T>;

template <typename T, std::size_t... Is>
auto repeat_vector_type(std::index_sequence<Is...>) -> std::tuple<always_vector_t<T, Is>...>;

template <typename T, std::size_t N>
using repeated_vector_tuple = decltype(repeat_vector_type<T>(std::make_index_sequence<N>{}));

template <typename store_word_type, typename states_dict_t>
class bit_grid {
    public:

    constexpr static int needed_bits = states_dict_t::needed_bits;
    static constexpr int word_store_bits = sizeof(store_word_type) * 8;

    using storage_tuple_t = repeated_vector_tuple<store_word_type, needed_bits>;

    bit_grid(std::size_t height, std::size_t width) {
        grid = storage_tuple_t{};
        
        auto x_size = width / word_store_bits;
        auto y_size = height;

        for_each_bit([&]<std::size_t bit_idx>() {
            std::get<bit_idx>(grid).resize(y_size * x_size);
        });
    }

    private:
    storage_tuple_t grid;

    template <typename Callback, std::size_t... Is>
    void for_each_bit_impl(Callback&& cb, std::index_sequence<Is...>) {
        (cb.template operator()<Is>(), ...);
    }

    template <typename Callback>
    void for_each_bit(Callback&& cb) {
        for_each_bit_impl(std::forward<Callback>(cb), std::make_index_sequence<needed_bits>{});
    }
};

}

#endif // CELLULAR_BIT_MODE_HPP