#include <stdbool.h>

#include "stm32f4xx.h"

#include "core.h"
#include "board.h"

#include "system.h"
#include "twait.h"
#include "audio.h"
#include "audio_analog.h"

#include "conf/audio_i2s.h"

// This file is temporary and will be merged into the general I2S audio
// implementation once I write some preprocessor code that can handle
// all required configurations.

#define ANALOG_CHANNELS 2

#if NUM_CHANNELS < ANALOG_CHANNELS
#error Number of audio channels needs to be at least 2!
#endif

static int32_t i2s_adc_buffer[2][BUFFER_SIZE * ANALOG_CHANNELS];
static int32_t i2s_dac_buffer[2][BUFFER_SIZE * ANALOG_CHANNELS];

volatile bool __CCMRAM audio_analog_adc_ready = false;
volatile bool __CCMRAM audio_analog_dac_ready = false;


__STATIC_FORCEINLINE float __VCVT_F32_S32(uint32_t value)
{
	float result;
	__ASM volatile ("vcvt.f32.s32 %0, %1, #31" : "=t" (result) : "t" (value));
	return result;
}

__STATIC_FORCEINLINE int32_t __VCVT_S32_F32(float value)
{
	int32_t result;
	__ASM volatile ("vcvt.s32.f32 %0, %1, #31" : "=t" (result) : "t" (value));
	return result;
}

static inline void audio_transfer_in(int in_buf, int dma_buf)
{
	for (int i = 0; i < BUFFER_SIZE; i++)
	{
		int j = i << 1;

		audio_in_buffers[in_buf][0][i] = __VCVT_F32_S32(__ROR(i2s_adc_buffer[dma_buf][j], 16));
		audio_in_buffers[in_buf][1][i] = __VCVT_F32_S32(__ROR(i2s_adc_buffer[dma_buf][j + 1], 16));
	}
}

static inline void audio_transfer_out(int out_buf, int dma_buf)
{
	for (int i = 0; i < BUFFER_SIZE; i++)
	{
		int j = i << 1;

		i2s_dac_buffer[dma_buf][j] = __ROR(__VCVT_S32_F32(audio_out_buffers[out_buf][0][i]), 16);
		i2s_dac_buffer[dma_buf][j + 1] = __ROR(__VCVT_S32_F32(audio_out_buffers[out_buf][1][i]), 16);
	}
}

void DMA1_Stream3_IRQHandler(void)
{
	SET_BIT(DMA1->LIFCR, DMA_LIFCR_CTCIF3);

	audio_transfer_in(!audio_in_buffer, !READ_BIT(DMA1_Stream3->CR, DMA_SxCR_CT));
	audio_in_buffer = !audio_in_buffer;

	audio_analog_adc_ready = true;
	sys_busy(&audio_analog_adc_ready);
}

void DMA1_Stream4_IRQHandler(void)
{
	SET_BIT(DMA1->HIFCR, DMA_HIFCR_CTCIF4);

	audio_transfer_out(!audio_out_buffer, !READ_BIT(DMA1_Stream4->CR, DMA_SxCR_CT));
	audio_out_buffer = !audio_out_buffer;

	audio_analog_dac_ready = true;
	sys_busy(&audio_analog_dac_ready);
}

static void audio_init_I2S_out(void)
{
	// Set up DMA
	MODIFY_REG(DMA1_Stream4->CR, DMA_SxCR_PL_Msk, 0b11 << DMA_SxCR_PL_Pos);		// Highest Priority
	MODIFY_REG(DMA1_Stream4->CR, DMA_SxCR_CHSEL_Msk, 2 << DMA_SxCR_CHSEL_Pos);	// Channel 2

	// Enable tranfer FIFO
	MODIFY_REG(DMA1_Stream4->FCR, DMA_SxFCR_DMDIS_Msk, 0b1 << DMA_SxFCR_DMDIS_Pos);
	MODIFY_REG(DMA1_Stream4->FCR, DMA_SxFCR_FTH_Msk, 0b01 << DMA_SxFCR_FTH_Pos);

	// 16 bit to 32 bit transfers
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
	DMA1_Stream4->PAR = (uint32_t) & (I2S2ext->DR);
	DMA1_Stream4->NDTR = (uint16_t)BUFFER_SIZE * 4;

	// Enable transfer complete interrupts
	SET_BIT(DMA1_Stream4->CR, DMA_SxCR_TCIE);
	NVIC_EnableIRQ(DMA1_Stream4_IRQn);

	// Enable DMA channel
	SET_BIT(DMA1_Stream4->CR, DMA_SxCR_EN);

	// Set up pins
	MODIFY_REG(GPIOB->MODER, GPIO_MODER_MODER14_Msk, 0b10 << GPIO_MODER_MODER14_Pos);
	MODIFY_REG(GPIOB->AFR[1], GPIO_AFRH_AFSEL14_Msk, 6 << GPIO_AFRH_AFSEL14_Pos);

	// Set I2S mode
	MODIFY_REG(I2S2ext->I2SCFGR, SPI_I2SCFGR_I2SMOD_Msk, 0b1 << SPI_I2SCFGR_I2SMOD_Pos);  // Enable I2S Mode on SPI peripheral
	MODIFY_REG(I2S2ext->I2SCFGR, SPI_I2SCFGR_I2SCFG_Msk, 0b00 << SPI_I2SCFGR_I2SCFG_Pos); // Set peripheral to slave transmit mode
	MODIFY_REG(I2S2ext->I2SCFGR, SPI_I2SCFGR_I2SSTD_Msk, 0b00 << SPI_I2SCFGR_I2SSTD_Pos); // Phillips I2S standard
	MODIFY_REG(I2S2ext->I2SCFGR, SPI_I2SCFGR_DATLEN_Msk, 0b01 << SPI_I2SCFGR_DATLEN_Pos); // 24 bit data length

	// Enable DMA
	SET_BIT(I2S2ext->CR2, SPI_CR2_TXDMAEN);
}

static void audio_init_I2S_in(void)
{
	// Set up DMA
	MODIFY_REG(DMA1_Stream3->CR, DMA_SxCR_PL_Msk, 0b11 << DMA_SxCR_PL_Pos);		// Highest Priority
	MODIFY_REG(DMA1_Stream3->CR, DMA_SxCR_CHSEL_Msk, 0 << DMA_SxCR_CHSEL_Pos);	// Channel 0

	// Enable tranfer FIFO
	MODIFY_REG(DMA1_Stream3->FCR, DMA_SxFCR_DMDIS_Msk, 0b1 << DMA_SxFCR_DMDIS_Pos);
	MODIFY_REG(DMA1_Stream3->FCR, DMA_SxFCR_FTH_Msk, 0b01 << DMA_SxFCR_FTH_Pos);

	// 32 bit to 16 bit transfers
	MODIFY_REG(DMA1_Stream3->CR, DMA_SxCR_MSIZE_Msk, 0b10 << DMA_SxCR_MSIZE_Pos);
	MODIFY_REG(DMA1_Stream3->CR, DMA_SxCR_PSIZE_Msk, 0b01 << DMA_SxCR_PSIZE_Pos);

	// Peripheral to memory with memory increment
	MODIFY_REG(DMA1_Stream3->CR, DMA_SxCR_DIR_Msk, 0b00 << DMA_SxCR_DIR_Pos);
	SET_BIT(DMA1_Stream3->CR, DMA_SxCR_MINC);

	// Circular mode with double buffering
	SET_BIT(DMA1_Stream3->CR, DMA_SxCR_CIRC);
	SET_BIT(DMA1_Stream3->CR, DMA_SxCR_DBM);

	// Set base addresses
	DMA1_Stream3->M0AR = (uint32_t)&i2s_adc_buffer[0];
	DMA1_Stream3->M1AR = (uint32_t)&i2s_adc_buffer[1];
	DMA1_Stream3->PAR = (uint32_t) & (SPI2->DR);
	DMA1_Stream3->NDTR = (uint16_t)BUFFER_SIZE * 4;

	// Enable transfer complete interrupts
	SET_BIT(DMA1_Stream3->CR, DMA_SxCR_TCIE);
	NVIC_EnableIRQ(DMA1_Stream3_IRQn);

	// Enable DMA channel
	SET_BIT(DMA1_Stream3->CR, DMA_SxCR_EN);

	// Enable SPI2 periphery clock
	SET_BIT(RCC->APB1ENR, RCC_APB1ENR_SPI2EN);

	// Set up pins
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
	MODIFY_REG(SPI2->I2SCFGR, SPI_I2SCFGR_I2SCFG_Msk, 0b11 << SPI_I2SCFGR_I2SCFG_Pos); // Set peripheral to master receive mode
	MODIFY_REG(SPI2->I2SCFGR, SPI_I2SCFGR_I2SSTD_Msk, 0b00 << SPI_I2SCFGR_I2SSTD_Pos); // Phillips I2S standard
	MODIFY_REG(SPI2->I2SCFGR, SPI_I2SCFGR_DATLEN_Msk, 0b01 << SPI_I2SCFGR_DATLEN_Pos); // 24 bit data length

	// Set I2S clock
	MODIFY_REG(SPI2->I2SPR, SPI_I2SPR_MCKOE_Msk, 0b1 << SPI_I2SPR_MCKOE_Pos);
	MODIFY_REG(SPI2->I2SPR, SPI_I2SPR_I2SDIV_Msk, I2SDIV << SPI_I2SPR_I2SDIV_Pos);
	MODIFY_REG(SPI2->I2SPR, SPI_I2SPR_ODD_Msk, I2SODD << SPI_I2SPR_ODD_Pos);

	// Enable DMA
	SET_BIT(SPI2->CR2, SPI_CR2_RXDMAEN);
}

void audio_analog_init(void)
{
	// Set up I2S clock
	MODIFY_REG(RCC->PLLI2SCFGR, RCC_PLLI2SCFGR_PLLI2SR_Msk, PLLR << RCC_PLLI2SCFGR_PLLI2SR_Pos);
	MODIFY_REG(RCC->PLLI2SCFGR, RCC_PLLI2SCFGR_PLLI2SN_Msk, PLLN << RCC_PLLI2SCFGR_PLLI2SN_Pos);
	SET_BIT(RCC->CR, RCC_CR_PLLI2SON);
	TIMEOUT_WAIT(!READ_BIT(RCC->CR, RCC_CR_PLLI2SRDY), 2500);

	audio_init_I2S_in();
	audio_init_I2S_out();

	// Enable both at the same time so their interrupts line up
	SET_BIT(SPI2->I2SCFGR, SPI_I2SCFGR_I2SE);
	SET_BIT(I2S2ext->I2SCFGR, SPI_I2SCFGR_I2SE);
}