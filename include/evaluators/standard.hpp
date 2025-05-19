#ifndef CELLIB_STANDARD_EVALUATORS_HPP
#define CELLIB_STANDARD_EVALUATORS_HPP

#include "../core/ast.hpp"
#include "../memory/interface.hpp"

#ifdef __CUDACC__
#define CUDA_CALLABLE __host__ __device__
#else
#define CUDA_CALLABLE
#endif

namespace cellib::evaluators::standard {

using namespace cellib::ast;
using namespace cellib::memory;

template <typename cell_type, typename Expression>
struct evaluator {};

template <typename cell_type>
using state_t = grids::point_in_grid<cell_type*>;

template <typename cell_type, typename const_type, const_type Value>
struct evaluator<cell_type, constant<const_type, Value>> {
    static CUDA_CALLABLE const_type evaluate(state_t<cell_type> /* state */) {
        return Value;
    }
};

template <typename cell_type, typename state_type, state_type Value>
struct evaluator<cell_type, state_constant<state_type, Value>> {
    static CUDA_CALLABLE state_type evaluate(state_t<cell_type> /* state */) {
        return Value;
    }
};

template <typename cell_type, typename Condition, typename Then, typename Else>
struct evaluator<cell_type, if_then_else<Condition, Then, Else>> {
    static CUDA_CALLABLE cell_type evaluate(state_t<cell_type> state) {
        if (evaluator<cell_type, Condition>::evaluate(state)) {
            return evaluator<cell_type, Then>::evaluate(state);
        } else {
            return evaluator<cell_type, Else>::evaluate(state);
        }
    }
};

template <typename cell_type, typename Left, typename Right>
struct evaluator<cell_type, and_<Left, Right>> {
    static CUDA_CALLABLE bool evaluate(state_t<cell_type> state) {
        return evaluator<cell_type, Left>::evaluate(state) && evaluator<cell_type, Right>::evaluate(state);
    }
};

template <typename cell_type, typename Left, typename Right>
struct evaluator<cell_type, or_<Left, Right>> {
    static CUDA_CALLABLE bool evaluate(state_t<cell_type> state) {
        return evaluator<cell_type, Left>::evaluate(state) || evaluator<cell_type, Right>::evaluate(state);
    }
};

template <typename cell_type, typename Left, typename Right>
struct evaluator<cell_type, equals<Left, Right>> {
    static CUDA_CALLABLE bool evaluate(state_t<cell_type> state) {
        return evaluator<cell_type, Left>::evaluate(state) == evaluator<cell_type, Right>::evaluate(state);
    }
};

template <typename cell_type, typename Left, typename Right>
struct evaluator<cell_type, greater_than<Left, Right>> {
    static CUDA_CALLABLE bool evaluate(state_t<cell_type> state) {
        return evaluator<cell_type, Left>::evaluate(state) > evaluator<cell_type, Right>::evaluate(state);
    }
};

template <typename cell_type, typename Left, typename Right>
struct evaluator<cell_type, not_equals<Left, Right>> {
    static CUDA_CALLABLE bool evaluate(state_t<cell_type> state) {
        return evaluator<cell_type, Left>::evaluate(state) != evaluator<cell_type, Right>::evaluate(state);
    }
};

template <typename cell_type, int x_offset, int y_offset>
struct evaluator<cell_type, neighbor_at<x_offset, y_offset>> {
    static CUDA_CALLABLE cell_type evaluate(state_t<cell_type> state) {
        return state.grid[(state.position.x + x_offset) + (state.position.y + y_offset) * state.properties.x_size];
    }
};

template <typename cell_type, typename CellStateValue>
struct evaluator<cell_type, count_neighbors<CellStateValue, moore_8_neighbors>> {
    static CUDA_CALLABLE int evaluate(state_t<cell_type> state) {
        int sum = 0;
        for (int dx = -1; dx <= 1; ++dx) {
            for (int dy = -1; dy <= 1; ++dy) {
                if (dx == 0 && dy == 0) continue;

                int nx = state.position.x + dx;
                int ny = state.position.y + dy;

                auto state_at_nxy = state.grid[nx + ny * state.properties.x_size];

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
    static CUDA_CALLABLE int evaluate(state_t<cell_type> state) {
        int sum = 0;

        const int dx[] = {0, 0, 1, -1};
        const int dy[] = {1, -1, 0, 0};

        for (int i = 0; i < 4; ++i) {
            int nx = state.position.x + dx[i];
            int ny = state.position.y + dy[i];

            auto state_at_nxy = state.grid[nx + ny * state.properties.x_size];

            if (state_at_nxy == evaluator<cell_type, CellStateValue>::evaluate(state)) {
                sum += 1;
            }
        }
        return sum;
    }
};

} // namespace cellib::evaluators::standard

#endif // CELLIB_STANDARD_EVALUATORS_HPP