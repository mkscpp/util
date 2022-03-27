//
// Created by Michal Němec on 27/10/2020.
//

#include "string_compare.h"
#include <vector>
#include <chrono>
#include <iostream>
#include "dur2str.h"
//#define USE_SIMD
#include "levenshtein-sse.hpp"

double
mks::string_similarity(const std::string& str1, const std::string& str2)
{
    if(str1.empty() && str2.empty()) {
        return 1.0;
    }
    if(str1.empty() && !str2.empty()) {
        return 0.0;
    }
    if(!str1.empty() && str2.empty()) {
        return 0.0;
    }
    if(str1 == str2) {
        return 1.0;
    }

    auto l1 = str1.size();
    auto l2 = str2.size();
    auto dist = (double)levenshteinSSE::levenshtein(str1, str2);
    return (1.0 - dist/std::max(l1, l2));
}