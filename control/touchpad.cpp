#include "touchpad.h"
#include <linux/input.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <stdexcept>
#include <cstring>

#define SUPPORT_BIT(bit, array) (array[(bit) / 8] & (1 << ((bit) % 8)))

namespace touchpad {

bool isTouchpad(int fd) {
    unsigned char evBits[(EV_MAX + 7) / 8];
    std::memset(evBits, 0, sizeof(evBits));
    if (ioctl(fd, EVIOCGBIT(0, sizeof(evBits)), evBits) < 0) return false;
    if (!SUPPORT_BIT(EV_SYN, evBits) ||
        !SUPPORT_BIT(EV_KEY, evBits) ||
        !SUPPORT_BIT(EV_ABS, evBits)) {
        return false;
    }

    unsigned char keyBits[(KEY_MAX + 7) / 8];
    std::memset(keyBits, 0, sizeof(keyBits));
    if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keyBits)), keyBits) < 0) return false;
    if (!SUPPORT_BIT(BTN_TOUCH, keyBits)) return false;

    unsigned char absBits[(ABS_MAX + 7) / 8];
    std::memset(absBits, 0, sizeof(absBits));
    if (ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(absBits)), absBits) < 0) return false;
    if (!SUPPORT_BIT(ABS_X, absBits) || !SUPPORT_BIT(ABS_Y, absBits)) return false;

    return true;
}

static int openGrabAndVerify(const std::string& deviceFile) {
    int fd = open(deviceFile.c_str(), O_RDWR);
    if (fd < 0) throw std::runtime_error("Unable to open touchpad device file");
    if (!isTouchpad(fd)) {
        close(fd);
        throw std::runtime_error("The specified device is not a touchpad");
    }
    if (ioctl(fd, EVIOCGRAB, 1) < 0) {
        close(fd);
        throw std::runtime_error("Failed to grab touchpad device");
    }
    return fd;
}

int openAndGrab(const std::string& deviceFile) {
    return openGrabAndVerify(deviceFile);
}

int detectAndGrab() {
    struct dirent **namelist;
    int n = scandir("/dev/input", &namelist, nullptr, alphasort);
    if (n < 0) throw std::runtime_error("Unable to scan /dev/input directory");

    int fd = -1;
    int found = 0;
    for (int i = 0; i < n; ++i) {
        if (std::strncmp(namelist[i]->d_name, "event", 5) != 0) {
            free(namelist[i]);
            continue;
        }
        std::string devicePath = std::string("/dev/input/") + namelist[i]->d_name;
        int testFd = open(devicePath.c_str(), O_RDWR);
        if (testFd >= 0) {
            if (isTouchpad(testFd)) {
                ++found;
                if (fd < 0) {
                    fd = testFd;
                } else {
                    close(testFd);
                }
            } else {
                close(testFd);
            }
        }
        free(namelist[i]);
    }
    free(namelist);

    if (found == 0) throw std::runtime_error("No touchpad device found");
    if (found > 1) {
        close(fd);
        throw std::runtime_error("Multiple touchpad devices found");
    }
    if (ioctl(fd, EVIOCGRAB, 1) < 0) {
        close(fd);
        throw std::runtime_error("Failed to grab touchpad device");
    }
    return fd;
}

} // namespace touchpad

void Touchpad::initialize() {
    struct input_absinfo absInfo;
    if (ioctl(fd, EVIOCGABS(ABS_X), &absInfo) != 0) {
        throw std::runtime_error("Failed to get ABS_X info");
    }
    xMin = absInfo.minimum;
    xMax = absInfo.maximum;
    if (ioctl(fd, EVIOCGABS(ABS_Y), &absInfo) != 0) {
        throw std::runtime_error("Failed to get ABS_Y info");
    }
    yMin = absInfo.minimum;
    yMax = absInfo.maximum;

    leftThreshold = xMin + static_cast<int>((xMax - xMin) * EDGE_WIDTH);
    rightThreshold = xMax - static_cast<int>((xMax - xMin) * EDGE_WIDTH);
}

void Touchpad::resetBufferingState() {
    eventX = 0;
    eventY = 0;
    eventTouch = 0;
    eventDone = true;
    toProcess = false;
    toActivate = false;
    inLeftEdge = false;
    startY = 0;
    inCapturedTouch = false;
    scrollTriggered = false;
    frameBuffer.clear();
    sequenceBuffer.clear();
}

void Touchpad::flushBufferingState() {
    if (inCapturedTouch && !scrollTriggered) {
        for (const auto& e : sequenceBuffer) emulator.forward(e);
    }
    for (const auto& e : frameBuffer) emulator.forward(e);
    resetBufferingState();
}

Touchpad::Touchpad(int fd, Emulator& emulator, AtomicState& state,
                   bool invertY, bool invertX)
: fd(fd), emulator(emulator), state(state),
  invertY(invertY), invertX(invertX),
  xMin(0), xMax(0), yMin(0), yMax(0),
  leftThreshold(0), rightThreshold(0) {
    if (fd < 0) throw std::runtime_error("Invalid fd");
    try {
        initialize();
    } catch (...) {
        ioctl(fd, EVIOCGRAB, 0);
        close(fd);
        throw;
    }
}

Touchpad::~Touchpad() {
    if (fd >= 0) {
        ioctl(fd, EVIOCGRAB, 0);
        close(fd);
    }
}

void Touchpad::scrollUp() {
    if (inLeftEdge && !invertX) {
        emulator.trigger(invertY ? KEY_BRIGHTNESSDOWN : KEY_BRIGHTNESSUP);
    } else {
        emulator.trigger(invertY ? KEY_VOLUMEDOWN : KEY_VOLUMEUP);
    }
}

void Touchpad::scrollDown() {
    if (inLeftEdge && !invertX) {
        emulator.trigger(invertY ? KEY_BRIGHTNESSUP : KEY_BRIGHTNESSDOWN);
    } else {
        emulator.trigger(invertY ? KEY_VOLUMEUP : KEY_VOLUMEDOWN);
    }
}

void Touchpad::processFrame() {
    switch (eventTouch) {
        case 1: { // touch start
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
        case -1: { // release
            toProcess = false;
            toActivate = false;
            break;
        }
        case 0: { // movement
            if (!toProcess) break;
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
                    scrollTriggered = true;
                } else if (deltaYNorm < -SCROLL_THRESHOLD) {
                    toActivate = true;
                    startY = eventY;
                    scrollUp();
                    scrollTriggered = true;
                }
            } else {
                do {
                    if (deltaYNorm > SCROLL_STEP) {
                        scrollDown();
                        startY += static_cast<int>(SCROLL_STEP * (yMax - yMin));
                        deltaYNorm = static_cast<float>(eventY - startY) / (yMax - yMin);
                        scrollTriggered = true;
                    } else if (deltaYNorm < -SCROLL_STEP) {
                        scrollUp();
                        startY -= static_cast<int>(SCROLL_STEP * (yMax - yMin));
                        deltaYNorm = static_cast<float>(eventY - startY) / (yMax - yMin);
                        scrollTriggered = true;
                    } else {
                        break;
                    }
                } while (true);
            }
            break;
        }
    }
}

void Touchpad::decideFate() {
    if (inCapturedTouch) {
        for (const auto& e : frameBuffer) sequenceBuffer.push_back(e);

        bool release = (eventTouch == -1);
        bool edgeAbandoned = !toProcess;

        bool sequenceEnds = release || (edgeAbandoned && !scrollTriggered);

        if (sequenceEnds) {
            if (!scrollTriggered) {
                for (const auto& e : sequenceBuffer) emulator.forward(e);
            }
            sequenceBuffer.clear();
            inCapturedTouch = false;
            scrollTriggered = false;
        }
        return;
    }

    bool touchStartInEdge = (eventTouch == 1) && toProcess;
    if (touchStartInEdge) {
        sequenceBuffer = frameBuffer;
        inCapturedTouch = true;
        scrollTriggered = false;
    } else {
        for (const auto& e : frameBuffer) emulator.forward(e);
    }
}

void Touchpad::next() {
    struct input_event ev;
    ssize_t n = read(fd, &ev, sizeof(ev));
    if (n != sizeof(ev)) {
        if (errno == EINTR) return;
        throw std::runtime_error("Failed to read input event");
    }

    State s = state.load(std::memory_order_relaxed);
    if (s != lastState) {
        if (s == State::Inactive) {
            flushBufferingState();
        } else {
            resetBufferingState();
        }
        lastState = s;
    }

    if (s == State::DisableTouchpad) {
        // Drop everything.
        return;
    }
    if (s == State::Inactive) {
        // Forward raw, no processing.
        emulator.forward(ev);
        return;
    }

    frameBuffer.push_back(ev);

    if (ev.type == EV_KEY && ev.code == BTN_TOUCH) {
        eventTouch = (ev.value != 0 ? 1 : -1);
        eventDone = false;
    } else if (ev.type == EV_ABS) {
        if (ev.code == ABS_X) eventX = ev.value;
        else if (ev.code == ABS_Y) eventY = ev.value;
        if (eventDone) {
            eventTouch = 0;
            eventDone = false;
        }
    } else if (ev.type == EV_SYN && ev.code == SYN_REPORT) {
        if (!eventDone) {
            processFrame();
        }
        decideFate();
        frameBuffer.clear();
        eventDone = true;
        eventTouch = 0;
    }
}
