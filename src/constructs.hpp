#ifndef CELLULAR_AUTO_CONSTRUCTS_HPP
#define CELLULAR_AUTO_CONSTRUCTS_HPP

#include <tuple>
#include <vector>
#include <cstddef>
#include <utility>
#include <iostream>
#include <string>

namespace expr_tree {

template <typename value_t, value_t Value>
struct constant {
    using type = value_t;
    static constexpr value_t value = Value;
};

template <typename state_t, state_t Value>
struct state_constant {
    using type = state_t;
    static constexpr state_t value = Value;
};

struct current_state {};

template <int x_offset_val, int y_offset_val>
struct neighbor_at {
    static constexpr int x_offset = x_offset_val;
    static constexpr int y_offset = y_offset_val;
};

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
    static const_type evaluate(state_t<cell_type> /* state */) {
        return Value;
    }
};

template <typename cell_type, typename state_type, state_type Value>
struct evaluator<cell_type, state_constant<state_type, Value>> {
    static state_type evaluate(state_t<cell_type> /* state */) {
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

namespace iterators {

template <typename config_t>
class simple_grid_iterator {
    using algorithm_t = typename config_t::algorithm_t;
    using cell_state_t = typename config_t::cell_state_t;
    using print_config_t = typename config_t::print_config_t;

public:
    simple_grid_iterator() : _height(0), _width(0), _padded_height(0), _padded_width(0), steps_(0) {}

    void init(const std::vector<cell_state_t>& grid, std::size_t height, std::size_t width) {
        _height = height;
        _width = width;

        // Create padded grid with a border of 1 cell
        _padded_height = height + 2;
        _padded_width = width + 2;

        input_grid = create_padded_grid(grid, height, width);
        intermediate_grid.resize(_padded_height * _padded_width);
        std::copy(input_grid.begin(), input_grid.end(), intermediate_grid.begin());

        final_grid = &input_grid;
    }

    template <bool print = false>
    void run(int steps) {
        steps_ = steps;

        for (int step = 0; step < steps_; ++step) {
            // Process cells (skip border)
            for (std::size_t y = 1; y < _padded_height - 1; ++y) {
                for (std::size_t x = 1; x < _padded_width - 1; ++x) {
                    // Use simple evaluator
                    simple_evaluator::grid_config<cell_state_t> state;
                    state.grid = input_grid.data();
                    state.height = _padded_height;
                    state.width = _padded_width;
                    state.x = x;
                    state.y = y;

                    // Evaluate using simple evaluator
                    intermediate_grid[y * _padded_width + x] =
                        simple_evaluator::evaluator<cell_state_t, algorithm_t>::evaluate(state);
                }
            }

            if constexpr (print) {
                std::cout << "Step " << step + 1 << ":\n";
                print_grid(intermediate_grid);
            }

            std::swap(input_grid, intermediate_grid);
        }

        final_grid = &input_grid;
    }

    std::vector<cell_state_t> get_result() const {
        // Extract the non-padded part of the grid
        std::vector<cell_state_t> result(_height * _width);

        for (std::size_t y = 0; y < _height; ++y) {
            for (std::size_t x = 0; x < _width; ++x) {
                result[y * _width + x] = (*final_grid)[(y + 1) * _padded_width + (x + 1)];
            }
        }

        return result;
    }

    void print_current_grid() const {
        print_grid(*final_grid);
    }

private:
    std::vector<cell_state_t> input_grid;
    std::vector<cell_state_t> intermediate_grid;
    std::vector<cell_state_t>* final_grid;

    std::size_t _height;
    std::size_t _width;
    std::size_t _padded_height;
    std::size_t _padded_width;
    int steps_;

    std::vector<cell_state_t> create_padded_grid(
        const std::vector<cell_state_t>& grid, std::size_t height, std::size_t width) {

        std::vector<cell_state_t> padded_grid((_padded_height) * (_padded_width));

        // Fill padded grid with default values (assuming first enum value is the default)
        std::fill(padded_grid.begin(), padded_grid.end(), static_cast<cell_state_t>(0));

        // Copy the inner grid
        for (std::size_t y = 0; y < height; ++y) {
            for (std::size_t x = 0; x < width; ++x) {
                padded_grid[(y + 1) * _padded_width + (x + 1)] = grid[y * width + x];
            }
        }

        return padded_grid;
    }

    void print_grid(const std::vector<cell_state_t>& grid) const {
        // Print the inner grid (non-padded part)
        for (std::size_t y = 1; y < _padded_height - 1; ++y) {
            for (std::size_t x = 1; x < _padded_width - 1; ++x) {
                auto cell = grid[y * _padded_width + x];
                std::cout << print_config_t::get_str(cell) << " ";
            }
            std::cout << "\n";
        }
    }
};

}

#endif // CELLULAR_AUTO_CONSTRUCTS_HPP