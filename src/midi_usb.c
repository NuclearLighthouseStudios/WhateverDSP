#include <stdbool.h>
#include <string.h>

#include "board.h"
#include "core.h"

#include "system.h"
#include "usb.h"
#include "usb_config.h"
#include "usb_uac.h"
#include "midi.h"

#include "midi_usb.h"

static usb_in_endpoint __CCMRAM *midi_in_ep;
static usb_out_endpoint __CCMRAM *midi_out_ep;

#define RX_BUF_SIZE 8
static uint8_t __CCMRAM rx_buf[RX_BUF_SIZE];

#define TX_BUF_SIZE 8
static uint8_t __CCMRAM tx_buf[TX_BUF_SIZE];

static usb_interface_descriptor __CCMRAM midi_interface_desc = USB_INTERFACE_DESCRIPTOR_INIT(2, 0x01, 0x03, 0x00);
static usb_midi_header_descriptor __CCMRAM midi_header_desc = USB_MIDI_HEADER_DESCRIPTOR_INIT();

static usb_midi_in_jack_descriptor __CCMRAM emb_in_jack_desc = USB_MIDI_IN_JACK_DESCRIPTOR_INIT(USB_MIDI_JACK_EMBEDDED, 0x01);
static usb_midi_in_jack_descriptor __CCMRAM ext_in_jack_desc = USB_MIDI_IN_JACK_DESCRIPTOR_INIT(USB_MIDI_JACK_EXTERNAL, 0x02);
static usb_midi_out_jack_descriptor __CCMRAM emb_out_jack_desc = USB_MIDI_OUT_JACK_DESCRIPTOR_INIT(USB_MIDI_JACK_EMBEDDED, 0x03, 0x02);
static usb_midi_out_jack_descriptor __CCMRAM ext_out_jack_desc = USB_MIDI_OUT_JACK_DESCRIPTOR_INIT(USB_MIDI_JACK_EXTERNAL, 0x04, 0x01);

static usb_endpoint_descriptor __CCMRAM in_ep_desc;
static usb_midi_endpoint_descriptor __CCMRAM midi_in_desc = USB_MIDI_ENDPOINT_DESCRIPTOR_INIT(0x03);

static usb_endpoint_descriptor __CCMRAM out_ep_desc;
static usb_midi_endpoint_descriptor __CCMRAM midi_out_desc = USB_MIDI_ENDPOINT_DESCRIPTOR_INIT(0x01);

static bool tx_ready __CCMRAM = false;
static bool tx_started __CCMRAM = false;

#define TX_TIMEOUT_TIME 100
static uint32_t tx_timeout __CCMRAM;

static int midi_interface_num __CCMRAM = 0;

static void tx_callback(usb_in_endpoint *ep, size_t count)
{
	tx_ready = true;
}

static void rx_callback(usb_out_endpoint *ep, uint8_t *rx_buf, size_t count)
{
	static midi_command __CCMRAM midi_current_command = 0;
	static unsigned int __CCMRAM midi_current_channel;

	for (int i = 0; i < count;i += 4)
	{
		uint8_t *buf = rx_buf + i;
		int cin = *buf++ & 0x0f;
		int length;

		switch (cin)
		{
			case 0x05:
			case 0x0f:
				length = 1;
				break;

			case 0x02:
			case 0x06:
			case 0x0c:
			case 0x0d:
				length = 2;
				break;

			default:
				length = 3;
		}

		midi_message message;

		if ((cin >= 0x05) && (cin <= 0x07))
		{
			midi_current_command = buf[--length];
			midi_current_channel = 0;
		}
		else if (buf[0] & 0x80)
		{
			if ((buf[0] & 0xf0) != 0xf0)
			{
				midi_current_command = buf[0] & 0xf0;
				midi_current_channel = buf[0] & 0x0f;
			}
			else
			{
				midi_current_command = buf[0];
				midi_current_channel = 0;
			}

			buf++;
			length--;
		}

		message.interface_mask = 0b1 << midi_interface_num;
		message.command = midi_current_command;
		message.channel = midi_current_channel;

		if (midi_current_command == SYSEX)
		{
			for (int i = 0; i < length; i++)
			{
				message.data.sysex.data = buf[i];
				midi_receive(&message);
			}
		}
		else if (midi_current_command == SYSEX_END)
		{
			message.command = SYSEX;
			for (int i = 0; i < length; i++)
			{
				message.data.sysex.data = buf[i];
				midi_receive(&message);
			}

			message.command = SYSEX_END;
			midi_receive(&message);
		}
		else
		{
			memcpy(&(message.data), buf, length);
			midi_receive(&message);
		}
	}

	usb_receive(rx_buf, RX_BUF_SIZE, ep);
}

static void in_start(usb_in_endpoint *ep)
{
	tx_ready = true;
	tx_started = true;
}

static void in_stop(usb_in_endpoint *ep)
{
	tx_ready = true;
	tx_started = false;
}

static void out_start(usb_out_endpoint *ep)
{
	usb_receive(rx_buf, RX_BUF_SIZE, ep);
}

static void midi_usb_transmit(midi_message *message)
{
	static uint8_t __CCMRAM sysex_buffer[3];
	static size_t __CCMRAM sysex_length = 0;
	static bool __CCMRAM sysex_started = false;

	if ((!tx_started) || ((sys_ticks - tx_timeout > TX_TIMEOUT_TIME) && (!tx_ready)))
		return;

	tx_ready = false;
	tx_timeout = sys_ticks;

	uint8_t cin;

	switch (message->command)
	{
		case TIME_CODE:
		case SONG_SELECT:
			cin = 0x02;
			break;

		case SONG_POSITION:
			cin = 0x03;
			break;

		case SYSEX:
			cin = 0x04;
			break;

		case SYSEX_END:
		case TUNE_REQUEST:
			cin = 0x05;

		case CLOCK:
		case START:
		case CONTINUE:
		case STOP:
		case ACTIVE_SENSE:
		case SYS_RESET:
			cin = 0x0f;

		default:
			cin = message->command >> 0x04;
	}

	if (message->command == SYSEX)
	{
		if (!sysex_started)
		{
			sysex_length = 0;
			sysex_started = true;
			sysex_buffer[sysex_length++] = message->command;
		}

		sysex_buffer[sysex_length++] = message->data.sysex.data;

		if (sysex_length == 3)
		{
			tx_buf[0] = cin & 0x0f;
			memcpy(tx_buf + 1, sysex_buffer, sysex_length);
			usb_transmit(tx_buf, sysex_length + 1, midi_in_ep);
			sysex_length = 0;
		}
	}
	else if ((sysex_started) && (message->command == SYSEX_END))
	{
		sysex_started = false;

		tx_buf[0] = (cin + sysex_length) & 0x0f;

		memcpy(tx_buf + 1, sysex_buffer, sysex_length);

		tx_buf[sysex_length + 1] = message->command;

		usb_transmit(tx_buf, sysex_length + 2, midi_in_ep);
		sysex_length = 0;
	}
	else
	{
		tx_buf[0] = cin & 0x0f;

		if ((message->command & 0xf0) != 0xf0)
			tx_buf[1] = (message->command & 0xf0) | (message->channel & 0x0f);
		else
			tx_buf[1] = message->command;

		size_t length = midi_get_message_length(message->command);

		memcpy(tx_buf + 2, &(message->data), length);

		usb_transmit(tx_buf, length + 2, midi_in_ep);
	}
}

static bool midi_usb_can_transmit(void)
{
	if (tx_ready)
		return true;
	else
		return sys_ticks - tx_timeout > TX_TIMEOUT_TIME;
}

void midi_usb_init(void)
{
	midi_in_ep = usb_add_in_ep(EP_TYPE_BULK, 8, 0x18, &in_start, &in_stop);
	usb_set_tx_callback(midi_in_ep, &tx_callback);

	midi_out_ep = usb_add_out_ep(EP_TYPE_BULK, 8, &out_start, NULL);
	usb_set_rx_callback(midi_out_ep, &rx_callback);

	midi_interface_desc.iInterface = usb_config_add_string("USB MIDI Interface");
	usb_config_add_descriptor((usb_descriptor *)&midi_interface_desc);

	usb_uac_add_interface(&midi_interface_desc);

	midi_header_desc.wTotalLength = sizeof(midi_header_desc) +
		sizeof(emb_in_jack_desc) + sizeof(ext_in_jack_desc) +
		sizeof(emb_out_jack_desc) + sizeof(ext_out_jack_desc) +
		sizeof(in_ep_desc) + sizeof(midi_in_desc) +
		sizeof(out_ep_desc) + sizeof(midi_out_desc);
	usb_config_add_descriptor((usb_descriptor *)&midi_header_desc);

	usb_config_add_descriptor((usb_descriptor *)&emb_in_jack_desc);
	usb_config_add_descriptor((usb_descriptor *)&ext_in_jack_desc);
	usb_config_add_descriptor((usb_descriptor *)&emb_out_jack_desc);
	usb_config_add_descriptor((usb_descriptor *)&ext_out_jack_desc);

	in_ep_desc = USB_ENDPOINT_DESCRIPTOR_INIT_BULK(midi_in_ep->epnum | 0x80, 8);
	usb_config_add_descriptor((usb_descriptor *)&in_ep_desc);
	usb_config_add_descriptor((usb_descriptor *)&midi_in_desc);

	out_ep_desc = USB_ENDPOINT_DESCRIPTOR_INIT_BULK(midi_out_ep->epnum, 8);
	usb_config_add_descriptor((usb_descriptor *)&out_ep_desc);
	usb_config_add_descriptor((usb_descriptor *)&midi_out_desc);

	midi_interface_num = midi_add_interface(&midi_usb_transmit, &midi_usb_can_transmit);
}