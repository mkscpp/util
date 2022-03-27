//
// Created by Michal Němec on 09/08/2020.
//

#ifndef MKS_DISPATCH_QUEUE_H
#define MKS_DISPATCH_QUEUE_H

/*
 * Dispatch queue used for scheduling callbacks at some delays
 * dispatch_queue is not THREAD SAFE, and should be used with some synchronization to ensure correct scheduling
 */

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <deque>
#include <functional>
#include <utility>

#include <date/date.h>
#include <date/tz.h>

#include <mks/log.h>

using dispatch_clock = std::chrono::system_clock;
using dispatch_time_t = dispatch_clock::duration;

template <class T>
struct dispatch_message {
    dispatch_message<T> *prev_message = nullptr;
    dispatch_message<T> *next_message = nullptr;
    bool in_use = false;

    T val;
    dispatch_clock::time_point when;

    explicit dispatch_message(const T &val) : val(val) {}
    explicit dispatch_message(T &&val) : val(std::move(val)) {}
};

/**
 * Dispatch message pool for reusing messages passed to the dispatch_queue
 * internally we use same double linked list capabilities from dispatch_message
 * just without time information
 * @tparam T - object saved
 * @tparam MAX_OBJECTS - maximum size of the pool before objects are discarded
 */
template <typename T, std::size_t MAX_OBJECTS>
class dispatch_pool {
    std::size_t size_ = 0;
    dispatch_message<T> *head_message_ = nullptr;
    dispatch_message<T> *tail_message_ = nullptr;

    bool enqueue(dispatch_message<T> *message) {
        assert(!message->in_use);
        message->in_use = false;
        if(head_message_ == nullptr) {
            auto old_head_message = head_message_;
            head_message_ = message;
            if(old_head_message != nullptr) {
                old_head_message->prev_message = head_message_;
            } else {
                tail_message_ = head_message_;
            }
            head_message_->next_message = old_head_message;
        } else {
            message->prev_message = tail_message_;
            tail_message_->next_message = message;
            tail_message_ = message;
        }
        size_++;
        return true;
    }

    dispatch_message<T> *dequeue() {
        auto message = head_message_;
        if(message != nullptr) {
            head_message_ = message->next_message;
            if(head_message_ != nullptr) {
                head_message_->prev_message = nullptr;
            }
            message->prev_message = nullptr;
            message->next_message = nullptr;
            --size_;
            return message;
        }
        return nullptr;
    }

public:
    ~dispatch_pool() {
        while(!empty()) {
            auto *pv = dequeue();
            MKS_ASSERT(pv != nullptr);
            delete pv;
        }
    }

    std::size_t max_size() {
        return MAX_OBJECTS;
    }

    std::size_t size() const {
        return size_;
    }

    bool empty() const {
        return size() == 0;
    }

    void put(dispatch_message<T> *val) {
        if(size() >= MAX_OBJECTS) {
            delete val;
            return;
        }
        val->next_message = nullptr;
        val->prev_message = nullptr;
        val->val = T();
        enqueue(val);
    }

    dispatch_message<T> *get(T &&val) {
        if(empty()) {
            return new dispatch_message<T>(std::forward<T>(val));
        }
        auto *pv = dequeue();
        pv->val = std::forward<T>(val);
        return pv;
    }

    dispatch_message<T> *get(const T &val) {
        if(empty()) {
            return new dispatch_message<T>(val);
        }
        auto *pv = dequeue();
        pv->val = val;
        return pv;
    }
};

template <typename T>
class dispatch_queue {
    std::size_t size_ = 0;
    dispatch_message<T> *head_message_ = nullptr;
    dispatch_message<T> *tail_message_ = nullptr;

    dispatch_pool<T, 1024> msg_pool_;

    std::function<void(dispatch_time_t)> run_timer_cb_;

    static dispatch_clock::time_point now() {
        return dispatch_clock::now();
    }

    void signal_timer(dispatch_time_t time) {
        assert(run_timer_cb_);
        run_timer_cb_(time);
    }

    dispatch_message<T> *dequeue_head() {
        auto message = head_message_;
        if(message != nullptr) {
            head_message_ = message->next_message;
            if(head_message_ != nullptr) {
                head_message_->prev_message = nullptr;
            }
            message->prev_message = nullptr;
            message->next_message = nullptr;
            --size_;
            assert(message->in_use);
            message->in_use = false;
            return message;
        }
        return nullptr;
    }

    dispatch_message<T> *dequeue() {
        auto now = dispatch_clock::now();
        auto message = head_message_;
        if(message != nullptr) {
            if(now < message->when) {
                signal_timer(message->when - now);
                return nullptr;
            } else {
                head_message_ = message->next_message;
                if(head_message_ != nullptr) {
                    head_message_->prev_message = nullptr;
                }
                message->prev_message = nullptr;
                message->next_message = nullptr;
                assert(size_ > 0);
                --size_;
                assert(message->in_use);
                message->in_use = false;
                return message;
            }
        }
        return nullptr;
    }

    bool enqueue(dispatch_message<T> *message) {
        assert(!message->in_use);
        message->in_use = true;
        auto &when = message->when;
        if(head_message_ == nullptr || when < head_message_->when) {
            auto old_head_message = head_message_;
            head_message_ = message;
            if(old_head_message != nullptr) {
                old_head_message->prev_message = head_message_;
            } else {
                tail_message_ = head_message_;
            }
            head_message_->next_message = old_head_message;
        } else if(when >= tail_message_->when) {
            message->prev_message = tail_message_;
            tail_message_->next_message = message;
            tail_message_ = message;
        } else {
            auto *current_message = tail_message_;
            dispatch_message<T> *next_message = nullptr;
            for(;;) {
                next_message = current_message;
                current_message = current_message->prev_message;
                if(when >= current_message->when) {
                    break;
                }
            }
            message->next_message = next_message;
            message->prev_message = current_message;
            next_message->prev_message = message;
            current_message->next_message = message;
        }
        size_++;
        return true;
    }

    void schedule_timer() {
        auto now = dispatch_clock::now();
        auto message = head_message_;
        if(message != nullptr) {
            if(now < message->when) {
                auto diff = message->when - now;
                signal_timer(diff);
            } else {
                signal_timer(dispatch_time_t{0});
            }
        }
    }

public:
    ~dispatch_queue() {
        MKS_ASSERT(empty());
        // messages would be leaked if queue is not empty
        /*
        while(!empty()) {
            auto* ptr = dequeue_head();
            delete ptr;
        }
        */
    }

    template <class Rep, class Period>
    dispatch_message<T>* post_delayed(const T &arg, const std::chrono::duration<Rep, Period> &time) {
        auto *msg = msg_pool_.get(arg);
        msg->when = now() + time;
        enqueue(msg);
        schedule_timer();
        return msg;
    }

    template <class Rep, class Period>
    dispatch_message<T>* post_delayed(T &&arg, const std::chrono::duration<Rep, Period> &time) {
        auto *msg = msg_pool_.get(std::move(arg));
        msg->when = now() + time;
        enqueue(msg);
        schedule_timer();
        return msg;
    }

    template <class Clock, class Duration>
    dispatch_message<T>* post_at(const T &arg, const std::chrono::time_point<Clock, Duration> &time) {
        auto *msg = msg_pool_.get(arg);
        msg->when = time;
        enqueue(msg);
        schedule_timer();
        return msg;
    }

    template <class Clock, class Duration>
    dispatch_message<T>* post_at(T &&arg, const std::chrono::time_point<Clock, Duration> &time) {
        auto *msg = msg_pool_.get(std::move(arg));
        msg->when = time;
        enqueue(msg);
        schedule_timer();
        return msg;
    }

    void remove(dispatch_message<T>* msg) {
        assert(msg->in_use);
        auto* prev = msg->prev_message;
        auto* next = msg->next_message;
        if(prev != nullptr) {
            prev->next_message = next;
        }
        if(next != nullptr) {
            next->prev_message = prev;
        }
        msg->prev_message = nullptr;
        msg->next_message = nullptr;
        msg->in_use = false;
        if(msg == head_message_) {
            head_message_ = prev;
        } else if(msg == tail_message_) {
            tail_message_ = next;
        }
        assert(size_ > 0);
        --size_;
        msg_pool_.put(msg);
        schedule_timer();
    }

    void get(std::function<void(const T &)> cb) {
        auto *msg = dequeue();
        if(msg != nullptr) {
            auto val = std::move(msg->val);
            cb(val);
            msg_pool_.put(msg);
            schedule_timer();
        }
    }

    void get(std::function<void(T &&)> cb) {
        auto *msg = dequeue();
        if(msg != nullptr) {
            cb(std::move(msg->val));
            msg_pool_.put(msg);
            schedule_timer();
        }
    }

    void clear(const std::function<void(dispatch_message<T>*)>& cb) {
        dispatch_message<T>* msg;
        while((msg = dequeue_head()) != nullptr) {
            cb(msg);
            delete msg;
        }
        MKS_ASSERT(empty());
    }

    void on_timer(std::function<void(dispatch_time_t)> cb) {
        run_timer_cb_ = std::move(cb);
    }

    std::size_t size() const {
        return size_;
    }

    bool empty() const {
        return size() == 0;
    }
};


#endif // MKS_DISPATCH_QUEUE_H
