#ifndef STENCIL_ITERATOR_HPP
#define STENCIL_ITERATOR_HPP

#include <tuple>
#include <vector>
#include <cstddef>
#include <iostream>
#include <string>
#include <array>

#include "bit-evaluator.hpp"
#include "bit-mode.hpp"

namespace iterators {

// Change to use char arrays instead of std::string_view for C++17 compatibility
template<const char* Symbol, const char* Color>
struct print_pair {
    static constexpr const char* symbol = Symbol;
    static constexpr const char* color = Color;
    
    static std::string get_formatted_str() {
        return std::string(color) + std::string(symbol) + "\033[0m";
    }
};

// Print configuration with variadic template for state mappings
template<typename... Pairs>
struct print_config {
    template<typename T>
    static std::string get_str(T state_value) {
        const int index = static_cast<int>(state_value);
        return get_str_impl(index, std::make_index_sequence<sizeof...(Pairs)>{});
    }

private:
    template<size_t... Is>
    static std::string get_str_impl(int index, std::index_sequence<Is...>) {
        std::array<std::string, sizeof...(Pairs)> strings = {
            (Pairs::get_formatted_str())...
        };
        
        if (index >= 0 && index < static_cast<int>(strings.size())) {
            return strings[index];
        }
        return "?"; // Default for unknown states
    }
};

template <typename config_t>
class bit_grid_simple_iterator {
    using algorithm_t = typename config_t::algorithm_t;
    using state_dictionary_t = typename config_t::state_dictionary_t;

    using cell_row_t = typename config_t::cell_row_t;
    using step_state_t = bitwise_no_cache::grid_config<cell_row_t, state_dictionary_t>;

    using cell_orig_state_t = typename state_dictionary_t::state_t;
    using grid_t = bitwise::bit_grid<cell_row_t, state_dictionary_t>;

    using print_config_t = typename config_t::print_config_t;

    constexpr static int word_width_bits = sizeof(cell_row_t) * 8;

  public:
    bit_grid_simple_iterator() : 
        input_grid(1, word_width_bits), 
        intermediate_grid(1, word_width_bits),
        final_grid(&input_grid),
        steps_(0) {
    }

    void init(const std::vector<cell_orig_state_t>& grid, std::size_t height, std::size_t width) {
        auto padded_grid = get_padded_data(grid, height, width);

        auto padded_width = width + 2 * word_width_bits;
        auto padded_height = height + 2;

        input_grid = grid_t(padded_height, padded_width, padded_grid.data());
        intermediate_grid = grid_t(padded_height, padded_width);
    }

    template <bool print = false>
    void run(int steps) {
        this->steps_ = steps;

        const auto y_size = input_grid.y_size_physical();
        const auto x_size = input_grid.x_size_physical();

        auto input_data = input_grid.data();
        auto output_data = intermediate_grid.data();

        for (int step = 0; step < steps_; ++step) {
            for (std::size_t y = 1; y < y_size - 1; ++y) {
                for (std::size_t x = 1; x < x_size - 1; ++x) {
                    step_state_t state;
                    
                    state.x = x;
                    state.y = y;
                    state.width_b = x_size;
                    state.height_b = y_size;
                    state.bit_grid = input_data;

                    auto result = bitwise_no_cache::evaluator<cell_row_t, state_dictionary_t, algorithm_t>::evaluate(state);
                    result.save_to(output_data, y * x_size + x);
                }
            }

            if constexpr (print) {
                std::cout << "Step " << step + 1 << ":\n";
                print_grid(intermediate_grid);
            }

            std::swap(input_data, output_data);
            std::swap(input_grid, intermediate_grid);
        }

        final_grid = &input_grid;
    }

    std::vector<cell_orig_state_t> get_result() const {
        auto padded_data = final_grid->to_original_representation();

        auto padded_width = final_grid->x_size_original();
        
        auto original_width = final_grid->x_size_original() - 2 * word_width_bits;
        auto original_height = final_grid->y_size_original() - 2;

        std::vector<cell_orig_state_t> result(original_height * original_width);

        for (std::size_t y = 0; y < original_height; ++y) {
            for (std::size_t x = 0; x < original_width; ++x) {
                result[y * original_width + x] = padded_data[(y + 1) * padded_width + (x + word_width_bits)];
            }
        }

        return result;
    }

    void print_current_grid() const {
        print_grid(*final_grid);
    }

  private:
    grid_t input_grid, intermediate_grid;
    grid_t* final_grid;
    int steps_ = 0;

    std::vector<cell_orig_state_t> get_padded_data(const std::vector<cell_orig_state_t>& grid, std::size_t height, std::size_t width) {
        std::size_t padded_width = width + 2 * word_width_bits;
        std::size_t padded_height = height + 2;

        std::vector<cell_orig_state_t> padded_data(padded_height * padded_width, state_dictionary_t::index_to_state(0));

        for (std::size_t y = 0; y < height; ++y) {
            for (std::size_t x = 0; x < width; ++x) {
                padded_data[(y + 1) * padded_width + (x + word_width_bits)] = grid[y * width + x];
            }
        }

        return padded_data;
    }

    void print_grid(grid_t& grid) const {
        auto not_padded_width = grid.x_size_original() - 2 * word_width_bits;
        auto not_padded_height = grid.y_size_original() - 2;

        for (std::size_t y = 0; y < not_padded_height; ++y) {
            for (std::size_t x = 0; x < not_padded_width; ++x) {
                auto x_offset = x + word_width_bits;
                auto y_offset = y + 1;

                auto cell = grid.get_cell(x_offset, y_offset);
                
                std::cout << print_config_t::get_str(cell) << " ";
            }

            std::cout << "\n";
        }
    }
};

} 

#endif // STENCIL_ITERATOR_HPP