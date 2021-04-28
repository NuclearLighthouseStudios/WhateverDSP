#ifndef __MIDI_PHY_H
#define __MIDI_PHY_H

#include <stdbool.h>

extern void midi_phy_init(void);

extern void midi_phy_transmit(unsigned char *data, unsigned short int length);
extern bool midi_phy_can_transmit(void);

#endif