#include <stdbool.h>

#include "stm32f4xx.h"

#include "audio.h"

#include "wdsp.h"

static int32_t i2s_adc_buffer[2][BUFSIZE * 2];
static int32_t i2s_dac_buffer[2][BUFSIZE * 2];

static float __attribute__((section(".ccmram"))) in_buffer[BUFSIZE][2];
static float __attribute__((section(".ccmram"))) out_buffer[BUFSIZE][2];

volatile static bool __attribute__((section(".ccmram"))) adc_ready = false;
volatile static bool __attribute__((section(".ccmram"))) dsp_ready = false;
volatile static bool __attribute__((section(".ccmram"))) dac_empty = false;

static inline void audio_transfer_in(int buf)
{
	for (int i = 0; i < BUFSIZE; i++)
	{
		in_buffer[i][0] = (((i2s_adc_buffer[buf][i << 1] >> 16) & 0xffff) | ((i2s_adc_buffer[buf][i << 1] & 0xffff) << 16)) / (float)INT32_MAX;
		in_buffer[i][1] = (((i2s_adc_buffer[buf][(i << 1) + 1] >> 16) & 0xffff) | ((i2s_adc_buffer[buf][(i << 1) + 1] & 0xffff) << 16)) / (float)INT32_MAX;
	}
}

static inline void audio_transfer_out(int buf)
{
	for (int i = 0; i < BUFSIZE; i++)
	{
		int samp_l = out_buffer[i][0] * (float)INT32_MAX;
		int samp_r = out_buffer[i][1] * (float)INT32_MAX;
		i2s_dac_buffer[buf][i << 1] = ((samp_l >> 16) & 0xffff) | ((samp_l & 0xffff) << 16);
		i2s_dac_buffer[buf][(i << 1) + 1] = ((samp_r >> 16) & 0xffff) | ((samp_r & 0xffff) << 16);
	}
}

void DMA1_Stream0_IRQHandler(void)
{
	SET_BIT(DMA1->LIFCR, DMA_LIFCR_CTCIF0);

	audio_transfer_in(!READ_BIT(DMA1_Stream0->CR, DMA_SxCR_CT));

	dsp_ready = false;
	adc_ready = true;
}

void DMA1_Stream4_IRQHandler(void)
{
	SET_BIT(DMA1->HIFCR, DMA_HIFCR_CTCIF4);

	if (dsp_ready)
	{
		audio_transfer_out(!READ_BIT(DMA1_Stream4->CR, DMA_SxCR_CT));
		dsp_ready = false;
	}
	else
	{
		dac_empty = true;
	}
}

void audio_process(void)
{
	if (adc_ready)
	{
		adc_ready = false;

		wdsp_process(in_buffer, out_buffer, BUFSIZE);

		if (dac_empty)
		{
			audio_transfer_out(!READ_BIT(DMA1_Stream4->CR, DMA_SxCR_CT));
			dac_empty = false;
		}
		else
		{
			dsp_ready = true;
		}
	}
}

static void audio_init_I2S_out(void)
{
	// Set up DMA
	MODIFY_REG(DMA1_Stream4->CR, DMA_SxCR_PL_Msk, 0b11 << DMA_SxCR_PL_Pos);		 // Highest Priority
	MODIFY_REG(DMA1_Stream4->CR, DMA_SxCR_CHSEL_Msk, 0x0 << DMA_SxCR_CHSEL_Pos); // Channel 0 for SPI2TX

	// Enable tranfer FIFO
	MODIFY_REG(DMA1_Stream4->FCR, DMA_SxFCR_DMDIS_Msk, 0b1 << DMA_SxFCR_DMDIS_Pos);
	MODIFY_REG(DMA1_Stream4->FCR, DMA_SxFCR_FTH_Msk, 0b01 << DMA_SxFCR_FTH_Pos);

	// 32 bit to 16 bit transfers
	MODIFY_REG(DMA1_Stream4->CR, DMA_SxCR_MSIZE_Msk, 0b10 << DMA_SxCR_MSIZE_Pos);
	MODIFY_REG(DMA1_Stream4->CR, DMA_SxCR_PSIZE_Msk, 0b01 << DMA_SxCR_PSIZE_Pos);

	// Memory to peripheral with memory increment
	MODIFY_REG(DMA1_Stream4->CR, DMA_SxCR_DIR_Msk, 0b01 << DMA_SxCR_DIR_Pos);
	SET_BIT(DMA1_Stream4->CR, DMA_SxCR_MINC);

	// Circular mode with double buffering
	SET_BIT(DMA1_Stream4->CR, DMA_SxCR_CIRC);
	SET_BIT(DMA1_Stream4->CR, DMA_SxCR_DBM);

	// Set base addresses
	DMA1_Stream4->M0AR = (uint32_t)&i2s_dac_buffer[0];
	DMA1_Stream4->M1AR = (uint32_t)&i2s_dac_buffer[1];
	DMA1_Stream4->PAR = (uint32_t) & (SPI2->DR);
	DMA1_Stream4->NDTR = (uint16_t)BUFSIZE * 4;

	// Enable transfer complete interrupts
	SET_BIT(DMA1_Stream4->CR, DMA_SxCR_TCIE);
	NVIC_EnableIRQ(DMA1_Stream4_IRQn);

	// Enable DMA channel
	SET_BIT(DMA1_Stream4->CR, DMA_SxCR_EN);

	// Enable SPI2 periphery clock
	SET_BIT(RCC->APB1ENR, RCC_APB1ENR_SPI2EN);

	// Set up outpout pins
	MODIFY_REG(GPIOB->MODER, GPIO_MODER_MODER12_Msk, 0b10 << GPIO_MODER_MODER12_Pos);
	MODIFY_REG(GPIOB->AFR[1], GPIO_AFRH_AFSEL12_Msk, 5 << GPIO_AFRH_AFSEL12_Pos);
	MODIFY_REG(GPIOB->MODER, GPIO_MODER_MODER13_Msk, 0b10 << GPIO_MODER_MODER13_Pos);
	MODIFY_REG(GPIOB->AFR[1], GPIO_AFRH_AFSEL13_Msk, 5 << GPIO_AFRH_AFSEL13_Pos);
	MODIFY_REG(GPIOB->MODER, GPIO_MODER_MODER15_Msk, 0b10 << GPIO_MODER_MODER15_Pos);
	MODIFY_REG(GPIOB->AFR[1], GPIO_AFRH_AFSEL15_Msk, 5 << GPIO_AFRH_AFSEL15_Pos);

	MODIFY_REG(GPIOC->MODER, GPIO_MODER_MODER6_Msk, 0b10 << GPIO_MODER_MODER6_Pos);
	MODIFY_REG(GPIOC->AFR[0], GPIO_AFRL_AFSEL6_Msk, 5 << GPIO_AFRL_AFSEL6_Pos);

	// Set I2S mode
	MODIFY_REG(SPI2->I2SCFGR, SPI_I2SCFGR_I2SMOD_Msk, 0b1 << SPI_I2SCFGR_I2SMOD_Pos);  // Enable I2S Mode on SPI peripheral
	MODIFY_REG(SPI2->I2SCFGR, SPI_I2SCFGR_I2SCFG_Msk, 0b10 << SPI_I2SCFGR_I2SCFG_Pos); // Set peripheral to master send mode
	MODIFY_REG(SPI2->I2SCFGR, SPI_I2SCFGR_I2SSTD_Msk, 0b00 << SPI_I2SCFGR_I2SSTD_Pos); // Pillips I2S standard
	MODIFY_REG(SPI2->I2SCFGR, SPI_I2SCFGR_DATLEN_Msk, 0b01 << SPI_I2SCFGR_DATLEN_Pos); // 24 bit data length

	// Set I2S clock
	MODIFY_REG(SPI2->I2SPR, SPI_I2SPR_MCKOE_Msk, 0b1 << SPI_I2SPR_MCKOE_Pos);
	MODIFY_REG(SPI2->I2SPR, SPI_I2SPR_I2SDIV_Msk, 3 << SPI_I2SPR_I2SDIV_Pos);
	MODIFY_REG(SPI2->I2SPR, SPI_I2SPR_ODD_Msk, 0b1 << SPI_I2SPR_ODD_Pos);

	// Enable DMA
	SET_BIT(SPI2->CR2, SPI_CR2_TXDMAEN);

	// Enable I2S preriphery
	SET_BIT(SPI2->I2SCFGR, SPI_I2SCFGR_I2SE);
}

static void audio_init_I2S_in(void)
{
	// Set up DMA
	MODIFY_REG(DMA1_Stream0->CR, DMA_SxCR_PL_Msk, 0b11 << DMA_SxCR_PL_Pos);		 // Highest Priority
	MODIFY_REG(DMA1_Stream0->CR, DMA_SxCR_CHSEL_Msk, 0x0 << DMA_SxCR_CHSEL_Pos); // Channel 0 for SPI3RX

	// Enable tranfer FIFO
	MODIFY_REG(DMA1_Stream0->FCR, DMA_SxFCR_DMDIS_Msk, 0b1 << DMA_SxFCR_DMDIS_Pos);
	MODIFY_REG(DMA1_Stream0->FCR, DMA_SxFCR_FTH_Msk, 0b01 << DMA_SxFCR_FTH_Pos);

	// 16 bit to 32 bit transfers
	MODIFY_REG(DMA1_Stream0->CR, DMA_SxCR_MSIZE_Msk, 0b10 << DMA_SxCR_MSIZE_Pos);
	MODIFY_REG(DMA1_Stream0->CR, DMA_SxCR_PSIZE_Msk, 0b01 << DMA_SxCR_PSIZE_Pos);

	// Peripheral to memory with memory increment
	MODIFY_REG(DMA1_Stream0->CR, DMA_SxCR_DIR_Msk, 0b00 << DMA_SxCR_DIR_Pos);
	SET_BIT(DMA1_Stream0->CR, DMA_SxCR_MINC);

	// Circular mode with double buffering
	SET_BIT(DMA1_Stream0->CR, DMA_SxCR_CIRC);
	SET_BIT(DMA1_Stream0->CR, DMA_SxCR_DBM);

	// Set base addresses
	DMA1_Stream0->M0AR = (uint32_t)&i2s_adc_buffer[0];
	DMA1_Stream0->M1AR = (uint32_t)&i2s_adc_buffer[1];
	DMA1_Stream0->PAR = (uint32_t) & (SPI3->DR);
	DMA1_Stream0->NDTR = (uint16_t)BUFSIZE * 4;

	// Enable transfer complete interrupts
	SET_BIT(DMA1_Stream0->CR, DMA_SxCR_TCIE);
	NVIC_EnableIRQ(DMA1_Stream0_IRQn);

	// Enable DMA channel
	SET_BIT(DMA1_Stream0->CR, DMA_SxCR_EN);

	// Enable SPI3 periphery clock
	SET_BIT(RCC->APB1ENR, RCC_APB1ENR_SPI3EN);

	// Set up input pins
	MODIFY_REG(GPIOC->MODER, GPIO_MODER_MODER10_Msk, 0b10 << GPIO_MODER_MODER10_Pos);
	MODIFY_REG(GPIOC->AFR[1], GPIO_AFRH_AFSEL10_Msk, 6 << GPIO_AFRH_AFSEL10_Pos);
	MODIFY_REG(GPIOC->MODER, GPIO_MODER_MODER12_Msk, 0b10 << GPIO_MODER_MODER12_Pos);
	MODIFY_REG(GPIOC->AFR[1], GPIO_AFRH_AFSEL12_Msk, 6 << GPIO_AFRH_AFSEL12_Pos);
	MODIFY_REG(GPIOA->MODER, GPIO_MODER_MODER15_Msk, 0b10 << GPIO_MODER_MODER15_Pos);
	MODIFY_REG(GPIOA->AFR[1], GPIO_AFRH_AFSEL15_Msk, 6 << GPIO_AFRH_AFSEL15_Pos);

	MODIFY_REG(GPIOC->MODER, GPIO_MODER_MODER7_Msk, 0b10 << GPIO_MODER_MODER7_Pos);
	MODIFY_REG(GPIOC->AFR[0], GPIO_AFRL_AFSEL7_Msk, 6 << GPIO_AFRL_AFSEL7_Pos);

	// Set I2S mode
	MODIFY_REG(SPI3->I2SCFGR, SPI_I2SCFGR_I2SMOD_Msk, 0b1 << SPI_I2SCFGR_I2SMOD_Pos);  // Enable I2S Mode on SPI peripheral
	MODIFY_REG(SPI3->I2SCFGR, SPI_I2SCFGR_I2SCFG_Msk, 0b11 << SPI_I2SCFGR_I2SCFG_Pos); // Set peripheral to master receive mode
	MODIFY_REG(SPI3->I2SCFGR, SPI_I2SCFGR_I2SSTD_Msk, 0b00 << SPI_I2SCFGR_I2SSTD_Pos); // Pillips I2S standard
	MODIFY_REG(SPI3->I2SCFGR, SPI_I2SCFGR_DATLEN_Msk, 0b01 << SPI_I2SCFGR_DATLEN_Pos); // 24 bit data length

	// Set I2S clock
	MODIFY_REG(SPI3->I2SPR, SPI_I2SPR_MCKOE_Msk, 0b1 << SPI_I2SPR_MCKOE_Pos);
	MODIFY_REG(SPI3->I2SPR, SPI_I2SPR_I2SDIV_Msk, 3 << SPI_I2SPR_I2SDIV_Pos);
	MODIFY_REG(SPI3->I2SPR, SPI_I2SPR_ODD_Msk, 0b1 << SPI_I2SPR_ODD_Pos);

	// Enable DMA
	SET_BIT(SPI3->CR2, SPI_CR2_RXDMAEN);

	// Enable I2S preriphery
	SET_BIT(SPI3->I2SCFGR, SPI_I2SCFGR_I2SE);
}

void audio_init(void)
{
	// Set up I2S clock
	MODIFY_REG(RCC->PLLI2SCFGR, RCC_PLLI2SCFGR_PLLI2SR_Msk, 3 << RCC_PLLI2SCFGR_PLLI2SR_Pos);
	MODIFY_REG(RCC->PLLI2SCFGR, RCC_PLLI2SCFGR_PLLI2SN_Msk, 129 << RCC_PLLI2SCFGR_PLLI2SN_Pos);
	SET_BIT(RCC->CR, RCC_CR_PLLI2SON);
	while (!READ_BIT(RCC->CR, RCC_CR_PLLI2SRDY))
		__NOP();

	audio_init_I2S_in();
	audio_init_I2S_out();
}