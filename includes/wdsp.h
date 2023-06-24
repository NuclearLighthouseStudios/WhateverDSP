#ifndef __WDSP_H
#define __WDSP_H

#include "conf/wdsp.h"

extern void wdsp_init(void);
extern void wdsp_process(float **in_buffer, float **out_buffer);
extern void wdsp_idle(void);

#endif