#ifndef __CONF_IO_H
#define __CONF_IO_H

#include "board.h"
#include "pintypes.h"

static io_pin io_pins[] = {
	[POT_1] = AIN_PIN_DEF(GPIOA, 0, 0),
	[POT_2] = AIN_PIN_DEF(GPIOA, 1, 1),
	[POT_3] = AIN_PIN_DEF(GPIOA, 2, 2),
	[POT_4] = AIN_PIN_DEF(GPIOA, 3, 3),

	[LED_1] = DOUT_PIN_DEF(GPIOA, 6),
	[LED_2] = DOUT_PIN_DEF(GPIOA, 7),
	[LED_3] = DOUT_PIN_DEF(GPIOB, 0),
	[LED_4] = DOUT_PIN_DEF(GPIOB, 1),

	[BUTTON_1] = DIN_PIN_DEF(GPIOA, 4),
	[BUTTON_2] = DIN_PIN_DEF(GPIOC, 9),
	[BUTTON_3] = DIN_PIN_DEF(GPIOC, 8),
	[BUTTON_4] = DIN_PIN_DEF(GPIOC, 7),

	[BYPASS_R] = DOUT_PIN_DEF(GPIOC, 4),
	[BYPASS_L] = DOUT_PIN_DEF(GPIOC, 5),
};

#endif