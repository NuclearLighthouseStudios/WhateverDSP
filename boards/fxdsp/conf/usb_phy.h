#ifndef __CONF_USB_PHY_H
#define __CONF_USB_PHY_H

#include "stm32f4xx.h"

#define USB_PHY_RX_FIFO_SIZE 560

static inline void usb_phy_setup(void)
{
	// VBUS Sense Pin
	MODIFY_REG(GPIOA->MODER, GPIO_MODER_MODER9_Msk, 0b00 << GPIO_MODER_MODER9_Pos);
	MODIFY_REG(GPIOA->PUPDR, GPIO_PUPDR_PUPD9_Msk, 0b00 << GPIO_PUPDR_PUPD9_Pos);

	// D+ and D-
	MODIFY_REG(GPIOA->MODER, GPIO_MODER_MODER11_Msk, 0b10 << GPIO_MODER_MODER11_Pos);
	MODIFY_REG(GPIOA->MODER, GPIO_MODER_MODER12_Msk, 0b10 << GPIO_MODER_MODER12_Pos);
	MODIFY_REG(GPIOA->PUPDR, GPIO_PUPDR_PUPD11_Msk, 0b00 << GPIO_PUPDR_PUPD11_Pos);
	MODIFY_REG(GPIOA->PUPDR, GPIO_PUPDR_PUPD12_Msk, 0b00 << GPIO_PUPDR_PUPD12_Pos);
	MODIFY_REG(GPIOA->OSPEEDR, GPIO_OSPEEDR_OSPEED11_Msk, 0b11 << GPIO_OSPEEDR_OSPEED11_Pos);
	MODIFY_REG(GPIOA->OSPEEDR, GPIO_OSPEEDR_OSPEED12_Msk, 0b11 << GPIO_OSPEEDR_OSPEED12_Pos);
	MODIFY_REG(GPIOA->AFR[1], GPIO_AFRH_AFSEL11_Msk, 10 << GPIO_AFRH_AFSEL11_Pos);
	MODIFY_REG(GPIOA->AFR[1], GPIO_AFRH_AFSEL12_Msk, 10 << GPIO_AFRH_AFSEL12_Pos);

	// Enable USB FS clock
	SET_BIT(RCC->AHB2ENR, RCC_AHB2ENR_OTGFSEN);
}

#endif