#include <stdio.h>

#include "core.h"
#include "board.h"

#include "system.h"
#include "io.h"
#include "audio.h"
#include "usb.h"
#include "usb_config.h"
#include "usb_uac.h"
#include "midi.h"
#include "midi_uart.h"
#include "midi_usb.h"
#include "wdsp.h"

int main(void)
{
	static unsigned long int last_idle = 0;

	sys_init();
	io_init();

	usb_init();
	usb_config_init();
	usb_uac_init();

	audio_init();

	midi_init();
	midi_uart_init();
	midi_usb_init();

	usb_start();

	puts("Hey there! {^-^}~");
	printf("libWDSP running at %dhz with block size %d\n", SAMPLE_RATE, BLOCK_SIZE);

	wdsp_init(SAMPLE_RATE);

	while (1)
	{
		audio_process();
		usb_process();

		if (sys_ticks != last_idle)
		{
			last_idle = sys_ticks;
			wdsp_idle();
		}

		sys_idle();
	}
}