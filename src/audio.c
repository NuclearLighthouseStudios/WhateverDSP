#include <stdbool.h>
#include <stdio.h>

#include "stm32f4xx.h"

#include "core.h"
#include "board.h"

#include "wdsp.h"

#include "system.h"
#include "audio_phy.h"
#include "audio_usb.h"
#include "audio.h"

#include "conf/audio.h"

#if (BUFFER_SIZE % BLOCK_SIZE) != 0
#error BLOCK_SIZE must evenly divide into BUFFER_SIZE!
#endif

float __CCMRAM audio_in_buffers[2][NUM_STREAMS][BUFFER_SIZE];
float __CCMRAM audio_out_buffers[2][NUM_STREAMS][BUFFER_SIZE];

static float __CCMRAM *in_blocks[BUFFER_SIZE / BLOCK_SIZE][2][NUM_STREAMS];
static float __CCMRAM *out_blocks[BUFFER_SIZE / BLOCK_SIZE][2][NUM_STREAMS];

int __CCMRAM audio_in_buffer = 0;
int __CCMRAM audio_out_buffer = 0;

void audio_process(void)
{
#if BUFFER_SIZE != BLOCK_SIZE
	static bool __CCMRAM processing = false;
	static size_t __CCMRAM block = 0;
#endif

	if (audio_phy_adc_ready && audio_phy_dac_ready)
	{
		audio_phy_adc_ready = false;
		audio_phy_dac_ready = false;

		processing = true;
		block = 0;
		sys_busy(&processing);
	}

#if BUFFER_SIZE != BLOCK_SIZE
	if (processing)
	{
	#if USB_AUDIO_ENABLED == true
	#if (USB_AUDIO_IN_POS == pre) && (USB_INPUT_ENABLED == true)
		audio_usb_in(in_blocks[block][audio_in_buffer]);
	#endif
	#if (USB_AUDIO_OUT_POS == pre) && (USB_OUTPUT_ENABLED == true)
		audio_usb_out(in_blocks[block][audio_in_buffer]);
	#endif
	#endif

		wdsp_process(in_blocks[block][audio_in_buffer],
			out_blocks[block][audio_out_buffer]);
	#if USB_AUDIO_ENABLED == true
	#if (USB_AUDIO_IN_POS == post) && (USB_INPUT_ENABLED == true)
		audio_usb_in(out_blocks[block][audio_out_buffer]);
	#endif
	#if (USB_AUDIO_OUT_POS == post) && (USB_OUTPUT_ENABLED == true)
		audio_usb_out(out_blocks[block][audio_out_buffer]);
	#endif
	#endif

		block++;

		if (block >= BUFFER_SIZE / BLOCK_SIZE)
			processing = false;
	}
#endif
}

void audio_init(void)
{
	for (int i = 0; i < BUFFER_SIZE / BLOCK_SIZE; i++)
	{
		for (int s = 0; s < NUM_STREAMS; s++)
		{
			in_blocks[i][0][s] = audio_in_buffers[0][s] + i * BLOCK_SIZE;
			in_blocks[i][1][s] = audio_in_buffers[1][s] + i * BLOCK_SIZE;

			out_blocks[i][0][s] = audio_out_buffers[0][s] + i * BLOCK_SIZE;
			out_blocks[i][1][s] = audio_out_buffers[1][s] + i * BLOCK_SIZE;
		}
	}

	audio_phy_init();
}