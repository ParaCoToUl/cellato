#ifndef CELLIB_BIT_PLATES_GRID_HPP
#define CELLIB_BIT_PLATES_GRID_HPP

#include <tuple>
#include <vector>
#include <cstddef>
#include <stdexcept>
#include <utility>
#include <iostream>
#include <cstdint>
#include <algorithm>

#include "interface.hpp"
#include "standard_grid.hpp"
#include "grid_utils.hpp"

namespace cellib::memory::grids::bit_plates {

using namespace cellib::memory::grids::utils;
using namespace cellib::memory::grids;

template <typename store_word_type, typename states_dict_t>
class grid {
    public:
    constexpr static bool HAS_OWN_PRINT = false;

    constexpr static int needed_bits = states_dict_t::needed_bits;
    static constexpr int word_store_bits = sizeof(store_word_type) * 8;

    using storage_tuple_t = repeated_vector_tuple<store_word_type, needed_bits>;
    using storage_tuple_of_pointers = repeated_tuple_t<store_word_type*, needed_bits>;

    using original_state_t = typename states_dict_t::state_t;

    using cell_t = store_word_type;
    using store_type = cell_t;

    grid() = default;

    grid(std::size_t y_size, std::size_t x_size)
        : _x_size(x_size / word_store_bits), _y_size(y_size) {
        _grid = storage_tuple_t{};

        for_each_bit([&]<std::size_t bit_idx>() {
            std::get<bit_idx>(_grid).resize(y_size_physical() * x_size_physical());
        });
    }

    grid(std::size_t y_size, std::size_t x_size, const original_state_t* grid_input) {
        initialize_grid(y_size, x_size, grid_input);        
    }

    grid(const cellib::memory::grids::standard::grid<original_state_t>& standard_grid) {
        initialize_grid(
            standard_grid.y_size_physical(), standard_grid.x_size_physical(),
            standard_grid.data());
    }

    original_state_t get_cell(std::size_t x, std::size_t y) const {
        if (x >= x_size_original() || y >= y_size_original()) {
            throw std::out_of_range("Cell coordinates out of range");
        }

        auto bit_grid_idx = y * x_size_physical() + (x / word_store_bits);
        int state_idx = 0;

        for_each_bit([&]<std::size_t bit_idx>() {
            auto word = std::get<bit_idx>(_grid)[bit_grid_idx];
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

    cellib::memory::grids::standard::grid<original_state_t> to_standard() const {
        auto grid_data = to_original_representation();
        return cellib::memory::grids::standard::grid<original_state_t>(
            std::move(grid_data), x_size_original(), y_size_original()); 
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

    storage_tuple_of_pointers data() {
        storage_tuple_of_pointers result;
        for_each_bit([&]<std::size_t bit_idx>() {
            std::get<bit_idx>(result) = std::get<bit_idx>(_grid).data();
        });
        return result;
    }

  private:
    storage_tuple_t _grid;

    std::size_t _x_size, _y_size;

    template <typename Callback, std::size_t... Is>
    void for_each_bit_impl(Callback&& cb, std::index_sequence<Is...>) const {
        (cb.template operator()<Is>(), ...);
    }

    template <typename Callback>
    void for_each_bit(Callback&& cb) const {
        for_each_bit_impl(std::forward<Callback>(cb), std::make_index_sequence<needed_bits>{});
    }

    void initialize_grid(std::size_t y_size, std::size_t x_size, const original_state_t* grid_input) {
        _x_size = x_size / word_store_bits;
        _y_size = y_size;

        if (x_size % word_store_bits != 0) {
            throw std::invalid_argument("`x_size` must be a multiple of `word store bits`");
        }

        _grid = storage_tuple_t{};

        for_each_bit([&]<std::size_t bit_idx>() {
            std::get<bit_idx>(_grid).resize(y_size_physical() * x_size_physical());
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
                            word |= (static_cast<store_word_type>(1) << i);
                        }
                    }

                    std::get<bit_idx>(_grid)[word_idx] = word;
                });
            }
        }
    }
};

}

#endif // CELLIB_BIT_PLATES_GRID_HPP