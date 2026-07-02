#ifndef CONTROL_CAPABILITIES_H
#define CONTROL_CAPABILITIES_H

#include <linux/input.h>
#include <vector>

struct Capabilities {
    // Supported codes per event type. Each event type in `evTypes` has its
    // codes listed in the corresponding vector below.
    std::vector<int> evTypes;
    std::vector<int> keyCodes;
    std::vector<int> relCodes;
    std::vector<int> absCodes;
    std::vector<int> mscCodes;

    // ABS info (min/max/fuzz/flat/resolution) for each abs code in absCodes,
    // matched by index.
    std::vector<struct input_absinfo> absInfos;
};

// Probe the given input device file descriptor for its full event capability
// set. Used to mirror the source touchpad onto a uinput device so forwarded
// events look identical to the original.
Capabilities queryCapabilities(int fd);

#endif
