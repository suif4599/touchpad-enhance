#include "control/capabilities.h"
#include "control/emulator.h"
#include "control/ipc.h"
#include "control/state.h"
#include "control/touchpad.h"

#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <thread>
#include <iostream>
#include <stdexcept>
#include <string>

int main(int argc, char** argv) {
#ifdef TOUCHPAD_DEVICE
    std::string deviceFile = TOUCHPAD_DEVICE;
#else
    std::string deviceFile;
#endif

    State initial = State::Active;
    if (argc >= 2) {
        auto parsed = stateFromString(argv[1]);
        if (!parsed) {
            std::cerr << "Invalid initial state: " << argv[1] << "\n";
            std::cerr << "Valid: active | inactive | disable-touchpad\n";
            return 2;
        }
        initial = *parsed;
    }

    AtomicState state{initial};

    int fd = deviceFile.empty()
        ? touchpad::detectAndGrab()
        : touchpad::openAndGrab(deviceFile);

    Capabilities caps = queryCapabilities(fd);
    Emulator emulator(0x1234, 0x5678, "Touchpad Enhancer", caps);
    Touchpad tp(fd, emulator, state, INVERTY == 1, INVERTX == 1);

    // Self-pipe so the IPC thread can be woken for shutdown.
    int pipefd[2] = {-1, -1};
    if (pipe(pipefd) < 0) {
        std::cerr << "warning: pipe() failed; IPC thread will not be joinable\n";
        pipefd[0] = pipefd[1] = -1;
    }

    std::thread ipcThread([&state, shutdownFd = pipefd[0]]() {
        if (shutdownFd < 0) return;
        try {
            ipc::serve(state, shutdownFd);
        } catch (const std::exception& e) {
            std::cerr << "ipc server error: " << e.what() << "\n";
        }
    });

    try {
        while (true) {
            tp.next();
        }
    } catch (const std::exception& e) {
        std::cerr << "touchpad error: " << e.what() << "\n";
    }

    if (pipefd[1] >= 0) {
        char c = 0;
        ssize_t n;
        do { n = write(pipefd[1], &c, 1); } while (n < 0 && errno == EINTR);
        (void)n;
    }
    if (ipcThread.joinable()) ipcThread.join();

    return 1;
}
