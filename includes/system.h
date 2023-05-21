#ifndef __SYSTEM_H
#define __SYSTEM_H

#include <stdbool.h>

volatile extern unsigned long int sys_ticks;

typedef void (*sys_schedulable_func)(void);

extern void sys_init(void);
extern void sys_idle(void);

extern void sys_reset(bool enter_bootloader);

extern void sys_busy(volatile bool *flag);
extern void sys_schedule(sys_schedulable_func func);

extern void sys_delay(unsigned long int delay);

extern char *sys_get_serial(void);

#endif