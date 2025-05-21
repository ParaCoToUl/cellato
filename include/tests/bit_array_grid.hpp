#ifndef CELLIB_TESTS_BIT_ARRAY_GRID_HPP
#define CELLIB_TESTS_BIT_ARRAY_GRID_HPP

#include "manager.hpp"
#include "../memory/bit_array_grid.hpp"
#include "../memory/grid_utils.hpp"
#include "../memory/state_dictionary.hpp"
#include "../memory/standard_grid.hpp"
#include <random>
#include <ctime>

namespace cellib::tests {
// Create a nested namespace for bit_array tests to avoid conflicts
namespace bit_array {

// Define an enum for testing
enum class TestCellState {
    DEAD,
    ALIVE,
    DYING
};

// Stream operator for TestCellState to help with test output
inline std::string to_string(const TestCellState& state) {
    switch (state) {
        case TestCellState::DEAD: return "DEAD";
        case TestCellState::ALIVE: return "ALIVE";
        case TestCellState::DYING: return "DYING";
        default: return "UNKNOWN";
    }
}

// Define the state dictionary for testing
using TestStateDictionary = cellib::memory::grids::state_dictionary<
    TestCellState, 
    TestCellState::DEAD, 
    TestCellState::ALIVE, 
    TestCellState::DYING
>;

} // namespace bit_array

class bit_array_grid_test_suite : public test_suite {
private:
    // Test grid construction
    void test_grid_construction(test_case& tc) {
        std::cout << BLUE << "\n--- Testing bit_array_grid construction ---" << RESET << std::endl;
        
        // Test empty construction
        cellib::memory::grids::bit_array::grid<bit_array::TestStateDictionary> grid(10, 20);
        tc.assert_equal(size_t{20}, grid.x_size_logical(), "Grid width should be 20");
        tc.assert_equal(size_t{10}, grid.y_size_logical(), "Grid height should be 10");
        
        // All cells should be default-initialized to zero (which is DEAD in our enum)
        for (size_t y = 0; y < 5; ++y) {
            for (size_t x = 0; x < 5; ++x) {
                tc.assert_true(bit_array::TestCellState::DEAD == grid.get_cell(x, y), 
                             "Default cell value should be DEAD at (" + std::to_string(x) + ", " + std::to_string(y) + ")");
            }
        }
    }
    
    // Test get_cell and set_cell via construction
    void test_cell_access(test_case& tc) {
        std::cout << BLUE << "\n--- Testing bit_array_grid cell access ---" << RESET << std::endl;
        
        const size_t height = 5;
        const size_t width = 10;
        std::vector<bit_array::TestCellState> init_data(height * width, bit_array::TestCellState::DEAD);
        
        // Set specific cells
        init_data[0] = bit_array::TestCellState::ALIVE;          // (0,0)
        init_data[2] = bit_array::TestCellState::DYING;          // (2,0)
        init_data[width + 3] = bit_array::TestCellState::ALIVE;  // (3,1)
        
        cellib::memory::grids::bit_array::grid<bit_array::TestStateDictionary> grid(height, width, init_data.data());
        
        // Test specific locations
        tc.assert_true(bit_array::TestCellState::ALIVE == grid.get_cell(0, 0), "Cell (0,0) should be ALIVE");
        tc.assert_true(bit_array::TestCellState::DEAD == grid.get_cell(1, 0), "Cell (1,0) should be DEAD");
        tc.assert_true(bit_array::TestCellState::DYING == grid.get_cell(2, 0), "Cell (2,0) should be DYING");
        tc.assert_true(bit_array::TestCellState::ALIVE == grid.get_cell(3, 1), "Cell (3,1) should be ALIVE");
        
        // Test bounds checking
        bool exception_thrown = false;
        try {
            grid.get_cell(width, 0);
        } catch (const std::out_of_range&) {
            exception_thrown = true;
        }
        tc.assert_true(exception_thrown, "Should throw exception for out of bounds x coordinate");
        
        exception_thrown = false;
        try {
            grid.get_cell(0, height);
        } catch (const std::out_of_range&) {
            exception_thrown = true;
        }
        tc.assert_true(exception_thrown, "Should throw exception for out of bounds y coordinate");
    }
    
    // Test bit_array_grid sizes
    void test_grid_sizes(test_case& tc) {
        std::cout << BLUE << "\n--- Testing bit_array_grid sizes ---" << RESET << std::endl;
        
        const size_t height = 16;
        const size_t width = 32;
        
        cellib::memory::grids::bit_array::grid<bit_array::TestStateDictionary> grid(height, width);
        
        tc.assert_equal(width, grid.x_size_logical(), "Logical grid width should match input width");
        tc.assert_equal(height, grid.y_size_logical(), "Logical grid height should match input height");
        tc.assert_equal(width, grid.x_size_physical(), "Physical width should match logical width");
        tc.assert_equal(height, grid.y_size_physical(), "Physical height should match logical height");
        
        // Check the bits_per_cell and cells_per_word constants
        tc.assert_equal(bit_array::TestStateDictionary::needed_bits, 
                      cellib::memory::grids::bit_array::grid<bit_array::TestStateDictionary>::bits_per_cell, 
                      "bits_per_cell should match state dictionary needed_bits");
        
        // Calculate expected cells per word based on the store_word_type (defaulted to uint32_t)
        const size_t expected_cells_per_word = 32 / bit_array::TestStateDictionary::needed_bits; // 32-bit word / 2 bits per cell = 16 cells
        tc.assert_equal(expected_cells_per_word, 
                      cellib::memory::grids::bit_array::grid<bit_array::TestStateDictionary>::cells_per_word, 
                      "cells_per_word calculation should be correct");
    }
    
    // Test simple pattern storage and retrieval
    void test_simple_pattern(test_case& tc) {
        std::cout << BLUE << "\n--- Testing bit_array_grid simple pattern ---" << RESET << std::endl;
        
        const size_t height = 2;
        const size_t width = 8;
        std::vector<bit_array::TestCellState> init_data = {
            bit_array::TestCellState::DEAD, bit_array::TestCellState::ALIVE, bit_array::TestCellState::DYING, bit_array::TestCellState::DEAD,
            bit_array::TestCellState::ALIVE, bit_array::TestCellState::DEAD, bit_array::TestCellState::ALIVE, bit_array::TestCellState::DYING,
            
            bit_array::TestCellState::DYING, bit_array::TestCellState::ALIVE, bit_array::TestCellState::DEAD, bit_array::TestCellState::DEAD,
            bit_array::TestCellState::ALIVE, bit_array::TestCellState::ALIVE, bit_array::TestCellState::DYING, bit_array::TestCellState::DEAD
        };
        
        cellib::memory::grids::bit_array::grid<bit_array::TestStateDictionary> grid(height, width, init_data.data());
        
        // Verify original state can be reconstructed
        auto reconstructed = grid.to_original_representation();
        
        tc.assert_equal(height * width, reconstructed.size(), "Reconstructed vector should have the correct size");
        
        for (size_t y = 0; y < height; ++y) {
            for (size_t x = 0; x < width; ++x) {
                size_t idx = y * width + x;
                tc.assert_true(init_data[idx] == reconstructed[idx], 
                             "Cell at (" + std::to_string(x) + ", " + std::to_string(y) + ") should match original");
            }
        }
    }
    
    // Test more complex pattern across word boundaries
    void test_complex_pattern(test_case& tc) {
        std::cout << BLUE << "\n--- Testing bit_array_grid complex pattern ---" << RESET << std::endl;
        
        const size_t height = 3;
        const size_t width = 50;  // Ensure this crosses word boundaries 
        std::vector<bit_array::TestCellState> init_data(height * width, bit_array::TestCellState::DEAD);
        
        // Create a pattern that crosses word boundaries
        for (size_t i = 0; i < height * width; ++i) {
            switch (i % 3) {
                case 0: init_data[i] = bit_array::TestCellState::DEAD; break;
                case 1: init_data[i] = bit_array::TestCellState::ALIVE; break;
                case 2: init_data[i] = bit_array::TestCellState::DYING; break;
            }
        }
        
        cellib::memory::grids::bit_array::grid<bit_array::TestStateDictionary> grid(height, width, init_data.data());
        
        // Test retrieving specific cells from each row and different word boundaries
        for (size_t y = 0; y < height; ++y) {
            for (size_t x : {0, 15, 16, 31, 32, 45}) {  // Test cells from different words
                if (x < width) {
                    size_t idx = y * width + x;
                    tc.assert_true(init_data[idx] == grid.get_cell(x, y),
                                 "Cell at (" + std::to_string(x) + ", " + std::to_string(y) + ") should match original");
                }
            }
        }
        
        // Verify entire grid matches original 
        auto reconstructed = grid.to_original_representation();
        for (size_t i = 0; i < height * width; i += 5) {  // Check every 5th cell to reduce test time
            tc.assert_true(init_data[i] == reconstructed[i],
                         "Cell at index " + std::to_string(i) + " should match original");
        }
    }
    
    // Test conversion to standard grid
    void test_to_standard_conversion(test_case& tc) {
        std::cout << BLUE << "\n--- Testing bit_array_grid to standard grid conversion ---" << RESET << std::endl;
        
        const size_t height = 4;
        const size_t width = 20;
        std::vector<bit_array::TestCellState> init_data(height * width, bit_array::TestCellState::DEAD);
        
        // Create a pattern
        for (size_t i = 0; i < init_data.size(); ++i) {
            switch (i % 3) {
                case 0: init_data[i] = bit_array::TestCellState::DEAD; break;
                case 1: init_data[i] = bit_array::TestCellState::ALIVE; break;
                case 2: init_data[i] = bit_array::TestCellState::DYING; break;
            }
        }
        
        cellib::memory::grids::bit_array::grid<bit_array::TestStateDictionary> array_grid(height, width, init_data.data());
        
        // Convert to standard grid
        auto standard_grid = array_grid.to_standard();
        
        // Check dimensions
        tc.assert_equal(width, standard_grid.x_size_physical(), "Standard grid width should match original");
        tc.assert_equal(height, standard_grid.y_size_physical(), "Standard grid height should match original");
        
        // Check cell values
        for (size_t y = 0; y < height; ++y) {
            for (size_t x = 0; x < width; ++x) {
                size_t idx = y * width + x;
                tc.assert_true(init_data[idx] == standard_grid.data()[idx],
                             "Cell at (" + std::to_string(x) + ", " + std::to_string(y) + ") should match in standard grid");
            }
        }
    }
    
    // Test constructing from standard grid
    void test_from_standard_conversion(test_case& tc) {
        std::cout << BLUE << "\n--- Testing bit_array_grid from standard grid conversion ---" << RESET << std::endl;
        
        const size_t height = 4;
        const size_t width = 20;
        std::vector<bit_array::TestCellState> init_data(height * width, bit_array::TestCellState::DEAD);
        
        // Create a pattern
        for (size_t i = 0; i < init_data.size(); ++i) {
            switch (i % 3) {
                case 0: init_data[i] = bit_array::TestCellState::DEAD; break;
                case 1: init_data[i] = bit_array::TestCellState::ALIVE; break;
                case 2: init_data[i] = bit_array::TestCellState::DYING; break;
            }
        }
        
        // Create standard grid first
        cellib::memory::grids::standard::grid<bit_array::TestCellState> standard_grid(std::move(init_data), width, height);
        
        // Create bit array grid from standard grid
        cellib::memory::grids::bit_array::grid<bit_array::TestStateDictionary> array_grid(standard_grid);
        
        // Check dimensions
        tc.assert_equal(width, array_grid.x_size_logical(), "Bit array grid width should match original");
        tc.assert_equal(height, array_grid.y_size_logical(), "Bit array grid height should match original");
        
        // Verify cell values by converting both to vectors and comparing
        auto standard_vec = std::vector<bit_array::TestCellState>(standard_grid.data(), standard_grid.data() + width * height);
        auto array_vec = array_grid.to_original_representation();
        
        tc.assert_equal(standard_vec.size(), array_vec.size(), "Vector sizes should match");
        
        for (size_t i = 0; i < standard_vec.size(); ++i) {
            tc.assert_true(standard_vec[i] == array_vec[i],
                         "Cell at index " + std::to_string(i) + " should match between grid types");
        }
    }
    
    // Test with random grid
    void test_random_grid(test_case& tc) {
        std::cout << BLUE << "\n--- Testing bit_array_grid with random data ---" << RESET << std::endl;
        
        const size_t height = 8;
        const size_t width = 30;
        std::vector<bit_array::TestCellState> init_data(height * width);
        
        // Initialize with random values
        std::mt19937 rng(42); // Fixed seed for reproducibility
        std::uniform_int_distribution<int> dist(0, 2);
        
        for (size_t i = 0; i < init_data.size(); ++i) {
            int random_value = dist(rng);
            switch (random_value) {
                case 0: init_data[i] = bit_array::TestCellState::DEAD; break;
                case 1: init_data[i] = bit_array::TestCellState::ALIVE; break;
                case 2: init_data[i] = bit_array::TestCellState::DYING; break;
            }
        }
        
        cellib::memory::grids::bit_array::grid<bit_array::TestStateDictionary> grid(height, width, init_data.data());
        
        // Verify using to_original_representation
        auto reconstructed = grid.to_original_representation();
        
        tc.assert_equal(init_data.size(), reconstructed.size(), "Reconstructed size should match original");
        
        // Check a subset of cells
        for (size_t i = 0; i < init_data.size(); i += 5) {
            tc.assert_true(init_data[i] == reconstructed[i],
                         "Cell at index " + std::to_string(i) + " should match original");
        }
        
        // Check last cell
        tc.assert_true(init_data.back() == reconstructed.back(), "Last cell should match original");
    }
    
    // Test memory efficiency
    void test_memory_efficiency(test_case& tc) {
        std::cout << BLUE << "\n--- Testing bit_array_grid memory efficiency ---" << RESET << std::endl;
        
        // Create a large grid to demonstrate memory efficiency
        const size_t height = 10;
        const size_t width = 1000;
        
        // Calculate expected memory usage
        const size_t bits_per_cell = bit_array::TestStateDictionary::needed_bits;
        const size_t cells_per_word = (sizeof(uint32_t) * 8) / bits_per_cell;
        const size_t total_words_needed = (height * width + cells_per_word - 1) / cells_per_word;
        const size_t expected_bytes = total_words_needed * sizeof(uint32_t);
        
        // Create a message explaining the memory efficiency
        std::string efficiency_msg = "Memory usage: " + std::to_string(expected_bytes) + " bytes for " + 
                                    std::to_string(height * width) + " cells, compared to " + 
                                    std::to_string(height * width * sizeof(bit_array::TestCellState)) + 
                                    " bytes for standard grid";
        std::cout << GREEN << "  " << efficiency_msg << RESET << std::endl;
        
        // Create bit array grid
        cellib::memory::grids::bit_array::grid<bit_array::TestStateDictionary> grid(height, width);
        
        // Verify memory efficiency is as expected
        tc.assert_true(expected_bytes < height * width * sizeof(bit_array::TestCellState),
                     "Bit array grid should use less memory than standard grid");
        
        // Verify formulas are consistent
        const size_t expected_cells_per_word = (sizeof(uint32_t) * 8) / bits_per_cell;
        tc.assert_equal(expected_cells_per_word, 
                      cellib::memory::grids::bit_array::grid<bit_array::TestStateDictionary>::cells_per_word,
                      "cells_per_word calculation matches expected value");
    }
    
    // Test proxy array-like access and assignment
    void test_proxy_array_access(test_case& tc) {
        std::cout << BLUE << "\n--- Testing bit_array_grid proxy array access ---" << RESET << std::endl;
        
        const size_t height = 3;
        const size_t width = 10;
        cellib::memory::grids::bit_array::grid<bit_array::TestStateDictionary> grid(height, width);
        
        // Test assignment through proxy
        auto grid_data = grid.data();
        
        // Set values through proxy
        grid_data[0] = bit_array::TestCellState::ALIVE;
        grid_data[5] = bit_array::TestCellState::DYING;
        grid_data[width + 2] = bit_array::TestCellState::ALIVE;  // Second row
        grid_data[2 * width + 7] = bit_array::TestCellState::DYING;  // Third row
        
        // Verify with get_cell
        tc.assert_true(bit_array::TestCellState::ALIVE == grid.get_cell(0, 0), "Proxy assignment at (0,0) should set ALIVE");
        tc.assert_true(bit_array::TestCellState::DYING == grid.get_cell(5, 0), "Proxy assignment at (5,0) should set DYING");
        tc.assert_true(bit_array::TestCellState::ALIVE == grid.get_cell(2, 1), "Proxy assignment at (2,1) should set ALIVE");
        tc.assert_true(bit_array::TestCellState::DYING == grid.get_cell(7, 2), "Proxy assignment at (7,2) should set DYING");
        
        // Test retrieval through proxy
        tc.assert_true(bit_array::TestCellState::ALIVE == grid_data[0], "Proxy retrieval at index 0 should return ALIVE");
        tc.assert_true(bit_array::TestCellState::DYING == grid_data[5], "Proxy retrieval at index 5 should return DYING");
        tc.assert_true(bit_array::TestCellState::ALIVE == grid_data[width + 2], "Proxy retrieval at second row should return ALIVE");
        tc.assert_true(bit_array::TestCellState::DYING == grid_data[2 * width + 7], "Proxy retrieval at third row should return DYING");
    }
    
    // Test implicit conversion in various contexts
    void test_proxy_implicit_conversion(test_case& tc) {
        std::cout << BLUE << "\n--- Testing bit_array_grid proxy implicit conversion ---" << RESET << std::endl;
        
        const size_t height = 2;
        const size_t width = 8;
        
        // Initialize with specific states
        std::vector<bit_array::TestCellState> init_data(height * width, bit_array::TestCellState::DEAD);
        init_data[3] = bit_array::TestCellState::ALIVE;
        init_data[7] = bit_array::TestCellState::DYING;
        
        cellib::memory::grids::bit_array::grid<bit_array::TestStateDictionary> grid(height, width, init_data.data());
        auto grid_data = grid.data();
        
        // Test direct assignment to enum variable (implicit conversion)
        bit_array::TestCellState state1 = grid_data[3];
        bit_array::TestCellState state2 = grid_data[7];
        
        tc.assert_true(state1 == bit_array::TestCellState::ALIVE, "Implicit conversion to enum should work for ALIVE");
        tc.assert_true(state2 == bit_array::TestCellState::DYING, "Implicit conversion to enum should work for DYING");
        
        // Test in direct comparison
        tc.assert_true(grid_data[3] == bit_array::TestCellState::ALIVE, "Direct comparison with enum should work");
        tc.assert_true(grid_data[7] == bit_array::TestCellState::DYING, "Direct comparison with enum should work");
        
        // Test in function that expects the enum
        auto enum_to_int = [](bit_array::TestCellState s) -> int {
            return static_cast<int>(s);
        };
        
        tc.assert_equal(1, enum_to_int(grid_data[3]), "Implicit conversion should work in function calls");
        tc.assert_equal(2, enum_to_int(grid_data[7]), "Implicit conversion should work in function calls");
    }
    
    // Test complex proxy manipulations
    void test_proxy_complex_operations(test_case& tc) {
        std::cout << BLUE << "\n--- Testing bit_array_grid proxy complex operations ---" << RESET << std::endl;
        
        const size_t height = 4;
        const size_t width = 12;
        cellib::memory::grids::bit_array::grid<bit_array::TestStateDictionary> grid(height, width);
        auto grid_data = grid.data();
        
        // Set a pattern using proxy assignments
        for (size_t i = 0; i < width * height; ++i) {
            switch (i % 3) {
                case 0: grid_data[i] = bit_array::TestCellState::DEAD; break;
                case 1: grid_data[i] = bit_array::TestCellState::ALIVE; break;
                case 2: grid_data[i] = bit_array::TestCellState::DYING; break;
            }
        }
        
        // Verify pattern with proxy retrieval
        for (size_t i = 0; i < width * height; ++i) {
            bit_array::TestCellState expected;
            switch (i % 3) {
                case 0: expected = bit_array::TestCellState::DEAD; break;
                case 1: expected = bit_array::TestCellState::ALIVE; break;
                case 2: expected = bit_array::TestCellState::DYING; break;
            }
            
            tc.assert_true(expected == grid_data[i], 
                         "Complex pattern at index " + std::to_string(i) + " should match expected value");
        }
        
        // Test modifying values
        for (size_t i = 0; i < 10; ++i) {
            size_t idx = i * 4; // Test every 4th element
            if (idx < width * height) {
                // Cycle the state: DEAD -> ALIVE -> DYING -> DEAD
                bit_array::TestCellState current = grid_data[idx];
                bit_array::TestCellState next;
                
                switch (current) {
                    case bit_array::TestCellState::DEAD: 
                        next = bit_array::TestCellState::ALIVE; break;
                    case bit_array::TestCellState::ALIVE: 
                        next = bit_array::TestCellState::DYING; break;
                    case bit_array::TestCellState::DYING: 
                        next = bit_array::TestCellState::DEAD; break;
                }
                
                grid_data[idx] = next;
                tc.assert_true(next == grid_data[idx], 
                             "After cycling, state at index " + std::to_string(idx) + " should be updated");
            }
        }
    }
    
    // Test const and non-const proxies
    void test_proxy_const_behavior(test_case& tc) {
        std::cout << BLUE << "\n--- Testing bit_array_grid const proxy behavior ---" << RESET << std::endl;
        
        const size_t height = 2;
        const size_t width = 6;
        
        // Initialize with pattern
        std::vector<bit_array::TestCellState> init_data = {
            bit_array::TestCellState::DEAD, bit_array::TestCellState::ALIVE, bit_array::TestCellState::DYING,
            bit_array::TestCellState::ALIVE, bit_array::TestCellState::DYING, bit_array::TestCellState::DEAD,
            
            bit_array::TestCellState::DYING, bit_array::TestCellState::DEAD, bit_array::TestCellState::ALIVE,
            bit_array::TestCellState::DEAD, bit_array::TestCellState::ALIVE, bit_array::TestCellState::DYING
        };
        
        cellib::memory::grids::bit_array::grid<bit_array::TestStateDictionary> grid(height, width, init_data.data());
        
        // Get non-const proxy and modify
        {
            auto proxy = grid.data();
            proxy[1] = bit_array::TestCellState::DEAD; // Change ALIVE to DEAD
            proxy[8] = bit_array::TestCellState::DYING; // Change ALIVE to DYING
        }
        
        // Get const proxy and verify
        const auto& const_grid = grid;
        auto const_proxy = const_grid.data();
        
        tc.assert_true(bit_array::TestCellState::DEAD == const_proxy[1], "Const proxy should reflect changes made by non-const proxy");
        tc.assert_true(bit_array::TestCellState::DYING == const_proxy[8], "Const proxy should reflect changes made by non-const proxy");
        
        // Verify we can read but not write to const proxy
        bit_array::TestCellState state = const_proxy[0]; // Should compile
        tc.assert_true(bit_array::TestCellState::DEAD == state, "Reading from const proxy should work");
        
        // This would not compile if uncommented:
        // const_proxy[0] = bit_array::TestCellState::ALIVE; // Would cause compile error
    }

public:
    std::string name() const override {
        return "BitArrayGrid";
    }

    test_result run() override {
        test_result result;
        test_case tc(result, true);

        // Run all bit_array_grid tests
        test_grid_construction(tc);
        test_cell_access(tc);
        test_grid_sizes(tc);
        test_simple_pattern(tc);
        test_complex_pattern(tc);
        test_to_standard_conversion(tc);
        test_from_standard_conversion(tc);
        test_random_grid(tc);
        test_memory_efficiency(tc);
        
        // New tests for BitArrayProxy
        test_proxy_array_access(tc);
        test_proxy_implicit_conversion(tc);
        test_proxy_complex_operations(tc);
        test_proxy_const_behavior(tc);
        
        return result;
    }
};

// Helper function to register the suite with the manager
inline void register_bit_array_grid_tests() {
    static bit_array_grid_test_suite suite;
    test_manager::instance().register_suite(&suite);
}

} // namespace cellib::tests

#endif // CELLIB_TESTS_BIT_ARRAY_GRID_HPP
