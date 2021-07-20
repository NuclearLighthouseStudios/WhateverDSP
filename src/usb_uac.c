#include <stdbool.h>
#include <stdio.h>

#include "core.h"
#include "board.h"

#include "system.h"
#include "usb_config.h"

#include "usb_uac.h"

static usb_uac_header_descriptor __CCMRAM uac_header = USB_UAC_HEADER_INIT();
static usb_interface_descriptor __CCMRAM uac_interface = USB_INTERFACE_DESCRIPTOR_INIT(0, 0x01, 0x01, 0x00);

void usb_uac_add_interface(usb_interface_descriptor *interface)
{
	if (uac_header.bInCollection < USB_UAC_HEADER_MAX_INTERFACES)
	{
		uac_header.BaInterfaceNr[uac_header.bInCollection] = interface->bInterfaceNumber;
		uac_header.bInCollection += 1;
		uac_header.bLength += 1;
		uac_header.wTotalLength += 1;
	}
}

void usb_uac_init(void)
{
	usb_config_add_descriptor((usb_descriptor *)&uac_interface);
	usb_config_add_descriptor((usb_descriptor *)&uac_header);
}