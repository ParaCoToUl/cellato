#ifndef CELLATO_CORE_PROBABILITY_HPP
#define CELLATO_CORE_PROBABILITY_HPP

#include <cstdint>
#include <type_traits>

#include "cellato/core/ast.hpp"
#include "cellato/memory/interface.hpp"

namespace cellato::core::random {

// SplitMix64's unsigned mixing function. Each output supplies 64 fair-bit lanes.
CUDA_CALLABLE inline std::uint64_t mix(std::uint64_t value) {
    value = (value ^ (value >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    value = (value ^ (value >> 27)) * UINT64_C(0x94d049bb133111eb);
    return value ^ (value >> 31);
}

CUDA_CALLABLE constexpr std::uint64_t gcd(std::uint64_t a, std::uint64_t b) {
    while (b != 0) {
        const auto remainder = a % b;
        a = b;
        b = remainder;
    }
    return a;
}

// Compare a random binary fraction with a finite binary expansion. Each random
// word is one binary digit across all cells, so this lowers to AND/OR on words.
template <typename Word, std::uint64_t Numerator, std::uint64_t Denominator, typename RandomWords>
CUDA_CALLABLE Word sample_dyadic(RandomWords& source) {
    if constexpr (Numerator == 0) {
        return Word{0};
    } else if constexpr (Numerator == Denominator) {
        return static_cast<Word>(~Word{0});
    } else {
        const Word zero_bits = static_cast<Word>(~source());
        if constexpr (Numerator >= Denominator - Numerator) {
            return static_cast<Word>(zero_bits |
                                     sample_dyadic<Word, Numerator - (Denominator - Numerator), Denominator>(source));
        } else {
            return static_cast<Word>(zero_bits & sample_dyadic<Word, Numerator + Numerator, Denominator>(source));
        }
    }
}

// Exact rational Bernoulli sampling under independent uniform random words.
// For recurring binary expansions, keep drawing only until all active lanes
// differ from the threshold. There is no fixed precision or modulo bias.
template <typename Word, std::uint64_t Numerator, std::uint64_t Denominator, typename RandomWords>
CUDA_CALLABLE Word sample_ratio(RandomWords& source, Word active = static_cast<Word>(~Word{0})) {
    static_assert(std::is_integral_v<Word> && std::is_unsigned_v<Word> && !std::is_same_v<Word, bool>);
    static_assert(sizeof(Word) <= sizeof(std::uint64_t));
    static_assert(Denominator > 0, "Probability denominator must be positive");
    static_assert(Numerator <= Denominator, "Probability must lie between zero and one");

    if constexpr (Numerator == 0) {
        return Word{0};
    } else if constexpr (Numerator == Denominator) {
        return active;
    } else {
        constexpr auto divisor = gcd(Numerator, Denominator);
        constexpr auto numerator = Numerator / divisor;
        constexpr auto denominator = Denominator / divisor;
        if (active == 0) return Word{0};

        if constexpr ((denominator & (denominator - 1)) == 0) {
            return static_cast<Word>(active & sample_dyadic<Word, numerator, denominator>(source));
        } else {
            Word result = 0;
            auto remainder = numerator;
            while (active != 0) {
                const Word bits = source();
                // Compare before doubling to avoid overflow at UINT64_MAX.
                if (remainder >= denominator - remainder) {
                    remainder -= denominator - remainder;
                    result |= static_cast<Word>(active & static_cast<Word>(~bits));
                    active &= bits;
                } else {
                    remainder += remainder;
                    active &= static_cast<Word>(~bits);
                }
            }
            return result;
        }
    }
}

CUDA_CALLABLE inline std::int64_t wrap(std::int64_t coordinate, std::int64_t size) {
    if (size <= 0) return coordinate;
    const auto wrapped = coordinate % size;
    return wrapped < 0 ? wrapped + size : wrapped;
}

// Translate shared-memory coordinates back into global physical word coordinates
// before converting them to logical cell coordinates in an evaluator.
template <typename State>
CUDA_CALLABLE auto global_position(const State& state) {
    struct position {
        std::int64_t x;
        std::int64_t y;
    };
    const auto domain =
        state.random_domain.x_size > 0 && state.random_domain.y_size > 0 ? state.random_domain : state.properties;
    return position{wrap(static_cast<std::int64_t>(state.position.x) + state.random_origin.x, domain.x_size),
                    wrap(static_cast<std::int64_t>(state.position.y) + state.random_origin.y, domain.y_size)};
}

// Every layout draws from the same canonical horizontal groups of 64 cells.
// Smaller linear words extract a slice. Tiled words gather one byte per row;
// neither path generates random numbers separately for individual cells.
template <typename Word, int Rows = 1>
class word_source {
    static_assert(std::is_unsigned_v<Word> && !std::is_same_v<Word, bool> && sizeof(Word) <= 8);
    static_assert(Rows >= 1 && sizeof(Word) * 8 % Rows == 0);
    static constexpr int row_bits = sizeof(Word) * 8 / Rows;
    std::uint64_t _keys[Rows];
    std::uint64_t _counter = 0;
    unsigned _shift;

public:
    template <typename State>
    CUDA_CALLABLE word_source(const State& state, std::int64_t x, std::int64_t y, std::uint64_t stream)
        : _shift(static_cast<unsigned>(x) % 64) {
        auto key = mix(state.random_seed + UINT64_C(0x9e3779b97f4a7c15));
        key = mix(key ^ stream);
        key = mix(key ^ static_cast<std::uint64_t>(state.time_step));
        key = mix(key ^ static_cast<std::uint64_t>(x / 64));
        for (int row = 0; row < Rows; ++row) {
            _keys[row] = mix(key ^ static_cast<std::uint64_t>(y + row));
        }
    }

    CUDA_CALLABLE Word operator()() {
        _counter += UINT64_C(0x9e3779b97f4a7c15);
        if constexpr (Rows == 1) {
            return static_cast<Word>(mix(_keys[0] + _counter) >> _shift);
        } else {
            Word result = 0;
            constexpr auto mask = (UINT64_C(1) << row_bits) - 1;
            for (int row = 0; row < Rows; ++row) {
                const auto bits = (mix(_keys[row] + _counter) >> _shift) & mask;
                result |= static_cast<Word>(bits << (row * row_bits));
            }
            return result;
        }
    }
};

template <typename Probability, typename State>
CUDA_CALLABLE bool sample_cell(const State& state, std::int64_t x, std::int64_t y) {
    // A scalar evaluator uses the same random word as its packed counterparts,
    // and resolves only the requested lane of that word.
    word_source<std::uint64_t> source(state, x - x % 64, y, Probability::stream);
    const auto lane = UINT64_C(1) << (static_cast<unsigned>(x) % 64);
    return sample_ratio<std::uint64_t, Probability::numerator, Probability::denominator>(source, lane) != 0;
}

} // namespace cellato::core::random

#endif // CELLATO_CORE_PROBABILITY_HPP
