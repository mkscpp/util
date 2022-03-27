//
// Created by michalks on 3/17/18.
//

#ifndef MKS_GATE_H
#define MKS_GATE_H

#include <condition_variable>
#include <mutex>

namespace mks {

struct gate {
    bool gate_open = false;
    std::condition_variable cv;
    std::mutex m;

    void close();
    void open();
    void wait_to_open();
};

} // namespace mks
#endif // MKS_GATE_H
