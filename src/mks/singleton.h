//
// Created by Michal Němec on 21/01/2020.
//

#ifndef MKS_SINGLETON_H
#define MKS_SINGLETON_H

#include <atomic>
#include <mutex>
#include <typeinfo>
#include <unordered_map>
#include <vector>

#include "demangle.h"
#include "lifecycle.h"
#include "make_unique.h"

namespace mks {

template <typename T>
class singleton_double_lock {
protected:
    static std::atomic<T *> instance_;
    static std::unique_ptr<T> instance_ptr_;
    static std::mutex mutex_;
    singleton_double_lock() = default;

    template <typename... Args>
    static std::unique_ptr<T> make_unique(Args &&... args) {
        return std::unique_ptr<T>(
            new T(std::forward<Args>(args)...) /*, [&](T* ptr){
std::default_delete<T> deleter;
deleter(ptr);
instance_.store(nullptr);
}*/);
    }

    static T *get_raw_ptr() {
        return instance_;
    }

public:
    // T* operator->() {return &instance();}
    // const T* operator->() const {return &instance();}
    // T& operator*() {return *instance();}
    // const T& operator*() const {return instance();}

    singleton_double_lock(const singleton_double_lock &) = delete;
    singleton_double_lock(singleton_double_lock &&) = delete;

    static T &instance() {
        T *p = instance_.load(std::memory_order_relaxed);
        std::atomic_thread_fence(std::memory_order_acquire);
        if(p == nullptr) {
            std::lock_guard<std::mutex> lock{mutex_};
            p = instance_.load(std::memory_order_relaxed);
            if(p == nullptr) {
                // store unique ptr for automatic cleanup
                instance_ptr_ = make_unique();
                p = instance_ptr_.get();
                std::atomic_thread_fence(std::memory_order_release);
                instance_.store(p, std::memory_order_relaxed);
            }
        }
        return *p;
    }
};

template <typename T>
std::atomic<T *> singleton_double_lock<T>::instance_{nullptr};

template <typename T>
std::unique_ptr<T> singleton_double_lock<T>::instance_ptr_{nullptr};

template <typename T>
std::mutex singleton_double_lock<T>::mutex_{};


template <typename T>
class singleton_unique_ptr {
protected:
    static std::unique_ptr<T> instance_;
    static std::once_flag create_;

    template <typename... Args>
    static std::unique_ptr<T> make_unique(Args &&... args) {
        return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
    }

    singleton_unique_ptr() = default;

public:
    singleton_unique_ptr(const singleton_unique_ptr &) = delete;
    singleton_unique_ptr(singleton_unique_ptr &&) = delete;

    static T &instance() {
        std::call_once(create_, [=] {
            // when T has private constructor and this class is friend we would not be able to call std::make_unique
            instance_ = make_unique();
        });
        return *instance_.get();
    }
};

template <typename T>
std::unique_ptr<T> singleton_unique_ptr<T>::instance_{nullptr};

template <typename T>
std::once_flag singleton_unique_ptr<T>::create_{};

template <typename T>
class singleton_static {
protected:
    static std::once_flag register_;
    singleton_static() = default;

public:
    singleton_static(const singleton_static &) = delete;
    singleton_static(singleton_static &&) = delete;

    static T &instance() {
        static T instance;
        return instance;
    }
};

template <typename T>
std::once_flag singleton_static<T>::register_{};

template <typename T>
using singleton = singleton_double_lock<T>;

} // namespace mks

#endif // MKS_SINGLETON_H
