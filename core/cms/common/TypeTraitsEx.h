//
// Created by 韩呈杰 on 2023/11/6.
//

#ifndef ARGUSAGENT_TYPE_TRAITS_EX_H
#define ARGUSAGENT_TYPE_TRAITS_EX_H

#include <type_traits>

// 参见：
// 1. https://learn.microsoft.com/zh-cn/cpp/build/reference/zc-cplusplus?view=msvc-170
// 2. https://en.cppreference.com/w/cpp/preprocessor/replace#Predefined_macros
// 从vs2017开始才有宏_MSVC_LANG, msvc下是否有void_t，跟发行版本有关，跟__cplusplus无关
#if !defined(_MSVC_LANG) && __cplusplus < 201703L // C++17以下
namespace std {
    // Needed for some older versions of GCC
    template<typename...>
    struct voider {
        using type = void;
    };

    // std::void_t will be part of C++17, but until then define it ourselves:
    template<typename... T>
    using void_t = typename voider<T...>::type;
}
#endif

namespace detail {
    template<typename T, typename U = void>
    struct IsListImpl : std::false_type {
    };
    template<typename T>
    struct IsListImpl<T, std::void_t<typename T::value_type,
            decltype(std::declval<T &>().push_back(std::declval<const typename T::value_type &>()))>>
            : std::true_type {
    };

    template<typename T, typename U = void>
    struct IsMapImpl : std::false_type {
    };

    template<typename T>
    struct IsMapImpl<T, std::void_t<
            typename T::key_type,
            typename T::mapped_type,
            decltype(std::declval<T &>()[std::declval<const typename T::key_type &>()])>>
            : std::true_type {
    };
}

template<typename T>
struct IsMap : detail::IsMapImpl<T>::type {
};
template<typename T>
struct IsList : detail::IsListImpl<T>::type {
};


#endif //ARGUSAGENT_TYPE_TRAITS_EX_H
