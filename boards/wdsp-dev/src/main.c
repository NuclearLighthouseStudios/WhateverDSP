#include <stdio.h>
#include <math.h>

#include "stm32f4xx.h"

#include "system.h"
#include "io.h"
#include "audio.h"
#include "midi.h"
#include "wdsp.h"

void __attribute__((weak)) wdsp_init(unsigned long int sample_rate)
{
	return;
}

void __attribute__((weak)) wdsp_idle(void)
{
	return;
}

int main(void)
{
	sys_init();
	io_init();
	audio_init();
	midi_init();

	puts("Hey there! {^-^}~");

	wdsp_init(SAMPLE_RATE);

	while (1)
	{
		audio_process();
		wdsp_idle();
		sys_idle();
	}
}