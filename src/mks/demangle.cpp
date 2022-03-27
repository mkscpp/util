//
// Created by Michal Němec on 21/01/2020.
//

#include "demangle.h"

#ifdef _WIN32

// https://stackoverflow.com/questions/13777681/demangling-in-msvc
#include <windows.h>
#include <dbghelp.h>
#include <stdio.h>

//#pragma comment(lib, "dbghelp.lib")

//extern char *__unDName(char*, const char*, int, void*, void*, int);

std::string mks::cxx_demangle(const char *to_demangle) {
//    const char *decorated_name = to_demangle;
//    char undecorated_name[1024];
//    __unDName(undecorated_name, decorated_name+1, 1024, malloc, free, 0x2800);
    return to_demangle;
}


#else
#include <cxxabi.h>

std::string mks::cxx_demangle(const char *to_demangle) {
    int status = 0;
    std::size_t len;
    char *buff = __cxxabiv1::__cxa_demangle(to_demangle, nullptr, &len, &status);
    if(buff != nullptr) {
        // len contains zero-terminator, pass len-1
        std::string demangled = buff;
        std::free(buff);
        return demangled;
    }
    return to_demangle;
}

#endif