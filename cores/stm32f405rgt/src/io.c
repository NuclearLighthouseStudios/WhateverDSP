#include <stdbool.h>
#include <math.h>

#include "stm32f4xx.h"

#include "core.h"
#include "board.h"

#include "pintypes.h"

#include "io.h"
#include "conf/io.h"

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

	for (int i = 0; i < sizeof(io_pins) / sizeof(*io_pins); i++)
	{
		if (io_pins[i].mode == ANALOG_OUT)
		{
			switch (io_pins[i].channel)
			{
				case 1:
					SET_BIT(DAC->CR, DAC_CR_EN1);
					break;

				case 2:
					SET_BIT(DAC->CR, DAC_CR_EN2);
					break;

				default:
			}
		}
	}
}

void io_init(void)
{
	bool has_adc = false;
	bool has_dac = false;

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
		else if (io_pins[i].mode == ANALOG_IN)
		{
			has_adc = true;
			MODIFY_REG(io_pins[i].port->MODER, io_pins[i].pin_mode_mask, 0b11 << io_pins[i].pin_mode_pos);
			MODIFY_REG(io_pins[i].port->PUPDR, io_pins[i].pin_pupdr_mask, 0b00 << io_pins[i].pin_pupdr_pos);
		}
		else if (io_pins[i].mode == ANALOG_OUT)
		{
			has_dac = true;
			MODIFY_REG(io_pins[i].port->MODER, io_pins[i].pin_mode_mask, 0b11 << io_pins[i].pin_mode_pos);
			MODIFY_REG(io_pins[i].port->PUPDR, io_pins[i].pin_pupdr_mask, 0b00 << io_pins[i].pin_pupdr_pos);
		}
	}

	if (has_adc)
		io_init_ADC();

	if (has_dac)
		io_init_DAC();
}