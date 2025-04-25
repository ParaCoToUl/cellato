#include <iostream>
#include <vector>
#include <cstdint>
#include <string>
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

// Test basic properties and operations of vector_int
void test_vector_int_basics() {
    begin_test("vector_int - Basic Properties and Operations");
    
    // Create a vector_int with 3 bits of precision
    using vint = bitwise::vector_int<uint8_t, 3>;
    vint v;
    
    // Test setting and getting values
    v.set_at(0, 1);  // 001
    assert_equal(1, v.get_at(0), "Should correctly set and get value at index 0");
    
    v.set_at(1, 2);  // 010
    assert_equal(2, v.get_at(1), "Should correctly set and get value at index 1");
    
    v.set_at(2, 3);  // 011
    assert_equal(3, v.get_at(2), "Should correctly set and get value at index 2");
    
    v.set_at(3, 7);  // 111
    assert_equal(7, v.get_at(3), "Should correctly set and get value at index 3");
    
    // Test out of range handling
    try {
        v.set_at(9, 5);
        std::cout << RED << "✗ FAIL: Should throw exception for out of bounds set_at" << RESET << std::endl;
        failed_tests_count++;
    } catch (const std::out_of_range&) {
        std::cout << GREEN << "✓ PASS: Correctly throws exception for out of bounds set_at" << RESET << std::endl;
        tests_passed++;
    }
    
    try {
        v.get_at(9);
        std::cout << RED << "✗ FAIL: Should throw exception for out of bounds get_at" << RESET << std::endl;
        failed_tests_count++;
    } catch (const std::out_of_range&) {
        std::cout << GREEN << "✓ PASS: Correctly throws exception for out of bounds get_at" << RESET << std::endl;
        tests_passed++;
    }
}

// Test binary operations between vector_int instances
void test_vector_int_binary_operations() {
    begin_test("vector_int - Binary Operations");
    
    using vint = bitwise::vector_int<uint8_t, 3>;
    vint v1, v2;
    
    // Set up test values
    // v1: First 4 cells set to 1, 2, 3, 4
    v1.set_at(0, 1);
    v1.set_at(1, 2);
    v1.set_at(2, 3);
    v1.set_at(3, 4);
    
    // v2: First 4 cells set to 2, 3, 1, 5
    v2.set_at(0, 2);
    v2.set_at(1, 3);
    v2.set_at(2, 1);
    v2.set_at(3, 5);
    
    // Test addition
    auto v_add = v1.get_added(v2);
    assert_equal(3, v_add.get_at(0), "Addition at index 0 should be 1+2=3");
    assert_equal(5, v_add.get_at(1), "Addition at index 1 should be 2+3=5");
    assert_equal(4, v_add.get_at(2), "Addition at index 2 should be 3+1=4");
    assert_equal(1, v_add.get_at(3), "Addition at index 3 should be 4+5=9 i.e. 1 mod 8");
    
    // Test OR operation
    auto v_or = v1.get_ored(v2);
    assert_equal(3, v_or.get_at(0), "OR at index 0 should be 1|2=3");
    assert_equal(3, v_or.get_at(1), "OR at index 1 should be 2|3=3");
    assert_equal(3, v_or.get_at(2), "OR at index 2 should be 3|1=3");
    assert_equal(5, v_or.get_at(3), "OR at index 3 should be 4|5=5");
    
    // Test XOR operation
    auto v_xor = v1.get_xored(v2);
    assert_equal(3, v_xor.get_at(0), "XOR at index 0 should be 1^2=3");
    assert_equal(1, v_xor.get_at(1), "XOR at index 1 should be 2^3=1");
    assert_equal(2, v_xor.get_at(2), "XOR at index 2 should be 3^1=2");
    assert_equal(1, v_xor.get_at(3), "XOR at index 3 should be 4^5=1");
    
    // Test AND operation
    auto v_and = v1.get_anded(v2);
    assert_equal(0, v_and.get_at(0), "AND at index 0 should be 1&2=0");
    assert_equal(2, v_and.get_at(1), "AND at index 1 should be 2&3=2");
    assert_equal(1, v_and.get_at(2), "AND at index 2 should be 3&1=1");
    assert_equal(4, v_and.get_at(3), "AND at index 3 should be 4&5=4");
}

// Test binary operations between vector_int instances of different sizes
void test_vector_int_mixed_precision_operations() {
    begin_test("vector_int - Mixed Precision Binary Operations");
    
    // Define vectors with different bit precisions
    using vint_small = bitwise::vector_int<uint8_t, 2>; // 2-bit precision
    using vint_medium = bitwise::vector_int<uint8_t, 3>; // 3-bit precision
    using vint_large = bitwise::vector_int<uint8_t, 4>; // 4-bit precision
    
    // Create instances
    vint_small small;
    vint_medium medium;
    vint_large large;
    
    // Set values to test with
    // small: 2 bits can represent 0-3
    small.set_at(0, 1);  // 01
    small.set_at(1, 2);  // 10
    small.set_at(2, 3);  // 11
    
    // medium: 3 bits can represent 0-7
    medium.set_at(0, 3);  // 011
    medium.set_at(1, 5);  // 101
    medium.set_at(2, 6);  // 110
    
    // large: 4 bits can represent 0-15
    large.set_at(0, 7);   // 0111
    large.set_at(1, 10);  // 1010
    large.set_at(2, 12);  // 1100
    
    // CASE 1: Small vector operating with medium vector
    std::cout << "  Testing small + medium operations:" << std::endl;
    
    // Addition (should return a vector with the larger precision)
    auto small_plus_medium = medium.get_added(small);
    std::cout << "  result type is: " << decltype(small_plus_medium)::type_info() << std::endl; 
    assert_equal(4, small_plus_medium.get_at(0), "Small + Medium at index 0 should be 1+3=4");
    assert_equal(7, small_plus_medium.get_at(1), "Small + Medium at index 1 should be 2+5=7");
    assert_equal(1, small_plus_medium.get_at(2), "Small + Medium at index 2 should be 3+6=9 mod 8 = 1");
    
    // OR
    auto small_or_medium = medium.get_ored(small);
    assert_equal(3, small_or_medium.get_at(0), "Small | Medium at index 0 should be 1|3=3");
    assert_equal(7, small_or_medium.get_at(1), "Small | Medium at index 1 should be 2|5=7");
    assert_equal(7, small_or_medium.get_at(2), "Small | Medium at index 2 should be 3|6=7");
    
    // AND
    auto small_and_medium = medium.get_anded(small);
    assert_equal(1, small_and_medium.get_at(0), "Small & Medium at index 0 should be 1&3=1");
    assert_equal(0, small_and_medium.get_at(1), "Small & Medium at index 1 should be 2&5=0");
    assert_equal(2, small_and_medium.get_at(2), "Small & Medium at index 2 should be 3&6=2");
    
    // XOR
    auto small_xor_medium = medium.get_xored(small);
    assert_equal(2, small_xor_medium.get_at(0), "Small ^ Medium at index 0 should be 1^3=2");
    assert_equal(7, small_xor_medium.get_at(1), "Small ^ Medium at index 1 should be 2^5=7");
    assert_equal(5, small_xor_medium.get_at(2), "Small ^ Medium at index 2 should be 3^6=5");
    
    // CASE 2: Medium vector operating with large vector
    std::cout << "  Testing medium + large operations:" << std::endl;
    
    // Addition
    auto medium_plus_large = large.get_added(medium);
    assert_equal(10, medium_plus_large.get_at(0), "Medium + Large at index 0 should be 3+7=10");
    assert_equal(15, medium_plus_large.get_at(1), "Medium + Large at index 1 should be 5+10=15");
    assert_equal(2, medium_plus_large.get_at(2), "Medium + Large at index 2 should be 6+12=18 mod 16 = 2");
    
    // OR
    auto medium_or_large = large.get_ored(medium);
    assert_equal(7, medium_or_large.get_at(0), "Medium | Large at index 0 should be 3|7=7");
    assert_equal(15, medium_or_large.get_at(1), "Medium | Large at index 1 should be 5|10=15");
    assert_equal(14, medium_or_large.get_at(2), "Medium | Large at index 2 should be 6|12=14");
    
    // AND
    auto medium_and_large = large.get_anded(medium);
    assert_equal(3, medium_and_large.get_at(0), "Medium & Large at index 0 should be 3&7=3");
    assert_equal(0, medium_and_large.get_at(1), "Medium & Large at index 1 should be 5&10=0");
    assert_equal(4, medium_and_large.get_at(2), "Medium & Large at index 2 should be 6&12=4");
    
    // XOR
    auto medium_xor_large = large.get_xored(medium);
    assert_equal(4, medium_xor_large.get_at(0), "Medium ^ Large at index 0 should be 3^7=4");
    assert_equal(15, medium_xor_large.get_at(1), "Medium ^ Large at index 1 should be 5^10=15");
    assert_equal(10, medium_xor_large.get_at(2), "Medium ^ Large at index 2 should be 6^12=10");
    
    // CASE 3: Small vector operating with large vector
    std::cout << "  Testing small + large operations:" << std::endl;
    
    // Addition
    auto small_plus_large = large.get_added(small);
    assert_equal(8, small_plus_large.get_at(0), "Small + Large at index 0 should be 1+7=8");
    assert_equal(12, small_plus_large.get_at(1), "Small + Large at index 1 should be 2+10=12");
    assert_equal(15, small_plus_large.get_at(2), "Small + Large at index 2 should be 3+12=15");
    
    // OR
    auto small_or_large = large.get_ored(small);
    assert_equal(7, small_or_large.get_at(0), "Small | Large at index 0 should be 1|7=7");
    assert_equal(10, small_or_large.get_at(1), "Small | Large at index 1 should be 2|10=10");
    assert_equal(15, small_or_large.get_at(2), "Small | Large at index 2 should be 3|12=15");
    
    // AND
    auto small_and_large = large.get_anded(small);
    assert_equal(1, small_and_large.get_at(0), "Small & Large at index 0 should be 1&7=1");
    assert_equal(2, small_and_large.get_at(1), "Small & Large at index 1 should be 2&10=2");
    assert_equal(0, small_and_large.get_at(2), "Small & Large at index 2 should be 3&12=0");
    
    // XOR
    auto small_xor_large = large.get_xored(small);
    assert_equal(6, small_xor_large.get_at(0), "Small ^ Large at index 0 should be 1^7=6");
    assert_equal(8, small_xor_large.get_at(1), "Small ^ Large at index 1 should be 2^10=8");
    assert_equal(15, small_xor_large.get_at(2), "Small ^ Large at index 2 should be 3^12=15");
    
    // Verify return type precision
    static_assert(std::is_same_v<decltype(small_plus_medium), bitwise::vector_int<uint8_t, 3>>, 
                 "Small + Medium should return a vector with medium precision (3 bits)");
    
    static_assert(std::is_same_v<decltype(medium_plus_large), bitwise::vector_int<uint8_t, 4>>, 
                 "Medium + Large should return a vector with large precision (4 bits)");
    
    static_assert(std::is_same_v<decltype(small_plus_large), bitwise::vector_int<uint8_t, 4>>, 
                 "Small + Large should return a vector with large precision (4 bits)");
    
    // NEW SECTION: Test operations when calling from smaller to larger vector
    std::cout << "  Testing small.operation(large) - Small calling operations on larger vector:" << std::endl;
    
    // Small OR Large
    auto small_or_large_reverse = small.get_ored(large);
    assert_equal(7, small_or_large_reverse.get_at(0), "Small.get_ored(Large) at index 0 should be 1|7=7");
    assert_equal(10, small_or_large_reverse.get_at(1), "Small.get_ored(Large) at index 1 should be 2|10=10");
    assert_equal(15, small_or_large_reverse.get_at(2), "Small.get_ored(Large) at index 2 should be 3|12=15");
    
    // Check that result has same size as small (2 bits)
    static_assert(std::is_same_v<decltype(small_or_large_reverse), bitwise::vector_int<uint8_t, 4>>, 
                 "Small.get_ored(Large) should return a vector with small's precision (2 bits)");
    
    // Small AND Large
    auto small_and_large_reverse = small.get_anded(large);
    assert_equal(1, small_and_large_reverse.get_at(0), "Small.get_anded(Large) at index 0 should be 1&7=1");
    assert_equal(2, small_and_large_reverse.get_at(1), "Small.get_anded(Large) at index 1 should be 2&10=2");
    assert_equal(0, small_and_large_reverse.get_at(2), "Small.get_anded(Large) at index 2 should be 3&12=0");
    
    // Check that result has same size as small (2 bits)
    static_assert(std::is_same_v<decltype(small_and_large_reverse), bitwise::vector_int<uint8_t, 4>>, 
                 "Small.get_anded(Large) should return a vector with small's precision (2 bits)");
    
    // Small XOR Large
    auto small_xor_large_reverse = small.get_xored(large);
    assert_equal(6, small_xor_large_reverse.get_at(0), "Small.get_xored(Large) at index 0 should be 1^7=6");
    assert_equal(8, small_xor_large_reverse.get_at(1), "Small.get_xored(Large) at index 1 should be 2^10=8");
    assert_equal(15, small_xor_large_reverse.get_at(2), "Small.get_xored(Large) at index 2 should be 3^12=15");
    
    // Check that result has same size as small (2 bits)
    static_assert(std::is_same_v<decltype(small_xor_large_reverse), bitwise::vector_int<uint8_t, 4>>, 
                 "Small.get_xored(Large) should return a vector with small's precision (2 bits)");
    
    // Test medium.operation(large)
    std::cout << "  Testing medium.operation(large) - Medium calling operations on larger vector:" << std::endl;
    
    // Medium OR Large
    auto medium_or_large_reverse = medium.get_ored(large);
    assert_equal(7, medium_or_large_reverse.get_at(0), "Medium.get_ored(Large) at index 0 should be 3|7=7");
    assert_equal(15, medium_or_large_reverse.get_at(1), "Medium.get_ored(Large) at index 1 should be 5|10=15");
    assert_equal(14, medium_or_large_reverse.get_at(2), "Medium.get_ored(Large) at index 2 should be 6|12=14");
    
    // Medium AND Large
    auto medium_and_large_reverse = medium.get_anded(large);
    assert_equal(3, medium_and_large_reverse.get_at(0), "Medium.get_anded(Large) at index 0 should be 3&7=3");
    assert_equal(0, medium_and_large_reverse.get_at(1), "Medium.get_anded(Large) at index 1 should be 5&10=0");
    assert_equal(4, medium_and_large_reverse.get_at(2), "Medium.get_anded(Large) at index 2 should be 6&12=4");
    
    // Medium XOR Large
    auto medium_xor_large_reverse = medium.get_xored(large);
    assert_equal(4, medium_xor_large_reverse.get_at(0), "Medium.get_xored(Large) at index 0 should be 3^7=4");
    assert_equal(15, medium_xor_large_reverse.get_at(1), "Medium.get_xored(Large) at index 1 should be 5^10=15");
    assert_equal(10, medium_xor_large_reverse.get_at(2), "Medium.get_xored(Large) at index 2 should be 6^12=10");
}

// Test shift operations
void test_vector_int_shifts() {
    begin_test("vector_int - Shift Operations");
    
    using vint = bitwise::vector_int<uint8_t, 3>;
    vint v;
    
    // Set up test values - fill with a pattern
    for (int i = 0; i < 8; i++) {
        v.set_at(i, i % 4); // Pattern: 0,1,2,3,0,1,2,3
    }
    
    // Test right shift
    auto v_right = v.get_right_shifted_vector(1);
    assert_equal(1, v_right.get_at(0), "Right shift by 1 should move values right");
    assert_equal(2, v_right.get_at(1), "Right shift by 1 should move values right");
    assert_equal(3, v_right.get_at(2), "Right shift by 1 should move values right");
    
    // Test left shift
    auto v_left = v.get_left_shifted_vector(1);
    assert_equal(0, v_left.get_at(0), "Left shift by 1 should move values left");
    assert_equal(0, v_left.get_at(1), "Left shift by 1 should move values left");
    assert_equal(1, v_left.get_at(2), "Left shift by 1 should move values left");
    
    // Test larger shifts
    auto v_right_2 = v.get_right_shifted_vector(2);
    assert_equal(2, v_right_2.get_at(0), "Right shift by 2 should move values right by 2");
    assert_equal(3, v_right_2.get_at(1), "Right shift by 2 should move values right by 2");
    assert_equal(0, v_right_2.get_at(2), "Right shift by 2 should move values right by 2");
    assert_equal(1, v_right_2.get_at(3), "Right shift by 2 should move values right by 2");
    assert_equal(2, v_right_2.get_at(4), "Right shift by 2 should move values right by 2");
}

// Test NOT operation
void test_vector_int_not() {
    begin_test("vector_int - NOT Operation");
    
    using vint = bitwise::vector_int<uint8_t, 3>;
    vint v;
    
    // Set alternating pattern of 0s and 1s
    for (int i = 0; i < 8; i++) {
        v.set_at(i, i % 2);
    }
    
    auto v_not = v.get_noted();
    
    // Check that all bits are inverted
    for (int i = 0; i < 8; i++) {
        auto expected = (v.get_at(i) == 0) ? 7 : 6; // NOT 0 = 7 (111), NOT 1 = 6 (110)
        assert_equal(expected, v_not.get_at(i), "NOT operation should invert all bits at index " + std::to_string(i));
    }
}

// Test constant operations
void test_vector_int_constant_operations() {
    begin_test("vector_int - Constant Operations");
    
    using vint = bitwise::vector_int<uint8_t, 3>;
    vint v;
    
    // Fill with consecutive values
    for (int i = 0; i < 8; i++) {
        v.set_at(i, i); // 0, 1, 2, 3, 4, 5, 6, 7
    }
    
    // Test AND with constant
    auto v_and_const = v.get_anded<2>(); // AND with 010
    for (int i = 0; i < 8; i++) {
        auto expected = v.get_at(i) & 2;
        assert_equal(expected, v_and_const.get_at(i), "AND with constant 2 at index " + std::to_string(i));
    }
    
    // Test OR with constant
    auto v_or_const = v.get_ored<3>(); // OR with 011
    for (int i = 0; i < 8; i++) {
        auto expected = v.get_at(i) | 3;
        assert_equal(expected, v_or_const.get_at(i), "OR with constant 3 at index " + std::to_string(i));
    }
     
    // Test XOR with constant
    auto v_xor_const = v.get_xored<5>(); // XOR with 101
    for (int i = 0; i < 8; i++) {
        auto expected = v.get_at(i) ^ 5;
        assert_equal(expected, v_xor_const.get_at(i), "XOR with constant 5 at index " + std::to_string(i));
    }
}

// Test loading from storage
void test_vector_int_load_from() {
    begin_test("vector_int - Load From Storage");
    
    using vint = bitwise::vector_int<uint8_t, 3>;
    
    // Create storage vectors with test data
    std::vector<uint8_t> b0 = {0xF0, 0xAA, 0x00, 0x00};  // 11110000, 10101010
    std::vector<uint8_t> b1 = {0x0F, 0xCC, 0x00, 0x00};  // 00001111, 11001100
    std::vector<uint8_t> b2 = {0x00, 0xF0, 0x00, 0x00};  // 00000000, 11110000
    
    // Create storage tuples
    std::tuple<uint8_t*, uint8_t*, uint8_t*> storage = {b0.data(), b1.data(), b2.data()};
    
    // Load from storage at offset 0
    vint v0 = vint::load_from(storage, 0);
    assert_equal(2, v0.get_at(0), "First bit from storage at offset 0 should be 2");
    assert_equal(2, v0.get_at(1), "Second bit from storage at offset 0 should be 2");
    assert_equal(2, v0.get_at(2), "Third bit from storage at offset 0 should be 2");
    assert_equal(2, v0.get_at(3), "Fourth bit from storage at offset 0 should be 2");
    assert_equal(1, v0.get_at(4), "Fifth bit from storage at offset 0 should be 1");
    
    // Load from storage at offset 1
    vint v1 = vint::load_from(storage, 1);
    assert_equal(0, v1.get_at(0), "First bit from storage at offset 1 should be 0");
    assert_equal(1, v1.get_at(1), "Second bit from storage at offset 1 should be 1");
    assert_equal(2, v1.get_at(2), "Third bit from storage at offset 1 should be 2");
    assert_equal(3, v1.get_at(3), "Fourth bit from storage at offset 1 should be 3");
    assert_equal(4, v1.get_at(4), "Fifth bit from storage at offset 1 should be 4");
    
    // Test with smaller storage
    std::tuple<uint8_t*, uint8_t*> storage_small = {b0.data(), b1.data()};
    vint v2 = vint::load_from(storage_small, 1);
    
    assert_equal(0, v2.get_at(0), "First bit from small storage should be 0");
    assert_equal(1, v2.get_at(1), "Second bit from small storage should be 0");
    // Third bit should be zeroed since storage doesn't have it
}

// Test equals_to operations
void test_vector_int_equals_to() {
    begin_test("vector_int - Equals To Operations");
    
    using vint = bitwise::vector_int<uint8_t, 3>;
    vint v1, v2, v3;
    
    // Set up test values
    v1.set_at(0, 1);
    v1.set_at(1, 2);
    v1.set_at(2, 3);
    v1.set_at(3, 4);
    
    // v2 is identical to v1
    v2.set_at(0, 1);
    v2.set_at(1, 2);
    v2.set_at(2, 3);
    v2.set_at(3, 4);
    
    // v3 is different
    v3.set_at(0, 2);
    v3.set_at(1, 2); 
    v3.set_at(2, 3);
    v3.set_at(3, 5);
    
    // Test equals_to between vectors
    auto eq1_2 = v1.equals_to(v2);
    auto eq1_3 = v1.equals_to(v3);
    
    // Since equals_to returns a mask with 1s where equal
    // For identical vectors, we expect all 1s (0xFF)
    assert_equal(static_cast<int>(0xFF), static_cast<int>(eq1_2), 
                "equals_to should return all ones for identical vectors");
    
    // For different vectors, we expect some 0s
    assert_true(eq1_3 != 0xFF, 
                "equals_to should not return all ones for different vectors");
    
    // Test equals_to with constant
    auto eq1_const = v1.equals_to<1>();
    
    // Since v1[0] = 1, we expect bit 0 to be set in the equals_to result
    assert_true((eq1_const & 0x01) != 0, 
                "equals_to<1> should have bit 0 set for vector with 1 at position 0");
}

// Test random operations between vector_int instances of different sizes
void test_vector_int_random_operations() {
    begin_test("vector_int - Random Operations");
    
    // Set random seed for reproducible tests
    std::srand(42);
    
    // Define vectors with different bit precisions
    using vint_small = bitwise::vector_int<uint8_t, 2>;  // 2-bit precision (0-3)
    using vint_medium = bitwise::vector_int<uint8_t, 3>; // 3-bit precision (0-7)
    using vint_large = bitwise::vector_int<uint8_t, 4>;  // 4-bit precision (0-15)
    
    const int num_iterations = 1000;
    const int max_idx = 7; // Test up to 8 cells
    
    // Save previous verbosity setting and disable it for this test
    bool original_verbose = verbose_output;
    verbose_output = false;
    
    std::cout << "  Running " << num_iterations << " random operation tests..." << std::endl;
    
    for (int iter = 0; iter < num_iterations; iter++) {
        // Create vector instances
        vint_small small;
        vint_medium medium;
        vint_large large;
        
        // Fill with random values within their valid ranges
        for (int i = 0; i <= max_idx; i++) {
            int small_val = std::rand() % 4;   // 0-3 (2 bits)
            int medium_val = std::rand() % 8;  // 0-7 (3 bits)
            int large_val = std::rand() % 16;  // 0-15 (4 bits)
            
            small.set_at(i, small_val);
            medium.set_at(i, medium_val);
            large.set_at(i, large_val);
        }
        
        // Test index to verify (also random)
        int test_idx = std::rand() % (max_idx + 1);
        
        // Get the values at the test index
        int small_val = small.get_at(test_idx);
        int medium_val = medium.get_at(test_idx);
        int large_val = large.get_at(test_idx);
        
        // Test 1: Addition between different precision vectors
        // Note: addition returns vector with the larger precision
        
        // Small + Medium (returns Medium precision)
        auto small_plus_medium = small.get_added(medium);
        int expected_sum1 = (small_val + medium_val) % 8;  // Result must fit in 3 bits
        assert_equal(expected_sum1, small_plus_medium.get_at(test_idx), 
                     "Random Small + Medium at index " + std::to_string(test_idx));
        
        // Medium + Large (returns Large precision)
        auto medium_plus_large = medium.get_added(large);
        int expected_sum2 = (medium_val + large_val) % 16;  // Result must fit in 4 bits
        assert_equal(expected_sum2, medium_plus_large.get_at(test_idx),
                     "Random Medium + Large at index " + std::to_string(test_idx));
        
        // Small + Large (returns Large precision)
        auto small_plus_large = small.get_added(large);
        int expected_sum3 = (small_val + large_val) % 16;  // Result must fit in 4 bits
        assert_equal(expected_sum3, small_plus_large.get_at(test_idx),
                     "Random Small + Large at index " + std::to_string(test_idx));
        
        // Test 2: Bitwise operations (only smaller bit vector can be given as parameter)
        
        // Large OR Small
        auto large_or_small = large.get_ored(small);
        int expected_or1 = large_val | small_val;
        assert_equal(expected_or1, large_or_small.get_at(test_idx),
                     "Random Large | Small at index " + std::to_string(test_idx));
        
        // Large AND Small
        auto large_and_small = large.get_anded(small);
        int expected_and1 = large_val & small_val;
        assert_equal(expected_and1, large_and_small.get_at(test_idx),
                     "Random Large & Small at index " + std::to_string(test_idx));
        
        // Large XOR Small
        auto large_xor_small = large.get_xored(small);
        int expected_xor1 = large_val ^ small_val;
        assert_equal(expected_xor1, large_xor_small.get_at(test_idx),
                     "Random Large ^ Small at index " + std::to_string(test_idx));
        
        // Medium OR Small
        auto medium_or_small = medium.get_ored(small);
        int expected_or2 = medium_val | small_val;
        assert_equal(expected_or2, medium_or_small.get_at(test_idx),
                     "Random Medium | Small at index " + std::to_string(test_idx));
        
        // Medium AND Small
        auto medium_and_small = medium.get_anded(small);
        int expected_and2 = medium_val & small_val;
        assert_equal(expected_and2, medium_and_small.get_at(test_idx),
                     "Random Medium & Small at index " + std::to_string(test_idx));
        
        // Medium XOR Small
        auto medium_xor_small = medium.get_xored(small);
        int expected_xor2 = medium_val ^ small_val;
        assert_equal(expected_xor2, medium_xor_small.get_at(test_idx),
                     "Random Medium ^ Small at index " + std::to_string(test_idx));
        
        // NEW SECTION: Testing Small.operation(Large) - small vector calling operations on larger vector
        // Small OR Large
        auto small_or_large_reverse = small.get_ored(large);
        int expected_or1_rev = small_val | large_val;
        assert_equal(expected_or1_rev, small_or_large_reverse.get_at(test_idx),
                     "Random Small.get_ored(Large) at index " + std::to_string(test_idx));
        
        // Small AND Large
        auto small_and_large_reverse = small.get_anded(large);
        int expected_and1_rev = small_val & large_val;
        assert_equal(expected_and1_rev, small_and_large_reverse.get_at(test_idx),
                     "Random Small.get_anded(Large) at index " + std::to_string(test_idx));
        
        // Small XOR Large
        auto small_xor_large_reverse = small.get_xored(large);
        int expected_xor1_rev = small_val ^ large_val;
        assert_equal(expected_xor1_rev, small_xor_large_reverse.get_at(test_idx),
                     "Random Small.get_xored(Large) at index " + std::to_string(test_idx));
        
        // Medium OR Large
        auto medium_or_large_reverse = medium.get_ored(large);
        int expected_or2_rev = medium_val | large_val;
        assert_equal(expected_or2_rev, medium_or_large_reverse.get_at(test_idx),
                     "Random Medium.get_ored(Large) at index " + std::to_string(test_idx));
        
        // Medium AND Large
        auto medium_and_large_reverse = medium.get_anded(large);
        int expected_and2_rev = medium_val & large_val;
        assert_equal(expected_and2_rev, medium_and_large_reverse.get_at(test_idx),
                     "Random Medium.get_anded(Large) at index " + std::to_string(test_idx));
        
        // Medium XOR Large
        auto medium_xor_large_reverse = medium.get_xored(large);
        int expected_xor2_rev = medium_val ^ large_val;
        assert_equal(expected_xor2_rev, medium_xor_large_reverse.get_at(test_idx),
                     "Random Medium.get_xored(Large) at index " + std::to_string(test_idx));
        
        // Test 3: Constant operations (random constant based on vector size)
        int small_const = std::rand() % 4;  // Random 2-bit constant
        
        // Small AND Constant
        auto small_and_const = small.get_anded<2>();  // Using 2 as constant for simplicity
        int expected_and_const = small_val & 2;
        assert_equal(expected_and_const, small_and_const.get_at(test_idx),
                     "Random Small & Const at index " + std::to_string(test_idx));
        
        // Test 4: Shift operations with random shift amount
        int shift_amount = 1 + (std::rand() % 3);  // Random shift 1-3 positions
        
        // Shift operations depend on implementation details, so we calculate expected result
        // based on how your implementation works
        
        // Left shift
        auto small_left_shift = small.get_left_shifted_vector(shift_amount);
        int expected_left_shift = (test_idx >= shift_amount) ? 
                                  small.get_at(test_idx - shift_amount) : 0;
        assert_equal(expected_left_shift, small_left_shift.get_at(test_idx),
                     "Random left shift by " + std::to_string(shift_amount) + 
                     " at index " + std::to_string(test_idx));
        
        // Right shift
        auto medium_right_shift = medium.get_right_shifted_vector(shift_amount);
        int expected_right_shift = (test_idx + shift_amount <= max_idx) ? 
                                   medium.get_at(test_idx + shift_amount) : 0;
        assert_equal(expected_right_shift, medium_right_shift.get_at(test_idx),
                     "Random right shift by " + std::to_string(shift_amount) + 
                     " at index " + std::to_string(test_idx));
    }
    
    // Print summary of this random test
    std::cout << GREEN << "  Completed " << num_iterations << " random tests" << RESET << std::endl;
    
    // Restore original verbosity
    verbose_output = original_verbose;
}

int main() {
    std::cout << CYAN << "========================================" << std::endl;
    std::cout << "   RUNNING VECTOR_INT CLASS UNIT TESTS" << std::endl;
    std::cout << "========================================" << RESET << std::endl;
    
    // Run all tests
    test_vector_int_basics();
    test_vector_int_binary_operations();
    test_vector_int_mixed_precision_operations();
    test_vector_int_shifts();
    test_vector_int_not();
    test_vector_int_constant_operations();
    test_vector_int_load_from();
    test_vector_int_equals_to();
    test_vector_int_random_operations(); // Add the new random tests
    
    print_summary();
    
    // Return success if all tests pass
    return tests_passed == tests_run ? 0 : 1;
}
