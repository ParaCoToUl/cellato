#ifndef CELLULAR_BIT_MODE_HPP
#define CELLULAR_BIT_MODE_HPP

#include <tuple>
#include <vector>
#include <cstddef>
#include <utility>
#include <iostream>
#include <cstdint>

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

template <typename T, std::size_t I>
using always_type_t = T;

template <typename T, std::size_t... Is>
auto repeat_tuple_type(std::index_sequence<Is...>) -> std::tuple<always_type_t<T, Is>...>;

template <typename T, std::size_t N>
using repeated_tuple_t = decltype(repeat_tuple_type<T>(std::make_index_sequence<N>{}));

template <typename store_word_type, typename states_dict_t>
class bit_grid {
    public:

    constexpr static int needed_bits = states_dict_t::needed_bits;
    static constexpr int word_store_bits = sizeof(store_word_type) * 8;

    using storage_tuple_t = repeated_vector_tuple<store_word_type, needed_bits>;
    using storage_tuple_of_pointers = repeated_tuple_t<store_word_type*, needed_bits>;

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

        for_each_bit([&]<std::size_t bit_idx>() {
            std::get<bit_idx>(grid_pointers)[bit_idx] = std::get<bit_idx>(grid).data();
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
    storage_tuple_of_pointers grid_pointers;

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

template <typename const_t>
struct constats_ops {

    static constexpr const_t ones = ~static_cast<const_t>(0);

    template <const_t value>
    static constexpr int get_highest_set_bit() {
        if (value == 0) {
            return -1; // No bits are set
        }

        int highest_bit = 0;
        for (int i = 0; i < sizeof(const_t) * 8; ++i) {
            if ((value >> i) & 1) {
                highest_bit = i;
            }
        }

        return highest_bit;
    }

    template <const_t value>
    static constexpr const_t get_bit_row_at(int bit_idx) {
        return ((value >> bit_idx) & 1) * ones;
    }

    template <const_t value>
    static constexpr const_t is_set_at(int bit_idx) {
        return ((value >> bit_idx) & 1) * ones;
    }
};

enum class bit_action {
    NO_ACTION = 0,
    SET_ZERO = 1,
    SET_ONE = 2,
    FLIP = 3,
};

struct op_shift_right {
    template <typename T, typename U>
    static auto apply(T a, U b) {
        return a >> b;
    }
};

struct op_shift_left {
    template <typename T, typename U>
    static auto apply(T a, U b) {
        return a << b;
    }
};

struct op_and {
    template <typename T, typename U>
    static auto apply(T a, U b) {
        return a & b;
    }

    template <typename const_t, const_t value>
    static constexpr bit_action action_for_bit(int bit_idx) {
        auto bit_value = (value >> bit_idx) & 1;

        if (bit_value == 0) {
            return bit_action::SET_ZERO;
        } else {
            return bit_action::NO_ACTION;
        }
    }
};

struct op_or {
    template <typename T, typename U>
    static auto apply(T a, U b) {
        return a | b;
    }

    template <typename const_t, const_t value>
    static constexpr bit_action action_for_bit(int bit_idx) {
        auto bit_value = (value >> bit_idx) & 1;

        if (bit_value == 1) {
            return bit_action::SET_ONE;
        } else {
            return bit_action::NO_ACTION;
        }
    }
};

struct op_xor {
    template <typename T, typename U>
    static auto apply(T a, U b) {
        return a ^ b;
    }

    template <typename const_t, const_t value>
    static constexpr bit_action action_for_bit(int bit_idx) {
        auto bit_value = (value >> bit_idx) & 1;
        
        if (bit_value == 1) {
            return bit_action::FLIP;
        } else {
            return bit_action::NO_ACTION;
        }
    }
};

struct op_not {
    template <typename T>
    static auto apply(T a) {
        return ~a;
    }
};

struct vector_int_factory;

template <typename vector_store_type, int bits>
class vector_int {
  public:
    template <typename T, int N>
    friend class vector_int;

    friend class vector_int_factory;
    
    using store_t = repeated_tuple_t<vector_store_type, bits>;
    static constexpr int width_in_bits = sizeof(vector_store_type) * 8;

    template <int other_bits>
    using vector_int_higher_precision_t = vector_int<vector_store_type, (bits > other_bits ? bits : other_bits)>;
    
    static std::string type_info() {
        return "vector_int<" + std::to_string(bits) + ">";
    }

    template <typename val_t>
    void set_at(int index, val_t value) {
        if (index > width_in_bits) {
            throw std::out_of_range("Index out of range");
        }
        
        for_each_bit([&]<std::size_t bit_idx>() {            
            auto ith_bit = (value >> bit_idx) & 1;

            auto old_value = std::get<bit_idx>(numbers);
            auto new_value = (old_value & ~(1 << index)) | (ith_bit << index);
            
            std::get<bit_idx>(numbers) = new_value;
        });
    }

    template <typename val_t = int>
    val_t get_at(int index) const {
        if (index > width_in_bits) {
            throw std::out_of_range("Index out of range");
        }

        val_t value = 0;
        
        for_each_bit([&]<std::size_t bit_idx>() {
            auto word = std::get<bit_idx>(numbers);
            auto bit = (word >> index) & 1;
            
            value |= (bit << bit_idx);
        });

        return value;
    }

    std::string to_str() const {
        std::string str;

        for (std::size_t i = 0; i < width_in_bits; ++i) {
            str += std::to_string(get_at(i)) + " ";
        }
        
        return str;
    }

    template <int other_bits>
    vector_int_higher_precision_t<other_bits> get_added(
        vector_int<vector_store_type, other_bits> other) const {

        constexpr int res_bits = (bits > other_bits ? bits : other_bits);
        constexpr int min_bits = (bits < other_bits ? bits : other_bits);

        vector_int<vector_store_type, res_bits> result;

        std::get<0>(result.numbers) = std::get<0>(numbers) ^ std::get<0>(other.numbers); 
        vector_store_type carry = std::get<0>(numbers) & std::get<0>(other.numbers);

        for_each_in<min_bits - 1>([&]<std::size_t i>() {
            constexpr auto next_bit_idx = i + 1;

            // Get bits from both vectors at the current position
            auto a = std::get<next_bit_idx>(numbers);
            auto b = std::get<next_bit_idx>(other.numbers);
            
            // XOR the bits and XOR with carry for the result
            auto bit_xor = a ^ b;
            std::get<next_bit_idx>(result.numbers) = bit_xor ^ carry;
            
            // Calculate new carry: (a & b) | (carry & (a ^ b))
            carry = (a & b) | (carry & bit_xor);
        });

        if constexpr (other_bits > bits) {
            // Fix: Properly propagate carry through all bits of the larger vector
            for_each_in<other_bits - bits>([&]<std::size_t i>() {
                constexpr auto bit_idx = min_bits + i;
                
                // Get bit from the larger vector
                auto a = std::get<bit_idx>(other.numbers);
                
                // XOR with carry for the result
                std::get<bit_idx>(result.numbers) = a ^ carry;
                
                // Update carry - if both the bit and current carry are 1, we need a new carry
                carry = carry & a;
            });
        } else if constexpr (other_bits < bits) {
            // Fix: Same correction for when the first vector is larger
            for_each_in<bits - other_bits>([&]<std::size_t i>() {
                constexpr auto bit_idx = min_bits + i;
                
                // Get bit from the larger vector
                auto a = std::get<bit_idx>(numbers);
                
                // XOR with carry for the result
                std::get<bit_idx>(result.numbers) = a ^ carry;
                
                // Update carry
                carry = carry & a;
            });
        }

        return result;
    }
    
    template <int other_bits>
    vector_int_higher_precision_t<other_bits> get_ored(
        vector_int<vector_store_type, other_bits> other) const {
        
        return get_oped<other_bits, op_or>(other);
    }

    template <int other_bits>
    vector_int_higher_precision_t<other_bits> get_xored(
        vector_int<vector_store_type, other_bits> other) const {
        
        return get_oped<other_bits, op_xor>(other);
    }

    template <int other_bits>
    vector_int_higher_precision_t<other_bits> get_anded(
        vector_int<vector_store_type, other_bits> other) const {
        
        return get_oped<other_bits, op_and>(other);
    }

    template <int target_bits>
    vector_int<vector_store_type, target_bits> to_vector_with_bits() const {
        vector_int<vector_store_type, target_bits> result;
        constexpr auto min_bits = (target_bits < bits ? target_bits : bits);

        for_each_in<min_bits>([&]<std::size_t i>() {
            auto word = std::get<i>(numbers);
            std::get<i>(result.numbers) = word;
        });

        return result;
    }

    vector_int<vector_store_type, bits> get_right_shifted_vector(int shift) const {
        return get_shifted_vector<op_shift_right>(shift);
    }

    vector_int<vector_store_type, bits> get_left_shifted_vector(int shift) const {
        return get_shifted_vector<op_shift_left>(shift);
    }

    vector_int<vector_store_type, bits> get_noted() const {
        return get_oped<op_not>();
    }

    template <int constant>
    vector_int<vector_store_type, bits> get_anded() const {
        return get_oped<op_and, constant>();
    }

    template <int constant>
    vector_int<vector_store_type, bits> get_ored() const {
        return get_oped<op_or, constant>();
    }

    template <int constant>
    vector_int<vector_store_type, bits> get_xored() const {
        return get_oped<op_xor, constant>();
    }

    template <int constant>
    vector_int<vector_store_type, bits> get_left_shifted_vector() const {
        return get_shifted_vector<op_shift_left, constant>();
    }

    template <int constant>
    vector_int<vector_store_type, bits> get_right_shifted_vector() const {
        return get_shifted_vector<op_shift_right, constant>();
    }

    template <typename tuple_of_pointers_storage_t>
    static vector_int<vector_store_type, bits> load_from(
        tuple_of_pointers_storage_t storage, std::size_t offset) {

        vector_int<vector_store_type, bits> result;

        constexpr auto storage_size = std::tuple_size_v<tuple_of_pointers_storage_t>;
        constexpr auto loaded_bits = std::min<int>(storage_size, bits);
        
        for_each_in<loaded_bits>([&]<std::size_t bit_idx>() {
            auto ptr_to_ith_storage = std::get<bit_idx>(storage);
            std::get<bit_idx>(result.numbers) = ptr_to_ith_storage[offset];
        });

        return result;
    }

    template <int other_bits>
    vector_store_type equals_to(
        vector_int<vector_store_type, other_bits> other) const {
        
        constexpr int min_bits = (bits < other_bits ? bits : other_bits);
        vector_store_type result = constats_ops<vector_store_type>::ones;

        for_each_in<min_bits>([&]<std::size_t i>() {
            auto a = std::get<i>(numbers);
            auto b = std::get<i>(other.numbers);
            
            result &= ~(a ^ b);
        });

        if constexpr (other_bits < bits) {
            for_each_in<bits - other_bits>([&]<std::size_t i>() {
                auto a = std::get<min_bits + i>(numbers);
                result &= ~a;
            });
        }   
        else if constexpr (other_bits > bits) {
            for_each_in<other_bits - bits>([&]<std::size_t i>() {
                auto a = std::get<min_bits + i>(other.numbers);
                result &= ~a;
            });
        }

        return result;
    }

    template <int constant>
    vector_store_type equals_to() const {
        constexpr auto constat_bits 
            = constats_ops<int>::get_highest_set_bit<constant>();
        
        vector_store_type result = constats_ops<vector_store_type>::ones;

        for_each_in<bits>([&]<std::size_t i>() {
            auto a = std::get<i>(numbers);

            constexpr auto constant_is_set = 
                constats_ops<vector_store_type>::template is_set_at<constant>(i);
            
            if constexpr (constant_is_set) {
                result &= a;
            } else {
                result &= ~a;
            }
        });

        return result;
    }

  private:
    store_t numbers;
    
    template <typename Callback, std::size_t... Is>
    static void for_each_bit_impl(Callback&& cb, std::index_sequence<Is...>) {
        (cb.template operator()<Is>(), ...);
    }

    template <typename Callback>
    static void for_each_bit(Callback&& cb) {
        for_each_bit_impl(std::forward<Callback>(cb), std::make_index_sequence<bits>{});
    }

    template <int count, typename Callback>
    static void for_each_in(Callback&& cb) {
        for_each_bit_impl(std::forward<Callback>(cb), std::make_index_sequence<count>{});
    }

    template <typename shift_op_t>
    vector_int<vector_store_type, bits> get_shifted_vector(int shift) const {
        vector_int<vector_store_type, bits> result;

        for_each_in<bits>([&]<std::size_t i>() {
            auto word = std::get<i>(numbers);
            auto shifted_word = shift_op_t::apply(word, shift); 

            std::get<i>(result.numbers) = shifted_word;
        });

        return result;
    }

    template <bit_action action>
    vector_store_type apply_action(vector_store_type word) const {
        if constexpr (action == bit_action::SET_ZERO) {
            return 0;
        } else if constexpr (action == bit_action::SET_ONE) {
            return constats_ops<vector_store_type>::ones;
        } else if constexpr (action == bit_action::FLIP) {
            return ~word;
        } else if constexpr (action == bit_action::NO_ACTION) {
            return word;
        } else {
            static_assert("Unknown action for bit operation");
        }
    }

    template <int other_bits, typename op_t>
    vector_int_higher_precision_t<other_bits> get_oped(
        vector_int<vector_store_type, other_bits> other) const {

        constexpr int min_bits = (bits < other_bits ? bits : other_bits);
        constexpr int max_bits = (bits > other_bits ? bits : other_bits);

        vector_int<vector_store_type, max_bits> result;

        for_each_in<min_bits>([&]<std::size_t i>() {
            auto a = std::get<i>(numbers);
            auto b = std::get<i>(other.numbers);
            
            std::get<i>(result.numbers) = op_t::apply(a, b);
        });

        if constexpr (other_bits < bits) {
            for_each_in<bits - min_bits>([&]<std::size_t i>() {
                constexpr auto action = op_t::template action_for_bit<int, 0>(0);
                
                auto a = std::get<min_bits + i>(numbers);
                std::get<min_bits + i>(result.numbers) = apply_action<action>(a);
            });
        } else if constexpr (other_bits > bits) {
            for_each_in<other_bits - min_bits>([&]<std::size_t i>() {
                constexpr auto action = op_t::template action_for_bit<int, 0>(0);
                
                auto a = std::get<min_bits + i>(other.numbers);
                std::get<min_bits + i>(result.numbers) = apply_action<action>(a);
            });
        }

        return result;
    }

    template <typename op_t>
    vector_int<vector_store_type, bits> get_oped() const {
        vector_int<vector_store_type, bits> result;

        for_each_in<bits>([&]<std::size_t i>() {
            auto word = std::get<i>(numbers);
            auto oped_word = op_t::apply(word); 

            std::get<i>(result.numbers) = oped_word;
        });

        return result;   
    }

    template <typename op_t, int constant>
    vector_int<vector_store_type, bits> get_oped() const {
        vector_int<vector_store_type, bits> result;

        for_each_in<bits>([&]<std::size_t i>() {
            constexpr auto action = op_t::template action_for_bit<int, constant>(i);

            auto word = std::get<i>(numbers);
            std::get<i>(result.numbers) = apply_action<action>(word);
        });

        return result;
    }
};

struct vector_int_factory {
    template <typename vector_store_type, int constant>
    static auto from_constant() {
        constexpr auto bits 
            = constats_ops<int>::get_highest_set_bit<constant>() + 1;
    
        if constexpr (bits == 0) {
            return vector_int<vector_store_type, 1>{};
        }

        vector_int<vector_store_type, bits + 1> result;

        for_each_in<bits + 1>([&]<std::size_t bit_idx>() {
            constexpr auto is_set 
                = constats_ops<vector_store_type>::template is_set_at<constant>(bit_idx);

            if constexpr (is_set) {
                std::get<bit_idx>(result.numbers) 
                    = constats_ops<vector_store_type>::ones;
            } else {
                std::get<bit_idx>(result.numbers) = 0;
            }
        });

        return result;
    }

  private:
    template <typename Callback, std::size_t... Is>
    static void for_each_bit_impl(Callback&& cb, std::index_sequence<Is...>) {
        (cb.template operator()<Is>(), ...);
    }

    template <int count, typename Callback>
    static void for_each_in(Callback&& cb) {
        for_each_bit_impl(std::forward<Callback>(cb), std::make_index_sequence<count>{});
    }
};

}

#endif // CELLULAR_BIT_MODE_HPP