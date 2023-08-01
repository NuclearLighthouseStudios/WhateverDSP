#ifndef __BOOTLOADER_H
#define __BOOTLOADER_H

#include <stdbool.h>

#include "cmsis_compiler.h"

typedef struct __PACKED
{
	uint8_t bLength;
	uint8_t bDescriptorType;

	struct __PACKED
	{
		bool bitCanDnload : 1;
		bool bitCanUpload : 1;
		bool bitManifestationTolerant : 1;
		bool bitWillDetach : 1;
	} bmAttributes;

	uint16_t wDetachTimeOut;
	uint16_t wTransferSize;
	uint16_t bcdDFUVersion;
} usb_dfu_functional_descriptor;

#define USB_DFU_FUNCTIONAL_DESCRIPTOR_INIT()\
(usb_dfu_functional_descriptor){\
	.bLength = 0x09,\
	.bDescriptorType = 0x21,\
	.bmAttributes.bitCanDnload = true,\
	.bmAttributes.bitCanUpload = true,\
	.bmAttributes.bitManifestationTolerant = false,\
	.bmAttributes.bitWillDetach = true,\
\
	.wDetachTimeOut = 0x0100,\
	.wTransferSize = 2048,\
	.bcdDFUVersion = 0x011a,\
}

extern void bootloader_check(void);
extern void bootloader_usb_init(void);

#endif