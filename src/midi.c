#include <stdio.h>
#include <stdbool.h>

#include "stm32f4xx.h"

#include "midi.h"

#define MQ_LENGTH 16

static midi_message message_queue[MQ_LENGTH];
static int mq_read_pos = 0;
static int mq_write_pos = 0;

void USART1_IRQHandler(void)
{
	CLEAR_BIT(USART1->SR, USART_SR_RXNE);
	unsigned char data = USART1->DR;

	midi_message *message = &(message_queue[mq_write_pos]);

	if (data & 0x80)
	{
		message->length = 0;
		message->command = (data >> 4) & 0x0f;
		message->channel = data & 0x0f;
	}
	else
	{
		if (message->length < 2)
		{
			message->data.raw[message->length] = data;
			message->length++;
		}

		if (message->length == 2)
		{
			mq_write_pos++;
			mq_write_pos %= MQ_LENGTH;

			if (mq_read_pos == mq_write_pos)
			{
				mq_read_pos++;
				mq_read_pos %= MQ_LENGTH;
			}
		}
	}
}

midi_message *midi_get_message(void)
{
	if (mq_read_pos == mq_write_pos)
	{
		return NULL;
	}

	midi_message *message = &(message_queue[mq_read_pos]);

	mq_read_pos++;
	mq_read_pos %= MQ_LENGTH;

	return message;
}

void midi_init(void)
{
	// Set up pins
	MODIFY_REG(GPIOB->MODER, GPIO_MODER_MODER7_Msk, 0b10 << GPIO_MODER_MODER7_Pos);
	MODIFY_REG(GPIOB->PUPDR, GPIO_PUPDR_PUPD7_Msk, 0b01 << GPIO_PUPDR_PUPD7_Pos);
	MODIFY_REG(GPIOB->AFR[0], GPIO_AFRL_AFSEL7_Msk, 7 << GPIO_AFRL_AFSEL7_Pos);

	// Enable USART1 periphery
	SET_BIT(RCC->APB2ENR, RCC_APB2ENR_USART1EN);

	// Set baud rate (31250 bps)
	MODIFY_REG(USART1->BRR, USART_BRR_DIV_Mantissa_Msk, 168ul << USART_BRR_DIV_Mantissa_Pos);

	SET_BIT(USART1->CR1, USART_CR1_RE);		// Enable receiver
	SET_BIT(USART1->CR1, USART_CR1_RXNEIE); // Enable receive interrupt

	NVIC_EnableIRQ(USART1_IRQn);

	SET_BIT(USART1->CR1, USART_CR1_UE); // Enable UART
}