#ifndef __DSP_H
#define __DSP_H

extern void wdsp_process(float in_buffer[][2], float out_buffer[][2], unsigned long int nBlocks);
extern void wdsp_init(unsigned long int sample_rate);

#endif