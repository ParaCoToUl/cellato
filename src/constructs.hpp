#ifndef CELLULAR_AUTO_CONSTRUCTS_HPP
#define CELLULAR_AUTO_CONSTRUCTS_HPP

#include <tuple>
#include <vector>
#include <cstddef>
#include <utility>
#include <iostream>

namespace expr_tree {

template <typename value_t, value_t Value>
struct constant {
    static constexpr value_t value = Value;
};

struct current_state {};

struct moore_8_neighbors {};
struct moore_4_neighbors {};

template <typename Constant, typename Neighborhood>
struct count_neighbors {
    using state_constant = Constant;
    using neighborhood = Neighborhood;
};

template <typename Condition, typename Then, typename Else>
struct if_then_else {
    using condition = Condition;
    using then_expr = Then;
    using else_expr = Else;
};

template <typename Left, typename Right>
struct and_ {
    using left = Left;
    using right = Right;
};

template <typename Left, typename Right>
struct or_ {
    using left = Left;
    using right = Right;
};

template <typename Left, typename Right>
struct equals {
    using left = Left;
    using right = Right;
};

template <typename Left, typename Right>
struct greater_than {
    using left = Left;
    using right = Right;
};

template <typename Left, typename Right>
struct not_equals {
    using left = Left;
    using right = Right;
};

template <typename... Args>
class __unpacked_if;

template <typename E>
class __unpacked_if<E> {
public:
    using nested_if_then_else = E;
};

template <typename C, typename T, typename... Chain>
class __unpacked_if<C, T, Chain...> {
public:
    using nested_if_then_else = if_then_else<
        C, T,
        typename __unpacked_if<Chain...>::nested_if_then_else>;
};

template <typename Condition, typename ...ChainOfThenElse>
struct if_ {
    template <typename Then>
    struct then_ {
        template <typename Else>
        using else_ = typename __unpacked_if<Condition, ChainOfThenElse..., Then, Else>::nested_if_then_else;

        template <typename ElseCondition>
        using elif_ = if_<Condition, ChainOfThenElse..., Then, ElseCondition>; 
    };
};


template <typename Left, template <typename, typename> class Operator, typename Right>
using p = Operator<Left, Right>;

namespace operations {

namespace highest_constant {
    template <typename T, typename U>
    struct of {
        static constexpr auto value = (T::value > U::value) ? T::value : U::value;
    };
}

}

}

namespace simple_evaluator {

using namespace expr_tree;

template <typename cell_type, typename Expression>
struct evaluator {};

template <typename cell_type>
struct grid_config {
    using cell_t = cell_type;

    cell_t* grid;
    
    std::size_t height;
    std::size_t width;

    std::size_t x, y;
};

template <typename cell_type>
using state_t = grid_config<cell_type>;
 
template <typename cell_type, typename const_type, const_type Value>
struct evaluator<cell_type, constant<const_type, Value>> {
    static const_type evaluate(state_t<cell_type> state) {
        return Value;
    }
};

template <typename cell_type, typename Condition, typename Then, typename Else>
struct evaluator<cell_type, if_then_else<Condition, Then, Else>> {
    static cell_type evaluate(state_t<cell_type> state) {
        if (evaluator<cell_type, Condition>::evaluate(state)) {
            return evaluator<cell_type, Then>::evaluate(state);
        } else {
            return evaluator<cell_type, Else>::evaluate(state);
        }
    }
};

template <typename cell_type, typename Left, typename Right>
struct evaluator<cell_type, and_<Left, Right>> {
    static bool evaluate(state_t<cell_type> state) {
        return evaluator<cell_type, Left>::evaluate(state) && evaluator<cell_type, Right>::evaluate(state);
    }
};

template <typename cell_type, typename Left, typename Right>
struct evaluator<cell_type, or_<Left, Right>> {
    static bool evaluate(state_t<cell_type> state) {
        return evaluator<cell_type, Left>::evaluate(state) || evaluator<cell_type, Right>::evaluate(state);
    }
};

template <typename cell_type, typename Left, typename Right>
struct evaluator<cell_type, equals<Left, Right>> {
    static bool evaluate(state_t<cell_type> state) {
        return evaluator<cell_type, Left>::evaluate(state) == evaluator<cell_type, Right>::evaluate(state);
    }
};

template <typename cell_type, typename Left, typename Right>
struct evaluator<cell_type, greater_than<Left, Right>> {
    static bool evaluate(state_t<cell_type> state) {
        return evaluator<cell_type, Left>::evaluate(state) > evaluator<cell_type, Right>::evaluate(state);
    }
};

template <typename cell_type, typename Left, typename Right>
struct evaluator<cell_type, not_equals<Left, Right>> {
    static bool evaluate(state_t<cell_type> state) {
        return evaluator<cell_type, Left>::evaluate(state) != evaluator<cell_type, Right>::evaluate(state);
    }
};

template <typename cell_type>
struct evaluator<cell_type, current_state> {
    static cell_type evaluate(state_t<cell_type> state) {
        return state.grid[state.x + state.y * state.width];
    }
};

template <typename cell_type, typename CellStateValue>
struct evaluator<cell_type, count_neighbors<CellStateValue, moore_8_neighbors>> {
    static int evaluate(state_t<cell_type> state) {
        int sum = 0;
        for (int dx = -1; dx <= 1; ++dx) {
            for (int dy = -1; dy <= 1; ++dy) {
                if (dx == 0 && dy == 0) continue;

                int nx = state.x + dx;
                int ny = state.y + dy;
                
                auto state_at_nxy = state.grid[nx + ny * state.width];

                if (state_at_nxy == evaluator<cell_type, CellStateValue>::evaluate(state)) {
                    sum += 1;
                }
            }
        }
        return sum;
    }
};

template <typename cell_type, typename CellStateValue>
struct evaluator<cell_type, count_neighbors<CellStateValue, moore_4_neighbors>> {
    static int evaluate(state_t<cell_type> state) {
        int sum = 0;

        const int dx[] = {0, 0, 1, -1};
        const int dy[] = {1, -1, 0, 0};
        
        for (int i = 0; i < 4; ++i) {
            int nx = state.x + dx[i];
            int ny = state.y + dy[i];
            
            auto state_at_nxy = state.grid[nx + ny * state.width];
            
            if (state_at_nxy == evaluator<cell_type, CellStateValue>::evaluate(state)) {
                sum += 1;
            }
        }
        return sum;
    }
};

}

#endif // CELLULAR_AUTO_CONSTRUCTS_HPP