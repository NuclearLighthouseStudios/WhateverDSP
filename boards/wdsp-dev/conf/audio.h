#ifndef __CONF_AUDIO_H
#define __CONF_AUDIO_H

#define pre 0
#define post 1

#include "config.h"

#define BLOCK_SIZE CONFIG_AUDIO_BLOCK_SIZE
#define BUFFER_SIZE CONFIG_AUDIO_BUFFER_SIZE
#define SAMPLE_RATE CONFIG_AUDIO_SAMPLE_RATE
#define NUM_STREAMS 2

#if (CONFIG_MODULES_USB == true) && (CONFIG_AUDIO_USB == true)

#define USB_AUDIO_ENABLED true

#define USB_AUDIO_IN_POS CONFIG_AUDIO_USB_IN_POS
#define USB_AUDIO_OUT_POS CONFIG_AUDIO_USB_OUT_POS

#define USB_INPUT_ENABLED CONFIG_AUDIO_USB_INPUT
#define USB_OUTPUT_ENABLED CONFIG_AUDIO_USB_OUTPUT

#else

#define USB_AUDIO_ENABLED false

#endif

#endif