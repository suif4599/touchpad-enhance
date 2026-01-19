#ifndef CONTROL_TOUCHPAD_H
#define CONTROL_TOUCHPAD_H

#include "config.h"
#include "control/emulator.h"
#include <string>
#include <utility>


class Touchpad {
    int fd;
    bool invertY;
    bool invertX;
    Emulator& emulator;
    int xMin, xMax, yMin, yMax;
    int eventX, eventY, eventTouch; // eventTouch: 0 - no event, 1 - touch, -1 - release
    bool eventDone;
    bool toProcess; // Whether the touchpad is pressed and always in edge zone
    bool toActivate; // Whether the touchpad movement exceeds SCROLL_THRESHOLD
    bool inLeftEdge; // Whether the touch is in left edge zone
    int startY; // If toProcess is true, the Y position when touch started
                // If toActivate is true, the last Y position when scroll action was triggered
    int leftThreshold;
    int rightThreshold;
    bool isTouchpad(int fd);
    void initialize();
    void processEvent();
    void scrollUp();
    void scrollDown();
public:
    Touchpad(std::string deviceFile, Emulator& emulator, bool invertY = false, bool invertX = false);
    Touchpad(Emulator& emulator, bool invertY = false, bool invertX = false);
    ~Touchpad();
    void next(); // Blocking
};

#endif // CONTROL_TOUCHPAD_H