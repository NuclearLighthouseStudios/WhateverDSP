#ifndef __IO_H
#define __IO_H

#include <stdbool.h>

typedef enum
{
	POT_1,
	POT_2,
	POT_3,
	POT_4,
	AIN_1,
	AIN_2,
	AIN_3,
	AIN_4,
	AOUT_1,
	AOUT_2,
	LED_1,
	BUTTON_1,
	BUTTON_2,
	DOUT_1,
	DOUT_2,
	DOUT_3,
	DIN_1,
	DIN_2,
	DIN_3,
	MUTE,
} io_pin_idx;

extern void io_digital_out(io_pin_idx idx, bool val);
extern bool io_digital_in(io_pin_idx idx);

extern float io_analog_in(io_pin_idx idx);

extern void io_init(void);

#endif