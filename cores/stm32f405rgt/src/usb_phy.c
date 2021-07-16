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


static int __CCMRAM fifo_alloc_pos;
static int __CCMRAM fifo_alloc_num;


static int usb_phy_fifo_alloc(size_t size)
{
	if (fifo_alloc_num == 0)
	{
		MODIFY_REG(USB_OTG_FS->DIEPTXF0_HNPTXFSIZ, USB_OTG_TX0FD_Msk, size << USB_OTG_TX0FD_Pos);
		MODIFY_REG(USB_OTG_FS->DIEPTXF0_HNPTXFSIZ, USB_OTG_TX0FSA_Msk, fifo_alloc_pos << USB_OTG_TX0FSA_Pos);
	}
	else
	{
		MODIFY_REG(USB_OTG_FS->DIEPTXF[fifo_alloc_num - 1], USB_OTG_DIEPTXF_INEPTXFD_Msk, size << USB_OTG_DIEPTXF_INEPTXFD_Pos);
		MODIFY_REG(USB_OTG_FS->DIEPTXF[fifo_alloc_num - 1], USB_OTG_DIEPTXF_INEPTXSA_Msk, fifo_alloc_pos << USB_OTG_DIEPTXF_INEPTXSA_Pos);
	}

	fifo_alloc_pos += size;
	fifo_alloc_num++;

	return fifo_alloc_num - 1;
}


static void usb_phy_fifo_read(uint8_t *buf, size_t size)
{
	while (size > 0)
	{
		uint32_t data = USB_OTG_FS_DFIFO(0);

		if (buf == NULL)
			continue;

		for (int i = 0; i < 4; i++)
		{
			*(buf++) = data & 0xFF;
			data >>= 8;
			if (--size <= 0)
				break;
		}
	}
}

static void usb_phy_fifo_write(uint8_t *buf, size_t size, int fifo_num)
{
	while (size > 0)
	{
		uint32_t data = 0x00;

		for (int i = 0; i < 4; i++)
		{
			data >>= 8;
			if (size > 0)
			{
				data |= *(buf++) << 24;
				size--;
			}
		}

		USB_OTG_FS_DFIFO(fifo_num) = data;
	}
}


void usb_phy_transmit(usb_in_endpoint *ep)
{
	size_t size = ep->tx_size;
	int count = 1;

	if (size > 0)
	{
		count = size / ep->max_packet_size;
		if (size % ep->max_packet_size != 0)
			count += 1;
	}

	MODIFY_REG(USB_OTG_FS_INEP(ep->epnum)->DIEPTSIZ, USB_OTG_DIEPTSIZ_PKTCNT_Msk, count << USB_OTG_DIEPTSIZ_PKTCNT_Pos);
	MODIFY_REG(USB_OTG_FS_INEP(ep->epnum)->DIEPTSIZ, USB_OTG_DIEPTSIZ_XFRSIZ_Msk, size << USB_OTG_DIEPTSIZ_XFRSIZ_Pos);

	SET_BIT(USB_OTG_FS_INEP(ep->epnum)->DIEPCTL, USB_OTG_DIEPCTL_EPENA | USB_OTG_DIEPCTL_CNAK);

	if (size > 0)
	{
		SET_BIT(USB_OTG_FS_DEVICE->DIEPEMPMSK, 0b1 << ep->epnum);
	}
}

void usb_phy_receive(usb_out_endpoint *ep)
{
	size_t size = ep->rx_size;
	int count = 1;

	if (size > 0)
	{
		count = size / ep->max_packet_size;
		if (size % ep->max_packet_size != 0)
			count += 1;
	}

	MODIFY_REG(USB_OTG_FS_OUTEP(ep->epnum)->DOEPTSIZ, USB_OTG_DOEPTSIZ_PKTCNT_Msk, count << USB_OTG_DOEPTSIZ_PKTCNT_Pos);
	MODIFY_REG(USB_OTG_FS_OUTEP(ep->epnum)->DOEPTSIZ, USB_OTG_DOEPTSIZ_XFRSIZ_Msk, size << USB_OTG_DOEPTSIZ_XFRSIZ_Pos);

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
	if (epnum == 0)
	{
		USB_OTG_FS_INEP(epnum)->DIEPCTL = 0x00;

		SET_BIT(USB_OTG_FS_INEP(epnum)->DIEPCTL, USB_OTG_DIEPCTL_EPDIS | USB_OTG_DIEPCTL_SNAK);

		int fifo_num = usb_phy_fifo_alloc(ep->fifo_size);
		ep->fifo_num = fifo_num;
		MODIFY_REG(USB_OTG_FS_INEP(epnum)->DIEPCTL, USB_OTG_DIEPCTL_TXFNUM_Msk, fifo_num << USB_OTG_DIEPCTL_TXFNUM_Pos);

		USB_OTG_FS_INEP(epnum)->DIEPTSIZ = 0U;
		USB_OTG_FS_INEP(epnum)->DIEPINT = 0xFFFFFFFFu;
	}
}

void usb_phy_out_ep_init(usb_out_endpoint *ep)
{
	int epnum = ep->epnum;
	if (epnum == 0)
	{
		USB_OTG_FS_OUTEP(epnum)->DOEPCTL = 0x00;

		SET_BIT(USB_OTG_FS_OUTEP(epnum)->DOEPCTL, USB_OTG_DOEPCTL_EPDIS | USB_OTG_DOEPCTL_SNAK);

		USB_OTG_FS_OUTEP(epnum)->DOEPTSIZ = 0U;
		MODIFY_REG(USB_OTG_FS_OUTEP(epnum)->DOEPTSIZ, USB_OTG_DOEPTSIZ_STUPCNT_Msk, 3 << USB_OTG_DOEPTSIZ_STUPCNT_Pos);
		USB_OTG_FS_OUTEP(epnum)->DOEPINT = 0xFFFFFFFFu;
	}
}


void OTG_FS_IRQHandler(void)
{
	if (READ_BIT(USB_OTG_FS->GINTSTS, USB_OTG_GINTSTS_USBRST))
	{
		usb_reset();
		SET_BIT(USB_OTG_FS->GINTSTS, USB_OTG_GINTSTS_USBRST);
	}

	if (READ_BIT(USB_OTG_FS->GINTSTS, USB_OTG_GINTSTS_RXFLVL))
	{
		CLEAR_BIT(USB_OTG_FS->GINTMSK, USB_OTG_GINTSTS_RXFLVL);

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
					usb_phy_fifo_read((uint8_t *)ep->rx_buffer + ep->rx_count, size);
					ep->rx_count += size;
					break;

				case 0b0110: // Setup packet
					usb_phy_fifo_read((uint8_t *)&(ep->setup_packet), 8);
					break;

				default:
					printf("EP:%d, S:%d, T:0x%02x\n", epnum, size, type);
					break;
			}
		}

		SET_BIT(USB_OTG_FS->GINTMSK, USB_OTG_GINTSTS_RXFLVL);
	}

	// OUT endpoint interrupts (receive)
	if (READ_BIT(USB_OTG_FS_DEVICE->DAINT, USB_OTG_DAINT_OEPINT))
	{
		uint32_t eps = (USB_OTG_FS_DEVICE->DAINT & USB_OTG_DAINT_OEPINT) >> USB_OTG_DAINT_OEPINT_Pos;
		int epnum = 0;

		while (eps)
		{
			if (eps & 0b1)
			{
				uint32_t ints = USB_OTG_FS_OUTEP(epnum)->DOEPINT;
				ints &= USB_OTG_FS_DEVICE->DOEPMSK;

				usb_out_endpoint *ep = &(usb_out_eps[epnum]);

				if (READ_BIT(ints, USB_OTG_DOEPINT_STUP))
				{
					ep->setup_ready = true;
					MODIFY_REG(USB_OTG_FS_OUTEP(epnum)->DOEPTSIZ, USB_OTG_DOEPTSIZ_STUPCNT_Msk, 3 << USB_OTG_DOEPTSIZ_STUPCNT_Pos);
					SET_BIT(USB_OTG_FS_OUTEP(epnum)->DOEPINT, USB_OTG_DOEPINT_STUP);
				}

				if (READ_BIT(ints, USB_OTG_DOEPINT_XFRC))
				{
					ep->rx_ready = true;
					SET_BIT(USB_OTG_FS_OUTEP(epnum)->DOEPINT, USB_OTG_DOEPINT_XFRC);
				}
			}

			epnum++;
			eps >>= 1;
		}
	}

	// IN endpoint interrupts (transmit)
	if (READ_BIT(USB_OTG_FS_DEVICE->DAINT, USB_OTG_DAINT_IEPINT))
	{
		uint32_t eps = (USB_OTG_FS_DEVICE->DAINT & USB_OTG_DAINT_IEPINT) >> USB_OTG_DAINT_IEPINT_Pos;
		int epnum = 0;

		while (eps)
		{
			if (eps & 0b1)
			{
				uint32_t ints = USB_OTG_FS_INEP(epnum)->DIEPINT;
				ints &= USB_OTG_FS_DEVICE->DIEPMSK | USB_OTG_DIEPINT_TXFE;

				usb_in_endpoint *ep = &(usb_in_eps[epnum]);

				if ((READ_BIT(ints, USB_OTG_DIEPINT_TXFE)) && (READ_BIT(USB_OTG_FS_DEVICE->DIEPEMPMSK, 0b1 << epnum)))
				{
					size_t empty = USB_OTG_FS_INEP(epnum)->DTXFSTS;

					size_t count = ep->tx_size;
					if (count > empty) count = empty;

					usb_phy_fifo_write(ep->tx_buffer + ep->tx_count, count, ep->fifo_num);
					ep->tx_count += count;

					CLEAR_BIT(USB_OTG_FS_DEVICE->DIEPEMPMSK, 0b1 << epnum);
				}

				if (READ_BIT(ints, USB_OTG_DIEPINT_XFRC))
				{
					ep->tx_ready = true;
					SET_BIT(USB_OTG_FS_INEP(epnum)->DIEPINT, USB_OTG_DIEPINT_XFRC);
				}
			}

			epnum++;
			eps >>= 1;
		}
	}
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

		SET_BIT(USB_OTG_FS_INEP(i)->DIEPCTL, USB_OTG_DIEPCTL_EPDIS | USB_OTG_DIEPCTL_SNAK);
		SET_BIT(USB_OTG_FS_OUTEP(i)->DOEPCTL, USB_OTG_DOEPCTL_EPDIS | USB_OTG_DOEPCTL_SNAK);

		USB_OTG_FS_INEP(i)->DIEPTSIZ = 0U;
		USB_OTG_FS_INEP(i)->DIEPINT = 0xFFFFFFFFu;

		USB_OTG_FS_OUTEP(i)->DOEPTSIZ = 0U;
		USB_OTG_FS_OUTEP(i)->DOEPINT = 0xFFFFFFFFu;
	}

	// Set receive fifo size
	USB_OTG_FS->GRXFSIZ = USB_PHY_RX_FIFO_SIZE;
	fifo_alloc_pos = USB_PHY_RX_FIFO_SIZE;
	fifo_alloc_num = 0;
}

void usb_phy_flush_fifos(void)
{
	// Flush transmit fifos
	SET_BIT(USB_OTG_FS->GRSTCTL, 0x10 << USB_OTG_GRSTCTL_TXFNUM_Pos);
	SET_BIT(USB_OTG_FS->GRSTCTL, USB_OTG_GRSTCTL_TXFFLSH);

	while (READ_BIT(USB_OTG_FS->GRSTCTL, USB_OTG_GRSTCTL_TXFFLSH))
		__NOP();

	// Flush receive fifo
	SET_BIT(USB_OTG_FS->GRSTCTL, USB_OTG_GRSTCTL_RXFFLSH);

	while (READ_BIT(USB_OTG_FS->GRSTCTL, USB_OTG_GRSTCTL_RXFFLSH))
		__NOP();
}


void usb_phy_set_address(int address)
{
	MODIFY_REG(USB_OTG_FS_DEVICE->DCFG, USB_OTG_DCFG_DAD_Msk, (address & 0x7f) << USB_OTG_DCFG_DAD_Pos);
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
	USB_OTG_FS->GINTMSK = USB_OTG_GINTMSK_USBRST | USB_OTG_GINTMSK_RXFLVLM |
		USB_OTG_GINTMSK_IEPINT | USB_OTG_GINTMSK_OEPINT;

	// Get transfer complete interrupts from IN endpoints
	USB_OTG_FS_DEVICE->DIEPMSK = USB_OTG_DIEPMSK_XFRCM;

	// Get setup and transfer complete interrupts from OUT endpoints
	USB_OTG_FS_DEVICE->DOEPMSK = USB_OTG_DOEPMSK_STUPM | USB_OTG_DOEPMSK_XFRCM;

	// Enable interrupts form EP0
	SET_BIT(USB_OTG_FS_DEVICE->DAINTMSK, 0b0001 << USB_OTG_DAINTMSK_OEPM_Pos);
	SET_BIT(USB_OTG_FS_DEVICE->DAINTMSK, 0b0001 << USB_OTG_DAINTMSK_IEPM_Pos);

	// Turn on USB interrupts globally
	SET_BIT(USB_OTG_FS->GAHBCFG, USB_OTG_GAHBCFG_GINT);

	// Go out of soft disconnect
	CLEAR_BIT(USB_OTG_FS_DEVICE->DCTL, USB_OTG_DCTL_SDIS);
}