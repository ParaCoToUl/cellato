cmake_minimum_required(VERSION 3.24)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

project(cellato VERSION 0.1.0 LANGUAGES CXX)

# Detect CUDA by default, but an explicit OFF must not probe or require the toolkit.
if(NOT DEFINED CELLATO_ENABLE_CUDA OR CELLATO_ENABLE_CUDA)
    if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
        set(CMAKE_CUDA_ARCHITECTURES native CACHE STRING "CUDA architectures")
    endif()

    # Let nvcc select its default MSVC host compiler on Windows.
    if(NOT DEFINED CMAKE_CUDA_HOST_COMPILER AND NOT WIN32)
        find_program(CELLATO_DEFAULT_CUDA_HOST_COMPILER g++)
        if(CELLATO_DEFAULT_CUDA_HOST_COMPILER)
            set(CMAKE_CUDA_HOST_COMPILER "${CELLATO_DEFAULT_CUDA_HOST_COMPILER}" CACHE FILEPATH "CUDA host compiler")
        endif()
    endif()

    # CheckLanguage's probe does not inherit CMAKE_CUDA_HOST_COMPILER.
    # Pass it through the environment so custom host compiler choices also
    # apply during detection, then restore the caller's environment.
    if(DEFINED ENV{CUDAHOSTCXX})
        set(CELLATO_SAVED_CUDAHOSTCXX "$ENV{CUDAHOSTCXX}")
    endif()
    if(CMAKE_CUDA_HOST_COMPILER)
        set(ENV{CUDAHOSTCXX} "${CMAKE_CUDA_HOST_COMPILER}")
    endif()
    include(CheckLanguage)
    check_language(CUDA)
    if(DEFINED CELLATO_SAVED_CUDAHOSTCXX)
        set(ENV{CUDAHOSTCXX} "${CELLATO_SAVED_CUDAHOSTCXX}")
        unset(CELLATO_SAVED_CUDAHOSTCXX)
    else()
        unset(ENV{CUDAHOSTCXX})
    endif()
endif()

set(CELLATO_CUDA_DEFAULT OFF)
if(CMAKE_CUDA_COMPILER)
    set(CELLATO_CUDA_DEFAULT ON)
endif()
option(CELLATO_ENABLE_CUDA "Build CUDA traversers and link the CUDA runtime" ${CELLATO_CUDA_DEFAULT})

if(CELLATO_ENABLE_CUDA)
    if(NOT CMAKE_CUDA_COMPILER)
        message(FATAL_ERROR
            "CELLATO_ENABLE_CUDA=ON requires a CUDA compiler. Install the CUDA toolkit and configure in a fresh "
            "build directory (or set CMAKE_CUDA_COMPILER), or set CELLATO_ENABLE_CUDA=OFF.")
    endif()
    enable_language(CUDA)
    find_package(CUDAToolkit REQUIRED)
endif()
message(STATUS "Cellato CUDA support: ${CELLATO_ENABLE_CUDA}")

find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(CELLATO_COMPILE_MODE "" CACHE STRING "Optional compile mode: BENCHMARK or VERIFICATION")
set_property(CACHE CELLATO_COMPILE_MODE PROPERTY STRINGS "" BENCHMARK VERIFICATION)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(CELLATO_ENABLE_CUDA)
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
endif()

add_executable(cellato
    "${CMAKE_CURRENT_SOURCE_DIR}/src/app/main.cpp"
    ${CELLATO_AUTOMATA_CUDA_SRCS}
    ${CELLATO_GENERATED_CUDA_INSTANTIATION_SRCS}
)

add_executable(cellato_tests
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_runner.cpp"
)

set(CELLATO_TARGETS cellato cellato_tests)
if(CELLATO_ENABLE_CUDA)
    add_executable(cellato_probability_cuda_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/probability_cuda.cu"
    )
    list(APPEND CELLATO_TARGETS cellato_probability_cuda_tests)
endif()

if(MSVC)
    # These options also apply to clang-cl, which uses the MSVC command line.
    set(CELLATO_CXX_OPTIONS /W4 /O2 /permissive- /Zc:__cplusplus /bigobj /utf-8)
else()
    set(CELLATO_CXX_OPTIONS -Wall -Wextra -Wpedantic -O3)
    include(CheckCXXCompilerFlag)
    check_cxx_compiler_flag("-march=native" CELLATO_HAS_MARCH_NATIVE)
    check_cxx_compiler_flag("-mtune=native" CELLATO_HAS_MTUNE_NATIVE)
    if(CELLATO_HAS_MARCH_NATIVE)
        list(APPEND CELLATO_CXX_OPTIONS -march=native)
    endif()
    if(CELLATO_HAS_MTUNE_NATIVE)
        list(APPEND CELLATO_CXX_OPTIONS -mtune=native)
    endif()
    if(MINGW AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        # The CLI instantiates enough templates to exceed ordinary COFF limits.
        list(APPEND CELLATO_CXX_OPTIONS -Wa,-mbig-obj)
    endif()
endif()

foreach(target IN LISTS CELLATO_TARGETS)
    target_include_directories(${target} PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/automata"
    )

    if(CELLATO_ENABLE_CUDA)
        target_link_libraries(${target} PRIVATE CUDA::cudart)
    endif()

    target_compile_definitions(${target} PRIVATE NDEBUG CELLATO_ENABLE_CUDA=$<BOOL:${CELLATO_ENABLE_CUDA}>)

    target_compile_options(${target} PRIVATE
        "$<$<COMPILE_LANGUAGE:CXX>:${CELLATO_CXX_OPTIONS}>"
        $<$<COMPILE_LANGUAGE:CUDA>:-Wreorder -Wext-lambda-captures-this -O3 -Wno-deprecated-gpu-targets --expt-relaxed-constexpr>
    )
    if(CMAKE_CUDA_SIMULATE_ID STREQUAL "MSVC")
        target_compile_options(${target} PRIVATE
            $<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler=/W4,/bigobj,/utf-8,/Zc:__cplusplus>
        )
    else()
        target_compile_options(${target} PRIVATE
            $<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler=-std=c++20,-Wall,-Wextra,-march=native,-mtune=native>
        )
    endif()

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

if(CELLATO_ENABLE_CUDA)
    set_source_files_properties(
        ${CELLATO_GENERATED_CUDA_INSTANTIATION_SRCS}
        PROPERTIES GENERATED TRUE
    )
endif()

enable_testing()
add_test(NAME cellato_tests COMMAND cellato_tests)
if(CELLATO_ENABLE_CUDA)
    add_test(NAME cellato_probability_cuda_tests COMMAND cellato_probability_cuda_tests)
    set_tests_properties(cellato_probability_cuda_tests PROPERTIES SKIP_RETURN_CODE 77)
endif()

add_test(NAME cellato_cli_validation
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/cli_validation.py"
        --binary "$<TARGET_FILE:cellato>" "--compile-mode=${CELLATO_COMPILE_MODE}"
        "--cuda-enabled=$<BOOL:${CELLATO_ENABLE_CUDA}>"
)
