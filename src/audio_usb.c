#include <stdbool.h>
#include <string.h>
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

#if BLOCK_SIZE > 16
#warning For USB audio to perform reliably BLOCK_SIZE should to be smaller than 16
#endif

#if OUTPUT_ENABLED == true
static usb_in_endpoint __CCMRAM * audio_in_ep;

static uint8_t __CCMRAM in_alt_setting = 0;
static bool __CCMRAM in_active = false;
#endif

#if INPUT_ENABLED == true
static usb_out_endpoint __CCMRAM *audio_out_ep;
static usb_in_endpoint __CCMRAM *synch_in_ep;

static uint8_t __CCMRAM out_alt_setting = 0;
static bool __CCMRAM out_active = false;
#endif

#if OUTPUT_ENABLED == true
static usb_audio_input_terminal_descriptor __CCMRAM audio_input_terminal = USB_AUDIO_INPUT_TERMINAL_DESCRIPTOR_INIT(1, 0x0201, 2, 0x0003);
static usb_audio_output_terminal_descriptor __CCMRAM usb_output_terminal = USB_AUDIO_OUTPUT_TERMINAL_DESCRIPTOR_INIT(2, 0x0101, 1);
#endif

#if INPUT_ENABLED == true
static usb_audio_input_terminal_descriptor __CCMRAM usb_input_terminal = USB_AUDIO_INPUT_TERMINAL_DESCRIPTOR_INIT(3, 0x0101, 4, 0x0003);
static usb_audio_output_terminal_descriptor __CCMRAM audio_output_terminal = USB_AUDIO_OUTPUT_TERMINAL_DESCRIPTOR_INIT(4, 0x0301, 1);
#endif

#if OUTPUT_ENABLED == true
static usb_interface_descriptor __CCMRAM in_interface_desc_zb = USB_INTERFACE_DESCRIPTOR_INIT(0, 0x01, 0x02, 0x00);
static usb_interface_descriptor __CCMRAM in_interface_desc = USB_INTERFACE_DESCRIPTOR_INIT_ALT(1, 1, 0x01, 0x02, 0x00);

static usb_audio_interface_descriptor __CCMRAM in_audio_interface_desc = USB_AUDIO_INTERFACE_DESCRIPTOR_INIT(2, 0, FORMAT_TAG);
static usb_audio_format_i_descriptor __CCMRAM in_audio_format_desc = USB_AUDIO_FORMAT_I_DESCRIPTOR_INIT(2, SUBFRAME_SIZE, BIT_RESOLUTION, SAMPLE_RATE);

static usb_endpoint_descriptor __CCMRAM in_endpoint_desc;
static usb_audio_endpoint_descriptor __CCMRAM in_audio_endpoint_desc = USB_AUDIO_ENDPOINT_DESCRIPTOR_INIT();
#endif

#if INPUT_ENABLED == true
static usb_interface_descriptor __CCMRAM out_interface_desc_zb = USB_INTERFACE_DESCRIPTOR_INIT(0, 0x01, 0x02, 0x00);
static usb_interface_descriptor __CCMRAM out_interface_desc = USB_INTERFACE_DESCRIPTOR_INIT_ALT(1, 2, 0x01, 0x02, 0x00);

static usb_audio_interface_descriptor __CCMRAM out_audio_interface_desc = USB_AUDIO_INTERFACE_DESCRIPTOR_INIT(3, 0, FORMAT_TAG);
static usb_audio_format_i_descriptor __CCMRAM out_audio_format_desc = USB_AUDIO_FORMAT_I_DESCRIPTOR_INIT(2, SUBFRAME_SIZE, BIT_RESOLUTION, SAMPLE_RATE);

static usb_endpoint_descriptor __CCMRAM out_endpoint_desc;
static usb_audio_endpoint_descriptor __CCMRAM out_audio_endpoint_desc = USB_AUDIO_ENDPOINT_DESCRIPTOR_INIT();

static usb_endpoint_descriptor __CCMRAM synch_endpoint_desc;
#endif

#if OUTPUT_ENABLED == true
static uint8_t __CCMRAM tx_buf[2][OUT_BUF_SIZE + 4];
static uint32_t __CCMRAM tx_active_buf = 0;
static size_t __CCMRAM tx_buf_length = 0;
#endif

#if INPUT_ENABLED == true
static uint8_t __CCMRAM rx_buf[FRAME_SIZE];
static SAMPLE_TYPE __CCMRAM in_buf[2][IN_BUF_SIZE];
static uint32_t __CCMRAM in_read_pos = 0;
static uint32_t __CCMRAM in_write_pos = 0;
static bool __CCMRAM in_filled = false;

static uint32_t __CCMRAM num_samples = 0;
static uint32_t __CCMRAM num_frames = 0;
static uint32_t __CCMRAM in_buf_fill = 0;

static uint32_t __CCMRAM sync_sample_rate;
#endif


static void eof_callback(void)
{
#if INPUT_ENABLED == true
	if ((out_active) && (out_alt_setting != 0))
	{
		num_frames++;

		if (num_frames >= (1 << SYNC_INTERVAL))
		{
			float servo = (((float)in_buf_fill / (float)num_frames) - (float)IN_BUF_TARGET) * SYNC_SERVO_AMOUNT;
			sync_sample_rate = ((float)num_samples / (float)num_frames - servo) * (float)(1 << 14);

			num_frames = 0;
			num_samples = 0;
			in_buf_fill = 0;
		}

		usb_transmit((uint8_t *)&sync_sample_rate, 3, synch_in_ep);
		usb_receive((uint8_t *)(rx_buf), FRAME_SIZE, audio_out_ep);
	}
#endif

#if OUTPUT_ENABLED == true
	if ((in_active) && (in_alt_setting != 0))
	{
		size_t tx_size = tx_buf_length < FRAME_SIZE ? tx_buf_length : FRAME_SIZE;

		usb_transmit(tx_buf[tx_active_buf], tx_size, audio_in_ep);

		if (tx_size < tx_buf_length)
		{
			tx_buf_length -= tx_size;
			memcpy(tx_buf[!tx_active_buf], tx_buf[tx_active_buf] + tx_size, tx_buf_length);
		}
		else
		{
			tx_buf_length = 0;
		}

		tx_active_buf = !tx_active_buf;
	}
#endif
}

#if INPUT_ENABLED == true
static void rx_callback(usb_out_endpoint *ep, uint8_t *buf, size_t count)
{
	int32_t fill = in_write_pos - in_read_pos;
	if (fill < 0)
		fill += IN_BUF_SIZE;

	if (fill >= IN_BUF_TARGET * 2)
		in_filled = true;

	in_buf_fill += fill;

	for (int i = 0; i < count; i += SUBFRAME_SIZE * 2)
	{
		in_buf[0][in_write_pos] = *((SAMPLE_TYPE *)&buf[i]);
		in_buf[1][in_write_pos] = *((SAMPLE_TYPE *)&buf[i + SUBFRAME_SIZE]);

		in_write_pos++;

		if (in_write_pos >= IN_BUF_SIZE)
			in_write_pos = 0;

		if (in_write_pos == in_read_pos)
		{
			in_read_pos++;

			if (in_read_pos >= IN_BUF_SIZE)
				in_read_pos -= IN_BUF_SIZE;
		}
	}
}
#endif

#if OUTPUT_ENABLED == true
static void in_start(usb_in_endpoint *ep)
{
	in_active = true;
}

static void in_stop(usb_in_endpoint *ep)
{
	in_active = false;
	in_alt_setting = 0;
}
#endif

#if INPUT_ENABLED == true
static void out_start(usb_out_endpoint *ep)
{
	out_active = true;
}

static void out_stop(usb_out_endpoint *ep)
{
	out_active = false;
	out_alt_setting = 0;
}
#endif

void audio_usb_out(float *buffer[BLOCK_SIZE])
{
#if OUTPUT_ENABLED == true
	if ((!in_active) || (in_alt_setting == 0))
		return;

	SAMPLE_TYPE sample;

	for (int i = 0; i < BLOCK_SIZE; i++)
	{
		if (tx_buf_length + NUM_STREAMS * SUBFRAME_SIZE > OUT_BUF_SIZE)
			return;

		for (int s = 0; s < NUM_STREAMS; s++)
		{
		#ifdef SCALER
			sample = (SAMPLE_TYPE)(buffer[s][i] * SCALER) >> (sizeof(SAMPLE_TYPE) * 8 - BIT_RESOLUTION);
		#else
			sample = buffer[s][i];
		#endif

			*((SAMPLE_TYPE *)&tx_buf[tx_active_buf][tx_buf_length]) = sample;
			tx_buf_length += SUBFRAME_SIZE;
		}
	}
#endif
}

void audio_usb_in(float *buffer[BLOCK_SIZE])
{
#if INPUT_ENABLED == true
	if ((!out_active) || (out_alt_setting == 0))
		return;

	num_samples += BLOCK_SIZE;

	if (!in_filled)
		return;

	for (int i = 0; i < BLOCK_SIZE; i++)
	{
		if (in_read_pos == in_write_pos)
		{
			in_filled = false;
			break;
		}

		for (int s = 0; s < NUM_STREAMS; s++)
		{
		#ifdef SCALER
			buffer[s][i] += (in_buf[s][in_read_pos] << (sizeof(SAMPLE_TYPE) * 8 - BIT_RESOLUTION)) / (float)SCALER;
		#else
			buffer[s][i] += in_buf[s][in_read_pos];
		#endif
		}

		in_read_pos++;
		if (in_read_pos >= IN_BUF_SIZE)
			in_read_pos = 0;
	}
#endif
}

#if OUTPUT_ENABLED == true
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
#endif

#if INPUT_ENABLED == true
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
				sync_sample_rate = ((float)SAMPLE_RATE / 1000.0f) * (float)(1 << 14);
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
#endif

void audio_usb_init(void)
{
#if OUTPUT_ENABLED == true
	audio_in_ep = usb_add_in_ep(EP_TYPE_ISOCHRONOUS, FRAME_SIZE, FRAME_SIZE, &in_start, &in_stop);
#endif

#if INPUT_ENABLED == true
	audio_out_ep = usb_add_out_ep(EP_TYPE_ISOCHRONOUS, FRAME_SIZE, &out_start, &out_stop);
	usb_set_rx_callback(audio_out_ep, &rx_callback);

	synch_in_ep = usb_add_in_ep(EP_TYPE_ISOCHRONOUS, 8, 8, NULL, NULL);
#endif

#if OUTPUT_ENABLED == true
	usb_config_add_descriptor((usb_descriptor *)&audio_input_terminal);
	usb_uac_add_terminal((usb_descriptor *)&audio_input_terminal);

	usb_config_add_descriptor((usb_descriptor *)&usb_output_terminal);
	usb_uac_add_terminal((usb_descriptor *)&usb_output_terminal);
#endif

#if INPUT_ENABLED == true
	usb_config_add_descriptor((usb_descriptor *)&usb_input_terminal);
	usb_uac_add_terminal((usb_descriptor *)&usb_input_terminal);

	usb_config_add_descriptor((usb_descriptor *)&audio_output_terminal);
	usb_uac_add_terminal((usb_descriptor *)&audio_output_terminal);
#endif

#if (OUTPUT_ENABLED == true) || (INPUT_ENABLED == true)
	int interface_string = usb_config_add_string(INTERFACE_NAME);
#endif

#if OUTPUT_ENABLED == true
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
#endif

#if INPUT_ENABLED == true
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
#endif

	usb_phy_add_eof_callback(&eof_callback);
}