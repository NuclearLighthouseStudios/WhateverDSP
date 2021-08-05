#ifndef __WDSP_H
#define __WDSP_H

#include "conf/wdsp.h"

extern void wdsp_init(void);
extern void wdsp_process(float *in_buffer[BLOCK_SIZE], float *out_buffer[BLOCK_SIZE]);
extern void wdsp_idle(void);

#endif