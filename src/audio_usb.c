#include <stdbool.h>
#include <stdio.h>
#include <math.h>

#include "config.h"

#include "board.h"
#include "core.h"

#include "system.h"
#include "usb.h"
#include "usb_phy.h"
#include "usb_config.h"
#include "usb_uac.h"

#include "audio_usb.h"

#include "conf/audio_usb.h"


static usb_in_endpoint __CCMRAM *audio_in_ep;
static usb_out_endpoint __CCMRAM *audio_out_ep;
static usb_in_endpoint __CCMRAM *synch_in_ep;

static uint8_t __CCMRAM in_alt_setting = 0;
static uint8_t __CCMRAM out_alt_setting = 0;

static bool __CCMRAM in_active = false;
static bool __CCMRAM out_active = false;


static usb_audio_input_terminal_descriptor __CCMRAM audio_input_terminal = USB_AUDIO_INPUT_TERMINAL_DESCRIPTOR_INIT(1, 0x0201, 2, 0x0003);
static usb_audio_output_terminal_descriptor __CCMRAM usb_output_terminal = USB_AUDIO_OUTPUT_TERMINAL_DESCRIPTOR_INIT(2, 0x0101, 1);

static usb_audio_input_terminal_descriptor __CCMRAM usb_input_terminal = USB_AUDIO_INPUT_TERMINAL_DESCRIPTOR_INIT(3, 0x0101, 4, 0x0003);
static usb_audio_output_terminal_descriptor __CCMRAM audio_output_terminal = USB_AUDIO_OUTPUT_TERMINAL_DESCRIPTOR_INIT(4, 0x0301, 1);


static usb_interface_descriptor __CCMRAM in_interface_desc_zb = USB_INTERFACE_DESCRIPTOR_INIT(0, 0x01, 0x02, 0x00);
static usb_interface_descriptor __CCMRAM in_interface_desc = USB_INTERFACE_DESCRIPTOR_INIT_ALT(1, 1, 0x01, 0x02, 0x00);

static usb_audio_interface_descriptor __CCMRAM in_audio_interface_desc = USB_AUDIO_INTERFACE_DESCRIPTOR_INIT(2, 0, FORMAT_TAG);
static usb_audio_format_i_descriptor __CCMRAM in_audio_format_desc = USB_AUDIO_FORMAT_I_DESCRIPTOR_INIT(2, SUBFRAME_SIZE, BIT_RESOLUTION, SAMPLE_RATE);

static usb_endpoint_descriptor __CCMRAM in_endpoint_desc;
static usb_audio_endpoint_descriptor __CCMRAM in_audio_endpoint_desc = USB_AUDIO_ENDPOINT_DESCRIPTOR_INIT();


static usb_interface_descriptor __CCMRAM out_interface_desc_zb = USB_INTERFACE_DESCRIPTOR_INIT(0, 0x01, 0x02, 0x00);
static usb_interface_descriptor __CCMRAM out_interface_desc = USB_INTERFACE_DESCRIPTOR_INIT_ALT(1, 2, 0x01, 0x02, 0x00);

static usb_audio_interface_descriptor __CCMRAM out_audio_interface_desc = USB_AUDIO_INTERFACE_DESCRIPTOR_INIT(3, 0, FORMAT_TAG);
static usb_audio_format_i_descriptor __CCMRAM out_audio_format_desc = USB_AUDIO_FORMAT_I_DESCRIPTOR_INIT(2, SUBFRAME_SIZE, BIT_RESOLUTION, SAMPLE_RATE);

static usb_endpoint_descriptor __CCMRAM out_endpoint_desc;
static usb_audio_endpoint_descriptor __CCMRAM out_audio_endpoint_desc = USB_AUDIO_ENDPOINT_DESCRIPTOR_INIT();

static usb_endpoint_descriptor __CCMRAM synch_endpoint_desc;


static uint8_t __CCMRAM tx_buf[2][FRAME_SIZE];
static int __CCMRAM tx_active_buf = 0;
static int __CCMRAM tx_buf_length = 0;

#define IN_BUF_SIZE FRAME_SIZE / 4 * 2
static uint8_t __CCMRAM rx_buf[FRAME_SIZE];
static uint32_t __CCMRAM in_buf[IN_BUF_SIZE];
static int in_read_pos = 0;
static int in_write_pos = 0;
static int in_fill_at_rx = 0;

static uint32_t __CCMRAM num_samples = 0;
static uint32_t __CCMRAM num_frames = 0;


static void eof_callback(void)
{
	static uint32_t __CCMRAM sync_sample_rate;

	if ((out_active) && (out_alt_setting != 0))
	{
		num_frames++;

		if (num_frames >= (1 << SYNC_INTERVAL))
		{
			float servo = ((float)in_fill_at_rx - (float)(IN_BUF_SIZE / 4)) * SYNC_SERVO_AMOUNT;
			sync_sample_rate = ((float)num_samples / (float)num_frames - servo) * (float)(1 << 14);

			num_frames = 0;
			num_samples = 0;
		}

		usb_transmit((uint8_t *)&sync_sample_rate, 3, synch_in_ep);
		usb_receive((uint8_t *)(rx_buf), FRAME_SIZE, audio_out_ep);
	}

	if ((in_active) && (in_alt_setting != 0))
	{
		usb_transmit((uint8_t *)(tx_buf[tx_active_buf]), tx_buf_length, audio_in_ep);

		tx_buf_length = 0;
		tx_active_buf = !tx_active_buf;
	}
}

static void rx_callback(usb_out_endpoint *ep, uint8_t *buf, size_t count)
{
	in_fill_at_rx = in_write_pos - in_read_pos;
	if (in_fill_at_rx < 0)
		in_fill_at_rx += IN_BUF_SIZE;

	for (int i = 0; i < count; i += SUBFRAME_SIZE)
	{
		in_buf[in_write_pos++] = *((uint32_t *)&buf[i]);

		if (in_write_pos >= IN_BUF_SIZE)
			in_write_pos = 0;

		if (in_write_pos == in_read_pos)
			in_read_pos++;
	}
}

static void in_start(usb_in_endpoint *ep)
{
	in_active = true;
}

static void in_stop(usb_in_endpoint *ep)
{
	in_active = false;
	in_alt_setting = 0;
}

static void out_start(usb_out_endpoint *ep)
{
	out_active = true;
}

static void out_stop(usb_out_endpoint *ep)
{
	out_active = false;
	out_alt_setting = 0;
}

void audio_usb_out(float buffer[][2], int len)
{
	if ((!in_active) || (in_alt_setting == 0))
		return;

	SAMPLE_TYPE sample;

	for (int i = 0; i < len; i++)
	{
		if (tx_buf_length >= FRAME_SIZE - 2 * SUBFRAME_SIZE)
			return;

	#ifdef SCALER
		sample = (SAMPLE_TYPE)(buffer[i][0] * SCALER) >> (sizeof(SAMPLE_TYPE) * 8 - BIT_RESOLUTION);
	#else
		sample = buffer[i][0];
	#endif

		*((SAMPLE_TYPE *)&tx_buf[tx_active_buf][tx_buf_length]) = sample;
		tx_buf_length += SUBFRAME_SIZE;

	#ifdef SCALER
		sample = (SAMPLE_TYPE)(buffer[i][1] * SCALER) >> (sizeof(SAMPLE_TYPE) * 8 - BIT_RESOLUTION);
	#else
		sample = buffer[i][1];
	#endif
		*((SAMPLE_TYPE *)&tx_buf[tx_active_buf][tx_buf_length]) = sample;
		tx_buf_length += SUBFRAME_SIZE;
	}
}

void audio_usb_in(float buffer[][2], int len)
{
	if ((!out_active) || (out_alt_setting == 0))
		return;

	num_samples += len;

	SAMPLE_TYPE sample;

	for (int i = 0; i < len; i++)
	{
		if (in_read_pos == in_write_pos)
			break;

		sample = *((SAMPLE_TYPE *)&in_buf[in_read_pos]);

	#ifdef SCALER
		buffer[i][0] += (sample << (sizeof(SAMPLE_TYPE) * 8 - BIT_RESOLUTION)) / (float)SCALER;
	#else
		buffer[i][0] += sample;
	#endif

		in_read_pos++;
		if (in_read_pos >= IN_BUF_SIZE)
			in_read_pos = 0;


		if (in_read_pos == in_write_pos)
			break;

		sample = *((SAMPLE_TYPE *)&in_buf[in_read_pos]);

	#ifdef SCALER
		buffer[i][1] += (sample << (sizeof(SAMPLE_TYPE) * 8 - BIT_RESOLUTION)) / (float)SCALER;
	#else
		buffer[i][1] += sample;
	#endif

		in_read_pos++;
		if (in_read_pos >= IN_BUF_SIZE)
			in_read_pos = 0;
	}
}

static bool handle_in_setup(usb_setup_packet *packet, usb_in_endpoint *in_ep, usb_out_endpoint *out_ep)
{
	switch (packet->bRequest)
	{
		// GET_INTERFACE
		case 10:
		{
			size_t size = sizeof(in_alt_setting);

			if (size > packet->wLength)
				size = packet->wLength;

			usb_transmit((uint8_t *)&in_alt_setting, size, in_ep);
		}
		break;

		// SET_INTERFACE
		case 11:
		{
			in_alt_setting = packet->wValue & 0xff;

			if (in_alt_setting == 0)
				usb_cancel_transmit(audio_in_ep);
		}
		break;

		default:
			return false;
	}

	return true;
}

static bool handle_out_setup(usb_setup_packet *packet, usb_in_endpoint *in_ep, usb_out_endpoint *out_ep)
{
	switch (packet->bRequest)
	{
		// GET_INTERFACE
		case 10:
		{
			size_t size = sizeof(out_alt_setting);

			if (size > packet->wLength)
				size = packet->wLength;

			usb_transmit((uint8_t *)&out_alt_setting, size, in_ep);
		}
		break;

		// SET_INTERFACE
		case 11:
		{
			out_alt_setting = packet->wValue & 0xff;

			if (out_alt_setting != 0)
			{
				num_samples = 0;
				num_frames = 0;
				in_read_pos = 0;
				in_write_pos = 0;
			}
			else
			{
				usb_cancel_receive(audio_out_ep);
			}
		}
		break;

		default:
			return false;
	}

	return true;
}

void audio_usb_init(void)
{
	audio_in_ep = usb_add_in_ep(EP_TYPE_ISOCHRONOUS, FRAME_SIZE, FRAME_SIZE, &in_start, &in_stop);
	audio_out_ep = usb_add_out_ep(EP_TYPE_ISOCHRONOUS, FRAME_SIZE, &out_start, &out_stop);
	usb_set_rx_callback(audio_out_ep, &rx_callback);

	synch_in_ep = usb_add_in_ep(EP_TYPE_ISOCHRONOUS, 8, 8, NULL, NULL);

	usb_config_add_descriptor((usb_descriptor *)&audio_input_terminal);
	usb_uac_add_terminal((usb_descriptor *)&audio_input_terminal);

	usb_config_add_descriptor((usb_descriptor *)&usb_output_terminal);
	usb_uac_add_terminal((usb_descriptor *)&usb_output_terminal);

	usb_config_add_descriptor((usb_descriptor *)&usb_input_terminal);
	usb_uac_add_terminal((usb_descriptor *)&usb_input_terminal);

	usb_config_add_descriptor((usb_descriptor *)&audio_output_terminal);
	usb_uac_add_terminal((usb_descriptor *)&audio_output_terminal);


	int interface_string = usb_config_add_string(INTERFACE_NAME);

	in_interface_desc_zb.iInterface = interface_string;
	usb_config_add_interface(&in_interface_desc_zb, &handle_in_setup);

	in_interface_desc.iInterface = interface_string;
	usb_config_add_interface(&in_interface_desc, &handle_in_setup);
	usb_uac_add_interface(&in_interface_desc);

	usb_config_add_descriptor((usb_descriptor *)&in_audio_interface_desc);
	usb_config_add_descriptor((usb_descriptor *)&in_audio_format_desc);

	in_endpoint_desc = USB_ENDPOINT_DESCRIPTOR_INIT_AUDIO(audio_in_ep->epnum | 0x80, 0b01, 0b00, 0, audio_in_ep->max_packet_size);
	usb_config_add_descriptor((usb_descriptor *)&in_endpoint_desc);
	usb_config_add_descriptor((usb_descriptor *)&in_audio_endpoint_desc);


	out_interface_desc_zb.iInterface = interface_string;
	usb_config_add_interface(&out_interface_desc_zb, &handle_out_setup);

	out_interface_desc.iInterface = interface_string;
	usb_config_add_interface(&out_interface_desc, &handle_out_setup);
	usb_uac_add_interface(&out_interface_desc);

	usb_config_add_descriptor((usb_descriptor *)&out_audio_interface_desc);
	usb_config_add_descriptor((usb_descriptor *)&out_audio_format_desc);

	out_endpoint_desc = USB_ENDPOINT_DESCRIPTOR_INIT_AUDIO(audio_out_ep->epnum, 0b01, 0b00, synch_in_ep->epnum | 0x80, audio_out_ep->max_packet_size);
	usb_config_add_descriptor((usb_descriptor *)&out_endpoint_desc);
	usb_config_add_descriptor((usb_descriptor *)&out_audio_endpoint_desc);

	synch_endpoint_desc = USB_ENDPOINT_DESCRIPTOR_INIT_SYNCH(synch_in_ep->epnum | 0x80, 0b00, 0b01, SYNC_INTERVAL, synch_in_ep->max_packet_size);
	usb_config_add_descriptor((usb_descriptor *)&synch_endpoint_desc);

	usb_phy_add_eof_callback(&eof_callback);
}