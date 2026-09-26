cmake_minimum_required(VERSION 3.24)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
    set(CMAKE_CUDA_ARCHITECTURES native CACHE STRING "CUDA architectures")
endif()

if(NOT DEFINED CMAKE_CUDA_HOST_COMPILER)
    find_program(CELLATO_DEFAULT_CUDA_HOST_COMPILER g++)
    if(CELLATO_DEFAULT_CUDA_HOST_COMPILER)
        set(CMAKE_CUDA_HOST_COMPILER "${CELLATO_DEFAULT_CUDA_HOST_COMPILER}" CACHE FILEPATH "CUDA host compiler")
    endif()
endif()

project(cellato LANGUAGES CXX CUDA)

find_package(Python3 REQUIRED COMPONENTS Interpreter)
find_package(CUDAToolkit REQUIRED)

set(CELLATO_COMPILE_MODE "" CACHE STRING "Optional compile mode: BENCHMARK or VERIFICATION")
set_property(CACHE CELLATO_COMPILE_MODE PROPERTY STRINGS "" BENCHMARK VERIFICATION)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CUDA_STANDARD 20)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)
set(CMAKE_CUDA_EXTENSIONS OFF)

set(CELLATO_GENERATED_CUDA_INSTANTIATION_DIR "${CMAKE_BINARY_DIR}/generated/cuda_instantiations")
set(CELLATO_GENERATED_CUDA_INSTANTIATION_CMAKE "${CELLATO_GENERATED_CUDA_INSTANTIATION_DIR}/cuda_instantiations.cmake")

set_property(
    DIRECTORY
    APPEND
    PROPERTY CMAKE_CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/automata/registry.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_cuda_instantiations.py"
)

execute_process(
    COMMAND
        "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_cuda_instantiations.py"
        --registry "${CMAKE_CURRENT_SOURCE_DIR}/src/automata/registry.hpp"
        --output-dir "${CELLATO_GENERATED_CUDA_INSTANTIATION_DIR}"
        --cmake-fragment "${CELLATO_GENERATED_CUDA_INSTANTIATION_CMAKE}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMAND_ERROR_IS_FATAL ANY
)

include("${CELLATO_GENERATED_CUDA_INSTANTIATION_CMAKE}")

file(GLOB_RECURSE CELLATO_AUTOMATA_CUDA_SRCS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/automata/*.cu"
)

add_executable(cellato
    "${CMAKE_CURRENT_SOURCE_DIR}/src/app/main.cpp"
    ${CELLATO_AUTOMATA_CUDA_SRCS}
    ${CELLATO_GENERATED_CUDA_INSTANTIATION_SRCS}
)

add_executable(cellato_tests
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_runner.cpp"
)

add_executable(cellato_probability_cuda_tests
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/probability_cuda.cu"
)

foreach(target IN ITEMS cellato cellato_tests cellato_probability_cuda_tests)
    target_include_directories(${target} PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/automata"
        "${CUDAToolkit_INCLUDE_DIRS}"
    )

    target_link_libraries(${target} PRIVATE CUDA::cudart)

    target_compile_definitions(${target} PRIVATE NDEBUG)

    target_compile_options(${target} PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-Wall -Wextra -Wpedantic -O3 -march=native -mtune=native>
        $<$<COMPILE_LANGUAGE:CUDA>:-Wreorder -Wext-lambda-captures-this -O3 -Wno-deprecated-gpu-targets --expt-relaxed-constexpr>
        $<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler=-std=c++20,-Wall,-Wextra,-march=native,-mtune=native>
    )

    if(CELLATO_COMPILE_MODE STREQUAL "BENCHMARK")
        if(NOT target STREQUAL "cellato_probability_cuda_tests")
            target_compile_definitions(${target} PRIVATE BENCHMARK_COMPILE)
        endif()
    elseif(CELLATO_COMPILE_MODE STREQUAL "VERIFICATION")
        if(NOT target STREQUAL "cellato_probability_cuda_tests")
            target_compile_definitions(${target} PRIVATE VERIFICATION_COMPILE)
        endif()
    elseif(NOT CELLATO_COMPILE_MODE STREQUAL "")
        message(FATAL_ERROR "Unsupported CELLATO_COMPILE_MODE: ${CELLATO_COMPILE_MODE}")
    endif()
endforeach()

set_source_files_properties(
    ${CELLATO_GENERATED_CUDA_INSTANTIATION_SRCS}
    PROPERTIES GENERATED TRUE
)

enable_testing()
add_test(NAME cellato_tests COMMAND cellato_tests)
add_test(NAME cellato_probability_cuda_tests COMMAND cellato_probability_cuda_tests)
set_tests_properties(cellato_probability_cuda_tests PROPERTIES SKIP_RETURN_CODE 77)
