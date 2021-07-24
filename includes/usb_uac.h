#ifndef __USB_UAC_H
#define __USB_UAC_H

#define USB_UAC_HEADER_MAX_INTERFACES 8

typedef struct __PACKED
{
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bDescriptorSubtype;

	uint16_t bcdADC;

	uint16_t wTotalLength;

	uint8_t bInCollection;
	uint8_t BaInterfaceNr[USB_UAC_HEADER_MAX_INTERFACES];
} usb_uac_header_descriptor;

#define USB_UAC_HEADER_INIT()\
(usb_uac_header_descriptor){\
	.bLength = 8,\
	.bDescriptorType = 0x24,\
	.bDescriptorSubtype = 0x01,\
\
	.bcdADC = 0x0100,\
\
	.wTotalLength = 8,\
\
	.bInCollection = 0,\
	.BaInterfaceNr = {0},\
}

static inline usb_uac_header_descriptor usb_uac_header_init(void)
{
	usb_uac_header_descriptor foo = { .bLength = 66 };
	return foo;
}

extern void usb_uac_init(void);

extern void usb_uac_add_interface(usb_interface_descriptor *interface);

#endif