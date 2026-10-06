#ifndef CELLATO_APP_VALIDATION_HPP
#define CELLATO_APP_VALIDATION_HPP

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "cellato/config.hpp"
#include "cellato/experiments/run_params.hpp"
#if CELLATO_ENABLE_CUDA
#include "cellato/traversers/cuda/temporal_options.hpp"
#endif
#include "cellato/utils/type_list.hpp"

namespace input {

using run_params = cellato::run::run_params;

// Describe the actual compiled suites, including layout geometry, without
// allocating grids or starting a CPU/CUDA traversal during validation.
struct suite_options {
    std::string_view automaton, device, traverser, evaluator, layout;
    int word_size, x_tile_size, y_tile_size;
    int cells_per_word_x, cells_per_word_y;
    double halo_radius;
};
using suite_catalog = std::vector<suite_options>;

template <typename Suite>
suite_options describe_suite() {
    using layout = typename Suite::layout_traits;
    using traversal = typename Suite::traverser_traits;

    suite_options result{
        Suite::automaton::name,
        traversal::device_name,
        traversal::traverser_name,
        layout::evaluator_name,
        layout::layout_name,
        layout::word_size_matches(run_params{}) ? 0 : static_cast<int>(sizeof(typename Suite::grid_store_word_t) * 8),
        0,
        0,
        Suite::x_margin,
        Suite::y_margin,
        Suite::automaton::average_halo_radius};

    if constexpr (requires {
                      traversal::tile_size_x;
                      traversal::tile_size_y;
                  }) {
        result.x_tile_size = traversal::tile_size_x;
        result.y_tile_size = traversal::tile_size_y;
    }

    return result;
}

template <typename... Suites>
suite_catalog make_suite_catalog(cellato::utils::type_list<Suites...>) {
    return {describe_suite<Suites>()...};
}

inline std::string join(const std::vector<std::string>& values) {
    std::string result;
    for (const auto& value : values) {
        if (!result.empty()) result += ", ";
        result += value;
    }

    return result;
}

inline void append_unique(std::vector<std::string>& values, std::string value) {
    if (std::find(values.begin(), values.end(), value) == values.end()) values.push_back(std::move(value));
}

inline std::vector<std::string> names(const suite_catalog& catalog, std::string_view suite_options::* field) {
    std::vector<std::string> values;
    for (const auto& suite : catalog)
        append_unique(values, std::string(suite.*field));
    return values;
}

// First distinguish unknown names from known names in an unsupported combination.
// Progressive filtering makes the suggested alternatives valid for earlier choices.
inline suite_catalog select_suites(const run_params& params, const suite_catalog& catalog) {
#if !CELLATO_ENABLE_CUDA
    if (params.device == "CUDA") {
        throw std::invalid_argument("CUDA support is disabled in this build. Available --device values: CPU. "
                                    "Rebuild with -DCELLATO_ENABLE_CUDA=ON to enable CUDA.");
    }
#endif
    struct selector {
        const char* option;
        std::string_view suite_options::* field;
        const std::string* value;
    };
    const bool reference = params.reference_impl != "none";
    // Preserve the existing reference-only alias.
    const std::string automaton = reference && params.automaton == "fire" ? "forest-fire" : params.automaton;
    const selector selectors[] = {
        {"automaton", &suite_options::automaton, &automaton},
        {"device", &suite_options::device, &params.device},
        {"traverser", &suite_options::traverser, &params.traverser},
        {"evaluator", &suite_options::evaluator, &params.evaluator},
        {"layout", &suite_options::layout, &params.layout},
    };

    for (const auto& entry : selectors) {
        const auto allowed = names(catalog, entry.field);
        if (std::find(allowed.begin(), allowed.end(), *entry.value) == allowed.end()) {
            throw std::invalid_argument("Unknown --" + std::string(entry.option) + " '" + *entry.value +
                                        "'. Available values: " + join(allowed) + ".");
        }
        if (reference) break;
    }

    if (reference) {
        if (params.reference_impl != "baseline") {
            throw std::invalid_argument("Unknown --reference_impl '" + params.reference_impl +
                                        "'. Available values: none, baseline.");
        }
        return {};
    }

    auto candidates = catalog;
    std::string selected;
    for (const auto& entry : selectors) {
        auto matching = candidates;
        std::erase_if(matching, [&](const auto& suite) { return suite.*(entry.field) != *entry.value; });
        const auto choice = "--" + std::string(entry.option) + " " + *entry.value;
        if (matching.empty()) {
            throw std::invalid_argument("Unsupported combination: " + selected + " " + choice + ". Supported --" +
                                        entry.option + " values for " + selected + ": " +
                                        join(names(candidates, entry.field)) + ".");
        }
        if (!selected.empty()) selected += ' ';
        selected += choice;
        candidates = std::move(matching);
    }

    return candidates;
}

inline void require_minimum(const char* option, int value, int minimum) {
    if (value < minimum) {
        throw std::invalid_argument("Invalid --" + std::string(option) + " " + std::to_string(value) + ": must be " +
                                    (minimum == 0 ? "non-negative" : "positive") + ".");
    }
}

inline void require_multiple(const char* option, std::int64_t value, std::int64_t divisor, const std::string& reason) {
    if (value % divisor != 0) {
        throw std::invalid_argument("Invalid --" + std::string(option) + " " + std::to_string(value) +
                                    ": must be divisible by " + std::to_string(divisor) + " " + reason + ".");
    }
}

template <typename T, T... Values>
void require_compiled_option(const char* option, int value, std::integer_sequence<T, Values...>) {
    if (!((value == Values) || ...)) {
        throw std::invalid_argument(
            "Unsupported --" + std::string(option) + " " + std::to_string(value) +
            " for temporal traversal. Values compiled into this build: " + join({std::to_string(Values)...}) + ".");
    }
}

inline void validate_parameters(const run_params& params, const suite_catalog& candidates) {
    require_minimum("x_size", params.x_size, 1);
    require_minimum("y_size", params.y_size, 1);
    require_minimum("steps", params.steps, 0);
    require_minimum("rounds", params.rounds, 1);
    require_minimum("warmup_rounds", params.warmup_rounds, 0);

    if (params.reference_impl != "none") return;

    auto matching = candidates;
    if (matching.front().word_size != 0) {
        std::vector<std::string> allowed;
        for (const auto& suite : matching)
            append_unique(allowed, std::to_string(suite.word_size));
        std::erase_if(matching, [&](const auto& suite) { return suite.word_size != params.word_size; });
        if (matching.empty()) {
            throw std::invalid_argument("Unsupported --word_size " + std::to_string(params.word_size) +
                                        " for --evaluator " + params.evaluator + " --layout " + params.layout +
                                        ". Available values: " + join(allowed) + ".");
        }
    }

    if (params.traverser == "spatial_blocking") {
        std::vector<std::string> allowed;
        for (const auto& suite : matching) {
            append_unique(allowed,
                          "(--x_tile_size " + std::to_string(suite.x_tile_size) + " --y_tile_size " +
                              std::to_string(suite.y_tile_size) + ")");
        }
        std::erase_if(matching, [&](const auto& suite) {
            return suite.x_tile_size != params.x_tile_size || suite.y_tile_size != params.y_tile_size;
        });
        if (matching.empty()) {
            throw std::invalid_argument(
                "Unsupported spatial tile: --x_tile_size " + std::to_string(params.x_tile_size) + " --y_tile_size " +
                std::to_string(params.y_tile_size) + ". Available combinations: " + join(allowed) + ".");
        }
    }

    const auto& suite = matching.front();
    require_multiple("x_size", params.x_size, suite.cells_per_word_x, "for --layout " + params.layout);
    require_multiple("y_size", params.y_size, suite.cells_per_word_y, "for --layout " + params.layout);
#if CELLATO_ENABLE_CUDA
    if (params.device != "CUDA") return;

    require_minimum("cuda_block_size_x", params.cuda_block_size_x, 1);
    require_minimum("cuda_block_size_y", params.cuda_block_size_y, 1);

    if (static_cast<std::int64_t>(params.cuda_block_size_x) * params.cuda_block_size_y > 1024) {
        throw std::invalid_argument(
            "Invalid CUDA block: --cuda_block_size_x times --cuda_block_size_y must not exceed 1024.");
    }

    if (params.traverser == "simple") {
        require_multiple("x_size",
                         params.x_size,
                         static_cast<std::int64_t>(suite.cells_per_word_x) * params.cuda_block_size_x,
                         "for this layout and --cuda_block_size_x " + std::to_string(params.cuda_block_size_x));
        require_multiple("y_size",
                         params.y_size,
                         static_cast<std::int64_t>(suite.cells_per_word_y) * params.cuda_block_size_y,
                         "for this layout and --cuda_block_size_y " + std::to_string(params.cuda_block_size_y));
    }

    if (params.traverser != "temporal") return;

    namespace options = cellato::traversers::cuda::temporal::options;
    require_compiled_option("temporal_steps", params.temporal_steps, options::time_steps{});
    require_compiled_option("temporal_tile_size_y", params.temporal_tile_size_y, options::tile_size_y{});
    require_compiled_option("cuda_block_size_x", params.cuda_block_size_x, options::block_size_x{});
    require_compiled_option("cuda_block_size_y", params.cuda_block_size_y, options::block_size_y{});

    require_multiple("steps", params.steps, params.temporal_steps, "for --temporal_steps");
    require_multiple(
        "temporal_tile_size_y", params.temporal_tile_size_y, params.cuda_block_size_y, "for --cuda_block_size_y");

    const auto halo = static_cast<std::int64_t>(std::ceil(suite.halo_radius * params.temporal_steps));
    const auto halo_x = (halo + suite.cells_per_word_x - 1) / suite.cells_per_word_x;
    const auto halo_y = (halo + suite.cells_per_word_y - 1) / suite.cells_per_word_y;

    const auto effective_x = params.cuda_block_size_x - 2 * halo_x;
    const auto effective_y = params.temporal_tile_size_y - 2 * halo_y;
    if (effective_x <= 0 || effective_y <= 0) {
        throw std::invalid_argument("Invalid temporal tile: halos for --temporal_steps " +
                                    std::to_string(params.temporal_steps) + " leave an effective tile of " +
                                    std::to_string(effective_x) + "x" + std::to_string(effective_y) +
                                    " words. Increase the tile size or reduce temporal_steps.");
    }

    require_multiple(
        "x_size", params.x_size, effective_x * suite.cells_per_word_x, "for the effective temporal tile width");
    require_multiple(
        "y_size", params.y_size, effective_y * suite.cells_per_word_y, "for the effective temporal tile height");
#endif
}

} // namespace input

#endif // CELLATO_APP_VALIDATION_HPP
