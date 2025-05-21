#ifndef CELLIB_TEST_SUITES_HPP
#define CELLIB_TEST_SUITES_HPP

#include "../evaluators/standard.hpp"
#include "../evaluators/bit_plates.hpp"
#include "../memory/standard_grid.hpp"
#include "../traversers/cpu/simple.hpp"
#include "../memory/bit_plates_grid.hpp"
#include "../memory/bit_array_grid.hpp"
#include "../traversers/cuda/simple.hpp"
#include "../traversers/cuda/spacial_blocking.hpp"

namespace cellib::run::test_suites {

template <typename cellular_automaton>
struct cpu_standard {
    
    using original_cell_t = typename cellular_automaton::cell_state;
    using grid_store_word_t = original_cell_t;

    using algorithm_t = typename cellular_automaton::algorithm;
    
    using grid_t = cellib::memory::grids::standard::grid<original_cell_t>;
    using evaluator_t = cellib::evaluators::standard::evaluator<original_cell_t, algorithm_t>;
    
    using traverser_t = cellib::traversers::cpu::simple::traverser<evaluator_t, grid_t>;
    
    constexpr static int x_margin = 1;
    constexpr static int y_margin = 1;
};

template <typename cellular_automaton>
struct cuda_standard {
    
    using original_cell_t = typename cellular_automaton::cell_state;
    using grid_store_word_t = original_cell_t;

    using algorithm_t = typename cellular_automaton::algorithm;
    
    using grid_t = cellib::memory::grids::standard::grid<original_cell_t>;
    using evaluator_t = cellib::evaluators::standard::evaluator<original_cell_t, algorithm_t>;

    using traverser_t = cellib::traversers::cuda::simple::traverser<evaluator_t, grid_t>;

    constexpr static int x_margin = 1;
    constexpr static int y_margin = 1;
};

template <typename cellular_automaton, int y_tile_size = 4, int x_tile_size = 1>
struct cuda_spacial_blocking {
    
    using original_cell_t = typename cellular_automaton::cell_state;
    using grid_store_word_t = original_cell_t;

    using algorithm_t = typename cellular_automaton::algorithm;
    
    using grid_t = cellib::memory::grids::standard::grid<original_cell_t>;
    using evaluator_t = cellib::evaluators::standard::evaluator<original_cell_t, algorithm_t>;

    using traverser_t = cellib::traversers::cuda::spacial_blocking::traverser<evaluator_t, grid_t, y_tile_size, x_tile_size>;

    constexpr static int x_margin = 1;
    constexpr static int y_margin = 1;
};

template <typename store_word_type>
struct using_ {
    
    template <typename cellular_automaton>
    struct bit_array_cpu {
        
        using original_cell_t = typename cellular_automaton::cell_state;
        using grid_store_word_t = store_word_type;

        using algorithm_t = typename cellular_automaton::algorithm;
        using state_dictionary_t = typename cellular_automaton::state_dictionary;
        
        using grid_t = cellib::memory::grids::bit_array::grid<state_dictionary_t, grid_store_word_t>;
        using evaluator_t = cellib::evaluators::standard::evaluator<original_cell_t, algorithm_t, typename grid_t::cell_ptr_t>;
        
        using traverser_t = cellib::traversers::cpu::simple::traverser<evaluator_t, grid_t>;
        
        constexpr static int x_margin = 1;
        constexpr static int y_margin = 1;
    };

    template <typename cellular_automaton>
    struct bit_array_cuda {
        
        using original_cell_t = typename cellular_automaton::cell_state;
        using grid_store_word_t = store_word_type;

        using algorithm_t = typename cellular_automaton::algorithm;
        using state_dictionary_t = typename cellular_automaton::state_dictionary;
        
        using grid_t = cellib::memory::grids::bit_array::grid<state_dictionary_t, grid_store_word_t>;
        using evaluator_t = cellib::evaluators::standard::evaluator<original_cell_t, algorithm_t, typename grid_t::cell_ptr_t>;

        using traverser_t = cellib::traversers::cuda::spacial_blocking::traverser<
            evaluator_t, grid_t, 1, grid_t::cells_per_word>;
        
        constexpr static int x_margin = 1;
        constexpr static int y_margin = 1;
    };

    template <typename cellular_automaton>
    struct bit_plates_cpu {
        using original_cell_t = typename cellular_automaton::cell_state;
        using grid_store_word_t = store_word_type;

        using algorithm_t = typename cellular_automaton::algorithm;
        using state_dictionary_t = typename cellular_automaton::state_dictionary;

        using grid_t = cellib::memory::grids::bit_plates::grid<grid_store_word_t, state_dictionary_t>;
        using evaluator_t = cellib::evaluators::bit_plates::evaluator<grid_store_word_t, state_dictionary_t, algorithm_t>; 
        
        using traverser_t = cellib::traversers::cpu::simple::traverser<evaluator_t, grid_t>;

        constexpr static int x_margin = sizeof(grid_store_word_t) * 8;
        constexpr static int y_margin = 1;
    };

    template <typename cellular_automaton>
    struct bit_plates_cuda {
        using original_cell_t = typename cellular_automaton::cell_state;
        using grid_store_word_t = store_word_type;

        using algorithm_t = typename cellular_automaton::algorithm;
        using state_dictionary_t = typename cellular_automaton::state_dictionary;

        using grid_t = cellib::memory::grids::bit_plates::grid<grid_store_word_t, state_dictionary_t>;
        using evaluator_t = cellib::evaluators::bit_plates::evaluator<grid_store_word_t, state_dictionary_t, algorithm_t>; 

        using traverser_t = cellib::traversers::cuda::simple::traverser<evaluator_t, grid_t>;

        constexpr static int x_margin = sizeof(grid_store_word_t) * 8;
        constexpr static int y_margin = 1;
    };

};

} // namespace cellib::run

#endif // CELLIB_TEST_SUITES_HPP