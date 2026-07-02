#ifndef CONTROL_EMULATOR_H
#define CONTROL_EMULATOR_H

#include <cstdint>
#include <linux/input.h>
#include <string>

#include "capabilities.h"

using CodeType = decltype(input_event::code);

class Emulator {
    int forwardFd;
    int triggerFd;
public:
    Emulator(uint16_t vendor_id, uint16_t product_id, std::string name,
             const Capabilities& caps);
    ~Emulator();
    Emulator(const Emulator&) = delete;
    Emulator& operator=(const Emulator&) = delete;
    Emulator(Emulator&&) = delete;
    Emulator& operator=(Emulator&&) = delete;
    void trigger(CodeType code);
    void forward(const struct input_event& ev);
};

#endif
