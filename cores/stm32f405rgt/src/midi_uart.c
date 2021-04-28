#include "stm32f4xx.h"

#include "core.h"
#include "board.h"

#include "midi.h"

#include "midi_phy.h"
#include "conf/midi_uart.h"

void midi_phy_transmit(unsigned char *data, unsigned short int length)
{
	// Set base addresses
	MIDI_DMA_STREAM->M0AR = (uint32_t)(data);
	MIDI_DMA_STREAM->PAR = (uint32_t)(&(MIDI_UART->DR));
	MIDI_DMA_STREAM->NDTR = (uint16_t)(length);

	// Clear DMA transfer complete flag
	SET_BIT(MIDI_DMA->MIDI_DMA_TC_FLAG_REG, MIDI_DMA_TC_FLAG);

	// Enable DMA channel
	SET_BIT(MIDI_DMA_STREAM->CR, DMA_SxCR_EN);
}

bool midi_phy_can_transmit(void)
{
	return !READ_BIT(MIDI_DMA_STREAM->CR, DMA_SxCR_EN);
}

void MIDI_UART_IRQ_HANDLER(void)
{
	if (READ_BIT(MIDI_UART->SR, USART_SR_RXNE))
	{
		CLEAR_BIT(MIDI_UART->SR, USART_SR_RXNE);
		CLEAR_BIT(MIDI_UART->SR, USART_SR_ORE);

		midi_receive(MIDI_UART->DR);
	}

	if (READ_BIT(MIDI_UART->SR, USART_SR_TC))
	{
		CLEAR_BIT(MIDI_UART->SR, USART_SR_TC);
		midi_transmit();
	}
}

void midi_phy_init(void)
{
	midi_phy_setup();

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
}