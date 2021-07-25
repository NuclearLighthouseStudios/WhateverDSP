#ifndef __SYSTEM_H
#define __SYSTEM_H

#include <stdbool.h>

volatile extern unsigned long int sys_ticks;

extern void sys_init(void);
extern void sys_idle(void);

extern void sys_busy(volatile bool *flag);

extern void sys_delay(unsigned long int delay);

extern char *sys_get_serial(void);

#endif