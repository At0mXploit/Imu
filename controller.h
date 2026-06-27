// controller.h
#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    bool buttons[8];    // A, B, Select, Start, Up, Down, Left, Right
    uint8_t cursor;     // which button we're reading next
    bool strobe;        // when true, reads always return button A
} Controller;

void    controller_init(Controller *ctrl);
uint8_t controller_read(Controller *ctrl);
void    controller_write(Controller *ctrl, uint8_t value);

#endif
