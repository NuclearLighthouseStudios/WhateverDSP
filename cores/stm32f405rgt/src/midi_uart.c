#include "stm32f4xx.h"

#include "midi_phy.h"

#include "midi.h"

void midi_phy_transmit(unsigned char *data, unsigned short int length)
{
	// Set base addresses
	DMA2_Stream7->M0AR = (uint32_t)(data);
	DMA2_Stream7->PAR = (uint32_t)(&(USART1->DR));
	DMA2_Stream7->NDTR = (uint16_t)(length);

	// Clear DMA transfer complete flag
	SET_BIT(DMA2->HIFCR, DMA_HIFCR_CTCIF7);

	// Enable DMA channel
	SET_BIT(DMA2_Stream7->CR, DMA_SxCR_EN);
}

bool midi_phy_can_transmit(void)
{
	return !READ_BIT(DMA2_Stream7->CR, DMA_SxCR_EN);
}

void USART1_IRQHandler(void)
{
	if (READ_BIT(USART1->SR, USART_SR_RXNE))
	{
		CLEAR_BIT(USART1->SR, USART_SR_RXNE);
		CLEAR_BIT(USART1->SR, USART_SR_ORE);

		midi_receive(USART1->DR);
	}

	if (READ_BIT(USART1->SR, USART_SR_TC))
	{
		CLEAR_BIT(USART1->SR, USART_SR_TC);
		midi_transmit();
	}
}

void midi_phy_init(void)
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