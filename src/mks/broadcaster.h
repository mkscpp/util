//
// Created by Michal Němec on 31/01/2020.
//

#ifndef MKS_BROADCASTER_H
#define MKS_BROADCASTER_H

#include <functional>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <mks/log.h>

#include "has_unique_id.h"

namespace mks {

template<class Fp> class broadcaster;
template<class Fp> class observer;

template<class Fp>
using fn_t = std::function<Fp>;

template<class Rp, class ...ArgTypes>
class observer<Rp(ArgTypes...)> : public has_unique_id<observer<Rp(ArgTypes...)>> {
public:
    using cb_t = fn_t<Rp(ArgTypes...)>;

private:
    mutable std::mutex mutex_;
    std::function<void()> on_cancel_;
    std::function<void()> on_stop_ = [](){};
    cb_t cb_;
    bool stopped_ = true;
    bool verbose_ = false;

    void broadcaster_stopped() {
        if(verbose_) {
            MKS_LOG_D("router listener killed uid={}", uid());
        }
        std::function<void()> stop_cb;
        {
            // this mutex will block broadcaster callback when iterating in clear()
            // see cancel();
            std::lock_guard<std::mutex> ll{mutex_};
            stop_cb = on_stop_;
            stopped_ = true;
            reset_cancel_cb();
        }
        stop_cb();
    }

    void default_cancel() {
        if(verbose_) {
            MKS_LOG_D("default canceled");
        }
    }

    void reset_cancel_cb() {
        if(verbose_) {
            MKS_LOG_D("reset_cancel_cb uid={}", uid());
        }
        on_cancel_ = [&](){
            default_cancel();
        };
    }

    uint64_t uid() const noexcept {
        return observer<Rp(ArgTypes...)>::unique_id();
    }

public:
    observer() = default;
    observer(observer const&) = delete;
    observer& operator=(observer const&) = delete;
    observer(observer const&&) = delete;
    observer&& operator=(observer const&&) = delete;

    virtual ~observer() {
        if(verbose_) {
            MKS_LOG_D("destruct");
        }
        std::unique_lock<std::mutex> ll{mutex_};
        if(!stopped_) {
            if(verbose_) {
                MKS_LOG_D("calling cancel");
            }
            ll.unlock();
            cancel();
            ll.lock();
        }
        MKS_ASSERT(stopped_);
    }

    bool stopped() const noexcept {
        std::lock_guard<std::mutex> ll_obs{mutex_};
        return stopped_;
    }

    void on_stopped(std::function<void()> cb) {
        bool already_stopped;
        std::function<void()> stop_cb;
        {
            std::lock_guard<std::mutex> ll{mutex_};
            on_stop_ = std::move(cb);
            already_stopped = stopped_;
            stop_cb = on_stop_;
        }
        if(already_stopped) {
            stop_cb();
        }
    }

    virtual void on_data(cb_t cb) {
        std::lock_guard<std::mutex> ll{mutex_};
        cb_ = std::move(cb);
    }

    void cancel() {
        std::lock_guard<std::mutex> ll{mutex_};
        stopped_ = true;
        auto cb = on_cancel_;
        reset_cancel_cb();

        // ll.unlock() <-- WHY NOT, pretty crucial
        //
        // 'on_cancel_' is callback into the observer
        // it is called with mutex locked since otherwise
        // there can be condition that:
        // thread 1 = cancel is called, 'on_cancel_' stored in 'cb' and then changed to default in reset_cancel_cb()
        // thread 1 = calls callback to cancel, hits broadcasters mutex
        // thread 1 = if broadcaster mutex is locked then thread 1 is waiting
        // thread 2 = broadcaster is in the middle of clear() with locked mutex, clearing all its listeners together with this one
        //          = during clear() we iterate each listener and call broadcaster_stopped() method on them
        //          = when calling broadcaster_stopped() broadcaster mutex is unlocked
        //          = if we call broadcaster_stopped() on observer that waits broadcaster mutex in process_cancel() we will end on mutex
        //            that is held in this function
        // thread 1 = since broadcaster will be unlocked and waiting on this observers mutex, this observer will automatically
        //            be able to correctly obtain broadcasters mutex and removes itself from iterating map
        //
        // if ll.unlock() is called then we will sonner or later, mainly on slower debugger/valgrind get assert error in
        // process_cancel() method which requires that observer is always present in the listeners
        cb();
    }

    friend broadcaster<Rp(ArgTypes...)>;
};

template<class Rp, class ...ArgTypes>
class broadcaster<Rp(ArgTypes...)> {
public:
    using observer_t = observer<Rp(ArgTypes...)>;

private:
    mutable std::mutex mutex_;
    /*
     * We use raw pointers with respect only to this file
     * destructor of observer must always call cancel to remote itself from broadcaster
     */
    mutable bool listeners_iterating_ = false;
    mutable typename std::unordered_map<uint64_t, observer_t*>::iterator active_iterator_;
    mutable std::unordered_map<uint64_t, observer_t*> listeners_;
    mutable std::unordered_map<uint64_t, observer_t*> new_listeners_;
    bool verbose_ = false;

    bool remove_key(std::unordered_map<uint64_t, observer_t*>& map, uint64_t uid) {
        auto it = map.find(uid);
        if(it != map.end()) {
            auto erase_it = map.erase(it);
            if(listeners_iterating_) {
                if(verbose_) {
                    MKS_LOG_D("remove key active iterate");
                }
                if(it == active_iterator_) {
                    if(verbose_) {
                        MKS_LOG_D("moving to active iterator");
                    }
                    active_iterator_ = erase_it;
                }
            }
            return true;
        }
        return false;
    }

    void add_key(uint64_t key, observer_t* val) {
        // notify cached listeners
        if(listeners_iterating_) {
            if(verbose_) {
                MKS_LOG_D("add implicit key={} val={:p}", key, (void*) val);
            }
            new_listeners_[key] = val;
        } else {
            if(verbose_) {
                MKS_LOG_D("add direct key={} val={:p}", key, (void*) val);
            }
            listeners_[key] = val;
        }
    }

    void remove_all_keys() {
        listeners_.clear();
        new_listeners_.clear();
        if(listeners_iterating_) {
            active_iterator_ = listeners_.end();
        }
    }

    void iterate_listeners(std::unique_lock<std::mutex>& lock, const std::function<void(observer_t*)>& cb) const {
        listeners_iterating_ = true;
        for(auto it = listeners_.begin(); it != listeners_.end();) {
            auto prev = it;
            active_iterator_ = it;

            auto wl = (*it).second;
            MKS_ASSERT(wl != nullptr);
            lock.unlock();
            cb(wl);
            lock.lock();

            it = active_iterator_;
            if(prev == it) {
                ++it;
            }
        }
        listeners_iterating_ = false;
        if(!new_listeners_.empty()) {
            if(verbose_) {
                MKS_LOG_D("implicit add nex listeners size={}", new_listeners_.size());
            }
            /*
             * when inserting already present items will nto be added
             * but that should never happen so assert is here
             */
            auto prev_listener_size = listeners_.size();
            auto expected_size = prev_listener_size + new_listeners_.size();
            listeners_.insert(new_listeners_.begin(), new_listeners_.end());
            MKS_ASSERT(listeners_.size() == expected_size);
            new_listeners_.clear();
        }
    }

    void process_cancel(uint64_t uid) {
        std::lock_guard<std::mutex> ll{mutex_};
        if(!remove_key(listeners_, uid)) {
            if(verbose_) {
                MKS_LOG_D("cancel implicit uid={}", uid);
            }
            auto ret = remove_key(new_listeners_, uid);
            // we require strong relation that observer must be always present in listenes
            // see comments in observer::cancel() method
            MKS_ASSERT(ret);
        } else {
            if(verbose_) {
                MKS_LOG_D("cancel direct uid={}", uid);
            }
        }
    }

public:
    broadcaster() = default;
    broadcaster(broadcaster const&) = delete;
    broadcaster& operator=(broadcaster const&) = delete;
    broadcaster(broadcaster const&&) = delete;
    broadcaster&& operator=(broadcaster const&&) = delete;

    void bind(observer_t& obs) {
        std::unique_lock<std::mutex> lock_obs{obs.mutex_};
        MKS_ASSERT(obs.stopped_);
        std::unique_lock<std::mutex> ll{mutex_};
        auto v_uid =  obs.unique_id();
        obs.on_cancel_ = [this, v_uid](){
            process_cancel(v_uid);
        };
        obs.stopped_ = false;
        add_key(v_uid, &obs);
    }

    void clear() {
        std::unique_lock<std::mutex> lock{mutex_};
        if(verbose_) {
            MKS_LOG_D("stop has={} new_list={}", listeners_.size(), new_listeners_.size());
        }
        // we will iterate all current listeners without new ones that will come during that
        iterate_listeners(lock, [&](observer_t* obs){
            obs->broadcaster_stopped();
        });
        remove_all_keys();
    }

    // function invocation:
    std::size_t operator()(ArgTypes... arg) const {
        std::unique_lock<std::mutex> lock{mutex_};
        uint64_t called = 0;
        auto had = listeners_.size();
        iterate_listeners(lock, [&](observer_t* obs){
            obs->cb_(arg...);
            called++;
        });
        if(verbose_) {
            MKS_LOG_D("had={} has={} called={}", had, listeners_.size(), called);
        }
        return called;
    }

    std::size_t size() const noexcept {
        std::lock_guard<std::mutex> lock{mutex_};
        return listeners_.size()+new_listeners_.size();
    }

    std::size_t empty() const noexcept {
        std::lock_guard<std::mutex> lock{mutex_};
        return listeners_.empty() && new_listeners_.empty();
    }

};

}
#endif //MKS_BROADCASTER_H
