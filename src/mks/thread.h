//
// Created by Michal Němec on 08/09/2020.
//

#ifndef MKS_THREAD_H
#define MKS_THREAD_H

#include <cassert>
#include <thread>
#include <functional>
#include <mutex>
#include <string>
#include <tuple>
#include <utility>

namespace mks {

// implementation of joining thread
// C++20 has it in standard https://en.cppreference.com/w/cpp/thread/jthread

class jthread : private std::thread {
public:
    using std::thread::thread;

    jthread() = default;
    jthread(jthread&&) = default;
    jthread& operator=(jthread&&) = default;

    virtual ~jthread() {
        if(joinable()) {
            join(); // or detach() if you prefer}
        }
    }

    jthread(std::thread t) : std::thread(std::move(t)) {

    }

    using std::thread::join;
    using std::thread::detach;
    using std::thread::joinable;
    using std::thread::get_id;
    using std::thread::hardware_concurrency;

    void swap(jthread& x) {
        std::thread::swap(x);
    }
};

inline void swap(jthread& x, jthread& y) {x.swap(y);}

// Capture args and add them as additional arguments
// C++17 code only, see std::apply (it is possible to implement in C++11 if needed)
template <typename Lambda, typename ... Args>
auto capture_call(Lambda&& lambda, Args&& ... args){
    return [
        lambda = std::forward<Lambda>(lambda),
        capture_args = std::make_tuple(std::forward<Args>(args) ...)
    ](auto&& ... original_args)mutable{
      return std::apply([&lambda](auto&& ... args){
        lambda(std::forward<decltype(args)>(args) ...);
      }, std::tuple_cat(
          std::forward_as_tuple(original_args ...),
          std::apply([](auto&& ... args){
            return std::forward_as_tuple< Args ... >(
                std::move(args) ...);
          }, std::move(capture_args))
      ));
    };
}

enum class thread_state {
    none,
    created,
    starting,
    started,
    finished,
    joining,
    stopped,
    fail_create,
    destructed
};

struct thread_info {
    std::string name;
};

const char* thread_state_string(thread_state state);

class thread;
using thread_state_listener = std::function<void(thread*, thread_state, thread_state)>;
/**
 * Thread wrapper class that can be used for monitoring spawned threads
 */
class thread {
    static std::mutex state_mtx_;
    static thread_state_listener state_listener_;


    class thread_priv {
        mks::thread* self_ = nullptr;
        std::mutex th_mtx_;
        thread_info info_;
        thread_state state_ = thread_state::none;

        void update_state(thread_state state) {
            assert(self_ != nullptr);

            std::unique_lock<std::mutex> ll(th_mtx_);
            auto prev = state_;
            state_ = state;
            auto now = state;
            ll.unlock();

            std::unique_lock<std::mutex> ll_state(state_mtx_);
            auto listener = state_listener_;
            ll_state.unlock();

            if(listener) {
                listener(self_, prev, now);
            }
        }

        friend thread;
    };

    // since std::mutex doesnt have move we will allocate it on the heap
    std::unique_ptr<thread_priv> th_priv_;

    // std::thread has move
    std::thread th_;
public:

    thread() = default;

    /**
     * Construct thread with name for monitoring
     * @tparam Fp
     * @tparam Args
     * @param name
     * @param f
     * @param args
     */
    template <class Fp, class ...Args>
    explicit thread(thread_info info, Fp&& f, Args&&... args) {
        th_priv_ = std::make_unique<thread_priv>();
        assert(th_priv_);
        th_priv_->info_ = std::move(info);
        th_priv_->self_ = this;
        th_priv_->update_state(thread_state::created);
        th_priv_->update_state(thread_state::starting);
        try {
            auto *ptr = th_priv_.get();
            th_ = std::thread([ptr, fm = std::forward<Fp>(f)](Args&& ... args){
                // we must not capture this since when moved it would have pointer to deleted object
                // th_priv_ is moved during std::move but pointer is alive
                ptr->update_state(thread_state::started);
                // thread started
                fm(std::forward<Args>(args) ...);
                // thread stopping, join will be possible in a moment
                ptr->update_state(thread_state::finished);
            }, std::forward<Args>(args) ...);
        } catch(...) {
            th_priv_->update_state(thread_state::fail_create);
        }
    }

    /**
     * Construct thread with name for monitoring
     * @tparam Fp
     * @tparam Args
     * @param name
     * @param f
     * @param args
     */
    template <class Fp, class ...Args>
    explicit thread(std::string name, Fp&& f, Args&&... args) {
        th_priv_ = std::make_unique<thread_priv>();
        assert(th_priv_);
        th_priv_->self_ = this;
        th_priv_->info_.name = std::move(name);

        th_priv_->update_state(thread_state::created);
        th_priv_->update_state(thread_state::starting);
        try {
            auto *ptr = th_priv_.get();
            th_ = std::thread([ptr, fm = std::forward<Fp>(f)](Args&& ... args){
                // we must not capture this since when moved it would have pointer to deleted object
                // th_priv_ is moved during std::move but pointer is alive
                ptr->update_state(thread_state::started);
                // thread started
                fm(std::forward<Args>(args) ...);
                // thread stopping, join will be possible in a moment
                ptr->update_state(thread_state::finished);
            }, std::forward<Args>(args) ...);
        } catch(...) {
            th_priv_->update_state(thread_state::fail_create);
        }
    }

    /**
     * Construct thread without a name, since Fp is templated we need to explicitly say
     * that we cont want string to activate other constructor
     * @tparam Fp
     * @tparam Args
     * @param f
     * @param args
     */
    template <class Fp, class ...Args,
        class = typename std::enable_if<
            !std::is_same<Fp, std::string>::value &&
            !std::is_same<Fp, thread_info>::value &&
            !std::is_same<Fp, const char*>::value
        >::type>
    explicit thread(Fp&& f, Args&&... args) {
        th_priv_ = std::make_unique<thread_priv>();
        assert(th_priv_);
        th_priv_->self_ = this;
        th_priv_->update_state(thread_state::created);
        th_priv_->update_state(thread_state::starting);
        try {
            auto *ptr = th_priv_.get();
            th_ = std::thread([ptr, fm = std::forward<Fp>(f)](Args&& ... args){
                // we must not capture this since when moved it would have pointer to deleted object
                // th_priv_ is moved during std::move but pointer is alive
                ptr->update_state(thread_state::started);
                // thread started
                fm(std::forward<Args>(args) ...);
                // thread stopping, join will be possible in a moment
                ptr->update_state(thread_state::finished);
            }, std::forward<Args>(args) ...);
        } catch(...) {
            th_priv_->update_state(thread_state::fail_create);
        }
    }

    // disable copy
    thread(const thread&) = delete;
    thread& operator=(const thread&) = delete;

    // enable move
    thread(thread&& other) noexcept {
        // sanity check
        assert(other.th_priv_);
        assert(other.th_priv_->self_ == &other);

        // move private data
        th_priv_ = std::move(other.th_priv_);
        th_ = std::move(other.th_);
        assert(th_priv_);

        // reassign private data to actula instance
        th_priv_->self_ = this;
    }

    thread& operator=(thread&& other) noexcept {
        // sanity check
        assert(other.th_priv_);
        assert(other.th_priv_->self_ == &other);

        // move private data
        th_priv_ = std::move(other.th_priv_);
        th_ = std::move(other.th_);
        assert(th_priv_);

        // reassign private data to actula instance
        th_priv_->self_ = this;
        return *this;
    }

    ~thread() {
        if(th_priv_) {
            // sanity check
            assert(th_priv_->self_ == this);
            if(th_priv_->state_ != thread_state::stopped) {
                finish();
            }
            th_priv_->update_state(thread_state::destructed);
        }
    }

    bool joinable() const noexcept {
        return th_.joinable();
    }

    void finish() {
        if(th_.joinable()) {
            th_priv_->update_state(thread_state::joining);
            th_.joinable();
        }
        th_priv_->update_state(thread_state::stopped);
    }

    void join() {
        th_.join();
    }

    void detach() {
        th_.detach();
    }

    std::thread::id get_id() const noexcept {
        return th_.get_id();
    }

    std::thread& th() {
        return th_;
    }

    thread_state state() const {
        return th_priv_->state_;
    }

    const std::string& name() const {
        return th_priv_->info_.name;
    }

    static unsigned hardware_concurrency() noexcept {
        return std::thread::hardware_concurrency();
    }

    static void set_state_listener(thread_state_listener cb) {
        std::lock_guard<std::mutex> ll_state(state_mtx_);
        state_listener_ = std::move(cb);
    }

};


}

#endif // MKS_THREAD_H
