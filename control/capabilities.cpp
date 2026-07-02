#include "capabilities.h"
#include <sys/ioctl.h>
#include <cstring>

namespace {

void collectBits(const unsigned char* bits, int max, std::vector<int>& out) {
    for (int bit = 0; bit < max; ++bit) {
        if (bits[bit / 8] & (1 << (bit % 8))) {
            out.push_back(bit);
        }
    }
}

// Query a single event type's code bits. Returns true if the query succeeded.
bool queryTypeBits(int fd, int type, int max, std::vector<unsigned char>& buf) {
    buf.assign((max + 7) / 8, 0);
    long n = ioctl(fd, EVIOCGBIT(type, buf.size()), buf.data());
    if (n < 0) return false;
    return true;
}

}

Capabilities queryCapabilities(int fd) {
    Capabilities caps;

    unsigned char evBits[(EV_MAX + 7) / 8];
    std::memset(evBits, 0, sizeof(evBits));
    if (ioctl(fd, EVIOCGBIT(0, sizeof(evBits)), evBits) < 0) {
        return caps;
    }

    for (int type = 0; type < EV_MAX; ++type) {
        if (!(evBits[type / 8] & (1 << (type % 8)))) continue;
        // Skip output-only types — uinput doesn't accept them and they're
        // irrelevant for forwarding.
        if (type == EV_LED || type == EV_SND || type == EV_PWR ||
            type == EV_FF_STATUS) {
            continue;
        }
        caps.evTypes.push_back(type);
    }

    std::vector<unsigned char> buf;

    if (queryTypeBits(fd, EV_KEY, KEY_MAX, buf)) {
        collectBits(buf.data(), KEY_MAX, caps.keyCodes);
    }
    if (queryTypeBits(fd, EV_REL, REL_MAX, buf)) {
        collectBits(buf.data(), REL_MAX, caps.relCodes);
    }
    if (queryTypeBits(fd, EV_ABS, ABS_MAX, buf)) {
        collectBits(buf.data(), ABS_MAX, caps.absCodes);
    }
    if (queryTypeBits(fd, EV_MSC, MSC_MAX, buf)) {
        collectBits(buf.data(), MSC_MAX, caps.mscCodes);
    }

    for (int code : caps.absCodes) {
        struct input_absinfo info;
        if (ioctl(fd, EVIOCGABS(code), &info) == 0) {
            caps.absInfos.push_back(info);
        } else {
            struct input_absinfo zero{};
            caps.absInfos.push_back(zero);
        }
    }

    return caps;
}
