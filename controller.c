// controller.c
#include "controller.h"

void controller_init(Controller *ctrl) {
    for (int i = 0; i < 8; i++) ctrl->buttons[i] = false;
    ctrl->cursor = 0;
    ctrl->strobe = false;
}

uint8_t controller_read(Controller *ctrl) {
    // Past the 8 buttons? Always return 1
    if (ctrl->cursor >= 8) return 1;

    // Strobe on? Always return button A, don't advance
    if (ctrl->strobe) {
        return ctrl->buttons[0] ? 1 : 0;
    }

    // Normal read: return current button, advance cursor
    uint8_t value = ctrl->buttons[ctrl->cursor] ? 1 : 0;
    ctrl->cursor++;
    return value;
}

void controller_write(Controller *ctrl, uint8_t value) {
    ctrl->strobe = (value & 0x01) != 0; // extract bit 0 from value and conver to boolean
    if (ctrl->strobe) {
        ctrl->cursor = 0;  // reset both controllers
    }
}
