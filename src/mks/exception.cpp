//
// Created by Michal Němec on 21/01/2020.
//

#include "exception.h"

#ifdef _WIN32

std::string
mks::current_exception_name() {
    // find windows alternative
    return "";
}

#else
#include <cxxabi.h>
std::string
mks::current_exception_name() {
    auto* ex_type = __cxxabiv1::__cxa_current_exception_type();
    std::string ex_name;
    if(ex_type != nullptr) {
        ex_name = mks::cxx_demangle(ex_type->name());
    } else {
        ex_name = "<unknown>";
    }
    return ex_name;
}
#endif

#ifdef MKS_USE_DEBUG_LOG
void
mks::_guard_exception(const std::function<void()>& fn, const char* file, int line) {
    try {
        fn();
    }  catch(std::exception& ex) {
        MKS_LOG_E("[{}:{}] std::exception({}): {}", MKS_FILENAME(file), line, mks::current_exception_name(), ex.what());
    } catch(...) {
        MKS_LOG_E("[{}:{}] unknown exception type: {}", MKS_FILENAME(file), line, mks::current_exception_name());
    }
}
#else
void
mks::guard_exception(const std::function<void()>& fn) {
    try {
        fn();
    }  catch(std::exception& ex) {
        MKS_LOG_E("std::exception({}): {}", mks::current_exception_name(), ex.what());
    } catch(...) {
        MKS_LOG_E("unknown exception type: {}", mks::current_exception_name());
    }
}
#endif
