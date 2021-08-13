#include <stdbool.h>

#include "core.h"
#include "board.h"

#include "system.h"
#include "debug.h"
#include "usb_config.h"

#include "usb_uac.h"

static usb_interface_descriptor __CCMRAM uac_interface = USB_INTERFACE_DESCRIPTOR_INIT(0, 0x01, 0x01, 0x00);
static usb_uac_header_descriptor __CCMRAM uac_header = USB_UAC_HEADER_INIT();

void usb_uac_add_interface(usb_interface_descriptor *interface)
{
	if (uac_header.bInCollection < USB_UAC_HEADER_MAX_INTERFACES)
	{
		uac_header.BaInterfaceNr[uac_header.bInCollection] = interface->bInterfaceNumber;
		uac_header.bInCollection += 1;
		uac_header.bLength += 1;
		uac_header.wTotalLength += 1;
	}
	else
	{
		panic("Maximum number of interfaces exceeded!\n");
	}
}

void usb_uac_add_terminal(usb_descriptor *terminal)
{
	uac_header.wTotalLength += terminal->bLength;
}

void usb_uac_init(void)
{
	usb_config_add_interface(&uac_interface, NULL);
	usb_config_add_descriptor((usb_descriptor *)&uac_header);
}