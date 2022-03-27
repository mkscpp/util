//
// Created by Michal Němec on 02/02/2020.
//

#ifndef MKS_LIVE_DATA_H
#define MKS_LIVE_DATA_H

#include "broadcaster.h"
#include "meta_apply.h"

/*
 * live data is direct wrapper around observer
 * data obtained from broadcaster is saved and can be obtained in between calls
 */

namespace mks {

template<class Fp> class live_data;

template<class Rp, class ...ArgTypes>
class live_data<Rp(ArgTypes...)>  : public observer<Rp(ArgTypes...)>{

    using base = observer<Rp(ArgTypes...)>;

    mutable std::mutex mutex_;
    std::tuple<ArgTypes...> value_;
    typename base::cb_t live_data_cb_;

public:
    live_data() {
        base::on_data([&](ArgTypes... arg){
            // store values
            std::tuple<ArgTypes...> value{arg...};
            {
                // swap with internal storage
                std::lock_guard<std::mutex> ll{mutex_};
                value_.swap(value);
            }
            if(live_data_cb_) {
                live_data_cb_(arg...);
            }
        });
    }

    void on_data(typename base::cb_t cb) override {
        std::lock_guard<std::mutex> ll{mutex_};
        live_data_cb_ = std::move(cb);
    }

    std::tuple<ArgTypes...> values() const {
        std::lock_guard<std::mutex> ll{mutex_};
        return value_;
    }

    template< std::size_t I>
    typename std::tuple_element<I, std::tuple<ArgTypes...>>::type
    value() noexcept {
        std::lock_guard<std::mutex> ll{mutex_};
        return std::get<I>(value_);
    }

    template< std::size_t I>
    typename std::tuple_element<I, std::tuple<ArgTypes...>>::type&
    value_ref() noexcept {
        std::lock_guard<std::mutex> ll{mutex_};
        return std::get<I>(value_);
    }

    template< std::size_t I>
    typename std::tuple_element<I, std::tuple<ArgTypes...>>::type&&
    value_move() noexcept {
        std::lock_guard<std::mutex> ll{mutex_};
        return std::get<I>(value_);
    }

    template< std::size_t I>
    typename std::tuple_element<I, std::tuple<ArgTypes...>>::type const&
    value_const_ref() noexcept {
        std::lock_guard<std::mutex> ll{mutex_};
        return std::get<I>(value_);
    }

    template< std::size_t I>
    typename std::tuple_element<I, std::tuple<ArgTypes...>>::type const&&
    value_const_move() noexcept {
        std::lock_guard<std::mutex> ll{mutex_};
        return std::get<I>(value_);
    }

    /**
     * instead of parsing each value from tuple we can invoke function with arguments of the tuple directly
     * same as on_data callbacks but with saved data
     *
     * live_data<void(int, int)> obs;
     * obs.apply([](int v1, int v2){
     *      //process v1, v2 directly
     * })
     *
     * @tparam T - callable type T::operator(ArgTypes...)
     * @param fn - callback object
     */
    template<typename T>
    void apply(const T& fn) {
        std::unique_lock<std::mutex> ll{mutex_};
        // make copy, do not block new data waiting from broadcaster
        std::tuple<ArgTypes...> value{value_};
        ll.unlock();
        // run callback
        mks::apply(fn, value);
    }

    template<typename T>
    void apply(T&& fn) {
        std::unique_lock<std::mutex> ll{mutex_};
        // make copy, do not block new data waiting from broadcaster
        std::tuple<ArgTypes...> value{value_};
        ll.unlock();
        // run callback
        mks::apply(std::forward<T>(fn), value);
    }
};

}

#endif //MKS_LIVE_DATA_H
