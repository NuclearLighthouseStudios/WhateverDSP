#ifndef __AUDIO_H
#define __AUDIO_H

#include "conf/audio.h"

extern float audio_in_buffers[2][NUM_STREAMS][BUFFER_SIZE];
extern float audio_out_buffers[2][NUM_STREAMS][BUFFER_SIZE];

extern int audio_in_buffer;
extern int audio_out_buffer;

extern void audio_init(void);
extern void audio_process(void);

#endif