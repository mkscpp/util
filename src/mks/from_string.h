//
// Created by Michal Němec on 09/08/2020.
//

#ifndef MKS_FROM_STRING_H
#define MKS_FROM_STRING_H

#include <sstream>
#include <string>

template <typename T>
int from_string(const std::string &rem, T def = T()) {
    std::istringstream iss{rem};
    T ret;
    if(!(iss >> ret)) {
        return def;
    }
    return ret;
}

#endif // MKS_FROM_STRING_H
