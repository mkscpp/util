//
// Created by Michal Němec on 17/08/2020.
//

#ifndef MKS_MEMORY_POOL_H
#define MKS_MEMORY_POOL_H

#include <cassert>
#include <functional>
#include <mks/log.h>
#include <deque>
#include <mutex>

template<typename T>
class memory_pool {
    int max_size_ = 64;
    std::deque<T*> pool_;
public:
    bool verbose = false;

    ~memory_pool(){
        MKS_LOG_CD(verbose, "memory pool delete {}", pool_.size());
        for(auto& ptr : pool_) {
            assert(ptr);
            MKS_LOG_CD(verbose, "deleting {}", (void*)ptr);
            delete ptr;
        }
    }

    std::size_t size() const {
        return pool_.size();
    }


    void set_max_size(std::size_t size) {
        max_size_ = size;
    }

    template<typename ...Args>
    T* get(Args&&... args) {
        if(pool_.empty()) {
            auto* ret = new T(std::forward<Args>(args)...);
            MKS_LOG_CD(verbose, "allocated {}", (void*)ret);
            assert(ret != nullptr);
            return ret;
        }
        auto* ret = pool_.front();
        pool_.pop_front();
        MKS_LOG_CD(verbose, "reused {}", (void*)ret);
        assert(ret != nullptr);
        return ret;
    }

    void put(T* ptr) {
        assert(ptr != nullptr);
        if(pool_.size() > max_size_) {
            delete ptr;
            return;
        }
        pool_.push_back(ptr);
    }
};

/**
 * Thread safe implementation of memory pool,
 * we can use shared/unique pointers with custom deleter to return object to the pool
 * @tparam T
 */
template<typename T>
class memory_pool_ts {
    mutable std::mutex mtx_;
    int max_size_ = 64;
    std::deque<T*> pool_;
    std::shared_ptr<memory_pool_ts<T>> self_;
public:
    bool verbose = false;

    using pool_deleter = std::function<void(T*)>;
    using unique_ptr = std::unique_ptr<T, pool_deleter>;
    using shared_ptr = std::shared_ptr<T>;

    memory_pool_ts() : self_(std::shared_ptr<memory_pool_ts<T>>(this, [](memory_pool_ts<T>* ptr){ /* dont delete this */})) {}

    ~memory_pool_ts(){
        MKS_LOG_CD(verbose, "memory pool delete {}", pool_.size());
        // delete remaining objects
        for(auto& ptr : pool_) {
            MKS_ASSERT(ptr);
            MKS_LOG_CD(verbose, "deleting {}", (void*)ptr);
            delete ptr;
        }
    }

    std::size_t size() const {
        std::lock_guard<std::mutex> ll{mtx_};
        return pool_.size();
    }

    void set_max_size(std::size_t size) {
        std::lock_guard<std::mutex> ll{mtx_};
        max_size_ = size;
    }

    void put(T* ptr) {
        std::lock_guard<std::mutex> ll{mtx_};
        MKS_ASSERT(ptr != nullptr);
        if(pool_.size() > max_size_) {
            MKS_LOG_CD(verbose, "deleting {}", (void*)ptr);
            delete ptr;
            return;
        }
        MKS_LOG_D(verbose, "returning {}", (void*)ptr);
        pool_.push_back(ptr);
    }

    template<typename ...Args>
    T* get(Args&&... args) {
        std::lock_guard<std::mutex> ll{mtx_};
        if(pool_.empty()) {
            auto* ret = new T(std::forward<Args>(args)...);
            MKS_LOG_D(verbose, "allocated {}", (void*)ret);
            MKS_ASSERT(ret != nullptr);
            return ret;
        }
        auto* ret = pool_.front();
        pool_.pop_front();
        MKS_LOG_D(verbose, "reused {}", (void*)ret);
        MKS_ASSERT(ret != nullptr);
        return ret;
    }

    template<typename ...Args>
    unique_ptr get_unique(Args&&... args) {
        std::weak_ptr<memory_pool_ts<T>> weak = self_;
        bool shared_ptr_verbose = verbose;
        return unique_ptr(get(std::forward<Args>(args)...), [weak, shared_ptr_verbose](T* ptr) {
            std::shared_ptr<memory_pool_ts<T>> self;
            if((self = weak.lock()) != nullptr) {
                // when deleted put back to pool
                self->put(ptr);
            } else {
                MKS_LOG_CD(shared_ptr_verbose, "default deleting {}", (void*)ptr);
                // memory pool lost
                std::default_delete<T> def;
                def(ptr);
            }
        });
    }

    template<typename ...Args>
    shared_ptr get_shared(Args&&... args) {
        std::weak_ptr<memory_pool_ts<T>> weak = self_;
        bool shared_ptr_verbose = verbose;
        return shared_ptr(get(std::forward<Args>(args)...), [weak, shared_ptr_verbose](T* ptr){
            std::shared_ptr<memory_pool_ts<T>> self;
            if((self = weak.lock()) != nullptr) {
                // when deleted put back to pool
                self->put(ptr);
            } else {
                MKS_LOG_CD(shared_ptr_verbose, "default deleting {}", (void*)ptr);
                // memory pool lost
                std::default_delete<T> def;
                def(ptr);
            }
        });
    }

};

#endif //MKS_MEMORY_POOL_H
