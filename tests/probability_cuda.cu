#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>

#include "cellato/evaluators/bit_array.hpp"
#include "cellato/evaluators/bit_planes.hpp"
#include "cellato/evaluators/standard.hpp"
#include "cellato/evaluators/tiled_bit_planes.hpp"
#include "cellato/memory/bit_array_grid.hpp"
#include "cellato/memory/state_dictionary.hpp"
#include "cellato/memory/tiled_bit_planes_grid.hpp"
#include "cellato/traversers/cpu/simple.hpp"
#include "cellato/traversers/cuda/simple.cuh"
#include "cellato/traversers/cuda/spatial_blocking.cuh"
#include "cellato/traversers/cuda/temporal.cuh"

namespace {

using namespace cellato::ast;
namespace grids = cellato::memory::grids;
namespace evaluators = cellato::evaluators;
namespace traversers = cellato::traversers;
using dictionary = grids::int_based_state_dictionary<2>;
using scalar_grid = grids::standard::grid<int>;

// Neighbor dependencies force random halo values to affect retained temporal
// cells. Independent streams exercise both recurring and finite binary ratios.
using rule = if_then_else<probability<1, 3, 11>,
    if_then_else<probability<3, 8, 12>, state_constant<2>, neighbor_at<0, 1>>,
    if_then_else<probability<1, 2, 13>, neighbor_at<0, -1>, state_constant<0>>>;
using scalar_evaluator = evaluators::standard::evaluator<int, rule>;

void check_cuda(cudaError_t status, const std::string& operation) {
    if (status != cudaSuccess) {
        throw std::runtime_error(operation + ": " + cudaGetErrorString(status));
    }
}

void check_equal(const scalar_grid& expected, const scalar_grid& actual, const std::string& label) {
    if (expected.x_size_physical() != actual.x_size_physical() ||
        expected.y_size_physical() != actual.y_size_physical()) {
        throw std::runtime_error(label + ": dimensions differ");
    }
    const int size = expected.x_size_physical() * expected.y_size_physical();
    for (int i = 0; i < size; ++i) {
        if (expected.data()[i] != actual.data()[i]) {
            throw std::runtime_error(label + ": cell " + std::to_string(i) + " differs");
        }
    }
}

scalar_grid initial_grid(int width, int height) {
    scalar_grid input(width, height);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            input.data()[y * width + x] = (x * 3 + y * 5 + x / 7) % 4;
        }
    }
    return input;
}

template <typename Traverser, typename Grid>
scalar_grid run(const Grid& input, const cellato::run::run_params& params) {
    Traverser traversal;
    traversal.init(input, params);
    if constexpr (Traverser::is_CUDA) check_cuda(cudaGetLastError(), "initializing CUDA traversal");
    traversal.run(params.steps);
    if constexpr (Traverser::is_CUDA) check_cuda(cudaDeviceSynchronize(), "running CUDA traversal");
    auto result = traversal.fetch_result().to_standard();
    if constexpr (Traverser::is_CUDA) check_cuda(cudaGetLastError(), "fetching CUDA result");
    return result;
}

cellato::run::run_params parameters() {
    cellato::run::run_params params;
    params.seed = 7351;
    params.steps = 8;
    params.cuda_block_size_x = 2;
    params.cuda_block_size_y = 2;
    params.temporal_steps = 4;
    params.temporal_tile_size_y = 32;
    return params;
}

template <typename Grid, typename Evaluator, bool Temporal = false>
void check_layout(int width, int height, const std::string& label) {
    auto params = parameters();
    const auto scalar_input = initial_grid(width, height);
    const Grid input(scalar_input);
    const auto reference = run<traversers::cpu::simple::traverser<scalar_evaluator, scalar_grid>>(
        scalar_input, params);
    const auto cpu = run<traversers::cpu::simple::traverser<Evaluator, Grid>>(input, params);
    check_equal(reference, cpu, label + " CPU/scalar");
    check_equal(cpu, run<traversers::cuda::simple::traverser<Evaluator, Grid>>(input, params),
        label + " CUDA simple");
    check_equal(cpu, run<traversers::cuda::spatial_blocking::traverser<Evaluator, Grid, 3, 2>>(input, params),
        label + " CUDA spatial");

    if constexpr (Temporal) {
        params.cuda_block_size_x = 32;
        params.cuda_block_size_y = 8;
        check_equal(cpu, run<traversers::cuda::temporal::traverser<Evaluator, Grid, 1.0>>(input, params),
            label + " CUDA temporal");
    }
    std::cout << "Passed " << label << '\n';
}

template <typename Word>
void check_packed_layouts() {
    constexpr int bits = sizeof(Word) * 8;
    const auto suffix = std::to_string(bits) + " bits";
    // Four physical tiles, including toroidal halos, for each temporal layout.
    check_layout<grids::bit_planes::grid<Word, dictionary>,
        evaluators::bit_planes::evaluator<Word, dictionary, rule>, true>(
            60 * bits, 48, "bit planes " + suffix);

    constexpr int tile_rows = sizeof(Word);
    constexpr int halo_rows = (4 + tile_rows - 1) / tile_rows;
    constexpr int physical_height = 2 * (32 - 2 * halo_rows);
    check_layout<grids::tiled_bit_planes::grid<Word, dictionary>,
        evaluators::tiled_bit_planes::evaluator<Word, dictionary, rule>, true>(
            60 * 8, physical_height * tile_rows, "tiled bit planes " + suffix);

    using array_grid = grids::bit_array::grid<dictionary, Word>;
    check_layout<array_grid, evaluators::bit_array::evaluator<array_grid, rule>>(
        60 * array_grid::cells_per_word, 48, "bit array " + suffix);
}

void check_seed_changes() {
    const auto input = initial_grid(128, 32);
    auto params = parameters();
    using cpu = traversers::cpu::simple::traverser<scalar_evaluator, scalar_grid>;
    using gpu = traversers::cuda::simple::traverser<scalar_evaluator, scalar_grid>;
    const auto first = run<gpu>(input, params);
    ++params.seed;
    const auto changed = run<gpu>(input, params);
    check_equal(run<cpu>(input, params), changed, "changed seed CPU/CUDA");
    for (int i = 0; i < 128 * 32; ++i) {
        if (first.data()[i] != changed.data()[i]) return;
    }
    throw std::runtime_error("Changing the seed did not change the CUDA result");
}

} // namespace

int main() {
    int device_count = 0;
    const auto status = cudaGetDeviceCount(&device_count);
    if (status == cudaErrorNoDevice || status == cudaErrorInsufficientDriver ||
        (status == cudaSuccess && device_count == 0)) {
        std::cout << "Skipping CUDA probability tests: no CUDA device available\n";
        return 77;
    }

    try {
        check_cuda(status, "detecting CUDA devices");
        check_layout<scalar_grid, scalar_evaluator>(128, 32, "scalar");
        check_seed_changes();
        check_packed_layouts<std::uint8_t>();
        check_packed_layouts<std::uint16_t>();
        check_packed_layouts<std::uint32_t>();
        check_packed_layouts<std::uint64_t>();
        std::cout << "All CUDA probability tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "CUDA probability test failed: " << error.what() << '\n';
        return 1;
    }
}
