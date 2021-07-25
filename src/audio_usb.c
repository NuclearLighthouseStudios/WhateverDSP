#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include "board.h"
#include "core.h"

#include "system.h"
#include "usb.h"
#include "usb_phy.h"
#include "usb_config.h"
#include "usb_uac.h"

#include "audio_usb.h"

static usb_in_endpoint __CCMRAM *audio_in_ep;

static usb_audio_input_terminal_descriptor __CCMRAM audio_input_terminal = USB_AUDIO_INPUT_TERMINAL_DESCRIPTOR_INIT(1, 0x0201, 1, 0x0000);
static usb_audio_output_terminal_descriptor __CCMRAM audio_output_terminal = USB_AUDIO_OUTPUT_TERMINAL_DESCRIPTOR_INIT(2, 0x0101, 1);

static usb_interface_descriptor __CCMRAM interface_desc = USB_INTERFACE_DESCRIPTOR_INIT(1, 0x01, 0x02, 0x00);

static usb_audio_interface_descriptor __CCMRAM audio_interface_desc = USB_AUDIO_INTERFACE_DESCRIPTOR_INIT(2, 0, 0x0003);
static usb_audio_format_i_descriptor __CCMRAM audio_format_desc = USB_AUDIO_FORMAT_I_DESCRIPTOR_INIT(2, 4, 32, SAMPLE_RATE);

static usb_endpoint_descriptor __CCMRAM endpoint_desc;
static usb_audio_endpoint_descriptor __CCMRAM audio_endpoint_desc = USB_AUDIO_ENDPOINT_DESCRIPTOR_INIT();

static float tx_buf[2][128];
static int active_buf = 0;
static int buff_length = 0;
static int tx_length = 0;

static bool active = false;
static bool tx_done = true;

#include "stm32f4xx.h"

static void sof_callback(uint16_t frame_num)
{
	if (active && tx_done)
	{
		tx_done = false;
		usb_transmit((uint8_t *)(tx_buf[!active_buf]), tx_length * 4, audio_in_ep);
	}
}

static void tx_callback(usb_in_endpoint *ep, size_t count)
{
	tx_done = true;
	active_buf = !active_buf;
	tx_length = buff_length;
	buff_length = 0;
}

static void in_start(usb_in_endpoint *ep)
{
	active = true;
	tx_done = true;
}

static void in_stop(usb_in_endpoint *ep)
{
	active = false;
}

void audio_usb_send(float in_buffer[][2], int len)
{
	if (!active)
		return;

	for (int i = 0; i < len; i++)
	{
		if (buff_length >= 126)
			return;

		tx_buf[active_buf][buff_length++] = in_buffer[i][0];
		tx_buf[active_buf][buff_length++] = in_buffer[i][1];
	}
}

void audio_usb_init(void)
{
	audio_in_ep = usb_add_in_ep(EP_TYPE_ISOCHRONOUS, 512, 128, &in_start, &in_stop);
	usb_set_tx_callback(audio_in_ep, &tx_callback);

	usb_config_add_descriptor((usb_descriptor *)&audio_input_terminal);
	usb_uac_add_terminal((usb_descriptor *)&audio_input_terminal);

	usb_config_add_descriptor((usb_descriptor *)&audio_output_terminal);
	usb_uac_add_terminal((usb_descriptor *)&audio_output_terminal);

	interface_desc.iInterface = usb_config_add_string("USB Audio Interface");
	usb_config_add_descriptor((usb_descriptor *)&interface_desc);
	usb_uac_add_interface(&interface_desc);

	usb_config_add_descriptor((usb_descriptor *)&audio_interface_desc);
	usb_config_add_descriptor((usb_descriptor *)&audio_format_desc);

	endpoint_desc = USB_ENDPOINT_DESCRIPTOR_INIT_ISO(audio_in_ep->epnum | 0x80, 0b01, 0b00, 512);
	usb_config_add_descriptor((usb_descriptor *)&endpoint_desc);
	usb_config_add_descriptor((usb_descriptor *)&audio_endpoint_desc);

	usb_phy_add_sof_callback(&sof_callback);
}