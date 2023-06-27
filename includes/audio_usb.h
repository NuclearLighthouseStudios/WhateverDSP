#ifndef __AUDIO_USB_H
#define __AUDIO_USB_H

#include <stdint.h>
#include "cmsis_compiler.h"

#include "conf/audio_usb.h"

typedef struct __PACKED
{
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;

	uint8_t bTerminalID;
	uint16_t wTerminalType;

	uint8_t bAssocTerminal;
	uint8_t bNrChannels;
	uint16_t wChannelConfig;

	uint8_t iChannelNames;
	uint8_t iTerminal;
} usb_audio_input_terminal_descriptor;

#define USB_AUDIO_INPUT_TERMINAL_DESCRIPTOR_INIT(terminal_id, terminal_type, num_channels, channel_config)\
(usb_audio_input_terminal_descriptor){\
	.bLength = 12,\
	.bDescriptorType = 0x24,\
	.bDescriptorSubtype = 0x02,\
\
	.bTerminalID = terminal_id,\
	.wTerminalType = terminal_type,\
\
	.bAssocTerminal = 0x00,\
	.bNrChannels = num_channels,\
	.wChannelConfig = channel_config,\
\
	.iChannelNames = 0,\
	.iTerminal = 0,\
}


typedef struct __PACKED
{
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;

	uint8_t bTerminalID;
	uint16_t wTerminalType;

	uint8_t bAssocTerminal;
	uint8_t bSourceID;

	uint8_t iTerminal;
} usb_audio_output_terminal_descriptor;

#define USB_AUDIO_OUTPUT_TERMINAL_DESCRIPTOR_INIT(terminal_id, terminal_type, source_id)\
(usb_audio_output_terminal_descriptor){\
	.bLength = 9,\
	.bDescriptorType = 0x24,\
	.bDescriptorSubtype = 0x03,\
\
	.bTerminalID = terminal_id,\
	.wTerminalType = terminal_type,\
\
	.bAssocTerminal = 0x00,\
	.bSourceID = source_id,\
\
	.iTerminal = 0,\
}


typedef struct __PACKED
{
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;

	uint8_t bTerminalLink;
	uint8_t bDelay;
	uint16_t wFormatTag;
} usb_audio_interface_descriptor;

#define USB_AUDIO_INTERFACE_DESCRIPTOR_INIT(terminal_link, delay, format_tag)\
(usb_audio_interface_descriptor){\
	.bLength = 7,\
	.bDescriptorType = 0x24,\
	.bDescriptorSubtype = 0x01,\
\
	.bTerminalLink = terminal_link,\
	.bDelay = delay,\
	.wFormatTag = format_tag,\
}


typedef struct __PACKED
{
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;

	uint8_t bFormatType;
	uint8_t bNrChannels;
	uint8_t bSubFrameSize;
	uint8_t bBitResolution;
	uint8_t bSamFreqType;
	uint32_t tSamFreq : 24;
} usb_audio_format_i_descriptor;

#define USB_AUDIO_FORMAT_I_DESCRIPTOR_INIT(num_channels, subframe_size, bit_resolution, sample_freq)\
(usb_audio_format_i_descriptor){\
	.bLength = 11,\
	.bDescriptorType = 0x24,\
	.bDescriptorSubtype = 0x02,\
	.bFormatType = 0x01,\
\
	.bNrChannels = num_channels,\
	.bSubFrameSize = subframe_size,\
	.bBitResolution = bit_resolution,\
	.bSamFreqType = 0x01,\
	.tSamFreq = sample_freq,\
}


typedef struct __PACKED
{
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;

	uint8_t bmAttributes;
	uint8_t bLockDelayUnits;
	uint16_t wLockDelay;
} usb_audio_endpoint_descriptor;

#define USB_AUDIO_ENDPOINT_DESCRIPTOR_INIT()\
(usb_audio_endpoint_descriptor){\
	.bLength = 0x07,\
	.bDescriptorType = 0x25,\
	.bDescriptorSubtype = 0x01,\
\
	.bmAttributes = 0x00,\
	.bLockDelayUnits = 0x00,\
	.wLockDelay = 0x00,\
}


#define USB_ENDPOINT_DESCRIPTOR_INIT_AUDIO(endpoint_address, sync_type, usage_type, sync_address, max_packet_size)\
(usb_endpoint_descriptor){\
	.bLength = 9,\
	.bDescriptorType = 0x05,\
\
	.bEndpointAddress = endpoint_address,\
	.bmAttributes.type = EP_TYPE_ISOCHRONOUS,\
	.bmAttributes.sync = sync_type,\
	.bmAttributes.usage = usage_type,\
	.bInterval = 1,\
	.wMaxPacketSize = max_packet_size,\
\
	.bRefresh = 0,\
	.bSynchAddress = sync_address\
}

#define USB_ENDPOINT_DESCRIPTOR_INIT_SYNCH(endpoint_address, sync_type, usage_type, refresh, max_packet_size)\
(usb_endpoint_descriptor){\
	.bLength = 9,\
	.bDescriptorType = 0x05,\
\
	.bEndpointAddress = endpoint_address,\
	.bmAttributes.type = EP_TYPE_ISOCHRONOUS,\
	.bmAttributes.sync = sync_type,\
	.bmAttributes.usage = usage_type,\
	.bInterval = 1,\
	.wMaxPacketSize = max_packet_size,\
\
	.bRefresh = refresh,\
	.bSynchAddress = 0\
}


extern void audio_usb_init(void);

extern void audio_usb_out(float *buffer[]);
extern void audio_usb_in(float *buffer[]);

#endif