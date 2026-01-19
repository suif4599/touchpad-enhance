#include "control/emulator.h"
#include <fcntl.h>
#include <stdexcept>
#include <linux/uinput.h>
#include <cstring>
#include <unistd.h>

Emulator::Emulator(uint16_t vendor_id, uint16_t product_id, std::string name) {
    struct uinput_setup usetup;
    fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (fd < 0) {
        throw std::runtime_error("Unable to open /dev/uinput");
    }

    ioctl(fd, UI_SET_EVBIT, EV_KEY);
    for (int code = 0; code <= KEY_MAX; ++code) {
        ioctl(fd, UI_SET_KEYBIT, code);
    }

    memset(&usetup, 0, sizeof(usetup));
    usetup.id.bustype = BUS_USB;
    usetup.id.vendor = vendor_id;
    usetup.id.product = product_id;
    if (name.size() >= UINPUT_MAX_NAME_SIZE) {
        throw std::runtime_error("Device name too long");
    }
    strcpy(usetup.name, name.c_str());
    ioctl(fd, UI_DEV_SETUP, &usetup);
    ioctl(fd, UI_DEV_CREATE);
}

Emulator::~Emulator() {
    if (fd >= 0) {
        ioctl(fd, UI_DEV_DESTROY);
        close(fd);
    }
}

void Emulator::trigger(CodeType code) {
    struct input_event evs[4];
    memset(evs, 0, sizeof(evs));

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

    if (write(fd, evs, sizeof(evs)) < 0) {
        throw std::runtime_error("Failed to write events");
    }
}
