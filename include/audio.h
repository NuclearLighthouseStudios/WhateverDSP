#ifndef __AUDIO_H
#define __AUDIO_H

#define BUFSIZE 64
#define SAMPLE_RATE 48000

extern void audio_init(void);

extern void audio_process(void);

#endif