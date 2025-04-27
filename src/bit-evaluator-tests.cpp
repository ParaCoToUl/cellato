#include <iostream>
#include <vector>
#include <cstdint>

#include "bit-evaluator.hpp"
#include "bit-mode.hpp"

using namespace expr_tree;
using namespace bitwise;

enum class cell_state { dead, alive, dying };
using dict_t = state_dictionary<cell_state, cell_state::dead, cell_state::alive, cell_state::dying>;

using dead = state_constant<cell_state, cell_state::dead>;
using alive = state_constant<cell_state, cell_state::alive>;
using dying = state_constant<cell_state, cell_state::dying>;

template <typename Algorithm>
using bitwise_evaluator_t = bitwise_no_cache::evaluator<uint8_t, dict_t, Algorithm>;

void neighbor_accessor() {

    using top_left = neighbor_at<-1, -1>;
    using top = neighbor_at<0, -1>;
    using top_right = neighbor_at<1, -1>;
    using left = neighbor_at<-1, 0>;
    using self = neighbor_at<0, 0>;
    using right = neighbor_at<1, 0>;
    using bottom_left = neighbor_at<-1, 1>;
    using bottom = neighbor_at<0, 1>;
    using bottom_right = neighbor_at<1, 1>;

    std::vector<uint8_t> grid_0th_bit = {
        0b00000000, 0b00000000, 0b00000000,
        0b10000000, 0b00000000, 0b00000000,
        0b10000000, 0b00000000, 0b00000000,
        0b10000000, 0b00000000, 0b00000000,
        // 0b00000001, 0b00000010, 0b00000011,
        // 0b10000100, 0b00000101, 0b00000111,
        // 0b00000111, 0b00001000, 0b00001001,
    };

    std::vector<uint8_t> grid_1st_bit = {
        0b00000000, 0b00000000, 0b00000000,
        0b00000000, 0b00000000, 0b00000000,
        0b00000000, 0b00000000, 0b00000000,
        0b00000000, 0b00000000, 0b00000000,
    };

    std::tuple<uint8_t*, uint8_t*> grid = { grid_0th_bit.data(), grid_1st_bit.data() };
    std::size_t height = 4;
    std::size_t width = 3;

    bitwise_no_cache::grid_config<uint8_t, dict_t> state;
    state.bit_grid = grid;
    state.height_b = height;
    state.width_b = width;

    state.x = 1;
    state.y = 2;

    std::vector<std::string> neighbors_values = {
        "top_left    ",
        bitwise_evaluator_t<top_left>::evaluate(state).to_str(),
        "top         ",
        bitwise_evaluator_t<top>::evaluate(state).to_str(),
        "top_right   ",
        bitwise_evaluator_t<top_right>::evaluate(state).to_str(),
        "left        ",
        bitwise_evaluator_t<left>::evaluate(state).to_str(),
        "self        ",
        bitwise_evaluator_t<self>::evaluate(state).to_str(),
        "right       ",
        bitwise_evaluator_t<right>::evaluate(state).to_str(),
        "bottom_left ",
        bitwise_evaluator_t<bottom_left>::evaluate(state).to_str(),
        "bottom      ",
        bitwise_evaluator_t<bottom>::evaluate(state).to_str(),
        "bottom_right",
        bitwise_evaluator_t<bottom_right>::evaluate(state).to_str(),
    };

    std::vector<std::string> expected_values = {
        "top_left    ",
        "1 0 0 0 0 0 0 0",
        "top         ",
        "0 1 0 0 0 0 0 0",
        "top_right   ",
        "0 0 0 0 0 0 0 0",
        "left        ",
        "0 0 0 0 0 0 0 0",
        "self        ",
        "1 0 1 0 0 0 0 0",
        "right       ",
        "0 0 0 0 0 0 0 0",
        "bottom_left ",
        "0 0 0 0 0 0 0 0",
        "bottom      ",
        "0 0 0 1 0 0 0 0",
        "bottom_right",
        "0 0 0 0 0 0 0 0",
    };


    for (std::size_t i = 0; i < neighbors_values.size(); i += 2) {
        std::cout << neighbors_values[i] << ": ";

        for(auto&& c : neighbors_values[i + 1]) {
            if (c == '1') {
                std::cout << "\033[1;32m" << c << "\033[0m";
            } else if (c == '0') {
                std::cout << "\033[1;31m" << c << "\033[0m";
            } else {
                std::cout << c;
            }
        }

        std::cout << "\n  expected  " << ": " << expected_values[i + 1] << std::endl << std::endl;
    }

}

void neighborhood_sum() {
    using sum = count_neighbors<alive, moore_8_neighbors>;

    std::vector<uint8_t> grid_0th_bit = {
        0b00000000, 0b00000000, 0b00000000,
        0b00000000, 0b00000011, 0b00000000,
        0b10000000, 0b00000010, 0b00000000,
        0b00000000, 0b00000011, 0b00000000,
    };

    std::vector<uint8_t> grid_1st_bit = {
        0b00000000, 0b00000000, 0b00000000,
        0b00000000, 0b00000000, 0b00000000,
        0b00000000, 0b00000000, 0b00000000,
        0b00000000, 0b00000001, 0b00000000,
    };

    std::tuple<uint8_t*, uint8_t*> grid = { grid_0th_bit.data(), grid_1st_bit.data() };
    std::size_t height = 4;
    std::size_t width = 3;

    bitwise_no_cache::grid_config<uint8_t, dict_t> state;
    state.bit_grid = grid;
    state.height_b = height;
    state.width_b = width;

    state.x = 1;
    state.y = 2;

    auto result = bitwise_no_cache::evaluator<uint8_t, dict_t, sum>::evaluate(state);

    std::cout << "Type: " << decltype(result)::type_info() << std::endl;
    std::cout << "Result: " << result.to_str() << std::endl;
}

void if_then_else_test() {
    using cell_is_dying = p<current_state, equals, dying>;
    using cell_is_dying = p<current_state, equals, dying>;
    using next_state = if_<cell_is_dying>
        ::then_<dead>
        ::else_<alive>;
    
    using next_state_no_sugar = if_then_else<cell_is_dying, dead, alive>;

    std::vector<uint8_t> grid_0th_bit = {
        0b00000000, 0b00000000, 0b00000000,
        0b00000000, 0b00000011, 0b00000000,
        0b10000000, 0b01010010, 0b00000000,
        0b00000000, 0b00000011, 0b00000000,
    };

    std::vector<uint8_t> grid_1st_bit = {
        0b00000000, 0b00000000, 0b00000000,
        0b00000000, 0b00000000, 0b00000000,
        0b00000000, 0b01100010, 0b00000000,
        0b00000000, 0b00000001, 0b00000000,
    };

    
    std::tuple<uint8_t*, uint8_t*> grid = { grid_0th_bit.data(), grid_1st_bit.data() };
    std::size_t height = 4;
    std::size_t width = 3;

    bitwise_no_cache::grid_config<uint8_t, dict_t> state;
    state.bit_grid = grid;
    state.height_b = height;
    state.width_b = width;

    state.x = 1;
    state.y = 2;

    auto curr_state = bitwise_no_cache::evaluator<uint8_t, dict_t, current_state>::evaluate(state);
    std::cout << "curr_state: " << curr_state.to_str() << std::endl;

    auto is_dying_con_res = bitwise_no_cache::evaluator<uint8_t, dict_t, cell_is_dying>::evaluate(state);
    auto is_dying = vector_int_factory::from_condition_result<uint8_t>(is_dying_con_res);

    std::cout << "is_dying: " << is_dying.to_str() << std::endl;

    auto result = bitwise_no_cache::evaluator<uint8_t, dict_t, next_state>::evaluate(state);
    auto result_no_sugar = bitwise_no_cache::evaluator<uint8_t, dict_t, next_state_no_sugar>::evaluate(state);

    std::cout << "Type: " << decltype(result)::type_info() << std::endl;
    std::cout << "Result:\n  " << result.to_str() << std::endl;

    std::cout << "Result (no sugar):\n  " << result_no_sugar.to_str() << std::endl;
}

int main() {
    std::cout << "Running tests..." << std::endl;

    std::cout << "accessor" << std::endl;
    neighbor_accessor();

    std::cout << "\n\nneighborhood_sum" << std::endl;
    neighborhood_sum();

    std::cout << "\n\nif_then_else" << std::endl;
    if_then_else_test();
}