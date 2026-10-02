#ifndef CELLATO_TEST_SUITES_HPP
#define CELLATO_TEST_SUITES_HPP

#include <cstdint>

#include "./run_params.hpp"
#include "cellato/config.hpp"
#include "cellato/evaluators/bit_array.hpp"
#include "cellato/evaluators/bit_planes.hpp"
#include "cellato/evaluators/standard.hpp"
#include "cellato/evaluators/tiled_bit_planes.hpp"
#include "cellato/memory/bit_array_grid.hpp"
#include "cellato/memory/bit_planes_grid.hpp"
#include "cellato/memory/standard_grid.hpp"
#include "cellato/memory/tiled_bit_planes_grid.hpp"
#include "cellato/traversers/cpu/simple.hpp"
#if CELLATO_ENABLE_CUDA
#include "cellato/traversers/cuda/simple.hpp"
#include "cellato/traversers/cuda/spatial_blocking.hpp"
#include "cellato/traversers/cuda/temporal.hpp"
#endif
#include "cellato/utils/type_list.hpp"

namespace cellato::run::test_suites {

namespace grids = cellato::memory::grids;
namespace evaluators = cellato::evaluators;

namespace detail {

inline constexpr const char* cpu_opt = "CPU";
inline constexpr const char* simple_opt = "simple";
#if CELLATO_ENABLE_CUDA
inline constexpr const char* cuda_opt = "CUDA";
inline constexpr const char* temporal_opt = "temporal";
inline constexpr const char* spatial_blocking_opt = "spatial_blocking";
#endif

struct standard_layout {
    template <typename cellular_automaton>
    struct bind {
        using original_cell_t = typename cellular_automaton::cell_state;
        using grid_store_word_t = original_cell_t;
        using algorithm_t = typename cellular_automaton::algorithm;

        using grid_t = grids::standard::grid<original_cell_t>;
        using evaluator_t = evaluators::standard::evaluator<original_cell_t, algorithm_t>;

        static constexpr const char* evaluator_name = "standard";
        static constexpr const char* layout_name = "standard";
        static constexpr int x_margin = 1;
        static constexpr int y_margin = 1;

        static bool word_size_matches(const cellato::run::run_params&) { return true; }
    };
};

template <typename store_word_type>
struct bit_array_layout {
    template <typename cellular_automaton>
    struct bind {
        using original_cell_t = typename cellular_automaton::cell_state;
        using grid_store_word_t = store_word_type;
        using algorithm_t = typename cellular_automaton::algorithm;
        using state_dictionary_t = typename cellular_automaton::state_dictionary;

        using grid_t = grids::bit_array::grid<state_dictionary_t, grid_store_word_t>;
        using evaluator_t = evaluators::bit_array::evaluator<grid_t, algorithm_t>;

        static constexpr const char* evaluator_name = "bit_array";
        static constexpr const char* layout_name = "bit_array";
        static constexpr int x_margin = grid_t::cells_per_word;
        static constexpr int y_margin = 1;

        static bool word_size_matches(const cellato::run::run_params& params) {
            return params.word_size == sizeof(store_word_type) * 8;
        }
    };
};

template <typename store_word_type>
struct bit_planes_layout {
    template <typename cellular_automaton>
    struct bind {
        using original_cell_t = typename cellular_automaton::cell_state;
        using grid_store_word_t = store_word_type;
        using algorithm_t = typename cellular_automaton::algorithm;
        using state_dictionary_t = typename cellular_automaton::state_dictionary;

        using grid_t = grids::bit_planes::grid<grid_store_word_t, state_dictionary_t>;
        using evaluator_t = evaluators::bit_planes::evaluator<grid_store_word_t, state_dictionary_t, algorithm_t>;

        static constexpr const char* evaluator_name = "bit_planes";
        static constexpr const char* layout_name = "bit_planes";
        static constexpr int x_margin = sizeof(grid_store_word_t) * 8;
        static constexpr int y_margin = 1;

        static bool word_size_matches(const cellato::run::run_params& params) {
            return params.word_size == sizeof(store_word_type) * 8;
        }
    };
};

template <typename store_word_type>
struct tiled_bit_planes_layout {
    template <typename cellular_automaton>
    struct bind {
        using original_cell_t = typename cellular_automaton::cell_state;
        using grid_store_word_t = store_word_type;
        using algorithm_t = typename cellular_automaton::algorithm;
        using state_dictionary_t = typename cellular_automaton::state_dictionary;

        using grid_t = grids::tiled_bit_planes::grid<grid_store_word_t, state_dictionary_t>;
        using evaluator_t = evaluators::tiled_bit_planes::evaluator<grid_store_word_t, state_dictionary_t, algorithm_t>;

        static constexpr const char* evaluator_name = "tiled_bit_planes";
        static constexpr const char* layout_name = "tiled_bit_planes";
        static constexpr int x_margin = grid_t::x_word_tile_size;
        static constexpr int y_margin = grid_t::y_word_tile_size;

        static bool word_size_matches(const cellato::run::run_params& params) {
            return params.word_size == sizeof(store_word_type) * 8;
        }
    };
};

struct cpu_simple_traverser {
    static constexpr const char* device_name = cpu_opt;
    static constexpr const char* traverser_name = simple_opt;

    template <typename evaluator_t, typename grid_t, typename>
    using type = cellato::traversers::cpu::simple::traverser<evaluator_t, grid_t>;

    static bool options_match(const cellato::run::run_params&) { return true; }
};

#if CELLATO_ENABLE_CUDA
struct cuda_simple_traverser {
    static constexpr const char* device_name = cuda_opt;
    static constexpr const char* traverser_name = simple_opt;

    template <typename evaluator_t, typename grid_t, typename>
    using type = cellato::traversers::cuda::simple::traverser<evaluator_t, grid_t>;

    static bool options_match(const cellato::run::run_params&) { return true; }
};

struct cuda_temporal_traverser {
    static constexpr const char* device_name = cuda_opt;
    static constexpr const char* traverser_name = temporal_opt;

    template <typename evaluator_t, typename grid_t, typename cellular_automaton>
    using type =
        cellato::traversers::cuda::temporal::traverser<evaluator_t, grid_t, cellular_automaton::average_halo_radius>;

    static bool options_match(const cellato::run::run_params&) { return true; }
};

template <int y_tile_size, int x_tile_size>
struct cuda_spatial_blocking_traverser {
    static constexpr int tile_size_x = x_tile_size;
    static constexpr int tile_size_y = y_tile_size;
    static constexpr const char* device_name = cuda_opt;
    static constexpr const char* traverser_name = spatial_blocking_opt;

    template <typename evaluator_t, typename grid_t, typename>
    using type = cellato::traversers::cuda::spatial_blocking::traverser<evaluator_t, grid_t, y_tile_size, x_tile_size>;

    static bool options_match(const cellato::run::run_params& params) {
        return params.y_tile_size == y_tile_size && params.x_tile_size == x_tile_size;
    }
};
#endif

template <typename cellular_automaton, typename layout, typename traverser>
struct suite {
    using automaton = cellular_automaton;
    using traverser_traits = traverser;
    using layout_traits = typename layout::template bind<cellular_automaton>;

    using original_cell_t = typename layout_traits::original_cell_t;
    using grid_store_word_t = typename layout_traits::grid_store_word_t;
    using grid_t = typename layout_traits::grid_t;
    using evaluator_t = typename layout_traits::evaluator_t;
    using traverser_t = typename traverser::template type<evaluator_t, grid_t, cellular_automaton>;

    static constexpr int x_margin = layout_traits::x_margin;
    static constexpr int y_margin = layout_traits::y_margin;

    static bool is_for(const cellato::run::run_params& params) {
        return params.automaton == cellular_automaton::name && params.traverser == traverser::traverser_name &&
               params.device == traverser::device_name && params.evaluator == layout_traits::evaluator_name &&
               params.layout == layout_traits::layout_name && layout_traits::word_size_matches(params) &&
               traverser::options_match(params);
    }
};

} // namespace detail

#if CELLATO_ENABLE_CUDA
namespace on_cuda {

template <typename cellular_automaton>
struct standard : detail::suite<cellular_automaton, detail::standard_layout, detail::cuda_simple_traverser> {
    template <int y_tile_size, int x_tile_size = 1>
    using with_spatial_blocking = detail::suite<cellular_automaton,
                                                detail::standard_layout,
                                                detail::cuda_spatial_blocking_traverser<y_tile_size, x_tile_size>>;
};

template <typename store_word_type>
struct using_ {
    template <typename cellular_automaton>
    using bit_array =
        detail::suite<cellular_automaton, detail::bit_array_layout<store_word_type>, detail::cuda_simple_traverser>;

    template <typename cellular_automaton>
    using bit_planes =
        detail::suite<cellular_automaton, detail::bit_planes_layout<store_word_type>, detail::cuda_simple_traverser>;

    template <typename cellular_automaton>
    using tiled_bit_planes = detail::
        suite<cellular_automaton, detail::tiled_bit_planes_layout<store_word_type>, detail::cuda_simple_traverser>;

    template <typename cellular_automaton>
    using temporal_tiled_bit_planes = detail::
        suite<cellular_automaton, detail::tiled_bit_planes_layout<store_word_type>, detail::cuda_temporal_traverser>;

    template <typename cellular_automaton>
    using temporal_linear_bit_planes =
        detail::suite<cellular_automaton, detail::bit_planes_layout<store_word_type>, detail::cuda_temporal_traverser>;
};

} // namespace on_cuda
#endif

namespace on_cpu {

template <typename cellular_automaton>
struct standard : detail::suite<cellular_automaton, detail::standard_layout, detail::cpu_simple_traverser> {};

template <typename store_word_type>
struct using_ {
    template <typename cellular_automaton>
    using bit_array =
        detail::suite<cellular_automaton, detail::bit_array_layout<store_word_type>, detail::cpu_simple_traverser>;

    template <typename cellular_automaton>
    using bit_planes =
        detail::suite<cellular_automaton, detail::bit_planes_layout<store_word_type>, detail::cpu_simple_traverser>;

    template <typename cellular_automaton>
    using tiled_bit_planes = detail::
        suite<cellular_automaton, detail::tiled_bit_planes_layout<store_word_type>, detail::cpu_simple_traverser>;
};

} // namespace on_cpu

template <typename automaton>
using suites_for =
    cellato::utils::type_list<on_cpu::standard<automaton>,
                              on_cpu::using_<std::uint32_t>::bit_array<automaton>,
                              on_cpu::using_<std::uint64_t>::bit_array<automaton>,
                              on_cpu::using_<std::uint32_t>::bit_planes<automaton>,
                              on_cpu::using_<std::uint64_t>::bit_planes<automaton>,
                              on_cpu::using_<std::uint32_t>::tiled_bit_planes<automaton>,
                              on_cpu::using_<std::uint64_t>::tiled_bit_planes<automaton>
#if CELLATO_ENABLE_CUDA
                              ,
                              on_cuda::using_<std::uint32_t>::tiled_bit_planes<automaton>,
                              on_cuda::using_<std::uint64_t>::tiled_bit_planes<automaton>,
                              on_cuda::standard<automaton>,
                              typename on_cuda::standard<automaton>::template with_spatial_blocking<1, 1>,
                              typename on_cuda::standard<automaton>::template with_spatial_blocking<2, 1>,
                              typename on_cuda::standard<automaton>::template with_spatial_blocking<4, 1>,
                              on_cuda::using_<std::uint32_t>::bit_array<automaton>,
                              on_cuda::using_<std::uint64_t>::bit_array<automaton>,
                              on_cuda::using_<std::uint32_t>::bit_planes<automaton>,
                              on_cuda::using_<std::uint64_t>::bit_planes<automaton>,
                              on_cuda::using_<std::uint32_t>::temporal_tiled_bit_planes<automaton>,
                              on_cuda::using_<std::uint64_t>::temporal_tiled_bit_planes<automaton>,
                              on_cuda::using_<std::uint32_t>::temporal_linear_bit_planes<automaton>,
                              on_cuda::using_<std::uint64_t>::temporal_linear_bit_planes<automaton>
#endif
                              >;

template <typename automata>
struct suites_for_all;

template <typename... automata>
struct suites_for_all<cellato::utils::type_list<automata...>> {
    using type = cellato::utils::concat_t<suites_for<automata>...>;
};

template <typename automata>
using suites_for_all_t = typename suites_for_all<automata>::type;

} // namespace cellato::run::test_suites

#endif // CELLATO_TEST_SUITES_HPP
