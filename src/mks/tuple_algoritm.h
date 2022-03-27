//
// Created by Michal Němec on 02/02/2020.
//

#ifndef MKS_TUPLE_ALGORITM_H
#define MKS_TUPLE_ALGORITM_H

#include <tuple>
#include <utility>
#include <typeindex>

#include <mks/log.h>

#include "demangle.h"
#include "meta_helpers.h"

namespace mks {

template<typename TupleType, typename FunctionType>
void tuple_for_each(TupleType&&, FunctionType
        , std::integral_constant<size_t, std::tuple_size<typename std::remove_reference<TupleType>::type >::value>) {}

template<std::size_t I, typename TupleType, typename FunctionType
        , typename = typename std::enable_if<I != std::tuple_size<typename std::remove_reference<TupleType>::type>::value>::type >
void tuple_for_each(TupleType&& t, FunctionType f, std::integral_constant<size_t, I>)
{
    f(std::get<I>(std::forward<TupleType>(t)));
    tuple_for_each(std::forward<TupleType>(t), f, std::integral_constant<size_t, I + 1>());
}

template<typename TupleType, typename FunctionType>
void tuple_for_each(TupleType&& t, FunctionType&& f)
{
    tuple_for_each(std::forward<TupleType>(t), f, std::integral_constant<size_t, 0>());
}

/*
template<typename T, typename U>
using is_same_types = std::is_same<
        typename std::remove_const<typename std::remove_reference<T>::type>::type,
        typename std::remove_const<typename std::remove_reference<U>::type>::type
>;
*/

template<typename T, typename U>
using is_same_types = std::is_same<
        typename std::decay<T>::type,
        typename std::decay<U>::type
>;
/**
 * specialization when FunctionType IS callable with ValueType
 * std::enable_if<is_same<>>
 */
template<typename FunctionType, typename ValueType>
typename std::enable_if<
    is_same_types<ValueType, first_argument<FunctionType>>::value
>::type
tuple_for_each_callable_cb(FunctionType&& fn, ValueType&& f)
{
    fn(std::forward<ValueType>(f));
}

/**
 * specialization when FunctionType IS-NOT callable with ValueType
 * std::enable_if<!is_same<>>
 */
template<typename FunctionType, typename ValueType>
typename std::enable_if<
    !is_same_types<ValueType, first_argument<FunctionType>>::value
>::type
tuple_for_each_callable_cb(FunctionType&& fn, ValueType&& f) {
}

template<typename TupleType, typename ValueType>
void tuple_for_each_callable(TupleType&&, ValueType&&
        , std::integral_constant<size_t, std::tuple_size<typename std::remove_reference<TupleType>::type >::value>) {
    MKS_LOG_D("tuple type={} not handled", cxx_demangle(typeid(ValueType).name()));
}

template<std::size_t I, typename TupleType, typename ValueType
        , typename = typename std::enable_if<I != std::tuple_size<typename std::remove_reference<TupleType>::type>::value>::type>
void tuple_for_each_callable(TupleType&& t, ValueType&& f, std::integral_constant<size_t, I>)
{
    auto callback = std::get<I>(std::forward<TupleType>(t));
    const auto is_same_t = is_same_types<ValueType, first_argument<decltype(callback)>>::value;
    if(is_same_t) {
        // call first found callback
        tuple_for_each_callable_cb(std::forward<decltype(callback)>(callback), std::forward<ValueType>(f));
    } else {
        // try next
        tuple_for_each_callable(std::forward<TupleType>(t), std::forward<ValueType>(f), std::integral_constant<size_t, I + 1>());
    }
}

template<typename TupleType, typename FunctionType>
void tuple_for_each_callable(TupleType&& t, FunctionType f)
{
    tuple_for_each_callable(std::forward<TupleType>(t), f, std::integral_constant<size_t, 0>());
}

template<typename TupleType, typename FunctionType>
void tuple_for_each_fn(TupleType&&, FunctionType&&
        , std::integral_constant<size_t, std::tuple_size<typename std::remove_reference<TupleType>::type >::value>) {}

template<std::size_t I, typename TupleType, typename FunctionType
        , typename = typename std::enable_if<I != std::tuple_size<typename std::remove_reference<TupleType>::type>::value>::type >
void tuple_for_each_fn(TupleType&& t, FunctionType&& functions, std::integral_constant<size_t, I>)
{
    const auto& tuple_item = std::get<I>(std::forward<TupleType>(t));
    // iterate all functions with this tuple item
    // find callback that can be called with this tuple item
    tuple_for_each_callable(functions, tuple_item);
    tuple_for_each_fn(std::forward<TupleType>(t), std::forward<FunctionType>(functions), std::integral_constant<size_t, I + 1>());
}

template<typename TupleType, typename ...FunctionType>
void tuple_for_each_fn(TupleType&& t, FunctionType... f)
{
    std::tuple<FunctionType...> functions{std::forward<FunctionType>(f)...};
    tuple_for_each_fn(std::forward<TupleType>(t), std::forward<std::tuple<FunctionType...>>(functions), std::integral_constant<size_t, 0>());
}

template<typename TupleType, typename FunctionType>
void tuple_for_each_type(FunctionType
        , std::integral_constant<size_t, std::tuple_size<typename std::remove_reference<TupleType>::type >::value>) {}

template<typename TupleType, std::size_t I, typename FunctionType
        , typename = typename std::enable_if<I != std::tuple_size<typename std::remove_reference<TupleType>::type>::value>::type >
void tuple_for_each_type(FunctionType f, std::integral_constant<size_t, I>)
{
    f(I, std::type_index(typeid(typename std::tuple_element<I,TupleType>::type)));
    tuple_for_each_type<TupleType>(f, std::integral_constant<size_t, I + 1>());
}

template<typename TupleType, typename FunctionType>
void tuple_for_each_type(FunctionType&& f)
{
    tuple_for_each_type<TupleType>(f, std::integral_constant<size_t, 0>());
}

}

#endif //MKS_TUPLE_ALGORITM_H
