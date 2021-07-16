#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "core.h"
#include "board.h"

#include "system.h"
#include "usb.h"
#include "usb_phy.h"
#include "usb_config.h"

#include "conf/usb_config.h"

static usb_in_endpoint __CCMRAM *setup_in_ep;
static usb_out_endpoint __CCMRAM *setup_out_ep;

#define MAX_CONF_DESCS 64
#define MAX_CONF_DESC_SIZE 1024

static usb_descriptor __CCMRAM *usb_conf_descriptors[MAX_CONF_DESCS];
static uint8_t __CCMRAM usb_conf_desc_buffer[MAX_CONF_DESC_SIZE];


static char *string_descriptors[] =
{
	USB_MANUFACTURER_STR,
	USB_PRODUCT_STR,
	NULL,
};

static usb_device_descriptor __CCMRAM device_descriptor =
{
	.bLength = 18,
	.bDescriptorType = 0x01,
	.bcdUSB = 0x0110,

	.bDeviceClass = 0x00,
	.bDeviceSubClass = 0x00,
	.bDeviceProtocol = 0x00,
	.bMaxPacketSize = 64,

	.idVendor = USB_VID,
	.idProduct = USB_DID,
	.bcdDevice = USB_DEVICE_VER,
	.iManufacturer = 1,
	.iProduct = 2,
	.iSerialNumber = 3,

	.bNumConfigurations = 1,
};

static usb_configuration_descriptor __CCMRAM configuration_descriptor =
{
	.bLength = 9,
	.bDescriptorType = 0x02,
	.wTotalLength = 9,

	.bNumInterfaces = 0,
	.bConfigurationValue = 1,
	.iConfiguration = 0,
	.bmAttributes = 0b11000000,
	.bMaxPower = 0,
};


static void send_descriptor(usb_descriptor *desc, size_t length)
{
	size_t size = desc->bLength;

	if (desc->bDescriptorType == 0x02)
		size = ((usb_configuration_descriptor *)desc)->wTotalLength;

	if (size > length)
		size = length;

	usb_transmit((uint8_t *)desc, size, setup_in_ep);
}

static void usb_config_setup(usb_out_endpoint *ep, usb_setup_packet *packet)
{
	switch (packet->bRequest)
	{
		// SET_ADDRESS
		case 5:
		{
			usb_phy_set_address(packet->wValue & 0x7f);
		}
		break;

		// GET_DESCRIPTOR
		case 6:
		{
			int type = (packet->wValue >> 8) & 0xFF;
			int index = packet->wValue & 0xFF;

			switch (type)
			{
				case 1:
				{
					send_descriptor((usb_descriptor *)&device_descriptor, packet->wLength);
				}
				break;

				case 2:
				{
					size_t total_length = 0;
					int num_interfaces = 0;

					for (int i = 0; i < MAX_CONF_DESCS; i++)
					{
						if (usb_conf_descriptors[i] != NULL)
						{
							total_length += usb_conf_descriptors[i]->bLength;

							if (usb_conf_descriptors[i]->bDescriptorType == 0x04)
							{
								((usb_interface_descriptor *)usb_conf_descriptors[i])->bInterfaceNumber = num_interfaces;
								num_interfaces++;
							}
						}
						else
						{
							break;
						}

					}

					configuration_descriptor.wTotalLength = total_length;
					configuration_descriptor.bNumInterfaces = num_interfaces;

					uint8_t *buf = usb_conf_desc_buffer;

					for (int i = 0; i < MAX_CONF_DESCS; i++)
					{
						if (usb_conf_descriptors[i] != NULL)
						{
							memcpy(buf, usb_conf_descriptors[i], usb_conf_descriptors[i]->bLength);
							buf += usb_conf_descriptors[i]->bLength;
						}
						else
						{
							break;
						}

					}

					send_descriptor((usb_descriptor *)usb_conf_desc_buffer, packet->wLength);
				}
				break;

				case 3:
				{
					static usb_string_descriptor __CCMRAM descriptor;

					if (index != 0)
					{
						if (index > sizeof(string_descriptors) / sizeof(*string_descriptors))
						{
							usb_phy_in_ep_stall(setup_in_ep);
							usb_phy_out_ep_stall(setup_out_ep);
							return;
						}

						char *str = string_descriptors[index - 1];

						if (str == NULL)
						{
							usb_phy_in_ep_stall(setup_in_ep);
							usb_phy_out_ep_stall(setup_out_ep);
							return;
						}

						size_t length = strlen(str);
						if (length > USB_CONFIG_MAX_STR_DESC_LENGTH)
							length = USB_CONFIG_MAX_STR_DESC_LENGTH;

						descriptor.bDescriptorType = 3;
						descriptor.bLength = length * 2 + 2;
						for (int i = 0; i < length; i++)
							descriptor.bString[i] = str[i];
					}
					else
					{
						descriptor.bDescriptorType = 3;
						descriptor.bLength = 4;
						descriptor.bString[0] = 0x0409;
					}

					send_descriptor((usb_descriptor *)&descriptor, packet->wLength);
				}
				break;

				default:
				{
					printf("GET_DESC? %d[%d] %d\n", type, index, packet->wLength);
					usb_phy_in_ep_stall(setup_in_ep);
					usb_phy_out_ep_stall(setup_out_ep);
					return;
				}
			}
		}
		break;

		// SET_CONFIGURATION
		case 9:
		{
			usb_configure();
		}
		break;

		default:
		{
			usb_phy_in_ep_stall(setup_in_ep);
			usb_phy_out_ep_stall(setup_out_ep);
			printf("SETUP? %d\n", packet->bRequest);
			return;
		}
	}

	// Do acknowledgement based on data direction
	if (packet->bmRequest & 0x80)
		usb_receive(NULL, 0, setup_out_ep);
	else
		usb_transmit(NULL, 0, setup_in_ep);
}

void usb_config_add_descriptor(usb_descriptor *desc)
{
	for (int i = 0; i < MAX_CONF_DESCS; i++)
	{
		if (usb_conf_descriptors[i] == NULL)
		{
			usb_conf_descriptors[i] = desc;
			break;
		}
	}
}

void usb_config_init(void)
{
	setup_in_ep = usb_add_in_ep(EP_TYPE_CONTROL, 64, 0x18, NULL, NULL);
	setup_out_ep = usb_add_out_ep(EP_TYPE_CONTROL, 64, NULL, NULL);
	usb_set_setup_callback(setup_out_ep, &(usb_config_setup));

	string_descriptors[2] = sys_get_serial();

	for (int i = 0; i < MAX_CONF_DESCS; i++)
		usb_conf_descriptors[i] = NULL;

	usb_conf_descriptors[0] = (usb_descriptor *)&configuration_descriptor;
}