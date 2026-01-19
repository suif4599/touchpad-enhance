#include "control/touchpad.h"
#include <linux/input.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <stdexcept>
#include <cstring>

#define SUPPORT_BIT(bit, array) (array[(bit) / 8] & (1 << ((bit) % 8)))

bool Touchpad::isTouchpad(int fd) {
    // Support EV_SYN, EV_KEY, EV_ABS
    unsigned char evBits[(EV_MAX + 7) / 8];
    memset(evBits, 0, sizeof(evBits));
    if (ioctl(fd, EVIOCGBIT(0, sizeof(evBits)), evBits) < 0) {
        return false;
    }
    if (!SUPPORT_BIT(EV_SYN, evBits) ||
        !SUPPORT_BIT(EV_KEY, evBits) ||
        !SUPPORT_BIT(EV_ABS, evBits)) {
        return false;
    }

    // Support BTN_TOUCH
    unsigned char keyBits[(KEY_MAX + 7) / 8];
    memset(keyBits, 0, sizeof(keyBits));
    if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keyBits)), keyBits) < 0) {
        return false;
    }
    if (!SUPPORT_BIT(BTN_TOUCH, keyBits)) {
        return false;
    }

    // Support ABS_X and ABS_Y
    unsigned char absBits[(ABS_MAX + 7) / 8];
    memset(absBits, 0, sizeof(absBits));
    if (ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(absBits)), absBits) < 0) {
        return false;
    }
    if (!SUPPORT_BIT(ABS_X, absBits) ||
        !SUPPORT_BIT(ABS_Y, absBits)) {
        return false;
    }

    return true;
}

void Touchpad::initialize() {
    // ABS_X and ABS_Y range
    struct input_absinfo absInfo;
    if (ioctl(fd, EVIOCGABS(ABS_X), &absInfo) == 0) {
        xMin = absInfo.minimum;
        xMax = absInfo.maximum;
    } else {
        throw std::runtime_error("Failed to get ABS_X info");
    }
    if (ioctl(fd, EVIOCGABS(ABS_Y), &absInfo) == 0) {
        yMin = absInfo.minimum;
        yMax = absInfo.maximum;
    } else {
        throw std::runtime_error("Failed to get ABS_Y info");
    }

    // Initialize state variables
    eventX = 0;
    eventY = 0;
    eventTouch = 0;
    eventDone = false;
    toProcess = false;
    toActivate = false;
    inLeftEdge = false;
    startY = 0;
    leftThreshold = xMin + static_cast<int>((xMax - xMin) * EDGE_WIDTH);
    rightThreshold = xMax - static_cast<int>((xMax - xMin) * EDGE_WIDTH);
}

void Touchpad::processEvent() {
    switch (eventTouch) {
        case 1: {
            // Touch event
            if (eventX <= leftThreshold) {
                toProcess = true;
                inLeftEdge = true;
                startY = eventY;
            } else if (eventX >= rightThreshold) {
                toProcess = true;
                inLeftEdge = false;
                startY = eventY;
            }
            break;
        }
        case -1: {
            // Release event
            toProcess = false;
            toActivate = false;
            break;
        }
        case 0: {
            // Movement event
            if (!toProcess) {
                break;
            }
            if (inLeftEdge && eventX > leftThreshold) {
                toProcess = false;
                toActivate = false;
                break;
            }
            if (!inLeftEdge && eventX < rightThreshold) {
                toProcess = false;
                toActivate = false;
                break;
            }
            int deltaY = eventY - startY;
            float deltaYNorm = static_cast<float>(deltaY) / (yMax - yMin);
            if (!toActivate) {
                if (deltaYNorm > SCROLL_THRESHOLD) {
                    toActivate = true;
                    startY = eventY;
                    scrollDown();
                } else if (deltaYNorm < -SCROLL_THRESHOLD) {
                    toActivate = true;
                    startY = eventY;
                    scrollUp();
                }
            } else {
                do {
                    if (deltaYNorm > SCROLL_STEP) {
                        scrollDown();
                        startY += static_cast<int>(SCROLL_STEP * (yMax - yMin));
                        deltaYNorm = static_cast<float>(eventY - startY) / (yMax - yMin);
                    } else if (deltaYNorm < -SCROLL_STEP) {
                        scrollUp();
                        startY -= static_cast<int>(SCROLL_STEP * (yMax - yMin));
                        deltaYNorm = static_cast<float>(eventY - startY) / (yMax - yMin);
                    } else {
                        break;
                    }
                } while (true);
            }
        }
    }
    eventDone = true;
}

void Touchpad::scrollUp() {
    if (inLeftEdge && !invertX) {
        // Brightness
        if (invertY) {
            emulator.trigger(KEY_BRIGHTNESSDOWN);
        } else {
            emulator.trigger(KEY_BRIGHTNESSUP);
        }
    } else {
        // Volume
        if (invertY) {
            emulator.trigger(KEY_VOLUMEDOWN);
        } else {
            emulator.trigger(KEY_VOLUMEUP);
        }
    }
}

void Touchpad::scrollDown() {
    if (inLeftEdge && !invertX) {
        // Brightness
        if (invertY) {
            emulator.trigger(KEY_BRIGHTNESSUP);
        } else {
            emulator.trigger(KEY_BRIGHTNESSDOWN);
        }
    } else {
        // Volume
        if (invertY) {
            emulator.trigger(KEY_VOLUMEUP);
        } else {
            emulator.trigger(KEY_VOLUMEDOWN);
        }
    }
}

Touchpad::Touchpad(std::string deviceFile, Emulator& emulator, bool invertY, bool invertX)
: emulator(emulator), invertY(invertY), invertX(invertX) {
    fd = open(deviceFile.c_str(), O_RDWR);
    if (fd < 0) {
        throw std::runtime_error("Unable to open touchpad device file");
    }
    if (!isTouchpad(fd)) {
        close(fd);
        throw std::runtime_error("The specified device is not a touchpad");
    }
    try{
        initialize();
    } catch (...) {
        close(fd);
        throw;
    }
}

Touchpad::Touchpad(Emulator& emulator, bool invertY, bool invertX)
: emulator(emulator), invertY(invertY), invertX(invertX) {
    struct dirent **namelist;
    int n = scandir("/dev/input", &namelist, nullptr, alphasort);
    if (n < 0) {
        throw std::runtime_error("Unable to scan /dev/input directory");
    }
    fd = -1;
    for (int i = 0; i < n; ++i) {
        // Starts with "event"
        if (strncmp(namelist[i]->d_name, "event", 5) != 0) {
            free(namelist[i]);
            continue;
        }
        std::string devicePath = std::string("/dev/input/") + namelist[i]->d_name;
        int testFd = open(devicePath.c_str(), O_RDWR);
        if (testFd >= 0) {
            if (isTouchpad(testFd)) {
                if (fd >= 0) {
                    close(testFd);
                    for (int j = 0; j < n; ++j) {
                        free(namelist[j]);
                    }
                    free(namelist);
                    close(fd);
                    throw std::runtime_error("Multiple touchpad devices found");
                }
                fd = testFd;
                break;
            } else {
                close(testFd);
            }
        }
        free(namelist[i]);
    }
    free(namelist);
    if (fd < 0) {
        throw std::runtime_error("No touchpad device found");
    }
    try {
        initialize();
    } catch (...) {
        close(fd);
        throw;
    }
}

Touchpad::~Touchpad() {
    if (fd >= 0) {
        close(fd);
    }
}

void Touchpad::next() {
    struct input_event ev;
    ssize_t n = read(fd, &ev, sizeof(ev));
    if (n == sizeof(ev)) {
        if (ev.type == EV_KEY && ev.code == BTN_TOUCH) {
            eventTouch = (ev.value != 0 ? 1 : -1);
            eventDone = false;
        } else if (ev.type == EV_ABS) {
            if (ev.code == ABS_X) {
                eventX = ev.value;
            } else if (ev.code == ABS_Y) {
                eventY = ev.value;
            }
            if (eventDone) {
                eventTouch = 0;
                eventDone = false;
            }
        } else if (ev.type == EV_SYN && ev.code == SYN_REPORT && !eventDone) {
            processEvent();
        }
    } else {
        if (errno != EINTR) {
            throw std::runtime_error("Failed to read input event");
        }
    }
}
