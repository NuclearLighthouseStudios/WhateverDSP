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

float __CCMRAM audio_in_buffers[2][NUM_STREAMS][BLOCK_SIZE];
float __CCMRAM audio_out_buffers[2][NUM_STREAMS][BLOCK_SIZE];

int __CCMRAM audio_in_buffer = 0;
int __CCMRAM audio_out_buffer = 0;

void audio_process(void)
{
	if (audio_phy_adc_ready && audio_phy_dac_ready)
	{
		audio_phy_adc_ready = false;
		audio_phy_dac_ready = false;

	#if USB_AUDIO_ENABLED == true
	#if (USB_AUDIO_IN_POS == pre) && (USB_INPUT_ENABLED == true)
		audio_usb_in(audio_in_buffers[audio_in_buffer]);
	#endif
	#if (USB_AUDIO_OUT_POS == pre) && (USB_OUTPUT_ENABLED == true)
		audio_usb_out(audio_in_buffers[audio_in_buffer]);
	#endif
	#endif

		wdsp_process(audio_in_buffers[audio_in_buffer], audio_out_buffers[audio_out_buffer]);

	#if USB_AUDIO_ENABLED == true
	#if (USB_AUDIO_IN_POS == post) && (USB_INPUT_ENABLED == true)
		audio_usb_in(audio_out_buffers[audio_out_buffer]);
	#endif
	#if (USB_AUDIO_OUT_POS == post) && (USB_OUTPUT_ENABLED == true)
		audio_usb_out(audio_out_buffers[audio_out_buffer]);
	#endif
	#endif
	}
}

void audio_init(void)
{
	audio_phy_init();
}