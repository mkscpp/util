//
// Created by Michal Němec on 02/01/2020.
//

#ifndef MKS_LIFECYCLE_H
#define MKS_LIFECYCLE_H

#include <functional>
#include <mutex>
#include <string>

#include <mks/log.h>

#define MKS_LIFECYCLE_ENABLE

#ifdef MKS_LIFECYCLE_ENABLE
#define MKS_LIFECYCLE_COMMA ,
#define MKS_LIFECYCLE_DDOT :
#define MKS_LIFECYCLE(x) mks::lifecycle<x>
#define MKS_LIFECYCLE_WATCH(x, listener) MKS_LIFECYCLE(x)::set_listener(listener)
#define MKS_LIFECYCLE_CREATED(x) MKS_LIFECYCLE(x)::created()
#define MKS_LIFECYCLE_DESTRUCTED(x) MKS_LIFECYCLE(x)::destructed()
#define MKS_LIFECYCLE_MOVED(x) MKS_LIFECYCLE(x)::moved()
#define MKS_LIFECYCLE_COPIED(x) MKS_LIFECYCLE(x)::copied()
#define MKS_LIFECYCLE_ALIVE(x) MKS_LIFECYCLE(x)::alive()
#define MKS_LIFECYCLE_WATCH_ALIVE(x)                                                               \
    do {                                                                                           \
        MKS_LIFECYCLE_WATCH(x, [](mks::lifecycle<x> *, mks::lifecycle_state state) {               \
            auto alive = MKS_LIFECYCLE_ALIVE(x);                                                   \
            MKS_LOG_D(#x " state: {}, alive: {}", mks::lifecycle_state_str(state), alive);         \
        });                                                                                        \
    } while(0)

namespace mks {

enum class lifecycle_state : int { created, copied, moved, destructed };

std::string lifecycle_state_str(lifecycle_state state);

template <typename T, typename lifecycle_t>
struct lifecycle;

template <typename T, typename lifecycle_t>
using lifecycle_cb = std::function<void(lifecycle<T, lifecycle_t> *, lifecycle_state)>;

template <typename T, typename lifecycle_t>
void lifecycle_empty_cb(lifecycle<T, lifecycle_t> *, lifecycle_state) {}

template <typename T, typename lifecycle_t = uint64_t>
struct lifecycle {
    lifecycle(const lifecycle &) {
        {
            std::lock_guard<std::mutex> ll{mutex_};
            ++created_;
            ++copied_;
            ++alive_;
        }
        cb_(this, lifecycle_state::copied);
    }

    lifecycle(lifecycle &&) noexcept {

        {
            std::lock_guard<std::mutex> ll{mutex_};
            ++created_;
            ++moved_;
            ++alive_;
        }
        cb_(this, lifecycle_state::moved);
    }

    lifecycle &operator=(const lifecycle &) noexcept = default;

    virtual ~lifecycle() {
        {
            std::lock_guard<std::mutex> ll{mutex_};
            ++destructed_;
            MKS_ASSERT(alive_ > 0);
            --alive_;
        }
        cb_(this, lifecycle_state::destructed);
    }


    static void set_listener(lifecycle_cb<T, lifecycle_t> cb) noexcept {
        cb_ = std::move(cb);
    }

    static lifecycle_t created() {
        lifecycle_t out;
        {
            std::lock_guard<std::mutex> ll{mutex_};
            out = created_;
        }
        return out;
    }

    static lifecycle_t moved() {
        lifecycle_t out;
        {
            std::lock_guard<std::mutex> ll{mutex_};
            out = moved_;
        }
        return out;
    }

    static lifecycle_t copied() {
        lifecycle_t out;
        {
            std::lock_guard<std::mutex> ll{mutex_};
            out = copied_;
        }
        return out;
    }

    static lifecycle_t destructed() {
        lifecycle_t out;
        {
            std::lock_guard<std::mutex> ll{mutex_};
            out = destructed_;
        }
        return out;
    }

    static lifecycle_t alive() {
        lifecycle_t out;
        {
            std::lock_guard<std::mutex> ll{mutex_};
            MKS_ASSERT(alive_ >= 0);
            out = alive_;
        }
        return out;
    }

protected:
    lifecycle() {
        {
            std::lock_guard<std::mutex> ll{mutex_};
            ++created_;
            ++alive_;
        }
        cb_(this, lifecycle_state::created);
    }

private:
    static std::mutex mutex_;
    static lifecycle_cb<T, lifecycle_t> cb_;
    static lifecycle_t created_;
    static lifecycle_t alive_;
    static lifecycle_t moved_;
    static lifecycle_t copied_;
    static lifecycle_t destructed_;
};

template <typename T, typename lifecycle_t>
lifecycle_cb<T, lifecycle_t> lifecycle<T, lifecycle_t>::cb_ = lifecycle_empty_cb<T, lifecycle_t>;

template <typename T, typename lifecycle_t>
std::mutex lifecycle<T, lifecycle_t>::mutex_{};

template <typename T, typename lifecycle_t>
lifecycle_t lifecycle<T, lifecycle_t>::created_{0};

template <typename T, typename lifecycle_t>
lifecycle_t lifecycle<T, lifecycle_t>::destructed_{0};

template <typename T, typename lifecycle_t>
lifecycle_t lifecycle<T, lifecycle_t>::moved_{0};

template <typename T, typename lifecycle_t>
lifecycle_t lifecycle<T, lifecycle_t>::copied_{0};

template <typename T, typename lifecycle_t>
lifecycle_t lifecycle<T, lifecycle_t>::alive_{0};

} // namespace mks

#else
#define MKS_LIFECYCLE_DDOT
#define MKS_LIFECYCLE_COMMA
#define MKS_LIFECYCLE(x)
#define MKS_LIFECYCLE_WATCH(x, listener)
#define MKS_LIFECYCLE_WATCH_ALIVE(x)
#define MKS_LIFECYCLE_CREATED(x) 0
#define MKS_LIFECYCLE_DESTRUCTED(x) 0
#define MKS_LIFECYCLE_MOVED(x) 0
#define MKS_LIFECYCLE_COPIED(x) 0
#define MKS_LIFECYCLE_ALIVE(x) 0
#endif


#endif // MKS_LIFECYCLE_H
