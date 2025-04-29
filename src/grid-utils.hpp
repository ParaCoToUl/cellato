#ifndef GRID_UTILS_HPP
#define GRID_UTILS_HPP

#include <iostream>
#include <vector>
#include <string>
#include <tuple>
#include <random>
#include <algorithm>
#include <numeric>
#include <chrono>
#include <iomanip>
#include <type_traits> // For std::is_enum

namespace grid_utils {

// Helper function to print enum values properly
template <typename T>
typename std::enable_if<std::is_enum<T>::value, std::ostream&>::type
operator<<(std::ostream& os, const T& value) {
    return os << static_cast<typename std::underlying_type<T>::type>(value);
}

// Print a grid with default formatting
template <typename T>
void print_grid(const T* grid, std::size_t height, std::size_t width) {
    for (std::size_t i = 0; i < height; ++i) {
        for (std::size_t j = 0; j < width; ++j) {
            auto state = grid[i * width + j];
            std::cout << state << " ";
        }
        std::cout << "\n";
    }
}

// Compare two grids and report differences
// Returns true if grids are identical, false if differences are found
template <typename CellState>
bool compare_grids(
    const std::vector<CellState>& grid1,
    const std::vector<CellState>& grid2,
    std::size_t height, std::size_t width,
    bool print_diffs = false,
    int max_diffs_to_print = 10) {

    // ANSI color codes for output formatting
    const std::string RESET = "\033[0m";
    const std::string RED = "\033[31m";

    if (grid1.size() != grid2.size()) {
        if (print_diffs) {
            std::cout << "Grids have different sizes: " << grid1.size() << " vs " << grid2.size() << std::endl;
        }
        return false;
    }

    bool equal = true;
    int diff_count = 0;
    std::vector<std::tuple<std::size_t, std::size_t, CellState, CellState>> differences;

    // Find all differences
    for (std::size_t y = 0; y < height; ++y) {
        for (std::size_t x = 0; x < width; ++x) {
            std::size_t idx = y * width + x;
            if (grid1[idx] != grid2[idx]) {
                equal = false;
                diff_count++;

                if ((int)differences.size() < max_diffs_to_print) {
                    differences.emplace_back(x, y, grid1[idx], grid2[idx]);
                }
            }
        }
    }

    // Print differences if requested
    if (!equal && print_diffs) {
        for (const auto& diff : differences) {
            std::size_t x = std::get<0>(diff);
            std::size_t y = std::get<1>(diff);
            CellState val1 = std::get<2>(diff);
            CellState val2 = std::get<3>(diff);

            std::cout << "Difference at (" << x << ", " << y << "): "
                      << "Grid1=" << val1
                      << ", Grid2=" << val2
                      << std::endl;
        }

        if ((int)diff_count > max_diffs_to_print) {
            std::cout << "More than " << max_diffs_to_print << " differences found..." << std::endl;
        }

        std::cout << "Total differences: " << diff_count << " out of " << (height * width) << " cells"
                  << " (" << (100.0 * diff_count / (height * width)) << "%)" << std::endl;
    }

    return equal;
}

// Generate a random grid with specified distribution of cell states
// Each tuple in probabilities contains (state, probability)
// Probabilities should sum to 1.0
template <typename CellState>
void generate_random_grid(
    std::vector<CellState>& grid,
    std::size_t height, std::size_t width,
    const std::vector<std::tuple<CellState, double>>& probabilities,
    unsigned int seed = 12345) {

    // Verify probabilities sum to approximately 1.0 (allowing for small floating point errors)
    double sum = 0.0;
    for (const auto& [state, prob] : probabilities) {
        sum += prob;
    }

    if (std::abs(sum - 1.0) > 0.001) {
        std::cerr << "Warning: Probabilities sum to " << sum << " instead of 1.0" << std::endl;
    }

    // Create cumulative distribution
    std::vector<std::tuple<CellState, double>> cumulative_dist;
    double cumulative = 0.0;

    for (const auto& [state, prob] : probabilities) {
        cumulative += prob;
        cumulative_dist.emplace_back(state, cumulative);
    }

    // Initialize random number generator
    std::mt19937 rng(seed);
    std::uniform_real_distribution<double> dist(0.0, 1.0);

    // Fill grid with random values based on the distribution
    for (std::size_t i = 0; i < height * width; ++i) {
        double r = dist(rng);

        // Find the first state with cumulative probability > r
        for (const auto& [state, threshold] : cumulative_dist) {
            if (r <= threshold) {
                grid[i] = state;
                break;
            }
        }
    }
}

// Shorthand for common case with just two states
template <typename CellState>
void generate_random_grid(
    std::vector<CellState>& grid,
    std::size_t height, std::size_t width,
    CellState primary_state, double primary_probability,
    CellState secondary_state,
    unsigned int seed = 12345) {

    std::vector<std::tuple<CellState, double>> probabilities = {
        {primary_state, primary_probability},
        {secondary_state, 1.0 - primary_probability}
    };

    generate_random_grid(grid, height, width, probabilities, seed);
}

// Check if width is a multiple of the target word size in bits
// Returns adjusted width if needed
template <typename WordType>
std::size_t adjust_width_for_word_size(std::size_t width, bool adjust_if_needed = true) {
    std::size_t bits_per_word = sizeof(WordType) * 8;

    if (width % bits_per_word != 0) {
        if (adjust_if_needed) {
            // Round up to the next multiple of bits_per_word
            return ((width / bits_per_word) + 1) * bits_per_word;
        }
        return 0; // Signal error
    }

    return width; // Already a valid width
}

// Performance measurement function that runs iterators for multiple iterations
template <typename Iterator1, typename Iterator2>
void measure_performance(Iterator1& iter1, Iterator2& iter2,
                         const std::string& name1, const std::string& name2,
                         int iterations) {
    std::cout << "Running performance measurement for " << iterations << " iterations..." << std::endl;

    std::cout << "Warming up..." << std::endl;

    // Warm up the first iterator
    for (int i = 0; i < iterations; i++) {
        iter1.template run<false>(1);
    }

    std::cout << "Running hot..." << std::endl;

    // Start timers and run first iterator
    auto start1 = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < iterations; i++) {
        iter1.template run<false>(1);
    }

    auto end1 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed1 = end1 - start1;

    // Reset iterator1 state by re-initializing (if needed)

    // warm up

    std::cout << "Warming up..." << std::endl;

    for (int i = 0; i < iterations; i++) {
        iter2.template run<false>(1);
    }

    std::cout << "Running hot..." << std::endl;

    // Run second iterator
    auto start2 = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < iterations; i++) {
        iter2.template run<false>(1);
    }

    auto end2 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed2 = end2 - start2;

    // Print performance results
    std::cout << "=== Performance Results ===" << std::endl;
    std::cout << std::fixed << std::setprecision(3);
    std::cout << name1 << ": " << elapsed1.count() << " ms" << std::endl;
    std::cout << name2 << ": " << elapsed2.count() << " ms" << std::endl;
    std::cout << "Performance ratio (" << name1 << "/" << name2 << "): "
              << std::setprecision(2) << (elapsed1.count() / elapsed2.count()) << "x" << std::endl;
    std::cout << "==========================" << std::endl;
}

// Function to print two grids side-by-side with optional formatter
template <typename CellState, typename Formatter>
void print_grids_side_by_side(
    const std::vector<CellState>& grid1, const std::vector<CellState>& grid2,
    std::size_t height, std::size_t width,
    const std::string& label1, const std::string& label2,
    Formatter formatter) {

    // Print headers
    std::cout << std::setw(width * 2) << std::left << label1
              << " | " << std::setw(width * 2) << std::left << label2
              << std::endl;

    std::cout << std::string(width * 2 + 3 + width * 2, '-') << std::endl;

    // Print grids side by side
    for (std::size_t y = 0; y < height; ++y) {
        // Print first grid row
        for (std::size_t x = 0; x < width; ++x) {
            std::cout << formatter(grid1[y * width + x]);
        }

        std::cout << " | "; // Separator

        // Print second grid row
        for (std::size_t x = 0; x < width; ++x) {
            std::cout << formatter(grid2[y * width + x]);
        }

        std::cout << std::endl;
    }
}

// Overload for default formatter
template <typename CellState>
void print_grids_side_by_side(
    const std::vector<CellState>& grid1, const std::vector<CellState>& grid2,
    std::size_t height, std::size_t width,
    const std::string& label1 = "Grid 1", const std::string& label2 = "Grid 2") {

    print_grids_side_by_side(grid1, grid2, height, width, label1, label2,
        [](const CellState& state) -> std::string {
            auto symbol = std::to_string(static_cast<typename std::underlying_type<CellState>::type>(state));
            if (symbol == "0") {
                return "\033[1;90m" + symbol + "\033[0m "; // Dead cell
            } else {
                return "\033[1;33m" + symbol + "\033[0m "; // Alive cell
            }
        });
}

// Run both iterators in parallel for comparison - modified to deduce cell state type
template <typename Iterator1, typename Iterator2>
void compare_iterators_step_by_step(
    Iterator1& iter1, Iterator2& iter2,
    const std::string& name1, const std::string& name2,
    std::size_t height, std::size_t width,
    int steps,
    bool print_grids_on_match = false,
    bool print_grids_on_mismatch = true,
    bool print_diff_details = true) {

    // Run first iteration to get cell state type
    iter1.template run<false>(1);
    iter2.template run<false>(1);

    // Get results to deduce cell state type
    auto result1 = iter1.get_result();
    auto result2 = iter2.get_result();

    // Use the actual results for the first comparison
    bool equal = compare_grids(result1, result2, height, width, print_diff_details, 10);

    // Print step info
    std::cout << "===== Step 1 of " << steps << " =====" << std::endl;
    std::cout << name1 << " vs " << name2 << ": "
              << (equal ? "\033[1;32mMATCH\033[0m" : "\033[1;31mMISMATCH\033[0m")
              << std::endl;

    // Print grids if requested
    if ((equal && print_grids_on_match) || (!equal && print_grids_on_mismatch)) {
        print_grids_side_by_side(result1, result2, height, width, name1, name2);
    }

    std::cout << std::endl;

    // Continue with the remaining steps
    for (int step = 1; step < steps; ++step) {
        // Run both iterators for one step
        iter1.template run<false>(1);
        iter2.template run<false>(1);

        // Get results
        result1 = iter1.get_result();
        result2 = iter2.get_result();

        // Compare results
        equal = compare_grids(result1, result2, height, width, print_diff_details && !equal, 10);

        // Print step info
        std::cout << "===== Step " << step + 1 << " of " << steps << " =====" << std::endl;
        std::cout << name1 << " vs " << name2 << ": "
                  << (equal ? "\033[1;32mMATCH\033[0m" : "\033[1;31mMISMATCH\033[0m")
                  << std::endl;

        // Print grids if requested
        if ((equal && print_grids_on_match) || (!equal && print_grids_on_mismatch)) {
            print_grids_side_by_side(result1, result2, height, width, name1, name2);
        }

        std::cout << std::endl;
    }
}

// Enum for print verbosity levels
enum class PrintMode {
    Silent,      // No printing
    Minimal,     // Print only step numbers and match/mismatch status
    Differences, // Print differences when found
    Verbose      // Print all grids at each step
};

} // namespace grid_utils

#endif // GRID_UTILS_HPP