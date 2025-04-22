#include <iostream>
#include <string>
#include <vector>
#include <cstdint>
#include <cstdlib>  // for rand()
#include <ctime>    // for time()
#include <sstream>  // for std::stringstream
#include "bit-mode.hpp"

// ANSI color codes for terminal output
const std::string RESET = "\033[0m";
const std::string RED = "\033[31m";
const std::string GREEN = "\033[32m";
const std::string YELLOW = "\033[33m";
const std::string BLUE = "\033[34m";
const std::string CYAN = "\033[36m";


int tests_run = 0;
int tests_passed = 0;
int failed_tests_count = 0;

// Global test settings
bool verbose_output = true;  // Can be set to false for less output
std::vector<std::string> failed_tests;

// Define an enum for testing
enum class CellState {
    DEAD,
    ALIVE,
    DYING
};

// Stream operator for CellState
std::ostream& operator<<(std::ostream& os, CellState state) {
    switch (state) {
        case CellState::DEAD:
            os << "DEAD";
            break;
        case CellState::ALIVE:
            os << "ALIVE";
            break;
        case CellState::DYING:
            os << "DYING";
            break;
        default:
            os << "UNKNOWN";
    }
    return os;
}

void begin_test(const std::string& name) {
    std::cout << BLUE << "\n====== TEST: " << name << " ======" << RESET << std::endl;
    tests_run++;
}

template<typename T, typename U>
void assert_equal(T expected, U actual, const std::string& message, bool always_print = false) {
    if (expected == actual) {
        if (verbose_output || always_print) {
            std::cout << GREEN << "✓ PASS: " << message << RESET << std::endl;
        }
        tests_passed++;
    } else {
        std::string failure_msg = message + " (Expected: " + std::to_string(expected) + ", Got: " + std::to_string(actual) + ")";
        if (verbose_output || always_print) {
            std::cout << RED << "✗ FAIL: " << failure_msg << RESET << std::endl;
        }
        failed_tests.push_back(failure_msg);
        failed_tests_count++;
    }
}

// Specialization for CellState
template<>
void assert_equal<CellState, CellState>(CellState expected, CellState actual, const std::string& message, bool always_print) {
    if (expected == actual) {
        if (verbose_output || always_print) {
            std::cout << GREEN << "✓ PASS: " << message << RESET << std::endl;
        }
        tests_passed++;
    } else {
        std::stringstream ss;
        ss << message << " (Expected: " << expected << ", Got: " << actual << ")";
        std::string failure_msg = ss.str();
        
        if (verbose_output || always_print) {
            std::cout << RED << "✗ FAIL: " << failure_msg << RESET << std::endl;
        }
        failed_tests.push_back(failure_msg);
        failed_tests_count++;
    }
}

void assert_true(bool condition, const std::string& message, bool always_print = false) {
    if (condition) {
        if (verbose_output || always_print) {
            std::cout << GREEN << "✓ PASS: " << message << RESET << std::endl;
        }
        tests_passed++;
    } else {
        if (verbose_output || always_print) {
            std::cout << RED << "✗ FAIL: " << message << RESET << std::endl;
        }
        failed_tests.push_back(message);
        failed_tests_count++;
    }
}

void print_summary() {
    std::cout << YELLOW << "\n====== TEST SUMMARY ======" << RESET << std::endl << std::endl;
    std::cout << BLUE << "Tests run: " << tests_run << std::endl;
    
    std::cout << GREEN << "  Asserts passed: " << tests_passed << RESET << std::endl;
    std::cout << RED << "  Asserts failed: " << failed_tests_count << RESET << std::endl << std::endl;

    if (failed_tests.size() > 0) {
        std::cout << RED << "Failed tests: " << failed_tests_count << RESET << std::endl;
        for (size_t i = 0; i < failed_tests.size(); ++i) {
            std::cout << RED << (i+1) << ". " << failed_tests[i] << RESET << std::endl;
        }
    } else {
        std::cout << GREEN << "All tests passed!" << RESET << std::endl << std::endl;
    }
}

// We don't need to extend the state_dictionary anymore since we added index_to_state to the base class
using TestStateDictionary = bitwise::state_dictionary<CellState, CellState::DEAD, CellState::ALIVE, CellState::DYING>;

// Test state_dictionary basic properties
void test_state_dictionary_basics() {
    begin_test("State Dictionary - Basic Properties");
    
    using dict = bitwise::state_dictionary<CellState, CellState::DEAD, CellState::ALIVE, CellState::DYING>;
    
    assert_equal(3, dict::number_of_values, "Dictionary should have 3 values");
    assert_equal(2, dict::needed_bits, "Should need 2 bits to represent 3 states");
}

// Test state_dictionary conversion functions
void test_state_dictionary_conversion() {
    begin_test("State Dictionary - State Conversion");
    
    using dict = bitwise::state_dictionary<CellState, CellState::DEAD, CellState::ALIVE, CellState::DYING>;
    
    assert_equal(0, dict::state_to_index(CellState::DEAD), "DEAD should map to index 0");
    assert_equal(1, dict::state_to_index(CellState::ALIVE), "ALIVE should map to index 1");
    assert_equal(2, dict::state_to_index(CellState::DYING), "DYING should map to index 2");
    
    try {
        dict::state_to_index(static_cast<CellState>(99));
        std::cout << RED << "✗ FAIL: Should throw exception for invalid state" << RESET << std::endl;
        failed_tests_count++;
    } catch (const std::out_of_range&) {
        std::cout << GREEN << "✓ PASS: Correctly throws exception for invalid state" << RESET << std::endl;
        tests_passed++;
    }
}

// Test bit_grid construction and size methods
void test_bit_grid_sizes() {
    begin_test("Bit Grid - Size Calculations");
    
    using dict = TestStateDictionary;
    
    // Create a 2x3 grid (2 rows, 3 word columns)
    const size_t height = 2;
    const size_t width = 24;
    const size_t word_bits = sizeof(uint8_t) * 8;
    
    // Initialize grid with DEAD cells
    std::vector<CellState> input_grid(height * width * word_bits, CellState::DEAD);
    
    bitwise::bit_grid<uint8_t, dict> grid(height, width, input_grid.data());

    assert_equal(width, grid.x_size_original(), "Grid width should be 3 words");
    assert_equal(height, grid.y_size_original(), "Grid height should be 2 rows");
    assert_equal(width / word_bits, grid.x_size_physical(), "Physical width should be width * bits_per_word");
    assert_equal(height, grid.y_size_physical(), "Physical height should match input height");
}

// Test bit_grid storage and retrieval of patterns
void test_bit_grid_pattern() {
    begin_test("Bit Grid - Pattern Storage and Retrieval");
    
    using dict = TestStateDictionary;
    
    // Create a 1x1 grid (1 row, 1 word)
    // For uint8_t, this stores 8 cells in a row
    std::vector<CellState> input_grid = {
        CellState::DEAD, CellState::ALIVE, CellState::DYING, CellState::DEAD,
        CellState::ALIVE, CellState::DEAD, CellState::ALIVE, CellState::DYING
    };

    bitwise::bit_grid<uint8_t, dict> grid(1, 8, input_grid.data());
    
    // Reconstruct and verify
    auto result = grid.to_original_representation();
    
    assert_equal(size_t{8}, result.size(), "Result should have 8 cells");
    
    // Check each cell matches what we put in
    const CellState expected[] = {
        CellState::DEAD, CellState::ALIVE, CellState::DYING, CellState::DEAD,
        CellState::ALIVE, CellState::DEAD, CellState::ALIVE, CellState::DYING
    };
    
    for (size_t i = 0; i < 8; ++i) {
        assert_equal(expected[i], result[i], "Cell " + std::to_string(i) + " should match input");
    }
}

// Test bit_grid with larger grid and complex pattern
void test_bit_grid_complex() {
    begin_test("Bit Grid - Complex Pattern");
    
    using dict = TestStateDictionary;
    
    // Create a 2x2 grid (2 rows, 2 word columns) for 2x16 cells with uint8_t
    const size_t word_bits = sizeof(uint8_t) * 8;
    std::vector<CellState> input_grid(2 * 2 * word_bits, CellState::DEAD);
    
    // Set specific cells to create a pattern
    // Row 0, positions 0, 3, 7 are ALIVE
    // Row 1, positions 1, 4, 9 are DYING
    input_grid[0] = CellState::ALIVE;
    input_grid[3] = CellState::ALIVE;
    input_grid[7] = CellState::ALIVE;
    input_grid[word_bits + 1] = CellState::DYING;
    input_grid[word_bits + 4] = CellState::DYING;
    input_grid[word_bits + 9] = CellState::DYING;
    
    bitwise::bit_grid<uint8_t, dict> grid(2, 16, input_grid.data());
    
    // Verify the reconstruction
    auto result = grid.to_original_representation();
    
    assert_equal(CellState::ALIVE, result[0], "Cell (0,0) should be ALIVE");
    assert_equal(CellState::ALIVE, result[3], "Cell (0,3) should be ALIVE");
    assert_equal(CellState::ALIVE, result[7], "Cell (0,7) should be ALIVE");
    assert_equal(CellState::DYING, result[word_bits + 1], "Cell (1,1) should be DYING");
    assert_equal(CellState::DYING, result[word_bits + 4], "Cell (1,4) should be DYING");
    assert_equal(CellState::DYING, result[word_bits + 9], "Cell (1,9) should be DYING");
}

// Test get_cell function
void test_get_cell() {
    begin_test("Bit Grid - get_cell");
    
    using dict = TestStateDictionary;
    
    // Create a 2x2 grid (2 rows, 2 word columns) with a specific pattern
    const size_t height = 2;
    const size_t width = 2;
    const size_t word_bits = sizeof(uint8_t) * 8;
    const size_t cells_per_row = width * word_bits;
    std::vector<CellState> input_grid(height * cells_per_row, CellState::DEAD);
    
    // Set specific cells based on their (x, y) coordinates
    // Row 0
    input_grid[0] = CellState::ALIVE;                    // (0,0)
    input_grid[3] = CellState::DYING;                    // (3,0)
    
    // Row 1 - offset by cells_per_row (16 for uint8_t, 2 words)
    input_grid[cells_per_row + 1] = CellState::ALIVE;    // (1,1)
    input_grid[cells_per_row + 7] = CellState::DYING;    // (7,1)
    input_grid[cells_per_row + 9] = CellState::ALIVE;    // (9,1)
    
    bitwise::bit_grid<uint8_t, dict> grid(height, cells_per_row, input_grid.data());
    
    // Test specific cell retrievals
    assert_equal(CellState::ALIVE, grid.get_cell(0, 0), "Cell (0,0) should be ALIVE");
    assert_equal(CellState::DEAD, grid.get_cell(1, 0), "Cell (1,0) should be DEAD");
    assert_equal(CellState::DYING, grid.get_cell(3, 0), "Cell (3,0) should be DYING");
    assert_equal(CellState::ALIVE, grid.get_cell(1, 1), "Cell (1,1) should be ALIVE");
    assert_equal(CellState::DYING, grid.get_cell(7, 1), "Cell (7,1) should be DYING");
    assert_equal(CellState::ALIVE, grid.get_cell(9, 1), "Cell (9,1) should be ALIVE");
    
    // Test bounds checking
    try {
        grid.get_cell(cells_per_row, 0);
        std::cout << RED << "✗ FAIL: Should throw exception for out of bounds x coordinate" << RESET << std::endl;
        failed_tests_count++;
    } catch (const std::out_of_range&) {
        std::cout << GREEN << "✓ PASS: Correctly throws exception for out of bounds x coordinate" << RESET << std::endl;
        tests_passed++;
    }
    
    try {
        grid.get_cell(0, height);
        std::cout << RED << "✗ FAIL: Should throw exception for out of bounds y coordinate" << RESET << std::endl;
        failed_tests_count++;
    } catch (const std::out_of_range&) {
        std::cout << GREEN << "✓ PASS: Correctly throws exception for out of bounds y coordinate" << RESET << std::endl;
        tests_passed++;
    }
    
    // Test all cells to ensure correct access
    for (size_t y = 0; y < height; ++y) {
        for (size_t x = 0; x < cells_per_row; ++x) {
            size_t idx = y * cells_per_row + x;
            CellState expected = input_grid[idx];
            CellState actual = grid.get_cell(x, y);
            assert_equal(expected, actual, 
                         "Cell (" + std::to_string(x) + "," + std::to_string(y) + ") should match");
        }
    }
}

// Test that to_original_representation uses get_cell correctly
void test_to_original_representation_with_get_cell() {
    begin_test("Bit Grid - to_original_representation using get_cell");
    
    using dict = TestStateDictionary;
    
    // Create a small random grid
    const size_t height = 10;
    const size_t width = 5;
    const size_t word_bits = sizeof(uint8_t) * 8;
    const size_t total_cells = height * width * word_bits;
    
    // Generate random cell states
    std::vector<CellState> input_grid(total_cells);
    for (size_t i = 0; i < input_grid.size(); ++i) {
        int random_value = rand() % 3;
        switch (random_value) {
            case 0: input_grid[i] = CellState::DEAD; break;
            case 1: input_grid[i] = CellState::ALIVE; break;
            case 2: input_grid[i] = CellState::DYING; break;
        }
    }
    
    bitwise::bit_grid<uint8_t, dict> grid(height, width * word_bits, input_grid.data());
    
    // Compare direct get_cell with to_original_representation results
    auto result = grid.to_original_representation();
    
    auto original_verbose = verbose_output;
    verbose_output = false;  // Temporarily disable verbose output

    for (size_t y = 0; y < height; ++y) {
        for (size_t x = 0; x < width * word_bits; ++x) {
            size_t idx = y * (width * word_bits) + x;
            CellState from_get_cell = grid.get_cell(x, y);
            CellState from_representation = result[idx];
            
            assert_equal(from_get_cell, from_representation, 
                         "get_cell and to_original_representation should return the same value");
        }
    }

    verbose_output = original_verbose;  // Restore original verbosity
}

// Test bit_grid with a large random grid
void test_bit_grid_large_random() {
    bool original_verbose = verbose_output;
    verbose_output = false;  // Temporarily disable verbose output
    
    begin_test("Bit Grid - Large Random Pattern");
    
    using dict = TestStateDictionary;
    
    // Create a 500x1000 grid (500 rows, 1000 word columns)
    const size_t height = 500;
    const size_t width = 1000;
    const size_t word_bits = sizeof(uint8_t) * 8;
    const size_t total_cells = height * width * word_bits;
    
    std::cout << "Generating random grid with " << total_cells << " cells..." << std::endl;
    std::vector<CellState> input_grid(total_cells);
    
    // Initialize with random values
    for (size_t i = 0; i < input_grid.size(); ++i) {
        int random_value = rand() % 3;
        switch (random_value) {
            case 0: input_grid[i] = CellState::DEAD; break;
            case 1: input_grid[i] = CellState::ALIVE; break;
            case 2: input_grid[i] = CellState::DYING; break;
        }
    }
    
    std::cout << "Creating bit grid..." << std::endl;
    bitwise::bit_grid<uint8_t, dict> grid(height, width * word_bits, input_grid.data());
    
    std::cout << "Converting back to original representation..." << std::endl;
    auto result = grid.to_original_representation();
    
    std::cout << "Verifying results..." << std::endl;
    assert_equal(input_grid.size(), result.size(), "Result size should match input size", true);
    
    // Check if all cells match what we put in
    bool all_match = true;
    size_t mismatches = 0;
    std::vector<size_t> mismatch_indices;
    
    for (size_t i = 0; i < input_grid.size(); ++i) {
        if (input_grid[i] != result[i]) {
            all_match = false;
            mismatches++;
            if (mismatches <= 10) {
                mismatch_indices.push_back(i);
            }
        }
    }
    
    if (mismatches > 0) {
        std::cout << RED << "Total mismatches: " << mismatches << RESET << std::endl;
        std::cout << RED << "First 10 mismatches:" << RESET << std::endl;
        
        for (size_t idx : mismatch_indices) {
            std::cout << RED << "  Mismatch at index " << idx 
                      << ": Expected " << input_grid[idx] 
                      << ", Got " << result[idx] << RESET << std::endl;
        }
    }
    
    assert_true(all_match, "All cells should match between input and output", true);
    
    verbose_output = original_verbose;  // Restore original verbosity
}

int main() {
    // Initialize random seed
    srand(time(nullptr));
    
    std::cout << CYAN << "========================================" << std::endl;
    std::cout << "   RUNNING BIT-MODE LIBRARY UNIT TESTS" << std::endl;
    std::cout << "========================================" << RESET << std::endl;
    
    // Set verbose output for individual tests here
    verbose_output = true;  // Set to false for less output
    
    // Run all tests
    test_state_dictionary_basics();
    test_state_dictionary_conversion();
    test_bit_grid_sizes();
    test_bit_grid_pattern();
    test_bit_grid_complex();
    test_get_cell();
    test_to_original_representation_with_get_cell();
    test_bit_grid_large_random();
    
    print_summary();
    
    // Return success if all tests pass
    return tests_passed == tests_run ? 0 : 1;
}