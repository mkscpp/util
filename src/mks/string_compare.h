//
// Created by Michal Němec on 27/10/2020.
//

#ifndef MKS_STRING_COMPARE_H
#define MKS_STRING_COMPARE_H

#include <string>

namespace mks {

size_t levenshtein_distance(const char* s, size_t n, const char* t, size_t m);
double string_similarity(const std::string& str1, const std::string& str2);

}


#endif // MKS_STRING_COMPARE_H
