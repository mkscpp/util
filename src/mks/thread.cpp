//
// Created by Michal Němec on 08/09/2020.
//
#include "thread.h"
using namespace mks;

std::mutex mks::thread::state_mtx_;
mks::thread_state_listener mks::thread::state_listener_;

#define thread_case(x) case x : return #x
const char*
mks::thread_state_string(thread_state state)
{
    switch(state) {
        thread_case(thread_state::none);
        thread_case(thread_state::created);
        thread_case(thread_state::starting);
        thread_case(thread_state::started);
        thread_case(thread_state::finished);
        thread_case(thread_state::joining);
        thread_case(thread_state::stopped);
        thread_case(thread_state::fail_create);
        thread_case(thread_state::destructed);
    }
    return "thread_state::undefined";
}