//
// Created by Michal Němec on 02/02/2020.
//

#ifndef MKS_META_FUNCTION_H
#define MKS_META_FUNCTION_H

#include "meta_helpers.h"
#include "tuple_algoritm.h"
#include <array>
#include <vector>

namespace mks {

// primary template.
template<class T>
struct function_traits_id : public function_traits_id<decltype(&T::operator())> {};
template <typename T>
struct function_traits_id<T&> : public function_traits_id<T> {};
template <typename T>
struct function_traits_id<const T&> : public function_traits_id<T> {};
template <typename T>
struct function_traits_id<volatile T&> : public function_traits_id<T> {};
template <typename T>
struct function_traits_id<const volatile T&> : public function_traits_id<T> {};
template <typename T>
struct function_traits_id<T&&> : public function_traits_id<T> {};
template <typename T>
struct function_traits_id<const T&&> : public function_traits_id<T> {};
template <typename T>
struct function_traits_id<volatile T&&> : public function_traits_id<T> {};
template <typename T>
struct function_traits_id<const volatile T&&> : public function_traits_id<T> {};

// partial specialization for function type
template<class R, class... Args>
struct function_traits_id<R(Args...)> {
    using result_type = R;
    using argument_types = std::tuple<Args...>;
    std::type_index type{typeid(R(Args...))};
    std::type_index return_type{typeid(R)};
    std::array<std::type_index, sizeof...(Args)> args{typeid(Args)...};
};

// partial specialization for function pointer
template<class R, class... Args>
struct function_traits_id<R (*)(Args...)> {
    using result_type = R;
    using argument_types = std::tuple<Args...>;
    std::type_index type{typeid(R(*)(Args...))};
    std::type_index return_type{typeid(R)};
    std::array<std::type_index, sizeof...(Args)> args{typeid(Args)...};
};

// partial specialization for std::function
template<class R, class... Args>
struct function_traits_id<std::function<R(Args...)>> {
    using result_type = R;
    using argument_types = std::tuple<Args...>;
    std::type_index type{typeid(std::function<R(Args...)>)};
    std::type_index return_type{typeid(R)};
    std::array<std::type_index, sizeof...(Args)> args{typeid(Args)...};
};

// partial specialization for pointer-to-member-function (i.e., operator()'s)
template<class T, class R, class... Args>
struct function_traits_id<R(T::*)(Args...)> {
    using result_type = R;
    using argument_types = std::tuple<Args...>;
    std::type_index type{typeid(T)};
    std::type_index return_type{typeid(R)};
    std::array<std::type_index, sizeof...(Args)> args{typeid(Args)...};
};

template<class T, class R, class... Args>
struct function_traits_id<R(T::*)(Args...) const> {
    using result_type = R;
    using argument_types = std::tuple<Args...>;
    std::type_index type{typeid(T)};
    std::type_index return_type{typeid(R)};
    std::array<std::type_index, sizeof...(Args)> args{typeid(Args)...};
};



}

#endif //MKS_META_FUNCTION_H
