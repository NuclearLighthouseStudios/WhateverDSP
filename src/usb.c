#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "core.h"
#include "board.h"

#include "system.h"
#include "usb_phy.h"
#include "usb.h"

#include "conf/usb.h"

static usb_in_endpoint __CCMRAM in_eps[NUM_ENDPOINTS];
static usb_out_endpoint __CCMRAM out_eps[NUM_ENDPOINTS];


void usb_transmit(uint8_t *buf, size_t size, usb_in_endpoint *ep)
{
	ep->tx_size = size;
	ep->tx_count = 0;
	ep->tx_buffer = buf;
	ep->tx_ready = false;

	usb_phy_transmit(ep);
}

void usb_receive(uint8_t *buf, size_t size, usb_out_endpoint *ep)
{
	ep->rx_size = size;
	ep->rx_count = 0;
	ep->rx_buffer = buf;
	ep->rx_ready = false;

	usb_phy_receive(ep);
}

void usb_cancel_transmit(usb_in_endpoint *ep)
{
	ep->tx_size = 0;

	usb_phy_cancel_transmit(ep);
}

void usb_cancel_receive(usb_out_endpoint *ep)
{
	ep->rx_size = 0;

	usb_phy_cancel_receive(ep);
}

usb_in_endpoint *usb_add_in_ep(usb_ep_type type, size_t max_packet_size, size_t fifo_size, usb_in_start_callback start_callback, usb_in_stop_callback stop_callback)
{
	int epnum = 0;
	while (in_eps[epnum].active)
	{
		epnum++;

		if (epnum >= NUM_ENDPOINTS)
			return NULL;
	}

	in_eps[epnum].active = true;
	in_eps[epnum].epnum = epnum;

	if (epnum == 0)
	{
		in_eps[epnum].type = EP_TYPE_CONTROL;
		in_eps[epnum].max_packet_size = 64;
	}
	else
	{
		in_eps[epnum].type = type;
		in_eps[epnum].max_packet_size = max_packet_size;
	}

	in_eps[epnum].fifo_size = fifo_size;

	in_eps[epnum].tx_buffer = NULL;
	in_eps[epnum].tx_ready = false;
	in_eps[epnum].tx_callback = NULL;

	in_eps[epnum].start_callback = start_callback;
	in_eps[epnum].stop_callback = stop_callback;

	return &(in_eps[epnum]);
}

usb_out_endpoint *usb_add_out_ep(usb_ep_type type, size_t max_packet_size, usb_out_start_callback start_callback, usb_out_stop_callback stop_callback)
{
	int epnum = 0;
	while (out_eps[epnum].active)
	{
		epnum++;

		if (epnum >= NUM_ENDPOINTS)
			return NULL;
	}

	out_eps[epnum].active = true;
	out_eps[epnum].epnum = epnum;

	if (epnum == 0)
	{
		out_eps[epnum].type = EP_TYPE_CONTROL;
		out_eps[epnum].max_packet_size = 64;
	}
	else
	{
		out_eps[epnum].type = type;
		out_eps[epnum].max_packet_size = max_packet_size;
	}

	out_eps[epnum].rx_buffer = NULL;
	out_eps[epnum].rx_ready = false;
	out_eps[epnum].rx_callback = NULL;

	out_eps[epnum].setup_ready = false;
	out_eps[epnum].setup_callback = NULL;

	out_eps[epnum].start_callback = start_callback;
	out_eps[epnum].stop_callback = stop_callback;

	return &(out_eps[epnum]);
}

void usb_set_rx_callback(usb_out_endpoint *ep, usb_rx_callback callback)
{
	ep->rx_callback = callback;
}

void usb_set_setup_callback(usb_out_endpoint *ep, usb_setup_callback callback)
{
	ep->setup_callback = callback;
}

void usb_set_tx_callback(usb_in_endpoint *ep, usb_tx_callback callback)
{
	ep->tx_callback = callback;
}

void usb_reset(void)
{
	usb_phy_reset();

	for (int i = 0; i < NUM_ENDPOINTS; i++)
	{
		if ((out_eps[i].active) && (out_eps[i].stop_callback))
			out_eps[i].stop_callback(&(out_eps[i]));

		if ((in_eps[i].active) && (in_eps[i].stop_callback))
			in_eps[i].stop_callback(&(in_eps[i]));
	}

	for (int i = 0; i < NUM_ENDPOINTS; i++)
	{
		in_eps[i].tx_ready = false;

		out_eps[i].rx_ready = false;
		out_eps[i].setup_ready = false;
	}

	usb_phy_in_ep_init(&(in_eps[0]));
	usb_phy_out_ep_init(&(out_eps[0]));

	if (out_eps[0].start_callback)
		out_eps[0].start_callback(&(out_eps[0]));

	if (in_eps[0].start_callback)
		in_eps[0].start_callback(&(in_eps[0]));
}

void usb_configure(void)
{
	for (int i = 1; i < NUM_ENDPOINTS; i++)
	{
		if (in_eps[i].active)
			usb_phy_in_ep_init(&(in_eps[i]));
		if (out_eps[i].active)
			usb_phy_out_ep_init(&(out_eps[i]));
	}

	for (int i = 1; i < NUM_ENDPOINTS; i++)
	{
		if ((out_eps[i].active) && (out_eps[i].start_callback))
			out_eps[i].start_callback(&(out_eps[i]));

		if ((in_eps[i].active) && (in_eps[i].start_callback))
			in_eps[i].start_callback(&(in_eps[i]));
	}
}

void usb_process(void)
{
	for (int i = 0; i < NUM_ENDPOINTS; i++)
	{
		if (out_eps[i].active)
		{
			if (out_eps[i].setup_ready)
			{
				out_eps[i].setup_ready = false;
				if (out_eps[i].setup_callback)
					out_eps[i].setup_callback(&(out_eps[i]), &(out_eps[i].setup_packet));
			}

			if (out_eps[i].rx_ready)
			{
				out_eps[i].rx_ready = false;
				if (out_eps[i].rx_callback)
					out_eps[i].rx_callback(&(out_eps[i]), out_eps[i].rx_buffer, out_eps[i].rx_count);
			}
		}

		if ((in_eps[i].active) && (in_eps[i].tx_ready))
		{
			in_eps[i].tx_ready = false;
			if (in_eps[i].tx_callback)
				in_eps[i].tx_callback(&(in_eps[i]), in_eps[i].tx_count);
		}
	}
}

void usb_init(void)
{
	usb_phy_init(in_eps, out_eps);

	for (int i = 0; i < NUM_ENDPOINTS; i++)
	{
		in_eps[i].active = false;
		out_eps[i].active = false;
	}
}

void usb_start(void)
{
	usb_reset();
	usb_phy_start();
}