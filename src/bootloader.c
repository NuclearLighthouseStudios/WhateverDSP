#include <stdbool.h>

#include "core.h"
#include "board.h"

#include "system.h"
#include "debug.h"
#include "io.h"
#include "usb_config.h"
#include "usb.h"
#include "bootloader.h"

#include "conf/bootloader.h"

static usb_interface_descriptor __CCMRAM dfu_interface = USB_INTERFACE_DESCRIPTOR_INIT(0, 0xFE, 0x01, 0x01);
static usb_dfu_functional_descriptor __CCMRAM dfu_func_desc = USB_DFU_FUNCTIONAL_DESCRIPTOR_INIT();

static void reset(void)
{
	sys_reset(true);
}

static bool handle_setup(usb_setup_packet *packet, usb_in_endpoint *in_ep, usb_out_endpoint *out_ep)
{
	switch (packet->bRequest)
	{
		// DFU_DETACH
		case 0:
		{
			sys_schedule(&reset);
		}
		break;

		default:
			error("Unknown DFU request %d\n", packet->bRequest);
			return false;
	}

	return true;
}

void bootloader_check(void)
{
	for (int i = 0; i < BOOTLOADER_TIMEOUT; i++)
	{
		io_digital_out(BOOTLOADER_LED, true);
		sys_delay(100);
		io_digital_out(BOOTLOADER_LED, false);
		sys_delay(100);
	}

	if (io_digital_in(BOOTLOADER_BUTTON) == true)
	{
		sys_reset(true);
	}
}

void bootloader_usb_init(void)
{
	int interface_string = usb_config_add_string("WDSP DFU");
	dfu_interface.iInterface = interface_string;
	usb_config_add_interface(&dfu_interface, &handle_setup);
	usb_config_add_descriptor((usb_descriptor *)&dfu_func_desc);
}