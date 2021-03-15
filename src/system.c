#include "stm32f4xx.h"

#include "system.h"

extern void initialise_monitor_handles(void);

volatile unsigned long int  __attribute__((section(".ccmram"))) sys_ticks = 0;

void SysTick_Handler(void)
{
	sys_ticks++;
}

static void sys_init_clock(void)
{
	// Set flash latency to 5 wait states
	MODIFY_REG(FLASH->ACR, FLASH_ACR_LATENCY_Msk, FLASH_ACR_LATENCY_5WS);

	// Enable Instruction and data cache and prefetch
	SET_BIT(FLASH->ACR, FLASH_ACR_PRFTEN);
	SET_BIT(FLASH->ACR, FLASH_ACR_ICEN);
	SET_BIT(FLASH->ACR, FLASH_ACR_DCEN);

	// Turn on external high speed oscillator
	SET_BIT(RCC->CR, RCC_CR_HSEON);
	while (!READ_BIT(RCC->CR, RCC_CR_HSERDY))
		__NOP();

	// Turn on clock security system
	SET_BIT(RCC->CR, RCC_CR_CSSON);

	// Enable PLL from external high speed clock
	MODIFY_REG(RCC->PLLCFGR, RCC_PLLCFGR_PLLSRC_Msk, RCC_PLLCFGR_PLLSRC_HSE);
	MODIFY_REG(RCC->PLLCFGR, RCC_PLLCFGR_PLLM_Msk, 8 << RCC_PLLCFGR_PLLM_Pos);
	MODIFY_REG(RCC->PLLCFGR, RCC_PLLCFGR_PLLN_Msk, 168 << RCC_PLLCFGR_PLLN_Pos);
	MODIFY_REG(RCC->PLLCFGR, RCC_PLLCFGR_PLLP_Msk, 0b00 << RCC_PLLCFGR_PLLP_Pos);
	MODIFY_REG(RCC->PLLCFGR, RCC_PLLCFGR_PLLQ_Msk, 7 << RCC_PLLCFGR_PLLQ_Pos);
	SET_BIT(RCC->CR, RCC_CR_PLLON);
	while (!READ_BIT(RCC->CR, RCC_CR_PLLRDY))
		__NOP();

	// Set up clock domain prescalers and set system clock source to PLL
	MODIFY_REG(RCC->CFGR, RCC_CFGR_PPRE1_Msk, RCC_CFGR_PPRE1_DIV4);
	MODIFY_REG(RCC->CFGR, RCC_CFGR_PPRE2_Msk, RCC_CFGR_PPRE2_DIV2);
	MODIFY_REG(RCC->CFGR, RCC_CFGR_HPRE_Msk, RCC_CFGR_HPRE_DIV1);
	MODIFY_REG(RCC->CFGR, RCC_CFGR_SW_Msk, RCC_CFGR_SW_PLL);
	while (!READ_BIT(RCC->CFGR, RCC_CFGR_SWS_PLL))
		__NOP();

	// Turn off internal oscillator
	CLEAR_BIT(RCC->CR, RCC_CR_HSION);

	// Enable System Tick
	SysTick_Config(CLOCK_SPEED / 1000ul);
}

void sys_enable_fpu(void)
{
	SCB->CPACR |= ((3UL << 10 * 2) | (3UL << 11 * 2));
}

void sys_init(void)
{
	// Enable System Configuration and Power clocks
	SET_BIT(RCC->APB2ENR, RCC_APB2ENR_SYSCFGEN);
	SET_BIT(RCC->APB1ENR, RCC_APB1ENR_PWREN);

	// Go into voltage scale mode 1 for MAXIMUM POWER
	MODIFY_REG(PWR->CR, PWR_CR_VOS, PWR_CR_VOS);

	sys_init_clock();

	// Enable DMA1 and 2 periphery
	SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_DMA1EN);
	SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_DMA2EN);

#ifdef PERFMON
	// Enable trace and cycle counter
	CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
	DWT->CYCCNT = 0;
	DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

	// Enable Instrumentation Trace Macrocell stdio
	initialise_monitor_handles();
#endif
}