#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "core.h"
#include "board.h"

#include "usb.h"
#include "usb_phy.h"
#include "usb_config.h"

#include "conf/usb_config.h"

static usb_in_endpoint __CCMRAM *setup_in_ep;
static usb_out_endpoint __CCMRAM *setup_out_ep;

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
					size_t size = sizeof(device_descriptor);

					if (size > packet->wLength)
						size = packet->wLength;

					usb_transmit((uint8_t *)&device_descriptor, size, setup_in_ep);
				}
				break;

				case 2:
				{
					size_t size = sizeof(configuration_descriptor);

					if (size > packet->wLength)
						size = packet->wLength;

					usb_transmit((uint8_t *)&configuration_descriptor, size, setup_in_ep);
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

					size_t size = descriptor.bLength;

					if (size > packet->wLength)
						size = packet->wLength;

					usb_transmit((uint8_t *)&descriptor, size, setup_in_ep);
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
			unsigned char conf = packet->wValue;
			printf("SET_CONF %d\n", conf);
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

void usb_config_init(void)
{
	setup_in_ep = usb_add_in_ep(EP_TYPE_CONTROL, 64, 0x80);
	setup_out_ep = usb_add_out_ep(EP_TYPE_CONTROL, 64);
	usb_set_setup_callback(setup_out_ep, &(usb_config_setup));
}