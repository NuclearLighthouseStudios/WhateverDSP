#include "wdsp.h"

// Just some weak defs for optional functions

void __attribute__((weak)) wdsp_init(void)
{
	return;
}

void __attribute__((weak)) wdsp_idle(void)
{
	return;
}
