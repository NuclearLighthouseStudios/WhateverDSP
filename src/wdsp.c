#include "wdsp.h"

// Just some weak defs for optional functions

void __attribute__((weak)) wdsp_init(unsigned long int sample_rate)
{
	return;
}

void __attribute__((weak)) wdsp_idle(void)
{
	return;
}
