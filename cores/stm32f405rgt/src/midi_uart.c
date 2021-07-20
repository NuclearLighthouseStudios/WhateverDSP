#include <stdbool.h>
#include <string.h>

#include "stm32f4xx.h"

#include "core.h"
#include "board.h"

#include "midi.h"

#include "midi.h"
#include "midi_uart.h"
#include "conf/midi_uart.h"

#define RX_BUFFER_LENGTH 4
static uint8_t __CCMRAM midi_rx_buffer[RX_BUFFER_LENGTH];
static midi_command __CCMRAM midi_current_command = 0;
static unsigned int __CCMRAM midi_current_channel;
static size_t __CCMRAM midi_data_length;

#define TX_BUFFER_LENGTH 8
static uint8_t midi_tx_buffer[TX_BUFFER_LENGTH];

static int midi_interface_num __CCMRAM = 0;

static void midi_uart_transmit(midi_message *message)
{
	midi_tx_buffer[0] = (message->command << 0x04) | (message->channel & 0x0f);

	memcpy(midi_tx_buffer + 1, &(message->data), message->length);

	// Set base addresses
	MIDI_DMA_STREAM->M0AR = (uint32_t)(midi_tx_buffer);
	MIDI_DMA_STREAM->PAR = (uint32_t)(&(MIDI_UART->DR));
	MIDI_DMA_STREAM->NDTR = (uint16_t)(message->length + 1);

	// Clear DMA transfer complete flag
	SET_BIT(MIDI_DMA->MIDI_DMA_TC_FLAG_REG, MIDI_DMA_TC_FLAG);

	// Enable DMA channel
	SET_BIT(MIDI_DMA_STREAM->CR, DMA_SxCR_EN);
}

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

static void midi_uart_receive(uint8_t data)
{
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

		if (midi_data_length < RX_BUFFER_LENGTH)
		{
			midi_rx_buffer[midi_data_length] = data;
			midi_data_length++;
		}

		size_t data_len = get_message_length(midi_current_command);

		if (midi_data_length == data_len)
		{
			midi_message message;

			message.interface_mask = 0b1 << midi_interface_num;
			message.length = midi_data_length;
			message.command = midi_current_command;
			message.channel = midi_current_channel;
			memcpy(&(message.data), midi_rx_buffer, midi_data_length);

			midi_data_length = 0;

			midi_receive(&message);
		}
	}
}

static bool midi_uart_can_transmit(void)
{
	return !READ_BIT(MIDI_DMA_STREAM->CR, DMA_SxCR_EN);
}

void MIDI_UART_IRQ_HANDLER(void)
{
	if (READ_BIT(MIDI_UART->SR, USART_SR_RXNE))
	{
		CLEAR_BIT(MIDI_UART->SR, USART_SR_RXNE);
		CLEAR_BIT(MIDI_UART->SR, USART_SR_ORE);

		midi_uart_receive(MIDI_UART->DR);
	}

	if (READ_BIT(MIDI_UART->SR, USART_SR_TC))
	{
		CLEAR_BIT(MIDI_UART->SR, USART_SR_TC);
		midi_transmit();
	}
}

void midi_uart_init(void)
{
	midi_uart_setup();

	// Set baud rate (31250 bps)
	MODIFY_REG(MIDI_UART->BRR, USART_BRR_DIV_Mantissa_Msk, 168ul << USART_BRR_DIV_Mantissa_Pos);

	SET_BIT(MIDI_UART->CR1, USART_CR1_RE);		// Enable receiver
	SET_BIT(MIDI_UART->CR1, USART_CR1_RXNEIE); // Enable receive interrupt

	// Set up transmit DMA
	MODIFY_REG(MIDI_DMA_STREAM->CR, DMA_SxCR_PL_Msk, 0b00 << DMA_SxCR_PL_Pos);		 // Lowest Priority
	MODIFY_REG(MIDI_DMA_STREAM->CR, DMA_SxCR_CHSEL_Msk, 0x4 << DMA_SxCR_CHSEL_Pos); // Channel 4 for USART1 TX

	// 8 bit to 8 bit transfers
	MODIFY_REG(MIDI_DMA_STREAM->CR, DMA_SxCR_MSIZE_Msk, 0b00 << DMA_SxCR_MSIZE_Pos);
	MODIFY_REG(MIDI_DMA_STREAM->CR, DMA_SxCR_PSIZE_Msk, 0b00 << DMA_SxCR_PSIZE_Pos);

	// Memory to peripheral with memory increment
	MODIFY_REG(MIDI_DMA_STREAM->CR, DMA_SxCR_DIR_Msk, 0b01 << DMA_SxCR_DIR_Pos);
	SET_BIT(MIDI_DMA_STREAM->CR, DMA_SxCR_MINC);

	SET_BIT(MIDI_UART->CR1, USART_CR1_TE);	  // Enable transmitter
	SET_BIT(MIDI_UART->CR1, USART_CR1_TCIE); // Enable transmit complete interrupt
	SET_BIT(MIDI_UART->CR3, USART_CR3_DMAT); // Enable transmit DMA

	NVIC_EnableIRQ(MIDI_UART_IRQ);

	SET_BIT(MIDI_UART->CR1, USART_CR1_UE); // Enable UART

	midi_interface_num = midi_add_interface(&midi_uart_transmit, &midi_uart_can_transmit);
}