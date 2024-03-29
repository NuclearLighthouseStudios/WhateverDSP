#include <stdio.h>
#include <stdbool.h>
#include <stdnoreturn.h>

#include "stm32f4xx.h"

#include "core.h"
#include "board.h"

#include "system.h"
#include "twait.h"
#include "debug.h"

#include "conf/system.h"

volatile unsigned long int __CCMRAM sys_ticks = 0;

#define MAX_BUSY_FLAGS 16
static volatile bool __CCMRAM *busy_flags[MAX_BUSY_FLAGS];

#define MAX_SCHEDULED_CALLS 16
static volatile sys_schedulable_func __CCMRAM scheduled_calls[MAX_SCHEDULED_CALLS];

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
	TIMEOUT_WAIT(!READ_BIT(RCC->CR, RCC_CR_HSERDY), 1500);

	// Turn on clock security system
	SET_BIT(RCC->CR, RCC_CR_CSSON);

	// Enable PLL from external high speed clock
	MODIFY_REG(RCC->PLLCFGR, RCC_PLLCFGR_PLLSRC_Msk, RCC_PLLCFGR_PLLSRC_HSE);
	MODIFY_REG(RCC->PLLCFGR, RCC_PLLCFGR_PLLM_Msk, PLL_M << RCC_PLLCFGR_PLLM_Pos);
	MODIFY_REG(RCC->PLLCFGR, RCC_PLLCFGR_PLLN_Msk, PLL_N << RCC_PLLCFGR_PLLN_Pos);
	MODIFY_REG(RCC->PLLCFGR, RCC_PLLCFGR_PLLP_Msk, PLL_P << RCC_PLLCFGR_PLLP_Pos);
	MODIFY_REG(RCC->PLLCFGR, RCC_PLLCFGR_PLLQ_Msk, PLL_Q << RCC_PLLCFGR_PLLQ_Pos);
	SET_BIT(RCC->CR, RCC_CR_PLLON);
	TIMEOUT_WAIT(!READ_BIT(RCC->CR, RCC_CR_PLLRDY), 500);

	// Set up clock domain prescalers and set system clock source to PLL
	MODIFY_REG(RCC->CFGR, RCC_CFGR_PPRE1_Msk, PPRE1);
	MODIFY_REG(RCC->CFGR, RCC_CFGR_PPRE2_Msk, PPRE2);
	MODIFY_REG(RCC->CFGR, RCC_CFGR_HPRE_Msk, HPRE);
	MODIFY_REG(RCC->CFGR, RCC_CFGR_SW_Msk, RCC_CFGR_SW_PLL);
	TIMEOUT_WAIT(!READ_BIT(RCC->CFGR, RCC_CFGR_SWS_PLL), 10);

	// Turn off internal oscillator
	CLEAR_BIT(RCC->CR, RCC_CR_HSION);

	// Enable System Tick
	SysTick_Config(CLOCK_SPEED / 1000ul);
}

void sys_enable_fpu(void)
{
	SCB->CPACR |= ((3UL << 10 * 2) | (3UL << 11 * 2));
}

noreturn void sys_reset(bool enter_bootloader)
{
	extern bool _enter_bootloader;
	_enter_bootloader = enter_bootloader;
	NVIC_SystemReset();
}

static inline void sys_itm_send_int(unsigned int data, unsigned int port)
{
#ifdef DEBUG
	if (((ITM->TCR & ITM_TCR_ITMENA_Msk) != 0UL) &&
		((ITM->TER & 1UL << port) != 0UL))
	{
		while (ITM->PORT[port].u32 == 0UL)
		{
			__NOP();
		}
		ITM->PORT[port].u32 = data;
	}
#endif
}

static inline void sys_itm_send_char(unsigned char data, unsigned int port)
{
#ifdef DEBUG
	if (((ITM->TCR & ITM_TCR_ITMENA_Msk) != 0UL) &&
		((ITM->TER & 1UL << port) != 0UL))
	{
		while (ITM->PORT[port].u32 == 0UL)
		{
			__NOP();
		}
		ITM->PORT[port].u8 = data;
	}
#endif
}

static inline void sys_itm_send_float(float value, unsigned int port)
{
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
	sys_itm_send_int(*(unsigned int *)&value, port);
}

int _write(int file, char *ptr, int len)
{
#ifdef DEBUG
	for (unsigned int i = 0; i < len; i++)
		sys_itm_send_char((*ptr++), 0);
#endif
	return len;
}

void _exit(int status)
{
#ifdef DEBUG
	__disable_irq();
	__BKPT(0);
	for (;;)
		__WFI();
#else
	sys_reset(false);
#endif
}

void sys_delay(unsigned long int delay)
{
	unsigned long int tickstart = sys_ticks;
	unsigned long int wait = delay;

	while ((sys_ticks - tickstart) < wait)
		__WFI();
}

char *sys_get_serial(void)
{
	static char __CCMRAM serial[25];
	for (int i = 0; i < 12; i++)
	{
		sprintf(serial + i * 2, "%02x", *(((uint8_t *)UID_BASE) + i));
	}
	return serial;
}

void sys_busy(volatile bool *flag)
{
	__disable_irq();

	for (int i = 0; i < MAX_BUSY_FLAGS; i++)
	{
		if ((!busy_flags[i]) || (busy_flags[i] == flag))
		{
			busy_flags[i] = flag;
			__SEV();
			__enable_irq();
			return;
		}
	}

	__enable_irq();
	error("Exceeded maximum number of busy flags!\n");
}

void sys_schedule(sys_schedulable_func func)
{
	__disable_irq();

	for (int i = 0; i < MAX_SCHEDULED_CALLS; i++)
	{
		if ((!scheduled_calls[i]) || (scheduled_calls[i] == func))
		{
			scheduled_calls[i] = func;
			__SEV();
			__enable_irq();
			return;
		}
	}

	__enable_irq();
	error("Exceeded maximum number of scheduled calls!\n");
}

void sys_idle(void)
{
	for (int i = 0; i < MAX_SCHEDULED_CALLS; i++)
	{
		if (scheduled_calls[i])
		{
			scheduled_calls[i]();
			scheduled_calls[i] = NULL;
		}
	}

	for (int i = 0; i < MAX_BUSY_FLAGS; i++)
	{
		if (busy_flags[i])
		{
			if (*busy_flags[i])
				return;
			else
				busy_flags[i] = NULL;
		}
	}

#ifdef DEBUG
	uint32_t sleep_start;
	static uint32_t sleep_end = 0;
	static uint32_t time_active = 0;
	static uint32_t time_idle = 0;
	static uint32_t last_print = 0;

	__disable_irq();
	time_active += DWT->CYCCNT - sleep_end;
	sleep_start = DWT->CYCCNT;

	__WFE();

	time_idle += DWT->CYCCNT - sleep_start;
	sleep_end = DWT->CYCCNT;
	__enable_irq();

	if ((sys_ticks - last_print) >= 1000)
	{
		last_print = sys_ticks;

		float load = ((float)time_active / (float)(time_idle + time_active) * 100.0f);
		sys_itm_send_float(load, 1);

		time_idle = 0;
		time_active = 0;
	}
#else
	__WFE();
#endif
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

	// Generate wakeup events on pending interrupts
	SET_BIT(SCB->SCR, SCB_SCR_SEVONPEND_Msk);

	for (int i = 0; i < MAX_BUSY_FLAGS; i++)
		busy_flags[i] = NULL;

	for (int i = 0; i < MAX_SCHEDULED_CALLS; i++)
		scheduled_calls[i] = NULL;

#ifdef DEBUG
	// Enable trace and cycle counter for performance monitoring
	SET_BIT(CoreDebug->DEMCR, CoreDebug_DEMCR_TRCENA_Msk);
	DWT->CYCCNT = 0;
	SET_BIT(DWT->CTRL, DWT_CTRL_CYCCNTENA_Msk);

	// Enable Asynchronous trace output
	SET_BIT(DBGMCU->CR, DBGMCU_CR_TRACE_IOEN);
	MODIFY_REG(DBGMCU->CR, DBGMCU_CR_TRACE_MODE_Msk, 0b00 << DBGMCU_CR_TRACE_MODE_Pos);
#endif
}