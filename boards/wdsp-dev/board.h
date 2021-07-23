#ifndef __BOARD_H
#define __BOARD_H

#ifndef BLOCK_SIZE
#define BLOCK_SIZE 16
#endif

#ifndef SAMPLE_RATE
#define SAMPLE_RATE 48000
#endif

typedef enum io_pin_idx
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

#endif