#include "ipc.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <poll.h>
#include <cstring>
#include <stdexcept>
#include <string>

namespace ipc {

namespace {

void buildAbstractAddr(struct sockaddr_un& addr, socklen_t& len) {
    std::memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    addr.sun_path[0] = '\0';
    constexpr size_t nameLen = std::char_traits<char>::length(SOCKET_NAME);
    if (nameLen >= sizeof(addr.sun_path) - 1) {
        throw std::runtime_error("Abstract socket name too long");
    }
    std::memcpy(&addr.sun_path[1], SOCKET_NAME, nameLen);
    len = offsetof(struct sockaddr_un, sun_path) + 1 + nameLen;
}

bool readLine(int fd, std::string& out) {
    out.clear();
    char c;
    while (true) {
        ssize_t n = read(fd, &c, 1);
        if (n == 0) return false;
        if (n < 0) {
            if (errno == EINTR) continue;
            throw std::runtime_error("IPC read failed");
        }
        if (c == '\n') return true;
        if (c == '\r') continue;
        out.push_back(c);
        if (out.size() > 64) {
            throw std::runtime_error("IPC line too long");
        }
    }
}

void writeAll(int fd, const std::string& s) {
    size_t off = 0;
    while (off < s.size()) {
        ssize_t n = write(fd, s.data() + off, s.size() - off);
        if (n < 0) {
            if (errno == EINTR) continue;
            throw std::runtime_error("IPC write failed");
        }
        off += static_cast<size_t>(n);
    }
}

std::string handleCommand(const std::string& line, AtomicState& state) {
    auto setOrErr = [&](State s) {
        state.store(s, std::memory_order_relaxed);
        return std::string("OK ") + stateToString(s);
    };
    if (line == "get") {
        return std::string("OK ") + stateToString(state.load(std::memory_order_relaxed));
    }
    if (line == "active")           return setOrErr(State::Active);
    if (line == "inactive")         return setOrErr(State::Inactive);
    if (line == "disable-touchpad") return setOrErr(State::DisableTouchpad);
    if (line == "control-only")     return setOrErr(State::ControlOnly);
    return std::string("ERR unknown command: ") + line;
}

} // namespace

std::string request(const std::string& command) {
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) throw std::runtime_error("socket() failed");
    struct sockaddr_un addr;
    socklen_t len;
    buildAbstractAddr(addr, len);
    if (connect(sock, reinterpret_cast<struct sockaddr*>(&addr), len) < 0) {
        close(sock);
        throw std::runtime_error("Failed to connect to touchpad-enhance daemon");
    }
    std::string req = command + "\n";
    writeAll(sock, req);
    if (shutdown(sock, SHUT_WR) < 0) {
        close(sock);
        throw std::runtime_error("shutdown() failed");
    }
    std::string reply;
    if (!readLine(sock, reply)) {
        close(sock);
        throw std::runtime_error("Daemon closed connection without reply");
    }
    close(sock);
    return reply;
}

void serve(AtomicState& state, int shutdownFd) {
    int listenFd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (listenFd < 0) throw std::runtime_error("socket() failed");

    struct sockaddr_un addr;
    socklen_t len;
    buildAbstractAddr(addr, len);

    if (bind(listenFd, reinterpret_cast<struct sockaddr*>(&addr), len) < 0) {
        close(listenFd);
        throw std::runtime_error("Failed to bind abstract socket");
    }
    if (listen(listenFd, 4) < 0) {
        close(listenFd);
        throw std::runtime_error("listen() failed");
    }

    while (true) {
        struct pollfd fds[2];
        fds[0].fd = listenFd;
        fds[0].events = POLLIN;
        fds[1].fd = shutdownFd;
        fds[1].events = POLLIN;
        if (poll(fds, 2, -1) < 0) {
            if (errno == EINTR) continue;
            break;
        }
        if (fds[1].revents & POLLIN) break; // shutdown signal

        int client = accept(listenFd, nullptr, nullptr);
        if (client < 0) {
            if (errno == EINTR) continue;
            continue;
        }
        std::string line;
        try {
            if (readLine(client, line)) {
                std::string reply = handleCommand(line, state) + "\n";
                writeAll(client, reply);
            }
        } catch (...) {}
        close(client);
    }
    close(listenFd);
}

} // namespace ipc
