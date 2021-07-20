#include <stdbool.h>
#include <string.h>

#include "board.h"
#include "core.h"

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

static usb_endpoint_descriptor __CCMRAM in_ep_desc = USB_ENDPOINT_DESCRIPTOR_INIT_BULK(0x81, 8);
static usb_midi_endpoint_descriptor __CCMRAM midi_in_desc = USB_MIDI_ENDPOINT_DESCRIPTOR_INIT(0x03);

static usb_endpoint_descriptor __CCMRAM out_ep_desc = USB_ENDPOINT_DESCRIPTOR_INIT_BULK(0x01, 8);
static usb_midi_endpoint_descriptor __CCMRAM midi_out_desc = USB_MIDI_ENDPOINT_DESCRIPTOR_INIT(0x01);

static bool tx_ready __CCMRAM = false;
static bool tx_started __CCMRAM = false;

static int midi_interface_num __CCMRAM = 0;

static void tx_callback(usb_in_endpoint *ep, size_t count)
{
	tx_ready = true;
}

static void rx_callback(usb_out_endpoint *ep, uint8_t *buf, size_t count)
{
	int length;
	int cin = buf[0] & 0x0f;

	if ((cin >= 0x08) && (cin <= 0x0e))
	{
		switch (cin)
		{
			case 0x0c:
			case 0x0d:
				length = 2;
				break;

			default:
				length = 3;
		}

		midi_message message;

		int command = (buf[1] & 0xf0) >> 0x04;
		int channel = buf[1] & 0x0f;

		message.interface_mask = 0b1 << midi_interface_num;
		message.length = length - 1;
		message.command = command;
		message.channel = channel;
		memcpy(&(message.data), buf + 2, length - 1);

		midi_receive(&message);
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
	if (!tx_started)
		return;

	tx_ready = false;

	uint8_t cin = message->command;

	tx_buf[0] = cin & 0x0f;
	tx_buf[1] = (message->command << 0x04) | (message->channel & 0x0f);

	memcpy(tx_buf + 2, &(message->data), message->length);

	usb_transmit(tx_buf, message->length + 2, midi_in_ep);
}

static bool midi_usb_can_transmit(void)
{
	return tx_ready;
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

	usb_config_add_descriptor((usb_descriptor *)&in_ep_desc);
	usb_config_add_descriptor((usb_descriptor *)&midi_in_desc);
	usb_config_add_descriptor((usb_descriptor *)&out_ep_desc);
	usb_config_add_descriptor((usb_descriptor *)&midi_out_desc);

	midi_interface_num = midi_add_interface(&midi_usb_transmit, &midi_usb_can_transmit);
}