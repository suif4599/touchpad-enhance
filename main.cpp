#include "control/touchpad.h"
#include "control/emulator.h"

int main() {
    Emulator emulator(0x1234, 0x5678, "Touchpad Enhancer");
#ifdef TOUCHPAD_DEVICE
    Touchpad tp(TOUCHPAD_DEVICE, emulator, INVERTX==1, INVERTY==1);
#else
    Touchpad tp(emulator, INVERTX==1, INVERTY==1);
#endif
    while (true) {
        tp.next();
    }
    return 0;
}
