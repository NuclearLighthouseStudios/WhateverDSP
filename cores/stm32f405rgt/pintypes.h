#ifndef __PINTYPES_H
#define __PINTYPES_H

#define DOUT_PIN_DEF(_port, _num)                        \
	{                                                    \
		.mode = DIGITAL_OUT,                             \
		.port = (_port),                                 \
		.pin_mode_mask = (GPIO_MODER_MODER##_num##_Msk), \
		.pin_mode_pos = (GPIO_MODER_MODER##_num##_Pos),  \
		.pin_dr_val = (GPIO_ODR_OD##_num),               \
	}

#define DIN_PIN_DEF(_port, _num)                         \
	{                                                    \
		.mode = DIGITAL_IN,                              \
		.port = (_port),                                 \
		.pin_mode_mask = (GPIO_MODER_MODER##_num##_Msk), \
		.pin_mode_pos = (GPIO_MODER_MODER##_num##_Pos),  \
		.pin_dr_val = (GPIO_IDR_ID##_num),               \
		.pin_pupdr_mask = (GPIO_PUPDR_PUPD##_num##_Msk), \
		.pin_pupdr_pos = (GPIO_PUPDR_PUPD##_num##_Pos),  \
	}

#define AIN_PIN_DEF(_port, _num, _channel)               \
	{                                                    \
		.mode = ANALOG_IN,                               \
		.port = (_port),                                 \
		.pin_mode_mask = (GPIO_MODER_MODER##_num##_Msk), \
		.pin_mode_pos = (GPIO_MODER_MODER##_num##_Pos),  \
		.pin_pupdr_mask = (GPIO_PUPDR_PUPD##_num##_Msk), \
		.pin_pupdr_pos = (GPIO_PUPDR_PUPD##_num##_Pos),  \
		.channel = (_channel)                            \
	}

#define AOUT_PIN_DEF(_port, _num, _channel)              \
	{                                                    \
		.mode = ANALOG_OUT,                              \
		.port = (_port),                                 \
		.pin_mode_mask = (GPIO_MODER_MODER##_num##_Msk), \
		.pin_mode_pos = (GPIO_MODER_MODER##_num##_Pos),  \
		.pin_pupdr_mask = (GPIO_PUPDR_PUPD##_num##_Msk), \
		.pin_pupdr_pos = (GPIO_PUPDR_PUPD##_num##_Pos),  \
		.channel = (_channel)                            \
	}

typedef enum
{
	NONE = 0,
	ANALOG_IN,
	ANALOG_OUT,
	DIGITAL_IN,
	DIGITAL_OUT
} io_mode;

typedef struct
{
	io_mode mode;
	GPIO_TypeDef *port;
	unsigned long int pin_mode_mask;
	unsigned long int pin_mode_pos;
	unsigned long int pin_pupdr_mask;
	unsigned long int pin_pupdr_pos;
	unsigned long int pin_dr_val;
	int channel;
	int seq_idx;
} io_pin;

#endif