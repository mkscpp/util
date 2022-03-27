//
// Created by michalks on 28.07.19.
//

#ifndef MKS_QUEUE_BUFFER_H
#define MKS_QUEUE_BUFFER_H

#include <condition_variable>
#include <deque>

namespace mks {

template <typename T, typename Alloc = std::allocator<T>, template <typename U = T, typename A = Alloc> class V = std::deque>
class queue_buffer {

public:

    void add_first(const T &num) {
        std::unique_lock<std::mutex> locker(mu_);
        cond_.wait(locker, [this]() { return buffer_.size() < size_; });
        buffer_.push_front(num);
        locker.unlock();
        cond_.notify_one();
    }

    void add_first(T &&num) {
        std::unique_lock<std::mutex> locker(mu_);
        cond_.wait(locker, [this]() { return buffer_.size() < size_; });
        buffer_.push_front(std::forward<T>(num));
        locker.unlock();
        cond_.notify_one();
    }

    void add(const T &num) {
        std::unique_lock<std::mutex> locker(mu_);
        cond_.wait(locker, [this]() { return buffer_.size() < size_; });
        buffer_.push_back(num);
        locker.unlock();
        cond_.notify_one();
    }

    void add(T &&num) {
        std::unique_lock<std::mutex> locker(mu_);
        cond_.wait(locker, [this]() { return buffer_.size() < size_; });
        buffer_.push_back(std::forward<T>(num));
        locker.unlock();
        cond_.notify_one();
    }

    void clear() {
        std::unique_lock<std::mutex> locker(mu_);
        buffer_.clear();
        locker.unlock();
        cond_.notify_one();
    }

    void clear_add(const T &val, std::size_t count = 1) {
        std::unique_lock<std::mutex> locker(mu_);
        buffer_.clear();
        for(std::size_t i = 0; i != count; ++i) {
            buffer_.push_back(val);
        }
        locker.unlock();
        cond_.notify_one();
    }

    T remove() {
        std::unique_lock<std::mutex> locker(mu_);
        cond_.wait(locker, [this]() { return buffer_.size() > 0; });
        T back = std::move(buffer_.front());
        buffer_.pop_front();
        locker.unlock();
        cond_.notify_one();
        return back;
    }

    bool try_remove(const std::function<void(T &)> cb) {
        std::unique_lock<std::mutex> locker(mu_);
        if(!buffer_.empty()) {
            T back = std::move(buffer_.front());
            buffer_.pop_front();
            locker.unlock();
            cond_.notify_one();
            cb(back);
            return true;
        }
        return false;
    }

    std::vector<T> state() {
        std::unique_lock<std::mutex> locker(mu_);
        std::vector<T> copy;
        for(const auto &it : buffer_) {
            copy.push_back(it);
        }
        return copy;
    }

    std::vector<T> state_move() {
        std::unique_lock<std::mutex> locker(mu_);
        std::vector<T> copy;
        for(auto &&it : buffer_) {
            copy.push_back(std::forward<T>(it));
        }
        buffer_.clear();
        return copy;
    }

    std::size_t size() const {
        return buffer_.size();
    }

    bool empty() const {
        return buffer_.empty();
    }

private:
    // Add them as member variables here
    std::mutex mu_;
    std::condition_variable cond_;

    // Your normal variables here
    V<T, Alloc> buffer_;
    const unsigned long size_ = std::numeric_limits<unsigned long>::max();
};

} // namespace mks
#endif // MKS_QUEUE_BUFFER_H
