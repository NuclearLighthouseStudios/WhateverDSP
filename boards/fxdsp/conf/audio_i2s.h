#ifndef __CONF_AUDIO_I2S_H
#define __CONF_AUDIO_I2S_H

#include "config.h"

#define BUFFER_SIZE CONFIG_AUDIO_BUFFER_SIZE
#define SAMPLE_RATE CONFIG_AUDIO_SAMPLE_RATE
#define NUM_CHANNELS CONFIG_AUDIO_CHANNELS

#if SAMPLE_RATE == 32000
#define PLLN 56
#define PLLR 2
#define I2SDIV 3
#define I2SODD 0b1
#elif SAMPLE_RATE == 44100
#define PLLN 193
#define PLLR 5
#define I2SDIV 3
#define I2SODD 0b1
#elif SAMPLE_RATE == 48000
#define PLLN 60
#define PLLR 2
#define I2SDIV 2
#define I2SODD 0b1
#elif SAMPLE_RATE == 96000
#define PLLN 96
#define PLLR 2
#define I2SDIV 2
#define I2SODD 0b0
#else
#error Unsupported Sample Rate!
#endif

#endif