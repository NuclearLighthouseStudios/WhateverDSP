#ifndef __USB_H
#define __USB_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

typedef struct __PACKED
{
	uint8_t bmRequestType;
	uint8_t bRequest;
	uint16_t wValue;
	uint16_t wIndex;
	uint16_t wLength;
} usb_setup_packet;

typedef enum
{
	EP_TYPE_CONTROL = 0b00,
	EP_TYPE_ISOCHRONOUS = 0b01,
	EP_TYPE_BULK = 0b10,
	EP_TYPE_INTERRUPT = 0b11,
} usb_ep_type;


typedef struct usb_in_endpoint usb_in_endpoint;
typedef struct usb_out_endpoint usb_out_endpoint;

typedef void (*usb_tx_callback)(usb_in_endpoint *ep, size_t count);
typedef void (*usb_in_start_callback)(usb_in_endpoint *ep);
typedef void (*usb_in_stop_callback)(usb_in_endpoint *ep);

typedef void (*usb_rx_callback)(usb_out_endpoint *ep, uint8_t *buf, size_t count);
typedef void (*usb_setup_callback)(usb_out_endpoint *ep, usb_setup_packet *packet);
typedef void (*usb_out_start_callback)(usb_out_endpoint *ep);
typedef void (*usb_out_stop_callback)(usb_out_endpoint *ep);


struct usb_in_endpoint
{
	bool active;
	int epnum;

	usb_ep_type type;
	size_t max_packet_size;

	size_t fifo_size;
	int fifo_num;

	size_t tx_size;
	size_t tx_count;
	uint8_t *tx_buffer;
	bool tx_ready;

	usb_tx_callback tx_callback;
	usb_in_start_callback start_callback;
	usb_in_stop_callback stop_callback;
};

struct usb_out_endpoint
{
	bool active;
	int epnum;

	usb_ep_type type;
	size_t max_packet_size;

	usb_setup_packet setup_packet;
	bool setup_ready;

	size_t rx_size;
	size_t rx_count;
	uint8_t *rx_buffer;
	bool rx_ready;

	usb_setup_callback setup_callback;
	usb_rx_callback rx_callback;
	usb_out_start_callback start_callback;
	usb_out_stop_callback stop_callback;
};


extern void usb_init(void);
extern void usb_start(void);
extern void usb_reset(void);

extern void usb_configure(void);

extern void usb_process(void);

extern usb_in_endpoint *usb_add_in_ep(usb_ep_type type, size_t max_packet_size, size_t fifo_size, usb_in_start_callback start_callback, usb_in_stop_callback stop_callback);
extern usb_out_endpoint *usb_add_out_ep(usb_ep_type type, size_t max_packet_size, usb_out_start_callback start_callback, usb_out_stop_callback stop_callback);

extern void usb_set_rx_callback(usb_out_endpoint *ep, usb_rx_callback callback);
extern void usb_set_setup_callback(usb_out_endpoint *ep, usb_setup_callback callback);
extern void usb_set_tx_callback(usb_in_endpoint *ep, usb_tx_callback callback);

extern void usb_transmit(uint8_t *buf, size_t size, usb_in_endpoint *ep);
extern void usb_receive(uint8_t *buf, size_t size, usb_out_endpoint *ep);

extern void usb_cancel_transmit(usb_in_endpoint *ep);
extern void usb_cancel_receive(usb_out_endpoint *ep);

#endif