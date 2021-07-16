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


#define USB_CONFIG_MAX_STR_DESC_LENGTH 64

typedef struct __PACKED
{
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint16_t bString[USB_CONFIG_MAX_STR_DESC_LENGTH];
} usb_string_descriptor;


extern void usb_config_init(void);
extern void usb_config_add_descriptor(usb_descriptor *desc);

#endif