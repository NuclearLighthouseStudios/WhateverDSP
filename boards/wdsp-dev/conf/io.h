#ifndef __CONF_IO_H
#define __CONF_IO_H

#include "board.h"
#include "pintypes.h"

static io_pin io_pins[] = {
	[POT_1] = AIN_PIN_DEF(GPIOC, 0, 10),
	[POT_2] = AIN_PIN_DEF(GPIOC, 2, 12),
	[POT_3] = AIN_PIN_DEF(GPIOC, 1, 11),
	[POT_4] = AIN_PIN_DEF(GPIOC, 3, 13),

	[AIN_1] = AIN_PIN_DEF(GPIOA, 3, 3),
	[AIN_2] = AIN_PIN_DEF(GPIOA, 2, 2),
	[AIN_3] = AIN_PIN_DEF(GPIOA, 1, 1),
	[AIN_4] = AIN_PIN_DEF(GPIOA, 0, 0),

	[AOUT_1] = AOUT_PIN_DEF(GPIOA, 5, 2),
	[AOUT_2] = AOUT_PIN_DEF(GPIOA, 4, 1),

	[LED_1] = DOUT_PIN_DEF(GPIOA, 6),

	[BUTTON_1] = DIN_PIN_DEF(GPIOB, 0),
	[BUTTON_2] = DIN_PIN_DEF(GPIOB, 1),

	[DOUT_1] = DOUT_PIN_DEF(GPIOA, 7),
	[DOUT_2] = DOUT_PIN_DEF(GPIOC, 4),
	[DOUT_3] = DOUT_PIN_DEF(GPIOC, 5),

	[DIN_1] = DIN_PIN_DEF(GPIOB, 2),
	[DIN_2] = DIN_PIN_DEF(GPIOB, 10),
	[DIN_3] = DIN_PIN_DEF(GPIOB, 11),

	[MUTE] = DOUT_PIN_DEF(GPIOB, 14),
};

#endif