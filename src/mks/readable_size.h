//
// Created by michalks on 28.5.18.
//

#ifndef MKS_READABLE_SIZE_H
#define MKS_READABLE_SIZE_H

#include <iomanip>
#include <sstream>

namespace mks {

std::string readable_fs(double size, bool siu = true);

}

#endif // MKS_READABLE_SIZE_H
