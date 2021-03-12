#ifndef __SYSTEM_H
#define __SYSTEM_H

#define CLOCK_SPEED 168000000ul

volatile extern unsigned long int sys_ticks;

extern void sys_init(void);

#endif