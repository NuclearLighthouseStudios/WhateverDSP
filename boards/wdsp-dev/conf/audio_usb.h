#ifndef __CONF_AUDIO_USB_H
#define __CONF_AUDIO_USB_H

#include <stdint.h>

#define f32 1
#define s24 2
#define s16 3

#include "config.h"

#define SAMPLE_RATE CONFIG_AUDIO_SAMPLE_RATE
#define FRAME_SIZE 480

#define INTERFACE_NAME CONFIG_AUDIO_USB_INTERFACE_NAME

#if CONFIG_AUDIO_USB_SAMPLE_FORMAT == f32
#define FORMAT_TAG 0x0003
#define SAMPLE_TYPE float
#define SUBFRAME_SIZE 4
#define BIT_RESOLUTION 32
#elif CONFIG_AUDIO_USB_SAMPLE_FORMAT == s24
#define FORMAT_TAG 0x0001
#define SAMPLE_TYPE int32_t
#define SUBFRAME_SIZE 3
#define BIT_RESOLUTION 24
#define SCALER (float)INT32_MAX
#elif CONFIG_AUDIO_USB_SAMPLE_FORMAT == s16
#define FORMAT_TAG 0x0001
#define SAMPLE_TYPE int32_t
#define SUBFRAME_SIZE 2
#define BIT_RESOLUTION 16
#define SCALER (float)INT32_MAX
#else
#error Unsupported Sample Format!
#endif

#endif