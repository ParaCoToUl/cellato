#ifndef CELLIB_TRAVERSERS_UTILS_HPP
#define CELLIB_TRAVERSERS_UTILS_HPP

#include <type_traits>

namespace cellib::traversers::utils {

template <typename T, typename = void>
struct has_save_to_method : std::false_type {};

template <typename T>
struct has_save_to_method<T, 
    std::void_t<decltype(std::declval<T>().save_to(
        std::declval<void*>(), 
        std::declval<std::size_t>()))>> 
    : std::true_type {};

}

#endif // CELLIB_TRAVERSERS_UTILS_HPP