#include <stdio.h>

#include "core.h"
#include "board.h"

#include "system.h"
#include "io.h"
#include "audio.h"
#include "midi.h"
#include "wdsp.h"

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