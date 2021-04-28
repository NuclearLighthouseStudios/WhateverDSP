#ifndef __SYSTEM_H
#define __SYSTEM_H

volatile extern unsigned long int sys_ticks;

extern void sys_init(void);
extern void sys_idle(void);

#endif