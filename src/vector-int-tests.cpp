#include <iostream>
#include <vector>
#include <cstdint>
#include "bit-mode.hpp"

int main() {
    using vint = bitwise::vector_int<uint8_t, 3>;

    vint v1;

    v1.set_at(0, 1);
    v1.set_at(1, 2);
    v1.set_at(2, 3);
    v1.set_at(3, 4);
    v1.set_at(4, 5);
    v1.set_at(5, 6);
    v1.set_at(6, 7);
    v1.set_at(7, 15);
    // v1.set_at(8, 9);
    // v1.set_at(9, 10);
    // v1.set_at(10, 11);
    // v1.set_at(11, 12);
    // v1.set_at(12, 13);
    // v1.set_at(13, 14);
    // v1.set_at(14, 15);
    // v1.set_at(15, 16);


    std::cout << "v1:" << std::endl;
    std::cout << v1.to_str() << std::endl;

    std::vector<uint8_t> b0 = {0xF0, 0b10101010, 0x00, 0x00};
    std::vector<uint8_t> b1 = {0x0F, 0b11001100, 0x00, 0x00};
    std::vector<uint8_t> b2 = {0x00, 0b11110000, 0x00, 0x00};

    std::tuple<uint8_t*, uint8_t*, uint8_t*> storage = {b0.data(), b1.data(), b2.data()};
    std::tuple<uint8_t*, uint8_t*> storage_small = {b0.data(), b1.data()};

    
    vint v2 = vint::load_from(storage, 1);

    std::cout << "v2:" << std::endl;
    std::cout << v2.to_str() << std::endl;

    vint v3 = vint::load_from(storage_small, 1);

    // std::cout << "v3:" << std::endl;
    // std::cout << v3.to_str() << std::endl;

    auto v1_plus_v2 = v1.get_added(v2);
    std::cout << "v1 + v2 = \n" << v1_plus_v2.to_str() << std::endl;

}
