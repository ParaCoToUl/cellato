#include <iostream>
#include "tests/manager.hpp"
#include "tests/vector-int.hpp"

int main(int argc, char* argv[]) {
    // Register all test suites
    cellib::tests::register_vector_int_tests();
    
    // Get the test manager
    auto& manager = cellib::tests::test_manager::instance();
    
    cellib::tests::test_result result;
    
    // Run specific test suite if provided as argument
    if (argc > 1) {
        std::string suite_name = argv[1];
        result = manager.run_suite(suite_name);
    } else {
        // Otherwise run all test suites
        result = manager.run_all();
    }
    
    // Return non-zero exit code if any tests failed
    return result.all_passed() ? 0 : 1;
}
