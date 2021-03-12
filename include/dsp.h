#ifndef __DSP_H
#define __DSP_H

extern void dsp_process(float in_buffer[2][BUFSIZE], float out_buffer[2][BUFSIZE]);
extern void dsp_init(void);

#endif