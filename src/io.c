#include <stdbool.h>
#include <math.h>

#include "stm32f4xx.h"

#include "io.h"

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

#define MAX_ADC_CHANNELS 16
static unsigned short int adc_buffer[MAX_ADC_CHANNELS];

void io_digital_out(io_pin_idx idx, bool val)
{
	if (io_pins[idx].mode != DIGITAL_OUT)
		return;

	if (val)
		SET_BIT(io_pins[idx].port->ODR, io_pins[idx].pin_dr_val);
	else
		CLEAR_BIT(io_pins[idx].port->ODR, io_pins[idx].pin_dr_val);
}

bool io_digital_in(io_pin_idx idx)
{
	if (io_pins[idx].mode != DIGITAL_IN)
		return false;

	return !(READ_BIT(io_pins[idx].port->IDR, io_pins[idx].pin_dr_val));
}

void io_analog_out(io_pin_idx idx, float val)
{
	if (io_pins[idx].mode != ANALOG_OUT)
		return;

	int ival = floor(fmax(fmin(val, 1.0f), 0.0f) * 4096.0f);

	if (io_pins[idx].channel == 1)
		DAC->DHR12R1 = ival;
	else if (io_pins[idx].channel == 2)
		DAC->DHR12R2 = ival;
}

float io_analog_in(io_pin_idx idx)
{
	if (io_pins[idx].mode != ANALOG_IN)
		return false;

	return (float)adc_buffer[io_pins[idx].seq_idx] / 4096.0f;
}

static void io_init_ADC(void)
{
	int seq_length = 0;

	// Enable ADC1 clock
	SET_BIT(RCC->APB2ENR, RCC_APB2ENR_ADC1EN);

	// 12 bit right aligned data
	MODIFY_REG(ADC1->CR1, ADC_CR1_RES_Msk, 0b00 << ADC_CR1_RES_Pos);
	MODIFY_REG(ADC1->CR2, ADC_CR2_ALIGN_Msk, 0b0 << ADC_CR2_ALIGN_Pos);

	// Sample for really long
	MODIFY_REG(ADC1->SMPR1, ADC_SMPR1_SMP10_Msk, 0b111 << ADC_SMPR1_SMP10_Pos);
	MODIFY_REG(ADC1->SMPR1, ADC_SMPR1_SMP11_Msk, 0b111 << ADC_SMPR1_SMP11_Pos);

	// Set up DMA
	MODIFY_REG(DMA2_Stream0->CR, DMA_SxCR_PL_Msk, 0b11 << DMA_SxCR_PL_Pos);		 // Highest Priority
	MODIFY_REG(DMA2_Stream0->CR, DMA_SxCR_CHSEL_Msk, 0x0 << DMA_SxCR_CHSEL_Pos); // Channel 0 for ADC1

	// 16 bit to 16 bit transfers
	MODIFY_REG(DMA2_Stream0->CR, DMA_SxCR_MSIZE_Msk, 0b01 << DMA_SxCR_MSIZE_Pos);
	MODIFY_REG(DMA2_Stream0->CR, DMA_SxCR_PSIZE_Msk, 0b01 << DMA_SxCR_PSIZE_Pos);

	// Peripheral to memory with memory increment
	MODIFY_REG(DMA2_Stream0->CR, DMA_SxCR_DIR_Msk, 0b00 << DMA_SxCR_DIR_Pos);
	SET_BIT(DMA2_Stream0->CR, DMA_SxCR_MINC);

	// Circular mode
	SET_BIT(DMA2_Stream0->CR, DMA_SxCR_CIRC);

	for (int i = 0; i < sizeof(io_pins) / sizeof(*io_pins); i++)
	{
		if (io_pins[i].mode == ANALOG_IN)
		{
			if (io_pins[i].channel <= 9)
			{
				unsigned int pos = io_pins[i].channel * 3;
				MODIFY_REG(ADC1->SMPR2, 0x7UL << pos, 0b111 << pos);
			}
			else
			{
				unsigned int pos = (io_pins[i].channel - 10) * 3;
				MODIFY_REG(ADC1->SMPR1, 0x7UL << pos, 0b111 << pos);
			}

			io_pins[i].seq_idx = seq_length;

			seq_length++;

			if (seq_length <= 6)
			{
				unsigned int pos = (seq_length - 1) * 5;
				MODIFY_REG(ADC1->SQR3, 0x1FUL << pos, (io_pins[i].channel & 0x1FUL) << pos);
			}
			else if (seq_length <= 12)
			{
				unsigned int pos = (seq_length - 7) * 5;
				MODIFY_REG(ADC1->SQR2, 0x1FUL << pos, (io_pins[i].channel & 0x1FUL) << pos);
			}
			else
			{
				unsigned int pos = (seq_length - 13) * 5;
				MODIFY_REG(ADC1->SQR1, 0x1FUL << pos, (io_pins[i].channel & 0x1FUL) << pos);
			}
		}
	}

	// Set sequence length and enable scan mode
	MODIFY_REG(ADC1->SQR1, ADC_SQR1_L_Msk, (seq_length - 1) << ADC_SQR1_L_Pos);
	SET_BIT(ADC1->CR1, ADC_CR1_SCAN);

	// Set base addresses
	DMA2_Stream0->M0AR = (uint32_t)adc_buffer;
	DMA2_Stream0->PAR = (uint32_t) & (ADC1->DR);
	DMA2_Stream0->NDTR = (uint16_t)seq_length;

	// Enable DMA channel
	SET_BIT(DMA2_Stream0->CR, DMA_SxCR_EN);

	// Enable ADC DMA channel
	SET_BIT(ADC1->CR2, ADC_CR2_DMA);
	SET_BIT(ADC1->CR2, ADC_CR2_DDS);

	// Start ADC conversion
	SET_BIT(ADC1->CR2, ADC_CR2_CONT);
	SET_BIT(ADC1->CR2, ADC_CR2_ADON);
	SET_BIT(ADC1->CR2, ADC_CR2_SWSTART);
}

static void io_init_DAC(void)
{
	// Enable DAC clock
	SET_BIT(RCC->APB1ENR, RCC_APB1ENR_DACEN);

	// Enable DAC channel 1 & 2
	SET_BIT(DAC->CR, DAC_CR_EN1);
	SET_BIT(DAC->CR, DAC_CR_EN2);
}

void io_init(void)
{
	// Enable GPIOA, GPIOB and GPIOC periphery clocks
	SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_GPIOAEN);
	SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_GPIOBEN);
	SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_GPIOCEN);

	for (int i = 0; i < sizeof(io_pins) / sizeof(*io_pins); i++)
	{
		if (io_pins[i].mode == DIGITAL_OUT)
		{
			MODIFY_REG(io_pins[i].port->MODER, io_pins[i].pin_mode_mask, 0b01 << io_pins[i].pin_mode_pos);
			CLEAR_BIT(io_pins[i].port->ODR, io_pins[i].pin_dr_val);
		}
		else if (io_pins[i].mode == DIGITAL_IN)
		{
			MODIFY_REG(io_pins[i].port->MODER, io_pins[i].pin_mode_mask, 0b00 << io_pins[i].pin_mode_pos);
			MODIFY_REG(io_pins[i].port->PUPDR, io_pins[i].pin_pupdr_mask, 0b01 << io_pins[i].pin_pupdr_pos);
		}
		else if ((io_pins[i].mode == ANALOG_IN) || (io_pins[i].mode == ANALOG_OUT))
		{
			MODIFY_REG(io_pins[i].port->MODER, io_pins[i].pin_mode_mask, 0b11 << io_pins[i].pin_mode_pos);
			MODIFY_REG(io_pins[i].port->PUPDR, io_pins[i].pin_pupdr_mask, 0b00 << io_pins[i].pin_pupdr_pos);
		}
	}

	io_init_ADC();
	io_init_DAC();
}