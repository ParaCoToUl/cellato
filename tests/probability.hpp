#ifndef CELLATO_TESTS_PROBABILITY_HPP
#define CELLATO_TESTS_PROBABILITY_HPP

#include "manager.hpp"
#include "cellato/core/ast.hpp"
#include "cellato/core/probability.hpp"
#include "cellato/evaluators/bit_array.hpp"
#include "cellato/evaluators/bit_planes.hpp"
#include "cellato/evaluators/standard.hpp"
#include "cellato/evaluators/tiled_bit_planes.hpp"
#include "cellato/memory/bit_array_grid.hpp"
#include "cellato/memory/tiled_bit_planes_grid.hpp"
#include "cellato/traversers/cpu/simple.hpp"

#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace cellato::tests {

class probability_test_suite : public test_suite {
    using dictionary = memory::grids::state_dictionary<0, 1, 2>;
    using scalar_state = memory::grids::point_in_grid<int*>;

    template <typename Word>
    using plane_state = memory::grids::point_in_grid<std::array<Word*, dictionary::needed_bits>>;
    template <typename Expression>
    using scalar_evaluator = evaluators::standard::evaluator<int, Expression>;
    template <typename Word, typename Expression>
    using plane_evaluator = evaluators::bit_planes::evaluator<Word, dictionary, Expression>;
    template <typename Word, typename Expression>
    using tile_evaluator = evaluators::tiled_bit_planes::evaluator<Word, dictionary, Expression>;

    template <typename Word>
    struct scripted_words {
        std::vector<Word> words;
        std::size_t calls = 0;
        Word operator()() {
            if (calls == words.size()) {
                throw std::runtime_error("Probability sampler exhausted its scripted random words");
            }
            return words[calls++];
        }
    };

public:
    std::string name() const override { return "Probability"; }
    test_result run() override {
        test_result result;
        test_case tc(result);
        test_exact_sampling(tc);
        test_statistics(tc);
        test_streams_and_replay(tc);
        test_layouts<std::uint8_t>(tc);
        test_layouts<std::uint16_t>(tc);
        test_layouts<std::uint32_t>(tc);
        test_layouts<std::uint64_t>(tc);
        test_temporal_coordinates(tc);
        test_cpu_traversal(tc);
        return result;
    }

private:
    void test_exact_sampling(test_case& tc) {
        using core::random::sample_ratio;
        scripted_words<std::uint8_t> unused{{}};
        tc.assert_equal(0, sample_ratio<std::uint8_t, 0, 3>(unused), "Probability zero clears every lane");
        tc.assert_equal(0xFF, sample_ratio<std::uint8_t, 3, 3>(unused), "Probability one sets every lane");
        tc.assert_equal(0x25, sample_ratio<std::uint8_t, 3, 3>(unused, 0x25), "Probability one respects active lanes");
        tc.assert_equal(0, sample_ratio<std::uint8_t, 1, 3>(unused, 0), "No active lanes need no random bits");
        tc.assert_equal(0u, unused.calls, "Constant events consume no random words");

        // Lane k represents k/8, with the most significant binary digit drawn first.
        scripted_words<std::uint8_t> half{{0xF0}}, quarter{{0xF0, 0xCC}};
        scripted_words<std::uint8_t> three_eighths{{0xF0, 0xCC, 0xAA}};
        scripted_words<std::uint8_t> third{{0xF0, 0xCC, 0xAA, 0, 0}};
        scripted_words<std::uint8_t> two_thirds{{0xF0, 0xCC, 0xAA, 0, 0}};
        tc.assert_equal(0x0F, sample_ratio<std::uint8_t, 1, 2>(half), "Half uses one independent random bit per lane");
        tc.assert_equal(0x03, sample_ratio<std::uint8_t, 1, 4>(quarter), "Quarter combines two random bit planes");
        tc.assert_equal(0x07, sample_ratio<std::uint8_t, 3, 8>(three_eighths), "Three eighths compares three random bit planes");
        tc.assert_equal(0x07, sample_ratio<std::uint8_t, 1, 3>(third), "One third resolves non-dyadic thresholds exactly");
        tc.assert_equal(0x3F, sample_ratio<std::uint8_t, 2, 3>(two_thirds), "Two thirds resolves non-dyadic thresholds exactly");
        tc.assert_equal(1u, half.calls, "Half requires one random word");
        tc.assert_equal(2u, quarter.calls, "Quarter requires two random words");
        tc.assert_equal(3u, three_eighths.calls, "Three eighths requires three random words");
        scripted_words<std::uint8_t> masked{{0xF0, 0xCC, 0xAA}};
        tc.assert_equal(0x05, sample_ratio<std::uint8_t, 3, 8>(masked, 0x55), "Sampling never sets inactive lanes");

        // 1/3 = 0.010101...: match 101 digits, then choose a smaller digit.
        scripted_words<std::uint64_t> long_prefix{{}};
        for (int digit = 1; digit <= 101; ++digit) {
            long_prefix.words.push_back(digit % 2 == 0 ? ~std::uint64_t{0} : 0);
        }
        long_prefix.words.push_back(0);
        tc.assert_equal(~std::uint64_t{0}, sample_ratio<std::uint64_t, 1, 3>(long_prefix),
                        "Rational sampling does not truncate after 64 random digits");
        tc.assert_equal(102u, long_prefix.calls, "An unresolved rational prefix continues drawing bits");
        constexpr auto maximum = std::numeric_limits<std::uint64_t>::max();
        scripted_words<std::uint64_t> tiny{std::vector<std::uint64_t>(64, 0)};
        scripted_words<std::uint64_t> almost_certain{std::vector<std::uint64_t>(64, maximum)};
        tc.assert_equal(maximum, sample_ratio<std::uint64_t, 1, maximum>(tiny),
                        "The smallest numerator works with the maximum denominator");
        tc.assert_equal(std::uint64_t{0}, sample_ratio<std::uint64_t, maximum - 1, maximum>(almost_certain),
                        "Large numerator arithmetic does not overflow");
    }

    template <std::uint64_t Numerator, std::uint64_t Denominator>
    void check_frequency(test_case& tc) {
        using event = ast::probability<Numerator, Denominator>;
        plane_state<std::uint64_t> state{};
        state.properties = {128, 16};
        std::uint64_t successes = 0;
        for (int seed = 0; seed < 4; ++seed) {
            state.random_seed = 91 + seed * 1237;
            state.time_step = seed;
            for (int y = 0; y < state.properties.y_size; ++y) {
                state.position.y = y;
                for (int x = 0; x < state.properties.x_size; ++x) {
                    state.position.x = x;
                    successes += std::popcount(plane_evaluator<std::uint64_t, event>::evaluate(state));
                }
            }
        }
        constexpr double count = 4 * 128 * 16 * 64;
        constexpr double expected = static_cast<double>(Numerator) / Denominator;
        // Fixed seeds keep this deterministic; tolerance is over seven standard deviations.
        const double tolerance = expected == 0 || expected == 1 ? 0 : 0.005;
        tc.assert_true(std::abs(successes / count - expected) <= tolerance,
                       "Empirical probability matches " + std::to_string(Numerator) + "/" + std::to_string(Denominator));
    }

    void test_statistics(test_case& tc) {
        check_frequency<0, 3>(tc);
        check_frequency<3, 3>(tc);
        check_frequency<1, 2>(tc);
        check_frequency<1, 4>(tc);
        check_frequency<3, 8>(tc);
        check_frequency<1, 3>(tc);
        check_frequency<2, 3>(tc);
    }

    void test_streams_and_replay(test_case& tc) {
        using event = ast::probability<1, 2, 7>;
        using independent = ast::probability<1, 2, 8>;
        using conjunction = ast::and_<event, independent>;
        using contradiction = ast::and_<event, ast::not_<event>>;
        using tautology = ast::or_<event, ast::not_<event>>;
        plane_state<std::uint64_t> state{};
        state.properties = {8, 8};
        state.position = {2, 3};
        state.time_step = 5;
        state.random_seed = 42;
        const auto first = plane_evaluator<std::uint64_t, event>::evaluate(state);
        const auto other = plane_evaluator<std::uint64_t, independent>::evaluate(state);
        tc.assert_true(first != 0 && first != ~std::uint64_t{0}, "Random bits vary between lanes of one word");
        tc.assert_true(first != other, "Distinct stream identifiers produce distinct events");
        tc.assert_equal(first, plane_evaluator<std::uint64_t, event>::evaluate(state), "Repeated evaluation replays an event");
        tc.assert_equal(first & other, plane_evaluator<std::uint64_t, conjunction>::evaluate(state),
                        "Logical conjunction composes lane masks");
        tc.assert_equal(std::uint64_t{0}, plane_evaluator<std::uint64_t, contradiction>::evaluate(state),
                        "Reusing a stream refers to the same event in an expression");
        tc.assert_equal(~std::uint64_t{0}, plane_evaluator<std::uint64_t, tautology>::evaluate(state),
                        "Logical negation and disjunction preserve lane masks");
        ++state.random_seed;
        tc.assert_true(first != plane_evaluator<std::uint64_t, event>::evaluate(state), "Changing the seed changes the event");
        --state.random_seed;
        ++state.time_step;
        tc.assert_true(first != plane_evaluator<std::uint64_t, event>::evaluate(state), "Changing the time step changes the event");
        --state.time_step;
        ++state.position.x;
        tc.assert_true(first != plane_evaluator<std::uint64_t, event>::evaluate(state), "Different words have distinct random bits");
        --state.position.x;
        tc.assert_equal(first, plane_evaluator<std::uint64_t, event>::evaluate(state), "Restoring coordinates and seed replays exactly");
        bool monotone = true;
        std::uint64_t both = 0;
        for (int time = 0; time < 1024; ++time) {
            state.time_step = time;
            const auto low = plane_evaluator<std::uint64_t, ast::probability<1, 3, 7>>::evaluate(state);
            const auto high = plane_evaluator<std::uint64_t, ast::probability<2, 3, 7>>::evaluate(state);
            monotone &= (low & ~high) == 0;
            both += std::popcount(plane_evaluator<std::uint64_t, conjunction>::evaluate(state));
        }
        tc.assert_true(monotone, "Thresholds sharing a stream are monotone in their ratio");
        tc.assert_true(std::abs(static_cast<double>(both) / (1024 * 64) - 0.25) < 0.01,
                       "Separate half-probability streams jointly occur with probability one quarter");
    }

    template <typename Word, typename Event>
    bool layouts_agree() {
        constexpr int width = 128, height = 16, word_bits = sizeof(Word) * 8;
        using packed_grid = memory::grids::bit_array::grid<dictionary, Word>;
        scalar_state scalar{};
        plane_state<Word> planes{}, tiles{};
        evaluators::bit_array::state_t<packed_grid> packed{typename packed_grid::cell_ptr_t{nullptr}};
        scalar.properties = {width, height};
        planes.properties = {width / word_bits, height};
        tiles.properties = {width / 8, height / (word_bits / 8)};
        packed.properties = {width / packed_grid::cells_per_word, height};
        scalar.random_seed = planes.random_seed = tiles.random_seed = packed.random_seed = 9321;
        scalar.time_step = planes.time_step = tiles.time_step = packed.time_step = 19;
        static_assert(std::is_same_v<decltype(scalar_evaluator<Event>::evaluate(scalar)), bool>);
        static_assert(std::is_same_v<decltype(plane_evaluator<Word, Event>::evaluate(planes)), Word>);
        static_assert(std::is_same_v<decltype(tile_evaluator<Word, Event>::evaluate(tiles)), Word>);
        static_assert(std::is_same_v<decltype(evaluators::bit_array::detail::_evaluator_impl<packed_grid, Event, 0>::evaluate(packed)), bool>);
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                scalar.position = {x, y};
                planes.position = {x / word_bits, y};
                tiles.position = {x / 8, y / (word_bits / 8)};
                packed.position = {x / packed_grid::cells_per_word, y};
                const bool expected = scalar_evaluator<Event>::evaluate(scalar);
                const auto plane_word = plane_evaluator<Word, Event>::evaluate(planes);
                const auto tile_word = tile_evaluator<Word, Event>::evaluate(tiles);
                const auto packed_word = evaluators::bit_array::evaluator<packed_grid, Event>::evaluate(packed);
                const int tile_lane = (y % (word_bits / 8)) * 8 + x % 8;
                const int packed_shift = (x % packed_grid::cells_per_word) * packed_grid::bits_per_cell;
                if (((plane_word >> (x % word_bits)) & 1) != expected ||
                    ((tile_word >> tile_lane) & 1) != expected ||
                    ((packed_word >> packed_shift) & packed_grid::cell_mask) != expected) {
                    return false;
                }
            }
        }
        return true;
    }

    template <typename Word>
    void test_layouts(test_case& tc) {
        const auto suffix = " for " + std::to_string(sizeof(Word) * 8) + "-bit words";
        tc.assert_true(layouts_agree<Word, ast::probability<0, 3>>(), "Zero agrees across every layout" + suffix);
        tc.assert_true(layouts_agree<Word, ast::probability<3, 3>>(), "One agrees across every layout" + suffix);
        tc.assert_true(layouts_agree<Word, ast::probability<3, 8, 17>>(), "Dyadic events agree across every layout" + suffix);
        tc.assert_true(layouts_agree<Word, ast::probability<1, 3, 23>>(), "Rational events agree across every layout" + suffix);
    }

    template <typename Evaluator, typename State>
    bool temporal_coordinates_agree(State global) {
        global.properties = {8, 8};
        global.position = {3, 4};
        global.random_seed = 8217;
        for (int time = 0; time < 64; ++time) {
            global.time_step = time;
            const auto expected = Evaluator::evaluate(global);
            auto local = global;
            local.properties = {9, 6};
            local.position = {5, 2};
            local.random_domain = global.properties;
            local.random_origin = {-2, 2};
            if (Evaluator::evaluate(local) != expected) {
                return false;
            }
            local.random_origin = {-18, 18};
            if (Evaluator::evaluate(local) != expected) {
                return false;
            }
        }
        return true;
    }

    void test_temporal_coordinates(test_case& tc) {
        using event = ast::probability<1, 3, 28>;
        using packed_grid = memory::grids::bit_array::grid<dictionary, std::uint32_t>;
        tc.assert_true(temporal_coordinates_agree<scalar_evaluator<event>>(scalar_state{}),
                       "Scalar temporal tiles preserve global coordinates and periodic wrapping");
        tc.assert_true(temporal_coordinates_agree<plane_evaluator<std::uint32_t, event>>(plane_state<std::uint32_t>{}),
                       "Bit-plane temporal tiles preserve global coordinates and periodic wrapping");
        tc.assert_true(temporal_coordinates_agree<tile_evaluator<std::uint32_t, event>>(plane_state<std::uint32_t>{}),
                       "Tiled bit-plane temporal tiles preserve global coordinates and periodic wrapping");
        tc.assert_true(temporal_coordinates_agree<evaluators::bit_array::evaluator<packed_grid, event>>(
                           evaluators::bit_array::state_t<packed_grid>{typename packed_grid::cell_ptr_t{nullptr}}),
                       "Packed temporal tiles preserve global coordinates and periodic wrapping");
    }

    template <typename Grid, typename Evaluator>
    std::vector<int> run_cpu(Grid input, int seed, int steps) {
        cellato::run::run_params params;
        params.seed = seed;
        traversers::cpu::simple::traverser<Evaluator, Grid> traverser;
        traverser.init(std::move(input), params);
        traverser.run(steps);
        auto output = traverser.fetch_result().to_standard();
        return std::vector<int>(output.data(), output.data() + output.x_size_physical() * output.y_size_physical());
    }

    void test_cpu_traversal(test_case& tc) {
        using condition = ast::and_<ast::probability<2, 3, 7>, ast::not_<ast::probability<1, 4, 9>>>;
        using even_rule = ast::if_then_else<condition, ast::state_constant<2>, ast::state_constant<1>>;
        using odd_rule = ast::if_then_else<ast::probability<3, 8, 11>, ast::state_constant<2>, ast::state_constant<1>>;
        using rule = ast::alternate_algorithms<even_rule, odd_rule>;
        using standard_grid = memory::grids::standard::grid<int>;
        using planes_grid = memory::grids::bit_planes::grid<std::uint32_t, dictionary>;
        using tiles_grid = memory::grids::tiled_bit_planes::grid<std::uint32_t, dictionary>;
        using packed_grid = memory::grids::bit_array::grid<dictionary, std::uint32_t>;
        standard_grid input(128, 16);
        const auto scalar = run_cpu<standard_grid, scalar_evaluator<rule>>(input, 12345, 3);
        const auto planes = run_cpu<planes_grid, plane_evaluator<std::uint32_t, rule>>(planes_grid(input), 12345, 3);
        const auto tiles = run_cpu<tiles_grid, tile_evaluator<std::uint32_t, rule>>(tiles_grid(input), 12345, 3);
        const auto packed = run_cpu<packed_grid, evaluators::bit_array::evaluator<packed_grid, rule>>(packed_grid(input), 12345, 3);
        tc.assert_true(scalar == planes && scalar == tiles && scalar == packed,
                       "CPU traversal produces identical conditional random rules in every layout");
        const auto odd_scalar = run_cpu<standard_grid, scalar_evaluator<rule>>(input, 12345, 2);
        tc.assert_true(odd_scalar == run_cpu<planes_grid, plane_evaluator<std::uint32_t, rule>>(planes_grid(input), 12345, 2) &&
                       odd_scalar == run_cpu<tiles_grid, tile_evaluator<std::uint32_t, rule>>(tiles_grid(input), 12345, 2) &&
                       odd_scalar == run_cpu<packed_grid, evaluators::bit_array::evaluator<packed_grid, rule>>(packed_grid(input), 12345, 2),
                       "Both alternate algorithm branches preserve random coordinates in every layout");
        tc.assert_true(scalar == run_cpu<standard_grid, scalar_evaluator<rule>>(input, 12345, 3),
                       "CPU traversal replays the same seed");
        tc.assert_true(scalar != run_cpu<standard_grid, scalar_evaluator<rule>>(input, 12346, 3),
                       "CPU traversal propagates the configured random seed");
        tc.assert_true(scalar != odd_scalar,
                       "CPU traversal advances the random event time step");
        scalar_state state{};
        state.properties = {128, 16};
        state.random_seed = 12345;
        state.time_step = 2;
        bool expected = true;
        for (int y = 0; y < 16; ++y) {
            for (int x = 0; x < 128; ++x) {
                state.position = {x, y};
                expected &= scalar[y * 128 + x] == scalar_evaluator<rule>::evaluate(state);
            }
        }
        tc.assert_true(expected, "CPU results use the requested seed and zero-based final evaluation time");
    }
};

inline void register_probability_tests() {
    static probability_test_suite suite;
    test_manager::instance().register_suite(&suite);
}

} // namespace cellato::tests

#endif // CELLATO_TESTS_PROBABILITY_HPP
