#ifndef CONTROL_IPC_H
#define CONTROL_IPC_H

#include "state.h"
#include <string>

namespace ipc {
    constexpr const char* SOCKET_NAME = "touchpad-enhance-ipc";

    std::string request(const std::string& command);
    void serve(AtomicState& state, int shutdownFd);
}

#endif
