#ifndef __CONF_SYSTEM_H
#define __CONF_SYSTEM_H

#include "stm32f4xx.h"

#define CLOCK_SPEED 168000000ul

#define PLL_M 8
#define PLL_N 168
#define PLL_P 0b00
#define PLL_Q 7

#define PPRE1 RCC_CFGR_PPRE1_DIV4
#define PPRE2 RCC_CFGR_PPRE2_DIV2
#define HPRE RCC_CFGR_HPRE_DIV1

#endif