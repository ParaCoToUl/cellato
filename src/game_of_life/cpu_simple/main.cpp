#include <iostream>
#include <string>
#include <map>
#include "memory/standard_grid.hpp"

// Simple cell state enum for Game of Life
enum class CellState {
    Dead,
    Alive
};

// Custom stream operator for CellState (for debugging)
std::ostream& operator<<(std::ostream& os, const CellState& state) {
    os << (state == CellState::Alive ? "Alive" : "Dead");
    return os;
}

int main() {
    std::cout << "=== Testing Standard Grid Implementation ===\n" << std::endl;
    
    // Create a grid of size 10x10
    cellib::memory::grids::standard::grid<CellState> grid(10, 10);
    
    // Initialize with all dead cells
    std::fill(grid.data(), grid.data() + 100, CellState::Dead);
    
    // Add a glider pattern
    // .O.
    // ..O
    // OOO
    auto* data = grid.data();
    data[1 + 10 * 0] = CellState::Alive;
    data[2 + 10 * 1] = CellState::Alive;
    data[0 + 10 * 2] = CellState::Alive;
    data[1 + 10 * 2] = CellState::Alive;
    data[2 + 10 * 2] = CellState::Alive;
    
    // Set up a print configuration
    auto config = cellib::memory::grids::standard::print_config<CellState>().
        with(CellState::Dead, ".").
        with(CellState::Alive, "O");

    // Print the grid
    std::cout << "Initial grid with glider pattern:" << std::endl;
    grid.print(std::cout, config);

    // Test adding margins - use the new functional approach
    std::cout << "\nAdding margins..." << std::endl;
    auto grid_with_margins = grid.with_empty_margins<2, 2>();
    
    std::cout << "Grid after adding margins:" << std::endl;
    grid_with_margins.print(std::cout, config);

    // Verify dimensions of the original and new grid
    std::cout << "\nOriginal grid dimensions:" << std::endl;
    std::cout << "Physical size: " << grid.x_size_physical() << " x " << grid.y_size_physical() << std::endl;
    std::cout << "Logical size: " << grid.x_size_logical() << " x " << grid.y_size_logical() << std::endl;
    
    std::cout << "\nNew grid with margins dimensions:" << std::endl;
    std::cout << "Physical size: " << grid_with_margins.x_size_physical() << " x " << grid_with_margins.y_size_physical() << std::endl;
    std::cout << "Logical size: " << grid_with_margins.x_size_logical() << " x " << grid_with_margins.y_size_logical() << std::endl;
    
    return 0;
}
