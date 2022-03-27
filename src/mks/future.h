//
// Created by Michal Němec on 09/09/2020.
//

#ifndef MKS_FUTURE_H
#define MKS_FUTURE_H

#include "broadcaster.h"
#include "live_data.h"
#include "gate.h"

namespace mks {

template<typename ...Args>
class future {

    bool empty = true;

    struct value_priv {
        gate gt_;
        live_data<void(Args...)> ld_;
    };
    std::unique_ptr<value_priv> priv_;

public:
    future() = default;

    future(future &&) = default;
    future & operator=(future &&) = default;

    future(const future &) = delete;
    future & operator=(const future &) = delete;

    explicit future(broadcaster<void(Args...)>& broadcaster) {
        priv_ = std::make_unique<value_priv>();
        empty = false;
        auto* ptr = priv_.get();
        priv_->ld_.on_data([ptr](Args... args){
            ptr->gt_.open();
        });
        broadcaster.bind(priv_->ld_);
    }

    template< std::size_t I = 0>
    typename std::tuple_element<I, std::tuple<Args...>>::type
    get() noexcept {
        assert(!empty);
        assert(priv_);

        priv_->gt_.wait_to_open();
        return priv_->ld_.template value<I>();
    }

    void apply(const std::function<Args...>& cb) noexcept {
        assert(!empty);
        assert(priv_);

        priv_->gt_.wait_to_open();
        priv_->ld_.apply(cb);
    }

    void apply(std::function<Args...>&& cb) noexcept {
        assert(!empty);
        assert(priv_);

        priv_->gt_.wait_to_open();
        priv_->ld_.apply(std::forward<std::function<Args...>>(cb));
    }

    std::tuple<Args...> values() {
        assert(!empty);
        assert(priv_);

        priv_->gt_.wait_to_open();
        return priv_->ld_.values();
    }
};

template<typename ...Args>
class future_generator {
    broadcaster<void(Args...)> brd;
    bool bind_called_ = false;
    bool notify_called_ = false;

public:
    void notify(Args&&... args) {
        // sanity check
        assert(!notify_called_);
        notify_called_ = true;
        brd(std::forward<Args>(args)...);
    }

    future<Args...> bind() {
        // sanity check
        assert(!bind_called_);
        bind_called_ = true;
        return future<Args...>(brd);
    }
};

}


#endif // MKS_FUTURE_H
