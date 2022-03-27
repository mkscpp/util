//
// Created by Michal Němec on 08/08/2020.
//

#ifndef MKS_MAKE_UNIQUE_H
#define MKS_MAKE_UNIQUE_H

#include <type_traits>
#include <memory>

#if __cplusplus == 201103L

namespace std {

template <typename T, typename... Args>
std::unique_ptr<T> make_unique(Args &&... args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

} // namespace std

#endif

#endif // MKS_MAKE_UNIQUE_H
