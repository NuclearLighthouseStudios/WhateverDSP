#include <stdio.h>

#include "core.h"
#include "board.h"

#include "system.h"
#include "io.h"
#include "usb.h"
#include "usb_config.h"
#include "audio.h"
#include "midi.h"
#include "wdsp.h"

int main(void)
{
	sys_init();
	io_init();
	usb_init();
	audio_init();
	midi_init();

	usb_config_init();

	usb_start();

	puts("Hey there! {^-^}~");

	wdsp_init(SAMPLE_RATE);

	while (1)
	{
		audio_process();
		usb_process();

		wdsp_idle();
		sys_idle();
	}
}