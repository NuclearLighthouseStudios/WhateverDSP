#include <stdbool.h>

#include "core.h"
#include "board.h"

#include "system.h"
#include "io.h"
#include "bootloader.h"

#include "conf/bootloader.h"

void bootloader_check(void)
{
	for (int i = 0; i < BOOTLOADER_TIMEOUT; i++)
	{
		io_digital_out(BOOTLOADER_LED, true);
		sys_delay(100);
		io_digital_out(BOOTLOADER_LED, false);
		sys_delay(100);
	}

	if (io_digital_in(BOOTLOADER_BUTTON) == true)
	{
		sys_reset(true);
	}
}