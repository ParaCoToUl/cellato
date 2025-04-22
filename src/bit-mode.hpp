#ifndef CELLULAR_BIT_MODE_HPP
#define CELLULAR_BIT_MODE_HPP

#include <tuple>
#include <vector>
#include <cstddef>
#include <utility>
#include <iostream>
#include <cstdint> // Add missing include

#include "constructs.hpp" 

namespace bitwise {
 
using namespace expr_tree;

template <typename states_enum, states_enum... states>
class state_dictionary {
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
        if (index >= 0 && index < sizeof...(states)) {
            return state_array[index];
        }
        throw std::out_of_range("Index out of range");
    }

private:
    constexpr static int log_2(int n) {
        return (n < 2) ? 0 : 1 + log_2(n / 2);
    }

    template <typename... Rest>
    static constexpr index_t state_to_index_impl(states_enum target, states_enum head) {
        return (target == head) ? 0 : throw std::out_of_range("State not found in dictionary");
    }

    template <typename... Rest>
    static constexpr index_t state_to_index_impl(states_enum target, states_enum head, Rest... tail) {
        return (target == head) ? 0 : 1 + state_to_index_impl(target, tail...);
    }
};

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
    using original_state_t = typename states_dict_t::state_t;

    bit_grid(std::size_t height, std::size_t width, const original_state_t* grid_input)
        : _x_size(width / word_store_bits), _y_size(height) {

        if (width % word_store_bits != 0) {
            throw std::invalid_argument("`Width` must be a multiple of `word store bits`");
        }

        grid = storage_tuple_t{};
        
        for_each_bit([&]<std::size_t bit_idx>() {
            std::get<bit_idx>(grid).resize(y_size_physical() * x_size_physical());
        });

        for (std::size_t y = 0; y < y_size_physical(); ++y) {
            for (std::size_t x = 0; x < x_size_physical(); ++x) {
                auto word_idx = x + y * x_size_physical();
                
                for_each_bit([&]<std::size_t bit_idx>() {
                    store_word_type word = 0;

                    for (std::size_t i = 0; i < word_store_bits; ++i) {
                        auto state = grid_input[y * x_size_original() + x * word_store_bits + i];
                        auto index = states_dict_t::state_to_index(state);
                        
                        auto set_bit = (index & (1 << bit_idx)) != 0;

                        if (set_bit) {
                            word |= (1 << i);
                        }
                    }

                    std::get<bit_idx>(grid)[word_idx] = word;
                });
            }
        }
    }

    original_state_t get_cell(std::size_t x, std::size_t y) const {
        if (x >= x_size_original() || y >= y_size_original()) {
            throw std::out_of_range("Cell coordinates out of range");
        }
        
        auto bit_grid_idx = y * x_size_physical() + (x / word_store_bits);
        int state_idx = 0;

        for_each_bit([&]<std::size_t bit_idx>() {
            auto word = std::get<bit_idx>(grid)[bit_grid_idx];
            auto bit = (word >> (x % word_store_bits)) & 1;
            state_idx |= (bit << bit_idx);
        });

        return states_dict_t::index_to_state(state_idx);
    }

    std::vector<original_state_t> to_original_representation() const {
        std::vector<original_state_t> result(x_size_original() * y_size_original());
        
        for (std::size_t y = 0; y < y_size_original(); ++y) {
            for (std::size_t x = 0; x < x_size_original(); ++x) {
                result[y * x_size_original() + x] = get_cell(x, y);
            }
        }

        return result;
    }

    std::size_t x_size_original() const {
        return x_size_physical() * word_store_bits;
    }

    std::size_t y_size_original() const {
        return y_size_physical();
    }

    std::size_t x_size_physical() const {
        return _x_size;
    }

    std::size_t y_size_physical() const {
        return _y_size;
    }

  private:
  
    storage_tuple_t grid;
    std::size_t _x_size, _y_size;
    
    template <typename Callback, std::size_t... Is>
    void for_each_bit_impl(Callback&& cb, std::index_sequence<Is...>) const {
        (cb.template operator()<Is>(), ...);
    }

    template <typename Callback>
    void for_each_bit(Callback&& cb) const {
        for_each_bit_impl(std::forward<Callback>(cb), std::make_index_sequence<needed_bits>{});
    }
};

template <typename vector_store_type, int bits>
class vector_int {


};

}

#endif // CELLULAR_BIT_MODE_HPP