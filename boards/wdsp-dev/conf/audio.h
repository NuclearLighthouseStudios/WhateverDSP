#ifndef __CONF_AUDIO_H
#define __CONF_AUDIO_H

#include "config.h"

#define BLOCK_SIZE CONFIG_AUDIO_BLOCK_SIZE
#define SAMPLE_RATE CONFIG_AUDIO_SAMPLE_RATE

#if SAMPLE_RATE == 44100
#define PLLN 79
#define PLLR 2
#define I2SDIV 3
#define I2SODD 0b1
#elif SAMPLE_RATE == 45000
#define PLLN 144
#define PLLR 5
#define I2SDIV 2
#define I2SODD 0b1
#elif SAMPLE_RATE == 48000
#define PLLN 86
#define PLLR 2
#define I2SDIV 3
#define I2SODD 0b1
#elif SAMPLE_RATE == 50000
#define PLLN 64
#define PLLR 2
#define I2SDIV 2
#define I2SODD 0b1
#else
#error Unsupported Sample Rate!
#endif

#endif