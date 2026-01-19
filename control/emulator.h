#ifndef CONTROL_EMULATOR_H
#define CONTROL_EMULATOR_H

#include <cstdint>
#include <vector>
#include <linux/input.h>
#include <string>

using CodeType = decltype(input_event::code);

class Emulator {
    int fd;
public:
    Emulator(uint16_t vendor_id, uint16_t product_id, std::string name);
    ~Emulator();
    Emulator(const Emulator&) = delete;
    Emulator& operator=(const Emulator&) = delete;
    Emulator(Emulator&&) = delete;
    Emulator& operator=(Emulator&&) = delete;
    void trigger(CodeType code);
};

#endif // CONTROL_EMULATOR_H