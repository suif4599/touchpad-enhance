#include "control/ipc.h"
#include "control/state.h"

#include <iostream>
#include <string>

namespace {

void usage(const char* prog) {
    std::cerr << "Usage: " << prog << " <get|active|inactive|disable-touchpad|control-only>\n";
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 2) {
        usage(argv[0]);
        return 2;
    }
    std::string cmd = argv[1];
    if (cmd == "-h" || cmd == "--help" || cmd == "help") {
        usage(argv[0]);
        return 0;
    }
    if (cmd != "get" && cmd != "active" && cmd != "inactive" &&
        cmd != "disable-touchpad" && cmd != "control-only") {
        std::cerr << "error: unknown command '" << cmd << "'\n";
        usage(argv[0]);
        return 2;
    }

    try {
        std::string reply = ipc::request(cmd);
        std::cout << reply << "\n";
        return (reply.rfind("OK", 0) == 0) ? 0 : 1;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
}
