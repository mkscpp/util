//
// Created by Michal Němec on 21/01/2020.
//

#ifndef MKS_EXCEPTION_H
#define MKS_EXCEPTION_H

#include <functional>

#include <mks/log.h>
#include "demangle.h"


namespace mks {

std::string current_exception_name();

#ifdef MKS_USE_DEBUG_LOG
    void _guard_exception(const std::function<void()>& fn, const char* file, int line);
    #define guard_exception(fn) _guard_exception(fn, __FILE__, __LINE__)
#else
    void guard_exception(const std::function<void()>& fn);
#endif

}

#endif //MKS_EXCEPTION_H
