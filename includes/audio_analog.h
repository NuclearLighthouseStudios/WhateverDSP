#ifndef __AUDIO_PHY_H
#define __AUDIO_PHY_H

extern volatile bool audio_analog_adc_ready;
extern volatile bool audio_analog_dac_ready;

extern void audio_analog_init(void);

#endif