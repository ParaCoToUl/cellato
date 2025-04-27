#ifndef BIT_EVALUATOR_HPP
#define BIT_EVALUATOR_HPP

#include "bit-mode.hpp"
#include "constructs.hpp"

namespace bitwise_no_cache {

using namespace expr_tree;
using namespace bitwise;

template <typename cell_row_type, typename state_dictionary_type, typename Expression>
struct evaluator {};

template <typename cell_row_type, typename state_dictionary_type>
struct grid_config {
    using cell_row_t = cell_row_type;
    using state_dictionary_t = state_dictionary_type;
    static constexpr int bits = state_dictionary_t::needed_bits;

    repeated_tuple_t<cell_row_t*, bits> bit_grid;
    
    std::size_t height_b;
    std::size_t width_b;

    std::size_t x, y;
};

template <typename cell_row_type, typename state_dictionary_type>
using state_t = grid_config<cell_row_type, state_dictionary_type>;
 
template <typename cell_row_type, typename state_dictionary_type, typename const_type, const_type Value>
struct evaluator<cell_row_type, state_dictionary_type, constant<const_type, Value>> {
    static auto evaluate(state_t<cell_row_type, state_dictionary_type> state) {
        return vector_int_factory::from_constant<cell_row_type, Value>();
    }
};

template <typename cell_row_type, typename state_dictionary_type, typename state_type, state_type Value>
struct evaluator<cell_row_type, state_dictionary_type, state_constant<state_type, Value>> {
    static auto evaluate(state_t<cell_row_type, state_dictionary_type> state) {
        constexpr auto index = state_dictionary_type::state_to_index(Value);
        return vector_int_factory::from_constant<cell_row_type, index>();
    }
};

template <typename cell_row_type, typename state_dictionary_type, typename Condition, typename Then, typename Else>
struct evaluator<cell_row_type, state_dictionary_type, if_then_else<Condition, Then, Else>> {

    template <typename E>
    using evaluator_t = evaluator<cell_row_type, state_dictionary_type, E>;

    static auto evaluate(state_t<cell_row_type, state_dictionary_type> state) {
        auto condition = evaluator_t<Condition>::evaluate(state);
        auto then_part = evaluator_t<Then>::evaluate(state);
        auto else_part = evaluator_t<Else>::evaluate(state);

        auto masked_then = then_part.mask_out_columns(condition);
        auto masked_else = else_part.mask_out_columns(~condition);

        return masked_then.get_ored(masked_else);
    }
};

template <typename cell_row_type, typename state_dictionary_type, typename Left, typename Right>
struct evaluator<cell_row_type, state_dictionary_type, and_<Left, Right>> {

    template <typename E>
    using evaluator_t = evaluator<cell_row_type, state_dictionary_type, E>;

    static cell_row_type evaluate(state_t<cell_row_type, state_dictionary_type> state) {
        cell_row_type left = evaluator_t<Left>::evaluate(state);
        cell_row_type right = evaluator_t<Right>::evaluate(state);

        return left & right;
    }
};

template <typename cell_row_type, typename state_dictionary_type, typename Left, typename Right>
struct evaluator<cell_row_type, state_dictionary_type, or_<Left, Right>> {
    
    template <typename E>
    using evaluator_t = evaluator<cell_row_type, state_dictionary_type, E>;

    static cell_row_type evaluate(state_t<cell_row_type, state_dictionary_type> state) {
        cell_row_type left = evaluator_t<Left>::evaluate(state);
        cell_row_type right = evaluator_t<Right>::evaluate(state);

        return left | right;
    }
};

template <typename cell_row_type, typename state_dictionary_type, typename Left, typename Right>
struct evaluator<cell_row_type, state_dictionary_type, equals<Left, Right>> {
    
    template <typename E>
    using evaluator_t = evaluator<cell_row_type, state_dictionary_type, E>;

    static auto evaluate(state_t<cell_row_type, state_dictionary_type> state) {
        auto left = evaluator_t<Left>::evaluate(state);
        auto right = evaluator_t<Right>::evaluate(state);

        return left.equals_to(right);
    }
};

template <typename cell_row_type, typename state_dictionary_type, typename Left, typename Right>
struct evaluator<cell_row_type, state_dictionary_type, greater_than<Left, Right>> {
    static cell_row_type evaluate(state_t<cell_row_type, state_dictionary_type> state) {
        return 0; // todo
    }
};

template <typename cell_row_type, typename state_dictionary_type, typename Left, typename Right>
struct evaluator<cell_row_type, state_dictionary_type, not_equals<Left, Right>> {
    
    template <typename E>
    using evaluator_t = evaluator<cell_row_type, state_dictionary_type, E>;

    static bool evaluate(state_t<cell_row_type, state_dictionary_type> state) {
        auto left = evaluator_t<Left>::evaluate(state);
        auto right = evaluator_t<Right>::evaluate(state);

        return left.not_equal_to(right);
    }
};

template <typename cell_row_type, typename state_dictionary_type>
struct evaluator<cell_row_type, state_dictionary_type, current_state> {
    static auto evaluate(state_t<cell_row_type, state_dictionary_type> state) {
        auto offset = state.x + state.y * state.width_b;
        return vector_int_factory::load_from<cell_row_type>(state.bit_grid, offset);
    }
};

template <typename cell_row_type, typename state_dictionary_type, int x_offset, int y_offset>
struct evaluator<cell_row_type, state_dictionary_type, neighbor_at<x_offset, y_offset>> {
    static constexpr int vector_width_bits = sizeof(cell_row_type) * 8;

    using state_t = grid_config<cell_row_type, state_dictionary_type>;

    static auto evaluate(state_t state) {
        auto center = get_center_vector_int(state);

        if constexpr (x_offset == 0) {
            return center;
        } 

        auto neighbor = get_neighbor_vector_int(state);      

        auto shifted_center = shift_center(center);
        auto shifted_neighbor = shift_neighbor(neighbor);

        return shifted_center.get_ored(shifted_neighbor);
    }

  private:
    using vint = vector_int<cell_row_type, state_dictionary_type::needed_bits>;

    static vint shift_center(vint center) {
        if constexpr (x_offset > 0) {
            return center.template get_right_shifted_vector<x_offset>();
        } else if constexpr (x_offset < 0) {
            return center.template get_left_shifted_vector<-x_offset>();
        } else {
            throw std::logic_error("Invalid x_offset value");
        }
    }
    
    static vint shift_neighbor(vint neighbor) {
        if constexpr (x_offset > 0) {
            return neighbor.template get_left_shifted_vector<vector_width_bits - x_offset>();
        } else if constexpr (x_offset < 0) {
            return neighbor.template get_right_shifted_vector<vector_width_bits + x_offset>();
        } else {
            throw std::logic_error("Invalid x_offset value");
        }
    }

    static vint get_center_vector_int(state_t state) {
        auto idx = (state.y + y_offset) * state.width_b + state.x;
        return vector_int_factory::load_from<cell_row_type>(state.bit_grid, idx);
    }

    static vint get_neighbor_vector_int(state_t state) {
        auto idx = (state.y + y_offset) * state.width_b + (state.x + x_offset);
        return vector_int_factory::load_from<cell_row_type>(state.bit_grid, idx);
    }
};

template <typename cell_row_type, typename state_dictionary_type, typename cell_state_type, cell_state_type CellStateValue>
struct evaluator<
    cell_row_type, state_dictionary_type,
    count_neighbors<
        state_constant<cell_state_type, CellStateValue>,
        moore_8_neighbors>> {

    template <typename E>
    using evaluator_t = evaluator<cell_row_type, state_dictionary_type, E>;

    static vector_int<cell_row_type, 4> evaluate(state_t<cell_row_type, state_dictionary_type> state) {
        constexpr auto cell_state = state_dictionary_type::state_to_index(CellStateValue);

        auto top_left_c     = evaluator_t<neighbor_at<-1, -1>>::evaluate(state).template equals_to<cell_state>();
        auto top_c          = evaluator_t<neighbor_at< 0, -1>>::evaluate(state).template equals_to<cell_state>();
        auto top_right_c    = evaluator_t<neighbor_at< 1, -1>>::evaluate(state).template equals_to<cell_state>();
        auto left_c         = evaluator_t<neighbor_at<-1,  0>>::evaluate(state).template equals_to<cell_state>();
        auto right_c        = evaluator_t<neighbor_at< 1,  0>>::evaluate(state).template equals_to<cell_state>();
        auto bottom_left_c  = evaluator_t<neighbor_at<-1,  1>>::evaluate(state).template equals_to<cell_state>();
        auto bottom_c       = evaluator_t<neighbor_at< 0,  1>>::evaluate(state).template equals_to<cell_state>();
        auto bottom_right_c = evaluator_t<neighbor_at< 1,  1>>::evaluate(state).template equals_to<cell_state>();

        auto top_left       = vector_int_factory::from_condition_result<cell_row_type>(top_left_c);
        auto top            = vector_int_factory::from_condition_result<cell_row_type>(top_c);
        auto top_right      = vector_int_factory::from_condition_result<cell_row_type>(top_right_c);
        auto left           = vector_int_factory::from_condition_result<cell_row_type>(left_c);
        auto right          = vector_int_factory::from_condition_result<cell_row_type>(right_c);
        auto bottom_left    = vector_int_factory::from_condition_result<cell_row_type>(bottom_left_c);
        auto bottom         = vector_int_factory::from_condition_result<cell_row_type>(bottom_c);
        auto bottom_right   = vector_int_factory::from_condition_result<cell_row_type>(bottom_right_c);

        return top_left.template to_vector_with_bits<2>()
            .get_added(top)
            .get_added(top_right).template to_vector_with_bits<3>()
            .get_added(left)
            .get_added(right)
            .get_added(bottom_left)
            .get_added(bottom).template to_vector_with_bits<4>()
            .get_added(bottom_right);
    }
};

template <typename cell_row_type, typename state_dictionary_type, typename CellStateValue>
struct evaluator<cell_row_type, state_dictionary_type, count_neighbors<CellStateValue, moore_4_neighbors>> {
    static vector_int<cell_row_type, 3> evaluate(state_t<cell_row_type, state_dictionary_type> state) {
        // todo
        return vector_int_factory::from_constant<cell_row_type, 8>();
        // auto top = evaluator<cell_row_type, state_dictionary_type, neighbor_at<0, -1>>::evaluate(state);
        // auto left = evaluator<cell_row_type, state_dictionary_type, neighbor_at<-1, 0>>::evaluate(state);
        // auto right = evaluator<cell_row_type, state_dictionary_type, neighbor_at<1, 0>>::evaluate(state);
        // auto bottom = evaluator<cell_row_type, state_dictionary_type, neighbor_at<0, 1>>::evaluate(state);

        // auto first_row = top.to_vector_with_bits<3>()
        //     .get_added(left).get_added(right).get_added(bottom);

        // return first_row;
    }
};

}

#endif // BIT_EVALUATOR_HPP