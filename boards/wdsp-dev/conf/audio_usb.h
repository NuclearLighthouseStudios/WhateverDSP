#ifndef __CONF_AUDIO_USB_H
#define __CONF_AUDIO_USB_H

#include <stdint.h>

#define f32 1
#define s24 2
#define s16 3

#include "config.h"

#define SAMPLE_RATE CONFIG_AUDIO_SAMPLE_RATE

#define SYNC_INTERVAL 6
#define SYNC_SERVO_AMOUNT 0.0001f

#define IN_BUF_SIZE (SAMPLE_RATE/1000*3)
#define IN_BUF_TARGET (IN_BUF_SIZE/3)

#define INTERFACE_NAME CONFIG_AUDIO_USB_INTERFACE_NAME

#if CONFIG_AUDIO_USB_SAMPLE_FORMAT == f32
#define FRAME_SIZE 448
#define FORMAT_TAG 0x0003
#define SAMPLE_TYPE float
#define SUBFRAME_SIZE 4
#define BIT_RESOLUTION 32
#elif CONFIG_AUDIO_USB_SAMPLE_FORMAT == s24
#define FRAME_SIZE 336
#define FORMAT_TAG 0x0001
#define SAMPLE_TYPE int32_t
#define SUBFRAME_SIZE 3
#define BIT_RESOLUTION 24
#define SCALER (float)INT32_MAX
#elif CONFIG_AUDIO_USB_SAMPLE_FORMAT == s16
#define FRAME_SIZE 224
#define FORMAT_TAG 0x0001
#define SAMPLE_TYPE int32_t
#define SUBFRAME_SIZE 2
#define BIT_RESOLUTION 16
#define SCALER (float)INT32_MAX
#else
#error Unsupported Sample Format!
#endif

#endif