#ifndef __WDSP_H
#define __WDSP_H

extern void wdsp_init(unsigned long int sample_rate);
extern void wdsp_process(float in_buffer[][2], float out_buffer[][2], unsigned long int nBlocks);
extern void wdsp_idle(void);

#endif