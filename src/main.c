#include <stdio.h>
#include <math.h>

#include "stm32f4xx.h"

#include "system.h"
#include "io.h"
#include "audio.h"

#ifdef DEBUG
// #define PERFMON
#endif

int main(void)
{
	sys_init();
	io_init();
	audio_init();

	puts("Hey there! {^-^}~");

	while (1)
	{
		audio_process();

#ifdef PERFMON
		uint32_t sleep_start;
		static uint32_t sleep_end = 0;
		static uint32_t time_active = 0;
		static uint32_t time_idle = 0;
		static uint32_t last_print = 0;

		__disable_irq();
		time_active += DWT->CYCCNT - sleep_end;
		sleep_start = DWT->CYCCNT;

		__WFI();

		time_idle += DWT->CYCCNT - sleep_start;
		sleep_end = DWT->CYCCNT;
		__enable_irq();

		if ((sys_ticks - last_print) >= 1000)
		{
			last_print = sys_ticks;

			float load = ((float)time_active / (float)(time_idle + time_active) * 100.0f);
			printf("Load: %5.2f%%\n", load);

			time_idle = 0;
			time_active = 0;
		}
#else
			__WFI();
#endif
	}
}