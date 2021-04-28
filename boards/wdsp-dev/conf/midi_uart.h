#ifndef __CONF_MIDI_UART_H
#define __CONF_MIDI_UART_H

#include "stm32f4xx.h"

static inline void midi_phy_setup(void)
{
	// Set up pins
	MODIFY_REG(GPIOB->MODER, GPIO_MODER_MODER7_Msk, 0b10 << GPIO_MODER_MODER7_Pos);
	MODIFY_REG(GPIOB->PUPDR, GPIO_PUPDR_PUPD7_Msk, 0b01 << GPIO_PUPDR_PUPD7_Pos);
	MODIFY_REG(GPIOB->AFR[0], GPIO_AFRL_AFSEL7_Msk, 7 << GPIO_AFRL_AFSEL7_Pos);

	MODIFY_REG(GPIOB->MODER, GPIO_MODER_MODER6_Msk, 0b10 << GPIO_MODER_MODER6_Pos);
	MODIFY_REG(GPIOB->AFR[0], GPIO_AFRL_AFSEL6_Msk, 7 << GPIO_AFRL_AFSEL6_Pos);

	// Enable USART1 periphery
	SET_BIT(RCC->APB2ENR, RCC_APB2ENR_USART1EN);
}

#define MIDI_UART USART1
#define MIDI_UART_IRQ USART1_IRQn
#define MIDI_UART_IRQ_HANDLER USART1_IRQHandler

#define MIDI_DMA DMA2
#define MIDI_DMA_CHANNEL 4
#define MIDI_DMA_STREAM DMA2_Stream7

#define MIDI_DMA_TC_FLAG DMA_HIFCR_CTCIF7
#define MIDI_DMA_TC_FLAG_REG HIFCR

#endif