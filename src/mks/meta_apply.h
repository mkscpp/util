//
// Created by Michal Němec on 02/02/2020.
//

#ifndef MKS_META_APPLY_H
#define MKS_META_APPLY_H

#include <tuple>
#include <utility>
#include "meta_helpers.h"
#include <functional>

namespace mks {

template<int...> struct inMKS_tuple{};

template<int I, typename IndexTuple, typename... Types>
struct make_indexes_impl;

template<int I, int... Indexes, typename T, typename ... Types>
struct make_indexes_impl<I, inMKS_tuple<Indexes...>, T, Types...>
{
    typedef typename make_indexes_impl<I + 1, inMKS_tuple<Indexes..., I>, Types...>::type type;
};

template<int I, int... Indexes>
struct make_indexes_impl<I, inMKS_tuple<Indexes...> >
{
    typedef inMKS_tuple<Indexes...> type;
};

template<typename ... Types>
struct make_indexes : make_indexes_impl<0, inMKS_tuple<>, Types...>
{};

template<typename T>
using apply_fn_arg_tuple = typename function_traits<T>::argument_types;
template<typename T>
using apply_fn_ret = typename function_traits<T>::result_type;

/*
 * const function input
 */
template<class Ret, class T, class... Args, int...Indexes >
Ret apply_helper(const T& fn, inMKS_tuple<Indexes...>, std::tuple<Args...>&& args)
{
    return fn(std::forward<Args>(std::get<Indexes>(args))...);
}

template<class Ret, typename T, class... Args>
Ret apply_tuple(const T& fn, std::tuple<Args...>&& args)
{
    // we have extracted class... Args
    return apply_helper<Ret>(fn, typename make_indexes<Args...>::type(), std::forward<std::tuple<Args...>>(args));
}

template<typename T>
apply_fn_ret<T> apply(const T& fn, apply_fn_arg_tuple<T>&& args)
{
    return apply_tuple<apply_fn_ret<T>>(fn, std::forward<apply_fn_arg_tuple<T>>(args));
}

template<typename T>
apply_fn_ret<T> apply(const T& fn, const apply_fn_arg_tuple<T>& args)
{
    return apply_tuple<apply_fn_ret<T>>(fn, std::forward<apply_fn_arg_tuple<T>>(args));
}

template<typename T>
apply_fn_ret<T> apply(const T& fn, apply_fn_arg_tuple<T>& args)
{
    return apply_tuple<apply_fn_ret<T>>(fn, std::forward<apply_fn_arg_tuple<T>>(args));
}

/*
 * non-const function input
 */
template<typename Ret, typename T, typename... Args, int...Indexes >
Ret apply_helper(T& fn, inMKS_tuple<Indexes...>, std::tuple<Args...>&& args)
{
    return fn(std::forward<Args>(std::get<Indexes>(args))...);
}

template<typename Ret, typename T, typename... Args>
Ret apply_tuple(T& fn, std::tuple<Args...>&& args)
{
    // we have extracted class... Args
    return apply_helper<Ret>(fn, typename make_indexes<Args...>::type(), std::forward<std::tuple<Args...>>(args));
}

template<typename T>
apply_fn_ret<T> apply(T& fn, apply_fn_arg_tuple<T>&& args)
{
    return apply_tuple<apply_fn_ret<T>>(fn, std::forward<apply_fn_arg_tuple<T>>(args));
}

template<typename T>
apply_fn_ret<T> apply(T& fn, const apply_fn_arg_tuple<T>& args)
{
    return apply_tuple<apply_fn_ret<T>>(fn, std::forward<apply_fn_arg_tuple<T>>(args));
}

template<typename T>
apply_fn_ret<T> apply(T& fn, apply_fn_arg_tuple<T>& args)
{
    return apply_tuple<apply_fn_ret<T>>(fn, std::forward<apply_fn_arg_tuple<T>>(args));
}

}

#endif //MKS_META_APPLY_H
