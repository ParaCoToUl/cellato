#ifndef ARGS_PARSER_HPP
#define ARGS_PARSER_HPP

#include <algorithm>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace input {

class parser {
public:
    parser(int argc, char* argv[]) {
        tokens.reserve(argc > 0 ? static_cast<std::size_t>(argc - 1) : 0);
        for (int i = 1; i < argc; ++i) {
            tokens.emplace_back(argv[i]);
        }
    }

    [[nodiscard]] std::optional<std::string> get(const std::string& option) const {
        const auto option_with_prefix = prefixed(option);
        auto itr = std::find(tokens.begin(), tokens.end(), option_with_prefix);
        if (itr == tokens.end()) {
            return std::nullopt;
        }

        ++itr;
        if (itr == tokens.end() || is_option(*itr)) {
            return std::nullopt;
        }

        return *itr;
    }

    [[nodiscard]] std::string require(const std::string& option) const {
        auto value = get(option);
        if (!value) {
            throw std::invalid_argument("Missing value for option: --" + option);
        }

        return *value;
    }

    [[nodiscard]] bool exists(const std::string& option) const {
        const auto option_with_prefix = prefixed(option);
        return std::find(tokens.begin(), tokens.end(), option_with_prefix) != tokens.end();
    }

private:
    [[nodiscard]] static std::string prefixed(const std::string& option) { return "--" + option; }

    [[nodiscard]] static bool is_option(const std::string& token) { return token.rfind("--", 0) == 0; }

    std::vector<std::string> tokens;
};

} // namespace input

#endif // ARGS_PARSER_HPP
