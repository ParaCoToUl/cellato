cmake_minimum_required(VERSION 3.24)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

project(cellato_your_own_ca LANGUAGES CXX)

find_package(CUDAToolkit REQUIRED)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(your_own_ca main.cpp)

target_include_directories(your_own_ca PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../../include"
    "${CUDAToolkit_INCLUDE_DIRS}"
)

target_link_libraries(your_own_ca PRIVATE CUDA::cudart)

target_compile_options(your_own_ca PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)
