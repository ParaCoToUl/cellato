#ifndef GRID_UTILS_HPP
#define GRID_UTILS_HPP

#include <iostream>
#include <vector>
#include <string>
#include <tuple>
#include <random>
#include <algorithm>
#include <numeric>
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
                
                if (differences.size() < max_diffs_to_print) {
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
        
        if (diff_count > max_diffs_to_print) {
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

} // namespace grid_utils

#endif // GRID_UTILS_HPP