#ifndef __MIDI_USB_H
#define __MIDI_USB_H

#include <stdint.h>
#include "cmsis_compiler.h"

typedef enum
{
	USB_MIDI_JACK_EMBEDDED = 0x01,
	USB_MIDI_JACK_EXTERNAL = 0x02,
} usb_midi_jack_type;


typedef struct __PACKED
{
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;

	uint16_t bcdADC;

	uint16_t wTotalLength;
} usb_midi_header_descriptor;

#define USB_MIDI_HEADER_DESCRIPTOR_INIT()\
(usb_midi_header_descriptor){\
	.bLength = 7,\
	.bDescriptorType = 0x24,\
	.bDescriptorSubtype = 0x01,\
\
	.bcdADC = 0x0100,\
\
	.wTotalLength = 7,\
}

typedef struct __PACKED
{
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;

	uint8_t bJackType;
	uint8_t bJackID;

	uint8_t iJack;
} usb_midi_in_jack_descriptor;

#define USB_MIDI_IN_JACK_DESCRIPTOR_INIT(jack_type, jack_id)\
(usb_midi_in_jack_descriptor){\
	.bLength = 6,\
	.bDescriptorType = 0x24,\
	.bDescriptorSubtype = 0x02,\
\
	.bJackType = jack_type,\
	.bJackID = jack_id,\
}


typedef struct __PACKED
{
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;

	uint8_t bJackType;
	uint8_t bJackID;

	uint8_t bNrInputPins;
	uint8_t bSourceID;
	uint8_t bSourcePin;

	uint8_t iJack;
} usb_midi_out_jack_descriptor;

#define USB_MIDI_OUT_JACK_DESCRIPTOR_INIT(jack_type, jack_id, source_id)\
(usb_midi_out_jack_descriptor){\
	.bLength = 9,\
	.bDescriptorType = 0x24,\
	.bDescriptorSubtype = 0x03,\
\
	.bJackType = jack_type,\
	.bJackID = jack_id,\
\
	.bNrInputPins = 1,\
	.bSourceID = source_id,\
	.bSourcePin = 0x01,\
}


typedef struct __PACKED
{
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;

	uint8_t bNumEmbMIDIJack;
	uint8_t baAssocJackID;
} usb_midi_endpoint_descriptor;

#define USB_MIDI_ENDPOINT_DESCRIPTOR_INIT(jack_id)\
(usb_midi_endpoint_descriptor){\
	.bLength = 5,\
	.bDescriptorType = 0x25,\
	.bDescriptorSubtype = 0x01,\
\
	.bNumEmbMIDIJack = 0x01,\
	.baAssocJackID = jack_id,\
}


extern void midi_usb_init(void);

#endif