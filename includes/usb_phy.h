#ifndef __USB_PHY_H
#define __USB_PHY_H

#include "usb.h"

typedef void (*usb_phy_sof_callback)(uint16_t frame_num);

extern void usb_phy_init(usb_in_endpoint in_eps[], usb_out_endpoint out_eps[]);
extern void usb_phy_start(void);

extern void usb_phy_reset(void);
extern void usb_phy_flush_fifos(void);

extern void usb_phy_set_address(int address);

extern void usb_phy_add_sof_callback(usb_phy_sof_callback callback);

extern void usb_phy_in_ep_init(usb_in_endpoint *ep);
extern void usb_phy_out_ep_init(usb_out_endpoint *ep);

extern void usb_phy_transmit(usb_in_endpoint *ep);
extern void usb_phy_receive(usb_out_endpoint *ep);

extern void usb_phy_in_ep_stall(usb_in_endpoint *ep);
extern void usb_phy_out_ep_stall(usb_out_endpoint *ep);

#endif