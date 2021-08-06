#ifndef __IO_H
#define __IO_H

#include <stdbool.h>

#include "board.h"

#ifdef LIB_BUILD
extern void io_init(void);
#endif

extern void io_digital_out(io_pin_idx idx, bool val);
extern bool io_digital_in(io_pin_idx idx);

extern float io_analog_in(io_pin_idx idx);
extern void io_analog_out(io_pin_idx idx, float val);

#endif