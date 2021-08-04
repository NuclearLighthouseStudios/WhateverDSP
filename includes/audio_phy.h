#ifndef __AUDIO_PHY_H
#define __AUDIO_PHY_H

extern volatile bool audio_phy_adc_ready;
extern volatile bool audio_phy_dac_ready;

extern void audio_phy_init(void);

#endif