#ifndef CELLIB_MEMORY_STANDARD_GRID_HPP
#define CELLIB_MEMORY_STANDARD_GRID_HPP

#include <cstddef>
#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <sstream>

#include "./interface.hpp"

namespace cellib::memory::grids::standard {
template <typename cell_type>
class print_config;

template <typename cell_type>
class grid {
public:
    using cell_t = cell_type;
    constexpr static bool HAS_OWN_PRINT = true;

    grid(std::size_t x_size, std::size_t y_size)
        : _properties{x_size, y_size}, _data(x_size * y_size) {}

    grid() = default;

    cell_type* data() const {
        return const_cast<cell_type*>(_data.data());
    }

    cell_type* data() {
        return _data.data();
    }

    std::size_t x_size_physical() const {
        return _properties.x_size;
    }

    std::size_t y_size_physical() const {
        return _properties.y_size;
    }

    std::size_t x_size_logical() const {
        return _properties.x_size;
    }

    std::size_t y_size_logical() const {
        return _properties.y_size;
    }

    template <int x_margin, int y_margin>
    grid<cell_type> with_empty_margins() const {
        grids::properties new_properties {
            _properties.x_size + 2 * x_margin,
            _properties.y_size + 2 * y_margin
        };

        std::vector<cell_type> new_data(new_properties.x_size * new_properties.y_size, cell_type{});

        for (std::size_t y = 0; y < _properties.y_size; ++y) {
            for (std::size_t x = 0; x < _properties.x_size; ++x) {
                new_data[new_properties.idx(x + x_margin, y + y_margin)] = _data[_properties.idx(x, y)];
            }
        }

        return grid<cell_type>(new_properties, std::move(new_data));
    }

    grid<cell_type> to_standard() const {
        return *this;
    }

    void print(std::ostream& os, print_config<cell_type> config = print_config<cell_type>()) const {
        for (std::size_t y = 0; y < _properties.y_size; ++y) {
            for (std::size_t x = 0; x < _properties.x_size; ++x) {
                os << config.get_str(_data[_properties.idx(x, y)]) << " ";
            }
            os << "\n";
        }
    }

    private:
    grids::properties _properties;
    std::vector<cell_type> _data;

    grid(const grids::properties& properties, std::vector<cell_type> data)
        : _properties(properties), _data(std::move(data)) {}
};

template <typename cell_type>
class print_config {
public:
    print_config& with(cell_type state, const std::string& symbol) {
        _state_to_symbol[state] = symbol;
        return *this;
    }

    std::string get_str(cell_type state) const {
        auto it = _state_to_symbol.find(state);
        
        if (it != _state_to_symbol.end()) {
            return it->second;
        }
        
        auto state_as_int = static_cast<int>(state);
        return std::to_string(state_as_int);
    }

    static print_config<cell_type> empty() {
        return print_config<cell_type>();
    }

private:
    std::map<cell_type, std::string> _state_to_symbol;
};

}

#endif // CELLIB_MEMORY_STANDARD_GRID_HPP