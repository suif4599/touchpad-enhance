#include "emulator.h"
#include <fcntl.h>
#include <stdexcept>
#include <linux/uinput.h>
#include <cstring>
#include <unistd.h>

namespace {

constexpr int kTriggerKeys[] = {
    KEY_VOLUMEUP, KEY_VOLUMEDOWN,
    KEY_BRIGHTNESSUP, KEY_BRIGHTNESSDOWN,
};

int createForwardDevice(uint16_t vendor, uint16_t product,
                        const std::string& name, const Capabilities& caps) {
    int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (fd < 0) throw std::runtime_error("Unable to open /dev/uinput (forward)");

    for (int type : caps.evTypes) {
        if (ioctl(fd, UI_SET_EVBIT, type) < 0) {
            close(fd);
            throw std::runtime_error("UI_SET_EVBIT failed on forward device");
        }
    }
    for (int code : caps.keyCodes) {
        ioctl(fd, UI_SET_KEYBIT, code);
    }
    for (int code : caps.relCodes) {
        ioctl(fd, UI_SET_RELBIT, code);
    }
    for (int code : caps.mscCodes) {
        ioctl(fd, UI_SET_MSCBIT, code);
    }
    for (size_t i = 0; i < caps.absCodes.size(); ++i) {
        struct uinput_abs_setup absSetup;
        std::memset(&absSetup, 0, sizeof(absSetup));
        absSetup.code = static_cast<__u16>(caps.absCodes[i]);
        absSetup.absinfo = caps.absInfos[i];
        ioctl(fd, UI_ABS_SETUP, &absSetup);
    }

    struct uinput_setup usetup;
    std::memset(&usetup, 0, sizeof(usetup));
    usetup.id.bustype = BUS_USB;
    usetup.id.vendor = vendor;
    usetup.id.product = product;
    if (name.size() >= UINPUT_MAX_NAME_SIZE) {
        close(fd);
        throw std::runtime_error("Device name too long");
    }
    std::strcpy(usetup.name, name.c_str());
    if (ioctl(fd, UI_DEV_SETUP, &usetup) < 0) {
        close(fd);
        throw std::runtime_error("UI_DEV_SETUP failed on forward device");
    }
    if (ioctl(fd, UI_DEV_CREATE) < 0) {
        close(fd);
        throw std::runtime_error("UI_DEV_CREATE failed on forward device");
    }
    return fd;
}

int createTriggerDevice(uint16_t vendor, uint16_t product,
                        const std::string& name) {
    int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (fd < 0) throw std::runtime_error("Unable to open /dev/uinput (trigger)");

    if (ioctl(fd, UI_SET_EVBIT, EV_KEY) < 0) {
        close(fd);
        throw std::runtime_error("UI_SET_EVBIT failed on trigger device");
    }
    if (ioctl(fd, UI_SET_EVBIT, EV_SYN) < 0) {
        close(fd);
        throw std::runtime_error("UI_SET_EVBIT(EV_SYN) failed on trigger device");
    }
    for (int k : kTriggerKeys) {
        if (ioctl(fd, UI_SET_KEYBIT, k) < 0) {
            close(fd);
            throw std::runtime_error("UI_SET_KEYBIT failed on trigger device");
        }
    }

    struct uinput_setup usetup;
    std::memset(&usetup, 0, sizeof(usetup));
    usetup.id.bustype = BUS_USB;
    usetup.id.vendor = vendor;
    usetup.id.product = product + 1; // distinguish from forward device
    std::string triggerName = name + " Hotkeys";
    if (triggerName.size() >= UINPUT_MAX_NAME_SIZE) triggerName = name;
    std::strcpy(usetup.name, triggerName.c_str());
    if (ioctl(fd, UI_DEV_SETUP, &usetup) < 0) {
        close(fd);
        throw std::runtime_error("UI_DEV_SETUP failed on trigger device");
    }
    if (ioctl(fd, UI_DEV_CREATE) < 0) {
        close(fd);
        throw std::runtime_error("UI_DEV_CREATE failed on trigger device");
    }
    return fd;
}

} // namespace

Emulator::Emulator(uint16_t vendor_id, uint16_t product_id, std::string name,
                   const Capabilities& caps)
: forwardFd(-1), triggerFd(-1) {
    forwardFd = createForwardDevice(vendor_id, product_id, name, caps);
    try {
        triggerFd = createTriggerDevice(vendor_id, product_id, name);
    } catch (...) {
        ioctl(forwardFd, UI_DEV_DESTROY);
        close(forwardFd);
        forwardFd = -1;
        throw;
    }
}

Emulator::~Emulator() {
    if (forwardFd >= 0) {
        ioctl(forwardFd, UI_DEV_DESTROY);
        close(forwardFd);
    }
    if (triggerFd >= 0) {
        ioctl(triggerFd, UI_DEV_DESTROY);
        close(triggerFd);
    }
}

void Emulator::trigger(CodeType code) {
    struct input_event evs[4];
    std::memset(evs, 0, sizeof(evs));

    evs[0].type = EV_KEY;
    evs[0].code = code;
    evs[0].value = 1;

    evs[1].type = EV_SYN;
    evs[1].code = SYN_REPORT;
    evs[1].value = 0;

    evs[2].type = EV_KEY;
    evs[2].code = code;
    evs[2].value = 0;

    evs[3].type = EV_SYN;
    evs[3].code = SYN_REPORT;
    evs[3].value = 0;

    if (write(triggerFd, evs, sizeof(evs)) < 0) {
        throw std::runtime_error("Failed to write trigger events");
    }
}

void Emulator::forward(const struct input_event& ev) {
    if (write(forwardFd, &ev, sizeof(ev)) < 0) {
        throw std::runtime_error("Failed to forward event");
    }
}
