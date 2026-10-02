cmake_minimum_required(VERSION 3.24)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

project(cellato_your_own_ca LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(your_own_ca main.cpp)

target_include_directories(your_own_ca PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../../include"
)

target_compile_definitions(your_own_ca PRIVATE CELLATO_ENABLE_CUDA=0)

if(MSVC)
    target_compile_options(your_own_ca PRIVATE /W4 /permissive- /Zc:__cplusplus /bigobj /utf-8)
else()
    target_compile_options(your_own_ca PRIVATE -Wall -Wextra -Wpedantic)
endif()

enable_testing()
add_test(NAME your_own_ca_smoke COMMAND your_own_ca)
set_tests_properties(your_own_ca_smoke PROPERTIES TIMEOUT 30)
