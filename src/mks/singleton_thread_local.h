//
// Created by Michal Němec on 21/01/2020.
//

#ifndef MKS_SINGLETON_THREAD_LOCAL_H
#define MKS_SINGLETON_THREAD_LOCAL_H

#include "demangle.h"
#include "lifecycle.h"
#include "make_unique.h"
#include <atomic>
#include <mutex>
#include <typeinfo>
#include <unordered_map>
#include <vector>

namespace mks {

template <typename T>
class singleton_thread_local_double_lock {
protected:
    static thread_local std::unique_ptr<T> instance_ptr_;
    singleton_thread_local_double_lock() = default;

    template <typename... Args>
    static std::unique_ptr<T> make_unique(Args &&... args) {
        return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
    }

    static T *get_raw_ptr() {
        return instance_ptr_.get();
    }

public:
    singleton_thread_local_double_lock(const singleton_thread_local_double_lock &) = delete;
    singleton_thread_local_double_lock(singleton_thread_local_double_lock &&) = delete;

    static T &instance() {
        T *p = instance_ptr_.get();
        if(p == nullptr) {
            instance_ptr_ = make_unique();
            p = instance_ptr_.get();
        }
        return *p;
    }
};

template <typename T>
thread_local std::unique_ptr<T> singleton_thread_local_double_lock<T>::instance_ptr_{nullptr};

template <typename T>
class singleton_thread_local_unique_ptr {
protected:
    static thread_local std::unique_ptr<T> instance_;
    static thread_local std::once_flag create_;

    template <typename... Args>
    static std::unique_ptr<T> make_unique(Args &&... args) {
        return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
    }

    singleton_thread_local_unique_ptr() = default;

public:
    singleton_thread_local_unique_ptr(const singleton_thread_local_unique_ptr &) = delete;
    singleton_thread_local_unique_ptr(singleton_thread_local_unique_ptr &&) = delete;

    static T &instance() {
        std::call_once(create_, [=] {
            // when T has private constructor and this class is friend we would not be able to call std::make_unique
            instance_ = make_unique();
        });
        return *instance_.get();
    }
};

template <typename T>
thread_local std::unique_ptr<T> singleton_thread_local_unique_ptr<T>::instance_{nullptr};

template <typename T>
thread_local std::once_flag singleton_thread_local_unique_ptr<T>::create_{};

template <typename T>
class singleton_thread_local_static {
protected:
    singleton_thread_local_static() = default;

public:
    singleton_thread_local_static(const singleton_thread_local_static &) = delete;
    singleton_thread_local_static(singleton_thread_local_static &&) = delete;

    static T &instance() {
        static thread_local T instance;
        return instance;
    }
};

template <typename T>
using singleton_thread_local = singleton_thread_local_double_lock<T>;

} // namespace ctl

#endif // MKS_SINGLETON_THREAD_LOCAL_H
