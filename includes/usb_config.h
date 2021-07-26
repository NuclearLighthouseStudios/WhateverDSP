#ifndef __USB_CONFIG_H
#define __USB_CONFIG_H

#include <stdbool.h>

#include "cmsis_compiler.h"

typedef struct __PACKED
{
	uint8_t bLength;
	uint8_t bDescriptorType;
} usb_descriptor;


typedef struct __PACKED
{
	uint8_t bLength;
	uint8_t bDescriptorType;

	uint16_t bcdUSB;

	uint8_t bDeviceClass;
	uint8_t bDeviceSubClass;
	uint8_t bDeviceProtocol;
	uint8_t bMaxPacketSize;

	uint16_t idVendor;
	uint16_t idProduct;
	uint16_t bcdDevice;
	uint8_t iManufacturer;
	uint8_t iProduct;
	uint8_t iSerialNumber;

	uint8_t bNumConfigurations;
} usb_device_descriptor;

#define USB_DEVICE_DESCRIPTOR_INIT(vid, pid, device_ver) \
(usb_device_descriptor){\
	.bLength = 18,\
	.bDescriptorType = 0x01,\
	.bcdUSB = 0x0110,\
\
	.bDeviceClass = 0x00,\
	.bDeviceSubClass = 0x00,\
	.bDeviceProtocol = 0x00,\
	.bMaxPacketSize = 64,\
\
	.idVendor = vid,\
	.idProduct = pid,\
	.bcdDevice = device_ver,\
	.iManufacturer = 0,\
	.iProduct = 0,\
	.iSerialNumber = 0,\
\
	.bNumConfigurations = 1,\
}


typedef struct __PACKED
{
	uint8_t bLength;
	uint8_t bDescriptorType;

	uint16_t wTotalLength;

	uint8_t bNumInterfaces;
	uint8_t bConfigurationValue;
	uint8_t iConfiguration;
	uint8_t bmAttributes;
	uint8_t bMaxPower;
} usb_configuration_descriptor;

#define USB_CONFIGURATION_DESCRIPTOR_INIT()\
(usb_configuration_descriptor){\
	.bLength = 9,\
	.bDescriptorType = 0x02,\
\
	.wTotalLength = 9,\
\
	.bNumInterfaces = 0,\
	.bConfigurationValue = 1,\
	.iConfiguration = 0,\
	.bmAttributes = 0b11000000,\
	.bMaxPower = 0,\
}


typedef struct __PACKED
{
	uint8_t bLength;
	uint8_t bDescriptorType;

	uint8_t bInterfaceNumber;
	uint8_t bAlternateSetting;
	uint8_t bNumEndpoints;

	uint8_t bInterfaceClass;
	uint8_t bInterfaceSubClass;
	uint8_t bInterfaceProtocol;

	uint8_t  iInterface;
} usb_interface_descriptor;

#define USB_INTERFACE_DESCRIPTOR_INIT(num_endpoints, class, sub_class, protocol)\
(usb_interface_descriptor){\
	.bLength = 9,\
	.bDescriptorType = 0x04,\
\
	.bInterfaceNumber = 0,\
	.bAlternateSetting = 0,\
	.bNumEndpoints = num_endpoints,\
\
	.bInterfaceClass = class,\
	.bInterfaceSubClass = sub_class,\
	.bInterfaceProtocol = protocol,\
\
	.iInterface = 0,\
}

#define USB_INTERFACE_DESCRIPTOR_INIT_ALT(alternate_setting, num_endpoints, class, sub_class, protocol)\
(usb_interface_descriptor){\
	.bLength = 9,\
	.bDescriptorType = 0x04,\
\
	.bInterfaceNumber = 0,\
	.bAlternateSetting = alternate_setting,\
	.bNumEndpoints = num_endpoints,\
\
	.bInterfaceClass = class,\
	.bInterfaceSubClass = sub_class,\
	.bInterfaceProtocol = protocol,\
\
	.iInterface = 0,\
}


typedef struct __PACKED
{
	uint8_t bLength;
	uint8_t bDescriptorType;

	uint8_t bEndpointAddress;

	struct __PACKED
	{
		uint8_t type : 2;
		uint8_t sync : 2;
		uint8_t usage : 2;
		uint8_t : 2;
	} bmAttributes;

	uint16_t wMaxPacketSize;

	uint8_t bInterval;
} usb_endpoint_descriptor;

#define USB_ENDPOINT_DESCRIPTOR_INIT_BULK(endpoint_address, max_packet_size)\
(usb_endpoint_descriptor){\
	.bLength = 7,\
	.bDescriptorType = 0x05,\
\
	.bEndpointAddress = endpoint_address,\
	.bmAttributes.type = EP_TYPE_BULK,\
	.wMaxPacketSize = max_packet_size,\
}

#define USB_ENDPOINT_DESCRIPTOR_INIT_ISO(endpoint_address, sync_type, usage_type, max_packet_size)\
(usb_endpoint_descriptor){\
	.bLength = 7,\
	.bDescriptorType = 0x05,\
\
	.bEndpointAddress = endpoint_address,\
	.bmAttributes.type = EP_TYPE_ISOCHRONOUS,\
	.bmAttributes.sync = sync_type,\
	.bmAttributes.usage = usage_type,\
	.bInterval = 1,\
	.wMaxPacketSize = max_packet_size,\
}


#define USB_CONFIG_MAX_STR_DESC_LENGTH 64

typedef struct __PACKED
{
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint16_t bString[USB_CONFIG_MAX_STR_DESC_LENGTH];
} usb_string_descriptor;


extern void usb_config_init(void);
extern void usb_config_add_descriptor(usb_descriptor *desc);
extern int usb_config_add_string(char *str);

#endif