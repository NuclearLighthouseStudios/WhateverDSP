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

#define MAX_CONF_DESCS 32
static usb_descriptor __CCMRAM *conf_descriptors[MAX_CONF_DESCS];
static int __CCMRAM num_conf_descriptors = 0;

#define MAX_CONF_DESC_SIZE 1024
static uint8_t __CCMRAM conf_desc_buffer[MAX_CONF_DESC_SIZE];

static int __CCMRAM num_interfaces = 0;

#define MAX_STRING_DESCS 8
static char *string_descriptors[MAX_STRING_DESCS];
static int __CCMRAM num_string_descriptors = 0;

static usb_device_descriptor __CCMRAM device_descriptor = USB_DEVICE_DESCRIPTOR_INIT(USB_VID, USB_PID, USB_DEVICE_VER);
static usb_configuration_descriptor __CCMRAM configuration_descriptor = USB_CONFIGURATION_DESCRIPTOR_INIT();

static bool __CCMRAM is_data_stage;


static void send_descriptor(usb_descriptor *desc, size_t length)
{
	size_t size = desc->bLength;

	if (desc->bDescriptorType == 0x02)
		size = ((usb_configuration_descriptor *)desc)->wTotalLength;

	if (size > length)
		size = length;

	usb_transmit((uint8_t *)desc, size, setup_in_ep);
}

static void handle_setup(usb_out_endpoint *ep, usb_setup_packet *packet)
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
				case 0x01:
				{
					send_descriptor((usb_descriptor *)&device_descriptor, packet->wLength);
				}
				break;

				case 0x02:
				{
					size_t total_length = 0;

					for (int i = 0; i < num_conf_descriptors; i++)
						total_length += conf_descriptors[i]->bLength;

					configuration_descriptor.wTotalLength = total_length;
					configuration_descriptor.bNumInterfaces = num_interfaces;

					uint8_t *buf = conf_desc_buffer;

					for (int i = 0; i < num_conf_descriptors; i++)
					{
						memcpy(buf, conf_descriptors[i], conf_descriptors[i]->bLength);
						buf += conf_descriptors[i]->bLength;
					}

					send_descriptor((usb_descriptor *)conf_desc_buffer, packet->wLength);
				}
				break;

				case 0x03:
				{
					static usb_string_descriptor __CCMRAM descriptor;

					if (index != 0)
					{
						if (index > num_string_descriptors)
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

						descriptor.bDescriptorType = 0x03;
						descriptor.bLength = length * 2 + 2;
						for (int i = 0; i < length; i++)
							descriptor.bString[i] = str[i];
					}
					else
					{
						descriptor.bDescriptorType = 0x03;
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

		// SET_INTERFACE
		case 11:
		{
			printf("SET_INTERFACE [%d] %d\n", packet->wIndex, packet->wValue);
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

	if (packet->wLength > 0)
	{
		// if the package has data delay the status stage till it's been transferred
		is_data_stage = true;
	}
	else
	{
		is_data_stage = false;

		// Otherwise do acknowledgement based on data direction immediately
		if (packet->bmRequest & 0x80)
			usb_receive(NULL, 0, setup_out_ep);
		else
			usb_transmit(NULL, 0, setup_in_ep);
	}
}

static void tx_done(usb_in_endpoint *ep, size_t count)
{
	if (is_data_stage)
	{
		is_data_stage = false;
		usb_receive(NULL, 0, setup_out_ep);
	}
}

static void rx_done(usb_out_endpoint *ep, uint8_t *buf, size_t count)
{
	if (is_data_stage)
	{
		is_data_stage = false;
		usb_transmit(NULL, 0, setup_in_ep);
	}
}

static void out_start(usb_out_endpoint *ep)
{
	is_data_stage = false;
}


void usb_config_add_descriptor(usb_descriptor *desc)
{
	if (num_conf_descriptors < MAX_CONF_DESCS)
	{
		conf_descriptors[num_conf_descriptors++] = desc;

		if (desc->bDescriptorType == 0x04)
		{
			if (((usb_interface_descriptor *)desc)->bAlternateSetting != 0)
				num_interfaces--;

			((usb_interface_descriptor *)desc)->bInterfaceNumber = num_interfaces;

			num_interfaces++;
		}
	}
}

int usb_config_add_string(char *str)
{
	if (num_string_descriptors < MAX_STRING_DESCS)
	{
		string_descriptors[num_string_descriptors++] = str;
		return num_string_descriptors;
	}
	else
	{
		return 0;
	}
}

void usb_config_init(void)
{
	setup_out_ep = usb_add_out_ep(EP_TYPE_CONTROL, 64, &out_start, NULL);
	usb_set_setup_callback(setup_out_ep, &(handle_setup));
	usb_set_rx_callback(setup_out_ep, &rx_done);

	setup_in_ep = usb_add_in_ep(EP_TYPE_CONTROL, 64, 0x18, NULL, NULL);
	usb_set_tx_callback(setup_in_ep, &tx_done);

	device_descriptor.iManufacturer = usb_config_add_string(USB_MANUFACTURER_STR);
	device_descriptor.iProduct = usb_config_add_string(USB_PRODUCT_STR);
	device_descriptor.iSerialNumber = usb_config_add_string(sys_get_serial());

	usb_config_add_descriptor((usb_descriptor *)&configuration_descriptor);
}