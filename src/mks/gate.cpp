//
// Created by michalks on 5/10/19.
//

#include "gate.h"

using namespace mks;

void gate::close() {
    std::unique_lock<std::mutex> lock(m);
    gate_open = false;
}

void gate::open() {
    std::unique_lock<std::mutex> lock(m);
    gate_open = true;
    cv.notify_all();
}

void gate::wait_to_open() {
    std::unique_lock<std::mutex> lock(m);
    cv.wait(lock, [this] { return gate_open; });
}