#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "stm32f4xx.h"

#include "midi.h"

#define MQ_LENGTH 16
#define MIDI_DATA_MAXLEN 2

static midi_message __attribute__((section(".ccmram"))) midi_message_queue[MQ_LENGTH];
static int __attribute__((section(".ccmram"))) mq_read_pos = 0;
static int __attribute__((section(".ccmram"))) mq_write_pos = 0;

static midi_command __attribute__((section(".ccmram"))) midi_current_command = 0;
static unsigned int __attribute__((section(".ccmram"))) midi_current_channel;
static unsigned char __attribute__((section(".ccmram"))) midi_data_buffer[MIDI_DATA_MAXLEN];
static unsigned int __attribute__((section(".ccmram"))) midi_data_length;

void USART1_IRQHandler(void)
{
	CLEAR_BIT(USART1->SR, USART_SR_RXNE);
	CLEAR_BIT(USART1->SR, USART_SR_ORE);
	unsigned char data = USART1->DR;

	if (data & 0x80)
	{
		midi_current_command = (data & 0xf0) >> 0x04;
		midi_current_channel = data & 0x0f;
		midi_data_length = 0;
	}
	else
	{
		// Ignore all system common and realtime messages,
		// also ignore data when we don't have a valid command yet
		if ((midi_current_command == 0x0f) || (midi_current_command == 0x00))
			return;

		if (midi_data_length < MIDI_DATA_MAXLEN)
		{
			midi_data_buffer[midi_data_length] = data;
			midi_data_length++;
		}

		int data_len = 0;

		switch (midi_current_command)
		{
		case PROGRAM_CHANGE:
		case AFTERTOUCH:
			data_len = 1;
			break;

		case NOTE_ON:
		case NOTE_OFF:
		case POLY_AFTERTOUCH:
		case CONTROL_CHANGE:
		case PITCHBEND:
			data_len = 2;
			break;
		}

		if (midi_data_length == data_len)
		{
			midi_message *message = &(midi_message_queue[mq_write_pos]);

			message->length = midi_data_length;
			message->command = midi_current_command;
			message->channel = midi_current_channel;
			memcpy(&(message->data), midi_data_buffer, midi_data_length);

			midi_data_length = 0;

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

	midi_message *message = &(midi_message_queue[mq_read_pos]);

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