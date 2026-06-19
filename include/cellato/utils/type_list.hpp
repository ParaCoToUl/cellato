#ifndef CELLATO_UTILS_TYPE_LIST_HPP
#define CELLATO_UTILS_TYPE_LIST_HPP

namespace cellato::utils {

template <typename... Ts>
struct type_list {};

template <typename... Lists>
struct concat;

template <>
struct concat<> {
    using type = type_list<>;
};

template <typename... Ts>
struct concat<type_list<Ts...>> {
    using type = type_list<Ts...>;
};

template <typename... Left, typename... Right, typename... Rest>
struct concat<type_list<Left...>, type_list<Right...>, Rest...> {
    using type = typename concat<type_list<Left..., Right...>, Rest...>::type;
};

template <typename... Lists>
using concat_t = typename concat<Lists...>::type;

} // namespace cellato::utils

#endif // CELLATO_UTILS_TYPE_LIST_HPP
