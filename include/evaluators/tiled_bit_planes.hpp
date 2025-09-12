#ifndef CELLATO_EVALUATORS_TILED_BIT_PLANES_HPP
#define CELLATO_EVALUATORS_TILED_BIT_PLANES_HPP

#include <array>
#include <vector>
#include <cstddef>
#include <stdexcept>
#include <utility>
#include <iostream>
#include <cstdint>
#include <algorithm>

#include "./bit_planes.hpp"

#include "../core/ast.hpp"
#include "../core/vector_int.hpp"
#include "../memory/bit_planes_grid.hpp"
#include "../memory/grid_utils.hpp"
#include "../memory/interface.hpp"

// Use the same CUDA_CALLABLE definition as in standard evaluator
#ifdef __CUDACC__
#define CUDA_CALLABLE __host__ __device__
#else
#define CUDA_CALLABLE
#endif

namespace cellato::evaluators::tiled_bit_planes {

using namespace cellato::ast;
using namespace cellato::core::bitwise;
using namespace cellato::memory::grids::utils;

template <typename cell_row_type, typename state_dictionary_type, template <typename, typename> class recursive_evaluator>
struct implementation_params {
    using cell_row_t = cell_row_type;
    using state_dict_t = state_dictionary_type;
    
    template <typename params, typename Expression>
    using evaluator_t = recursive_evaluator<params, Expression>;
};

template <typename params, typename Expression>
struct _evaluator_impl;

template <typename cell_row_type, typename state_dictionary_type, typename Expression>
using _simple_bit_planes_evaluator_implementation = cellato::evaluators::bit_planes::_evaluator_impl<
    implementation_params<cell_row_type, state_dictionary_type, _evaluator_impl>, Expression>;

template <typename cell_row_type, typename state_dictionary_type>
using grid_cell_data_type = std::array<cell_row_type*, state_dictionary_type::needed_bits>;

template <typename params>
using state_t = cellato::memory::grids::point_in_grid<
    grid_cell_data_type<typename params::cell_row_t, typename params::state_dict_t>>;

template <typename params, typename Expression>
struct _evaluator_impl {
    using eval_state_t = state_t<params>;

    CUDA_CALLABLE static auto evaluate(eval_state_t state) {
        // all but the 'neighbor_at' part is same as bit_planes
        return _simple_bit_planes_evaluator_implementation<typename params::cell_row_t, typename params::state_dict_t, Expression>::evaluate(state);
    }
};

template <typename cell_row_type,  typename state_dictionary_type, typename Expression>
using evaluator = _evaluator_impl<
    implementation_params<
        cell_row_type, state_dictionary_type, _evaluator_impl>,
    Expression>;


// Partial specialization for neighbor_at
template <typename params, int x_offset, int y_offset>
struct _evaluator_impl<params, neighbor_at<x_offset, y_offset>> {
    static constexpr int vector_width_bits = sizeof(typename params::cell_row_t) * 8;

    using eval_state_t = state_t<params>;
    using cell_row_type = typename params::cell_row_t;
    using state_dictionary_type = typename params::state_dict_t;

    constexpr static std::size_t x_offset_unsigned = static_cast<std::size_t>(x_offset);
    constexpr static std::size_t y_offset_unsigned = static_cast<std::size_t>(y_offset);

    CUDA_CALLABLE static auto evaluate(eval_state_t state) {
        // this specialization is never called for some reason
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

    CUDA_CALLABLE static vint shift_center(vint center) {
        if constexpr (x_offset > 0) {
            return center.template get_right_shifted_vector<x_offset>();
        } else if constexpr (x_offset < 0) {
            return center.template get_left_shifted_vector<-x_offset>();
        } else {
            #ifndef __CUDA_ARCH__
            throw std::logic_error("Invalid x_offset value");
            #else
            // In CUDA device code, we can't throw exceptions
            // Just return the unshifted center as a fallback
            return center;
            #endif
        }
    }

    CUDA_CALLABLE static vint shift_neighbor(vint neighbor) {
        if constexpr (x_offset > 0) {
            return neighbor.template get_left_shifted_vector<vector_width_bits - x_offset>();
        } else if constexpr (x_offset < 0) {
            return neighbor.template get_right_shifted_vector<vector_width_bits + x_offset>();
        } else {
            #ifndef __CUDA_ARCH__
            throw std::logic_error("Invalid x_offset value");
            #else
            // In CUDA device code, we can't throw exceptions
            return neighbor;
            #endif
        }
    }

    CUDA_CALLABLE static vint get_center_vector_int(eval_state_t state) {
        auto x = state.position.x;
        auto y = state.position.y;
        auto idx = state.properties.idx(x, y + y_offset_unsigned);

        return vector_int_factory::load_from<cell_row_type>(state.grid, idx);
    }

    CUDA_CALLABLE static vint get_neighbor_vector_int(eval_state_t state) {
        auto x = state.position.x;
        auto y = state.position.y;
        auto idx = state.properties.idx(x + x_offset_unsigned, y + y_offset_unsigned);

        return vector_int_factory::load_from<cell_row_type>(state.grid, idx);
    }
};

} // namespace cellato::evaluators::tiled_bit_planes

#endif // CELLATO_EVALUATORS_TILED_BIT_PLANES_HPP