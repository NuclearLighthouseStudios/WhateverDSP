#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "stm32f4xx.h"

#include "midi.h"

#define RXQ_LENGTH 16
#define MIDI_DATA_MAXLEN 2

static midi_message __attribute__((section(".ccmram"))) midi_rx_queue[RXQ_LENGTH];
static int __attribute__((section(".ccmram"))) rxq_read_pos = 0;
static int __attribute__((section(".ccmram"))) rxq_write_pos = 0;

static midi_command __attribute__((section(".ccmram"))) midi_current_command = 0;
static unsigned int __attribute__((section(".ccmram"))) midi_current_channel;
static unsigned char __attribute__((section(".ccmram"))) midi_data_buffer[MIDI_DATA_MAXLEN];
static unsigned int __attribute__((section(".ccmram"))) midi_data_length;

#define TXQ_LENGTH 16
#define TX_BUFFER_LENGTH 8

static midi_message __attribute__((section(".ccmram"))) midi_tx_queue[TXQ_LENGTH];
static int __attribute__((section(".ccmram"))) txq_read_pos = 0;
static int __attribute__((section(".ccmram"))) txq_write_pos = 0;

static unsigned char midi_tx_buffer[TX_BUFFER_LENGTH];

static inline int get_message_length(midi_command command)
{
	switch (command)
	{
	case PROGRAM_CHANGE:
	case AFTERTOUCH:
	default:
		return 1;

	case NOTE_ON:
	case NOTE_OFF:
	case POLY_AFTERTOUCH:
	case CONTROL_CHANGE:
	case PITCHBEND:
		return 2;
	}
}

static void midi_rec(void)
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

		int data_len = get_message_length(midi_current_command);

		if (midi_data_length == data_len)
		{
			midi_message *message = &(midi_rx_queue[rxq_write_pos]);

			message->length = midi_data_length;
			message->command = midi_current_command;
			message->channel = midi_current_channel;
			memcpy(&(message->data), midi_data_buffer, midi_data_length);

			midi_data_length = 0;

			rxq_write_pos++;
			rxq_write_pos %= RXQ_LENGTH;

			if (rxq_read_pos == rxq_write_pos)
			{
				rxq_read_pos++;
				rxq_read_pos %= RXQ_LENGTH;
			}
		}
	}
}

static void midi_tx(void)
{
	CLEAR_BIT(USART1->SR, USART_SR_TC);

	// Skip if there is already an active DMA transfer
	if (READ_BIT(DMA2_Stream7->CR, DMA_SxCR_EN))
		return;

	if (txq_read_pos == txq_write_pos)
		return;

	midi_message *message = &(midi_tx_queue[txq_read_pos]);

	txq_read_pos++;
	txq_read_pos %= TXQ_LENGTH;

	midi_tx_buffer[0] = (message->command << 0x04) | (message->channel & 0x0f);

	memcpy(&midi_tx_buffer[1], &(message->data), message->length);

	// Set base addresses
	DMA2_Stream7->M0AR = (uint32_t)(&midi_tx_buffer[0]);
	DMA2_Stream7->PAR = (uint32_t)(&(USART1->DR));
	DMA2_Stream7->NDTR = (uint16_t)(message->length + 1);

	// Clear DMA transfer complete flag
	SET_BIT(DMA2->HIFCR, DMA_HIFCR_CTCIF7);

	// Enable DMA channel
	SET_BIT(DMA2_Stream7->CR, DMA_SxCR_EN);
}

void USART1_IRQHandler(void)
{
	if (READ_BIT(USART1->SR, USART_SR_RXNE))
		midi_rec();

	if (READ_BIT(USART1->SR, USART_SR_TC))
		midi_tx();
}

midi_message *midi_get_message(void)
{
	if (rxq_read_pos == rxq_write_pos)
		return NULL;

	midi_message *message = &(midi_rx_queue[rxq_read_pos]);

	rxq_read_pos++;
	rxq_read_pos %= RXQ_LENGTH;

	return message;
}

void midi_send_message(midi_message tx_message)
{
	midi_message *message = &(midi_tx_queue[txq_write_pos]);

	message->length = tx_message.length;
	message->command = tx_message.command;
	message->channel = tx_message.channel;
	memcpy(&(message->data), &(tx_message.data), tx_message.length);

	txq_write_pos++;
	txq_write_pos %= TXQ_LENGTH;

	if (txq_read_pos == txq_write_pos)
	{
		txq_read_pos++;
		txq_read_pos %= TXQ_LENGTH;
	}

	midi_tx();
}

void midi_init(void)
{
	// Set up pins
	MODIFY_REG(GPIOB->MODER, GPIO_MODER_MODER7_Msk, 0b10 << GPIO_MODER_MODER7_Pos);
	MODIFY_REG(GPIOB->PUPDR, GPIO_PUPDR_PUPD7_Msk, 0b01 << GPIO_PUPDR_PUPD7_Pos);
	MODIFY_REG(GPIOB->AFR[0], GPIO_AFRL_AFSEL7_Msk, 7 << GPIO_AFRL_AFSEL7_Pos);

	MODIFY_REG(GPIOB->MODER, GPIO_MODER_MODER6_Msk, 0b10 << GPIO_MODER_MODER6_Pos);
	MODIFY_REG(GPIOB->AFR[0], GPIO_AFRL_AFSEL6_Msk, 7 << GPIO_AFRL_AFSEL6_Pos);

	// Enable USART1 periphery
	SET_BIT(RCC->APB2ENR, RCC_APB2ENR_USART1EN);

	// Set baud rate (31250 bps)
	MODIFY_REG(USART1->BRR, USART_BRR_DIV_Mantissa_Msk, 168ul << USART_BRR_DIV_Mantissa_Pos);

	SET_BIT(USART1->CR1, USART_CR1_RE);		// Enable receiver
	SET_BIT(USART1->CR1, USART_CR1_RXNEIE); // Enable receive interrupt

	// Set up transmit DMA
	MODIFY_REG(DMA2_Stream7->CR, DMA_SxCR_PL_Msk, 0b00 << DMA_SxCR_PL_Pos);		 // Lowest Priority
	MODIFY_REG(DMA2_Stream7->CR, DMA_SxCR_CHSEL_Msk, 0x4 << DMA_SxCR_CHSEL_Pos); // Channel 4 for USART1 TX

	// 8 bit to 8 bit transfers
	MODIFY_REG(DMA2_Stream7->CR, DMA_SxCR_MSIZE_Msk, 0b00 << DMA_SxCR_MSIZE_Pos);
	MODIFY_REG(DMA2_Stream7->CR, DMA_SxCR_PSIZE_Msk, 0b00 << DMA_SxCR_PSIZE_Pos);

	// Memory to peripheral with memory increment
	MODIFY_REG(DMA2_Stream7->CR, DMA_SxCR_DIR_Msk, 0b01 << DMA_SxCR_DIR_Pos);
	SET_BIT(DMA2_Stream7->CR, DMA_SxCR_MINC);

	SET_BIT(USART1->CR1, USART_CR1_TE);	  // Enable transmitter
	SET_BIT(USART1->CR1, USART_CR1_TCIE); // Enable transmit complete interrupt
	SET_BIT(USART1->CR3, USART_CR3_DMAT); // Enable transmit DMA

	NVIC_EnableIRQ(USART1_IRQn);

	SET_BIT(USART1->CR1, USART_CR1_UE); // Enable UART
}