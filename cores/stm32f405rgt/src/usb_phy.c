#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "stm32f4xx.h"

#include "core.h"
#include "board.h"

#include "system.h"

#include "usb.h"
#include "usb_phy.h"

#include "conf/usb_phy.h"


#define USB_OTG_FS_DEVICE ((USB_OTG_DeviceTypeDef *)(USB_OTG_FS_PERIPH_BASE + USB_OTG_DEVICE_BASE))
#define USB_OTG_FS_INEP(i) ((USB_OTG_INEndpointTypeDef *)(USB_OTG_FS_PERIPH_BASE + USB_OTG_IN_ENDPOINT_BASE + ((i)*USB_OTG_EP_REG_SIZE)))
#define USB_OTG_FS_OUTEP(i) ((USB_OTG_OUTEndpointTypeDef *)(USB_OTG_FS_PERIPH_BASE + USB_OTG_OUT_ENDPOINT_BASE + ((i)*USB_OTG_EP_REG_SIZE)))
#define USB_OTG_FS_DFIFO(i) *(__IO uint32_t *)(USB_OTG_FS_PERIPH_BASE + USB_OTG_FIFO_BASE + ((i)*USB_OTG_FIFO_SIZE))


static usb_in_endpoint __CCMRAM *usb_in_eps;
static usb_out_endpoint __CCMRAM *usb_out_eps;

#define MAX_EOF_CALLBACKS 4
static int __CCMRAM num_eof_callbacks = 0;
static usb_phy_eof_callback __CCMRAM eof_callbacks[MAX_EOF_CALLBACKS];

volatile uint16_t __CCMRAM usb_phy_frame_num = 0;

static int __CCMRAM fifo_alloc_pos;
static int __CCMRAM fifo_alloc_num;


static int fifo_alloc(size_t size)
{
	size_t word_size = (size + 3) / 4;

	if (fifo_alloc_num == 0)
	{
		MODIFY_REG(USB_OTG_FS->DIEPTXF0_HNPTXFSIZ, USB_OTG_TX0FD_Msk, word_size << USB_OTG_TX0FD_Pos);
		MODIFY_REG(USB_OTG_FS->DIEPTXF0_HNPTXFSIZ, USB_OTG_TX0FSA_Msk, fifo_alloc_pos << USB_OTG_TX0FSA_Pos);
	}
	else
	{
		MODIFY_REG(USB_OTG_FS->DIEPTXF[fifo_alloc_num - 1], USB_OTG_DIEPTXF_INEPTXFD_Msk, word_size << USB_OTG_DIEPTXF_INEPTXFD_Pos);
		MODIFY_REG(USB_OTG_FS->DIEPTXF[fifo_alloc_num - 1], USB_OTG_DIEPTXF_INEPTXSA_Msk, fifo_alloc_pos << USB_OTG_DIEPTXF_INEPTXSA_Pos);
	}

	fifo_alloc_pos += word_size;
	fifo_alloc_num++;

	return fifo_alloc_num - 1;
}


static void fifo_read(uint8_t *buf, size_t size)
{
	while (size > 0)
	{
		uint32_t data = USB_OTG_FS_DFIFO(0);

		if (buf == NULL)
		{
			size = size < 4 ? 0 : size - 4;
			continue;
		}

		for (int i = 0; i < 4; i++)
		{
			*(buf++) = data & 0xFF;
			data >>= 8;
			if (--size <= 0)
				break;
		}
	}
}

static void fifo_write(uint8_t *buf, size_t size, int fifo_num)
{
	uint32_t *wbuf = (uint32_t *)buf;
	int32_t count = (size + 3) / 4;

	while (count > 0)
	{
		USB_OTG_FS_DFIFO(fifo_num) = *wbuf++;
		count--;
	}
}

void OTG_FS_IRQHandler(void)
{
	if (READ_BIT(USB_OTG_FS->GINTSTS, USB_OTG_GINTSTS_USBRST))
	{
		SET_BIT(USB_OTG_FS->GINTSTS, USB_OTG_GINTSTS_USBRST);
		sys_schedule(&usb_reset);
	}

	if (READ_BIT(USB_OTG_FS->GINTSTS, USB_OTG_GINTSTS_EOPF))
	{
		SET_BIT(USB_OTG_FS->GINTSTS, USB_OTG_GINTSTS_EOPF);

		usb_phy_frame_num = ((USB_OTG_FS_DEVICE->DSTS & USB_OTG_DSTS_FNSOF_Msk) >> USB_OTG_DSTS_FNSOF_Pos) + 1;

		for (int i = 0; i < num_eof_callbacks; i++)
			sys_schedule(eof_callbacks[i]);
	}

	if (READ_BIT(USB_OTG_FS->GINTSTS, USB_OTG_GINTSTS_RXFLVL))
	{
		uint32_t status = USB_OTG_FS->GRXSTSP;

		int epnum = status & USB_OTG_GRXSTSP_EPNUM;
		int size = (status & USB_OTG_GRXSTSP_BCNT_Msk) >> USB_OTG_GRXSTSP_BCNT_Pos;
		int type = (status & USB_OTG_GRXSTSP_PKTSTS_Msk) >> USB_OTG_GRXSTSP_PKTSTS_Pos;

		usb_out_endpoint *ep = &(usb_out_eps[epnum]);

		if (size > 0)
		{
			switch (type)
			{
				case 0b0010: // Data packet
				{
					// If reading the data would overflow the rx buffer, read only as much as we can and discard the rest
					if (ep->rx_count + size > ep->rx_size)
					{
						fifo_read((uint8_t *)ep->rx_buffer + ep->rx_count, ep->rx_size - ep->rx_count);
						fifo_read(NULL, ep->rx_count + size - ep->rx_size);
						ep->rx_count = ep->rx_size;
					}
					else
					{
						fifo_read((uint8_t *)ep->rx_buffer + ep->rx_count, size);
						ep->rx_count += size;
					}
					break;
				}

				case 0b0110: // Setup packet
				{
					fifo_read((uint8_t *)&(ep->setup_packet), 8);
					break;
				}
			}
		}
	}

	// OUT endpoint interrupts (receive)
	if (READ_BIT(USB_OTG_FS_DEVICE->DAINT, USB_OTG_DAINT_OEPINT))
	{
		uint32_t ep_int_flags = (USB_OTG_FS_DEVICE->DAINT & USB_OTG_DAINT_OEPINT) >> USB_OTG_DAINT_OEPINT_Pos;
		int epnum = 0;

		while (ep_int_flags)
		{
			if (ep_int_flags & 0b1)
			{
				uint32_t ints = USB_OTG_FS_OUTEP(epnum)->DOEPINT;
				ints &= USB_OTG_FS_DEVICE->DOEPMSK;

				usb_out_endpoint *ep = &(usb_out_eps[epnum]);

				// Setup ready
				if (READ_BIT(ints, USB_OTG_DOEPINT_STUP))
				{
					ep->setup_ready = true;
					MODIFY_REG(USB_OTG_FS_OUTEP(epnum)->DOEPTSIZ, USB_OTG_DOEPTSIZ_STUPCNT_Msk, 3 << USB_OTG_DOEPTSIZ_STUPCNT_Pos);
					SET_BIT(USB_OTG_FS_OUTEP(epnum)->DOEPINT, USB_OTG_DOEPINT_STUP);
				}

				// Receive transfer complete
				if (READ_BIT(ints, USB_OTG_DOEPINT_XFRC))
				{
					if (ep->rx_count < ep->rx_size)
					{
						size_t left = (USB_OTG_FS_OUTEP(epnum)->DOEPTSIZ & USB_OTG_DOEPTSIZ_XFRSIZ_Msk) >> USB_OTG_DOEPTSIZ_XFRSIZ_Pos;

						// If the current transfer went through completely start another one.
						// Bulk transfers should always end with a short or zero length packet.
						if ((left == 0) && (ep->type == EP_TYPE_BULK))
							usb_phy_receive(ep);
						else
							ep->rx_ready = true;
					}
					else
					{
						ep->rx_ready = true;
					}

					SET_BIT(USB_OTG_FS_OUTEP(epnum)->DOEPINT, USB_OTG_DOEPINT_XFRC);
				}
			}

			epnum++;
			ep_int_flags >>= 1;
		}
	}

	// IN endpoint interrupts (transmit)
	if (READ_BIT(USB_OTG_FS_DEVICE->DAINT, USB_OTG_DAINT_IEPINT))
	{
		uint32_t ep_int_flags = (USB_OTG_FS_DEVICE->DAINT & USB_OTG_DAINT_IEPINT) >> USB_OTG_DAINT_IEPINT_Pos;
		int epnum = 0;

		while (ep_int_flags)
		{
			if (ep_int_flags & 0b1)
			{
				uint32_t ints = USB_OTG_FS_INEP(epnum)->DIEPINT;
				ints &= USB_OTG_FS_DEVICE->DIEPMSK | USB_OTG_DIEPINT_TXFE;

				usb_in_endpoint *ep = &(usb_in_eps[epnum]);

				// Transmit fifo empty
				if ((READ_BIT(ints, USB_OTG_DIEPINT_TXFE)) && (READ_BIT(USB_OTG_FS_DEVICE->DIEPEMPMSK, 0b1 << epnum)))
				{
					size_t empty = USB_OTG_FS_INEP(epnum)->DTXFSTS * 4;
					size_t count = (USB_OTG_FS_INEP(epnum)->DIEPTSIZ & USB_OTG_DIEPTSIZ_XFRSIZ_Msk) >> USB_OTG_DIEPTSIZ_XFRSIZ_Pos;

					if (count > empty) count = empty;

					fifo_write(ep->tx_buffer + ep->tx_count, count, ep->fifo_num);
					ep->tx_count += count;

					if (ep->tx_count >= ep->tx_size)
						CLEAR_BIT(USB_OTG_FS_DEVICE->DIEPEMPMSK, 0b1 << epnum);
				}

				// Transmit transfer complete
				if (READ_BIT(ints, USB_OTG_DIEPINT_XFRC))
				{
					if (ep->tx_count >= ep->tx_size)
					{
						// In case our amount of data to send doesn't result in a "short packet" at the end
						// we need to send one more zero size packet to signal the end of transmission for bulk transfers
						if ((ep->tx_size != 0) && (ep->tx_size % ep->max_packet_size == 0) && (ep->type == EP_TYPE_BULK))
						{
							ep->tx_size = 0;
							usb_phy_transmit(ep);
						}
						else
						{
							ep->tx_ready = true;
						}
					}
					else
					{
						usb_phy_transmit(ep);
					}

					SET_BIT(USB_OTG_FS_INEP(epnum)->DIEPINT, USB_OTG_DIEPINT_XFRC);
				}
			}

			epnum++;
			ep_int_flags >>= 1;
		}
	}

}

void usb_phy_cancel_transmit(usb_in_endpoint *ep)
{
	// SET_BIT(USB_OTG_FS_INEP(ep->epnum)->DIEPINT, USB_OTG_DIEPINT_EPDISD);
	SET_BIT(USB_OTG_FS_INEP(ep->epnum)->DIEPCTL, USB_OTG_DIEPCTL_EPDIS | USB_OTG_DIEPCTL_SNAK);
	// while (!READ_BIT(USB_OTG_FS_INEP(ep->epnum)->DIEPINT, USB_OTG_DIEPINT_EPDISD))
	// 	__NOP();

	MODIFY_REG(USB_OTG_FS->GRSTCTL, USB_OTG_GRSTCTL_TXFNUM_Msk, (ep->fifo_num << USB_OTG_GRSTCTL_TXFNUM_Pos) | USB_OTG_GRSTCTL_TXFFLSH);
	while (READ_BIT(USB_OTG_FS->GRSTCTL, USB_OTG_GRSTCTL_TXFFLSH))
		__NOP();
}

void usb_phy_transmit(usb_in_endpoint *ep)
{
	if (READ_BIT(USB_OTG_FS_INEP(ep->epnum)->DIEPCTL, USB_OTG_DIEPCTL_EPENA))
		usb_phy_cancel_transmit(ep);

	int pkg_count = 1;

	long int size = ep->tx_size - ep->tx_count;
	if (size < 0)
		size = 0;

	// EP0 only supports transactions up to 64 bytes
	if ((ep->epnum == 0) && (size > 64))
		size = 64;

	if (size > 0)
	{
		pkg_count = size / ep->max_packet_size;
		if (size % ep->max_packet_size != 0)
			pkg_count += 1;
	}

	USB_OTG_FS_INEP(ep->epnum)->DIEPTSIZ = 0;
	MODIFY_REG(USB_OTG_FS_INEP(ep->epnum)->DIEPTSIZ, USB_OTG_DIEPTSIZ_PKTCNT_Msk, pkg_count << USB_OTG_DIEPTSIZ_PKTCNT_Pos);
	MODIFY_REG(USB_OTG_FS_INEP(ep->epnum)->DIEPTSIZ, USB_OTG_DIEPTSIZ_XFRSIZ_Msk, size << USB_OTG_DIEPTSIZ_XFRSIZ_Pos);

	CLEAR_BIT(USB_OTG_FS_INEP(ep->epnum)->DIEPCTL, USB_OTG_DIEPCTL_STALL);

	if (ep->type == EP_TYPE_ISOCHRONOUS)
	{
		if (usb_phy_frame_num & 0b1)
			SET_BIT(USB_OTG_FS_INEP(ep->epnum)->DIEPCTL, USB_OTG_DIEPCTL_SODDFRM);
		else
			SET_BIT(USB_OTG_FS_INEP(ep->epnum)->DIEPCTL, USB_OTG_DIEPCTL_SD0PID_SEVNFRM);
	}

	SET_BIT(USB_OTG_FS_INEP(ep->epnum)->DIEPCTL, USB_OTG_DIEPCTL_EPENA | USB_OTG_DIEPCTL_CNAK);

	if (size > 0)
	{
		size_t empty = USB_OTG_FS_INEP(ep->epnum)->DTXFSTS * 4;

		if (empty <= 0)
		{
			SET_BIT(USB_OTG_FS_DEVICE->DIEPEMPMSK, 0b1 << ep->epnum);
			return;
		}

		size_t count = (USB_OTG_FS_INEP(ep->epnum)->DIEPTSIZ & USB_OTG_DIEPTSIZ_XFRSIZ_Msk) >> USB_OTG_DIEPTSIZ_XFRSIZ_Pos;

		if (count > empty) count = empty;

		fifo_write(ep->tx_buffer + ep->tx_count, count, ep->fifo_num);
		ep->tx_count += count;

		if (ep->tx_count < ep->tx_size)
			SET_BIT(USB_OTG_FS_DEVICE->DIEPEMPMSK, 0b1 << ep->epnum);
	}
}

void usb_phy_cancel_receive(usb_out_endpoint *ep)
{
	// SET_BIT(USB_OTG_FS_OUTEP(ep->epnum)->DOEPINT, USB_OTG_DOEPINT_EPDISD);
	SET_BIT(USB_OTG_FS_OUTEP(ep->epnum)->DOEPCTL, USB_OTG_DOEPCTL_EPDIS | USB_OTG_DOEPCTL_SNAK);
	// while (!READ_BIT(USB_OTG_FS_OUTEP(ep->epnum)->DOEPINT, USB_OTG_DOEPINT_EPDISD))
		// __NOP();
}

void usb_phy_receive(usb_out_endpoint *ep)
{
	if (READ_BIT(USB_OTG_FS_OUTEP(ep->epnum)->DOEPCTL, USB_OTG_DOEPCTL_EPENA))
		usb_phy_cancel_receive(ep);

	size_t size = ep->rx_size - ep->rx_count;
	int pkg_count = 1;

	// EP0 only supports transactions up to 64 bytes
	if ((ep->epnum == 0) && (size > 64))
		size = 64;

	if (size > 0)
	{
		pkg_count = size / ep->max_packet_size;
		if (size % ep->max_packet_size != 0)
			pkg_count += 1;
	}

	size = pkg_count * ep->max_packet_size;

	USB_OTG_FS_OUTEP(ep->epnum)->DOEPTSIZ = 0U;
	MODIFY_REG(USB_OTG_FS_OUTEP(ep->epnum)->DOEPTSIZ, USB_OTG_DOEPTSIZ_STUPCNT_Msk, 3 << USB_OTG_DOEPTSIZ_STUPCNT_Pos);
	MODIFY_REG(USB_OTG_FS_OUTEP(ep->epnum)->DOEPTSIZ, USB_OTG_DOEPTSIZ_PKTCNT_Msk, pkg_count << USB_OTG_DOEPTSIZ_PKTCNT_Pos);
	MODIFY_REG(USB_OTG_FS_OUTEP(ep->epnum)->DOEPTSIZ, USB_OTG_DOEPTSIZ_XFRSIZ_Msk, size << USB_OTG_DOEPTSIZ_XFRSIZ_Pos);

	CLEAR_BIT(USB_OTG_FS_OUTEP(ep->epnum)->DOEPCTL, USB_OTG_DOEPCTL_STALL);

	if (ep->type == EP_TYPE_ISOCHRONOUS)
	{
		if (usb_phy_frame_num & 0b1)
			SET_BIT(USB_OTG_FS_OUTEP(ep->epnum)->DOEPCTL, USB_OTG_DOEPCTL_SODDFRM);
		else
			SET_BIT(USB_OTG_FS_OUTEP(ep->epnum)->DOEPCTL, USB_OTG_DOEPCTL_SD0PID_SEVNFRM);
	}

	SET_BIT(USB_OTG_FS_OUTEP(ep->epnum)->DOEPCTL, USB_OTG_DOEPCTL_EPENA | USB_OTG_DOEPCTL_CNAK);
}


void usb_phy_in_ep_stall(usb_in_endpoint *ep)
{
	SET_BIT(USB_OTG_FS_INEP(ep->epnum)->DIEPCTL, USB_OTG_DIEPCTL_STALL);
}

void usb_phy_out_ep_stall(usb_out_endpoint *ep)
{
	SET_BIT(USB_OTG_FS_OUTEP(ep->epnum)->DOEPCTL, USB_OTG_DOEPCTL_STALL);
}


void usb_phy_in_ep_init(usb_in_endpoint *ep)
{
	int epnum = ep->epnum;

	USB_OTG_FS_INEP(epnum)->DIEPCTL = 0x00;

	SET_BIT(USB_OTG_FS_INEP(epnum)->DIEPCTL, USB_OTG_DIEPCTL_SNAK | USB_OTG_DIEPCTL_USBAEP);

	int fifo_num = fifo_alloc(ep->fifo_size);
	ep->fifo_num = fifo_num;
	MODIFY_REG(USB_OTG_FS_INEP(epnum)->DIEPCTL, USB_OTG_DIEPCTL_TXFNUM_Msk, fifo_num << USB_OTG_DIEPCTL_TXFNUM_Pos);

	while (READ_BIT(USB_OTG_FS->GRSTCTL, USB_OTG_GRSTCTL_TXFFLSH))
		__NOP();

	if (epnum == 0)
	{
		MODIFY_REG(USB_OTG_FS_INEP(epnum)->DIEPCTL, USB_OTG_DIEPCTL_EPTYP_Msk, EP_TYPE_CONTROL << USB_OTG_DIEPCTL_EPTYP_Pos);
		MODIFY_REG(USB_OTG_FS_INEP(epnum)->DIEPCTL, USB_OTG_DIEPCTL_MPSIZ_Msk, 0x00 << USB_OTG_DIEPCTL_MPSIZ_Pos);
	}
	else
	{
		MODIFY_REG(USB_OTG_FS_INEP(epnum)->DIEPCTL, USB_OTG_DIEPCTL_EPTYP_Msk, ep->type << USB_OTG_DIEPCTL_EPTYP_Pos);
		MODIFY_REG(USB_OTG_FS_INEP(epnum)->DIEPCTL, USB_OTG_DIEPCTL_MPSIZ_Msk, ep->max_packet_size << USB_OTG_DIEPCTL_MPSIZ_Pos);
	}

	USB_OTG_FS_INEP(epnum)->DIEPTSIZ = 0U;
	USB_OTG_FS_INEP(epnum)->DIEPINT = 0xFFFFFFFFu;

	SET_BIT(USB_OTG_FS_DEVICE->DAINTMSK, 0b1 << (USB_OTG_DAINTMSK_IEPM_Pos + epnum));
}

void usb_phy_out_ep_init(usb_out_endpoint *ep)
{
	int epnum = ep->epnum;

	USB_OTG_FS_OUTEP(epnum)->DOEPCTL = 0x00;

	SET_BIT(USB_OTG_FS_OUTEP(epnum)->DOEPCTL, USB_OTG_DOEPCTL_SNAK | USB_OTG_DOEPCTL_USBAEP);

	if (epnum == 0)
	{
		MODIFY_REG(USB_OTG_FS_OUTEP(epnum)->DOEPCTL, USB_OTG_DOEPCTL_EPTYP_Msk, EP_TYPE_CONTROL << USB_OTG_DOEPCTL_EPTYP_Pos);
		MODIFY_REG(USB_OTG_FS_OUTEP(epnum)->DOEPCTL, USB_OTG_DOEPCTL_MPSIZ_Msk, 0x00 << USB_OTG_DOEPCTL_MPSIZ_Pos);
	}
	else
	{
		MODIFY_REG(USB_OTG_FS_OUTEP(epnum)->DOEPCTL, USB_OTG_DOEPCTL_EPTYP_Msk, ep->type << USB_OTG_DOEPCTL_EPTYP_Pos);
		MODIFY_REG(USB_OTG_FS_OUTEP(epnum)->DOEPCTL, USB_OTG_DOEPCTL_MPSIZ_Msk, ep->max_packet_size << USB_OTG_DOEPCTL_MPSIZ_Pos);
	}

	USB_OTG_FS_OUTEP(epnum)->DOEPTSIZ = 0U;
	MODIFY_REG(USB_OTG_FS_OUTEP(epnum)->DOEPTSIZ, USB_OTG_DOEPTSIZ_STUPCNT_Msk, 3 << USB_OTG_DOEPTSIZ_STUPCNT_Pos);
	USB_OTG_FS_OUTEP(epnum)->DOEPINT = 0xFFFFFFFFu;

	SET_BIT(USB_OTG_FS_DEVICE->DAINTMSK, 0b1 << (USB_OTG_DAINTMSK_OEPM_Pos + epnum));
}


void usb_phy_add_eof_callback(usb_phy_eof_callback callback)
{
	if (num_eof_callbacks < MAX_EOF_CALLBACKS)
	{
		eof_callbacks[num_eof_callbacks] = callback;
		num_eof_callbacks++;
	}
}


void usb_phy_set_address(int address)
{
	MODIFY_REG(USB_OTG_FS_DEVICE->DCFG, USB_OTG_DCFG_DAD_Msk, (address & 0x7f) << USB_OTG_DCFG_DAD_Pos);
}


void usb_phy_reset(void)
{
	// Reset device address
	MODIFY_REG(USB_OTG_FS_DEVICE->DCFG, USB_OTG_DCFG_DAD_Msk, 0x00 << USB_OTG_DCFG_DAD_Pos);

	// Reset all endpoints
	for (int i = 0; i < USB_PHY_NUM_EPS; i++)
	{
		USB_OTG_FS_INEP(i)->DIEPCTL = 0x00;
		USB_OTG_FS_OUTEP(i)->DOEPCTL = 0x00;

		SET_BIT(USB_OTG_FS_INEP(i)->DIEPCTL, USB_OTG_DIEPCTL_EPDIS | USB_OTG_DIEPCTL_SNAK | USB_OTG_DIEPCTL_SD0PID_SEVNFRM);
		SET_BIT(USB_OTG_FS_OUTEP(i)->DOEPCTL, USB_OTG_DOEPCTL_EPDIS | USB_OTG_DOEPCTL_SNAK | USB_OTG_DIEPCTL_SD0PID_SEVNFRM);

		USB_OTG_FS_INEP(i)->DIEPTSIZ = 0U;
		USB_OTG_FS_INEP(i)->DIEPINT = 0xFFFFFFFFu;

		USB_OTG_FS_OUTEP(i)->DOEPTSIZ = 0U;
		USB_OTG_FS_OUTEP(i)->DOEPINT = 0xFFFFFFFFu;
	}

	// Set receive fifo size
	size_t rx_fifo_size = (USB_PHY_RX_FIFO_SIZE + 3) / 4;
	USB_OTG_FS->GRXFSIZ = rx_fifo_size;
	fifo_alloc_pos = rx_fifo_size;
	fifo_alloc_num = 0;

	// Flush transmit fifos
	MODIFY_REG(USB_OTG_FS->GRSTCTL, USB_OTG_GRSTCTL_TXFNUM_Msk, (0x10 << USB_OTG_GRSTCTL_TXFNUM_Pos) | USB_OTG_GRSTCTL_TXFFLSH);
	while (READ_BIT(USB_OTG_FS->GRSTCTL, USB_OTG_GRSTCTL_TXFFLSH))
		__NOP();

	// Flush receive fifo
	SET_BIT(USB_OTG_FS->GRSTCTL, USB_OTG_GRSTCTL_RXFFLSH);
	while (READ_BIT(USB_OTG_FS->GRSTCTL, USB_OTG_GRSTCTL_RXFFLSH))
		__NOP();
}

void usb_phy_init(usb_in_endpoint in_eps[], usb_out_endpoint out_eps[])
{
	usb_phy_setup();

	// Reset USB controller
	while (!READ_BIT(USB_OTG_FS->GRSTCTL, USB_OTG_GRSTCTL_AHBIDL))
		__NOP();

	SET_BIT(USB_OTG_FS->GRSTCTL, USB_OTG_GRSTCTL_CSRST);

	while (READ_BIT(USB_OTG_FS->GRSTCTL, USB_OTG_GRSTCTL_CSRST))
		__NOP();

	// Wait a little longer
	sys_delay(50);

	// Wait for AHB idle again
	while (!READ_BIT(USB_OTG_FS->GRSTCTL, USB_OTG_GRSTCTL_AHBIDL))
		__NOP();

	// Power on the transciever
	SET_BIT(USB_OTG_FS->GCCFG, USB_OTG_GCCFG_PWRDWN);

	// Force USB peripheral to device mode only
	CLEAR_BIT(USB_OTG_FS->GUSBCFG, USB_OTG_GUSBCFG_FHMOD);
	SET_BIT(USB_OTG_FS->GUSBCFG, USB_OTG_GUSBCFG_FDMOD);

	// Datasheet says we need to wait at least 25ms after setting device mode
	sys_delay(50);

	// Go into soft disconnect
	SET_BIT(USB_OTG_FS_DEVICE->DCTL, USB_OTG_DCTL_SDIS);

	// Enable VBUS Sensing to know when we're connected
	CLEAR_BIT(USB_OTG_FS->GCCFG, USB_OTG_GCCFG_NOVBUSSENS);
	SET_BIT(USB_OTG_FS->GCCFG, USB_OTG_GCCFG_VBUSBSEN);

	// Set device speed to full speed with internal PHY
	MODIFY_REG(USB_OTG_FS_DEVICE->DCFG, USB_OTG_DCFG_DSPD_Msk, 0b11 << USB_OTG_DCFG_DSPD_Pos);

	usb_in_eps = in_eps;
	usb_out_eps = out_eps;
}

void usb_phy_start(void)
{
	// Enable USB IRQs
	NVIC_EnableIRQ(OTG_FS_IRQn);

	// Clear all pending interrupts
	USB_OTG_FS->GINTSTS = 0xFFFFFFFFU;

	// Enable the interrupts we want
	USB_OTG_FS->GINTMSK =
		USB_OTG_GINTMSK_USBRST | USB_OTG_GINTMSK_RXFLVLM |
		USB_OTG_GINTMSK_IEPINT | USB_OTG_GINTMSK_OEPINT |
		USB_OTG_GINTSTS_EOPF;

	// Get transfer complete interrupts from IN endpoints
	USB_OTG_FS_DEVICE->DIEPMSK = USB_OTG_DIEPMSK_XFRCM;

	// Get setup and transfer complete interrupts from OUT endpoints
	USB_OTG_FS_DEVICE->DOEPMSK = USB_OTG_DOEPMSK_STUPM | USB_OTG_DOEPMSK_XFRCM;

	// Turn on USB interrupts globally
	SET_BIT(USB_OTG_FS->GAHBCFG, USB_OTG_GAHBCFG_GINT);

	// Go out of soft disconnect
	CLEAR_BIT(USB_OTG_FS_DEVICE->DCTL, USB_OTG_DCTL_SDIS);
}