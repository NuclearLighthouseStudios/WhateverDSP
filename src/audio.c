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

#if BUFFER_SIZE % BLOCK_SIZE != 0
#error BUFFER_SIZE must be a multiple of BLOCK_SIZE
#endif

#define NUM_BLOCKS (BUFFER_SIZE/BLOCK_SIZE)

float __CCMRAM audio_in_buffers[2][NUM_STREAMS][BUFFER_SIZE];
float __CCMRAM audio_out_buffers[2][NUM_STREAMS][BUFFER_SIZE];

static float __CCMRAM *audio_in_blocks[NUM_BLOCKS][2][NUM_STREAMS];
static float __CCMRAM *audio_out_blocks[NUM_BLOCKS][2][NUM_STREAMS];

int __CCMRAM audio_in_buffer = 0;
int __CCMRAM audio_out_buffer = 0;

void audio_process(void)
{
	static bool __CCMRAM processing = false;
	static int __CCMRAM block_num;

	if (audio_phy_adc_ready && audio_phy_dac_ready)
	{
		audio_phy_adc_ready = false;
		audio_phy_dac_ready = false;

		processing = true;
		sys_busy(&processing);
		block_num = 0;
	}

	if (processing)
	{
	#if USB_AUDIO_ENABLED == true
	#if (USB_AUDIO_IN_POS == pre) && (USB_INPUT_ENABLED == true)
		audio_usb_in(audio_in_blocks[block_num][audio_in_buffer]);
	#endif
	#if (USB_AUDIO_OUT_POS == pre) && (USB_OUTPUT_ENABLED == true)
		audio_usb_out(audio_in_blocks[block_num][audio_in_buffer]);
	#endif
	#endif

		wdsp_process(audio_in_blocks[block_num][audio_in_buffer], audio_out_blocks[block_num][audio_out_buffer]);

	#if USB_AUDIO_ENABLED == true
	#if (USB_AUDIO_IN_POS == post) && (USB_INPUT_ENABLED == true)
		audio_usb_in(audio_out_blocks[block_num][audio_out_buffer]);
	#endif
	#if (USB_AUDIO_OUT_POS == post) && (USB_OUTPUT_ENABLED == true)
		audio_usb_out(audio_out_blocks[block_num][audio_out_buffer]);
	#endif
	#endif

		block_num++;
		if (block_num >= NUM_BLOCKS)
			processing = false;
	}
}

void audio_init(void)
{
	for (int i = 0; i < NUM_BLOCKS; i++)
	{
		for (int s = 0; s < NUM_STREAMS; s++)
		{
			audio_in_blocks[i][0][s] = audio_in_buffers[0][s] + i * BLOCK_SIZE;
			audio_in_blocks[i][1][s] = audio_in_buffers[1][s] + i * BLOCK_SIZE;

			audio_out_blocks[i][0][s] = audio_out_buffers[0][s] + i * BLOCK_SIZE;
			audio_out_blocks[i][1][s] = audio_out_buffers[1][s] + i * BLOCK_SIZE;
		}
	}

	audio_phy_init();
}