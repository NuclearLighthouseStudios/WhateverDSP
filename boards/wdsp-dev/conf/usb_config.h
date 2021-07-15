#ifndef __CONF_USB_H
#define __CONF_USB_H

#include "usb_config.h"

static usb_device_descriptor device_descriptor =
{
	.bLength = 18,
	.bDescriptorType = 0x01,
	.bcdUSB = 0x0110,

	.bDeviceClass = 0x00,
	.bDeviceSubClass = 0x00,
	.bDeviceProtocol = 0x00,
	.bMaxPacketSize = 64,

	.idVendor = 0xb00f,
	.idProduct = 0xb00f,
	.bcdDevice = 0x1234,
	.iManufacturer = 1,
	.iProduct = 2,
	.iSerialNumber = 3,

	.bNumConfigurations = 1,
};

static usb_configuration_descriptor configuration_descriptor =
{
	.bLength = 9,
	.bDescriptorType = 0x02,
	.wTotalLength = 9,

	.bNumInterfaces = 0,
	.bConfigurationValue = 0,
	.iConfiguration = 0,
	.bmAttributes = 0b11000000,
	.bMaxPower = 0,
};

static char *string_descriptors[] =
{
	"Dicksmashers Inc.",
	"Ultra Dick Smasher",
	"420"
};

#endif