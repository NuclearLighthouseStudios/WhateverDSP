#include <stdio.h>

#include "config.h"

#include "core.h"
#include "board.h"

#include "system.h"
#include "io.h"
#include "bootloader.h"
#include "audio.h"
#include "audio_usb.h"
#include "usb.h"
#include "usb_config.h"
#include "usb_uac.h"
#include "midi.h"
#include "midi_usb.h"
#include "wdsp.h"

int main(void)
{
	static unsigned long int last_idle = 0;

	sys_init();
	io_init();

	bootloader_check();

#if CONFIG_MODULES_USB == true
	usb_init();
	usb_config_init();
	bootloader_usb_init();
#if ((CONFIG_MODULES_MIDI == true) && (CONFIG_MIDI_USB == true)) || (CONFIG_AUDIO_USB == true)
	usb_uac_init();
#endif
#endif

	audio_init();

#if (CONFIG_MODULES_USB == true) && (CONFIG_AUDIO_USB == true)
	audio_usb_init();
#endif

#if CONFIG_MODULES_MIDI == true
	midi_init();

#if (CONFIG_MODULES_USB == true) && (CONFIG_MIDI_USB == true)
	midi_usb_init();
#endif

#endif

#if CONFIG_MODULES_USB == true
	usb_start();
#endif

	puts("FXDSP Go! \\{^-^}/");
	printf("libWDSP running at %dhz with block size %d\n", SAMPLE_RATE, BLOCK_SIZE);

	wdsp_init();

	while (1)
	{
		audio_process();

	#if CONFIG_MODULES_USB == true
		usb_process();
	#endif

		if (sys_ticks != last_idle)
		{
			last_idle = sys_ticks;
			wdsp_idle(sys_ticks);
		}

		sys_idle();
	}
}