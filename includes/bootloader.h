#ifndef __BOOTLOADER_H
#define __BOOTLOADER_H

#include "io.h"

extern void bootloader_check(io_pin_idx led, io_pin_idx button);

#endif