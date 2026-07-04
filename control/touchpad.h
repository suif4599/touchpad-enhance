#ifndef CONTROL_TOUCHPAD_H
#define CONTROL_TOUCHPAD_H

#include "config.h"
#include "control/emulator.h"
#include "state.h"
#include <linux/input.h>
#include <string>
#include <utility>
#include <vector>

namespace touchpad {
    bool isTouchpad(int fd);
    int openAndGrab(const std::string& deviceFile);
    int detectAndGrab();
}

class Touchpad {
    int fd;
    Emulator& emulator;
    AtomicState& state;
    bool invertY;
    bool invertX;
    int xMin, xMax, yMin, yMax;
    int leftThreshold, rightThreshold;
    int eventX = 0;
    int eventY = 0;
    int eventTouch = 0; // 0 = none, 1 = touch, -1 = release
    bool eventDone = true;
    bool toProcess = false;
    bool toActivate = false;
    bool inLeftEdge = false;
    int startY = 0;
    bool inCapturedTouch = false;
    bool scrollTriggered = false;
    bool forwardingEnabled = true;   // false in ControlOnly: process but don't forward
    State lastState = State::Active;
    std::vector<struct input_event> frameBuffer;
    std::vector<struct input_event> sequenceBuffer;

    void initialize();
    void resetBufferingState();
    void flushBufferingState();
    void maybeForward(const struct input_event& ev);
    void processFrame();
    void scrollUp();
    void scrollDown();
    void decideFate();
public:
    Touchpad(int fd, Emulator& emulator, AtomicState& state,
             bool invertY = false, bool invertX = false);
    ~Touchpad();
    Touchpad(const Touchpad&) = delete;
    Touchpad& operator=(const Touchpad&) = delete;
    Touchpad(Touchpad&&) = delete;
    Touchpad& operator=(Touchpad&&) = delete;

    void next(); // Blocking
};

#endif
