#include <stdbool.h>

#include "core.h"
#include "board.h"

#include "system.h"
#include "io.h"

extern bool _enter_bootloader;

void bootloader_check(io_pin_idx led, io_pin_idx button)
{
	for (int i = 0; i < 3; i++)
	{
		io_digital_out(led, true);
		sys_delay(100);
		io_digital_out(led, false);
		sys_delay(100);
	}

	if (io_digital_in(button) == true)
	{
		_enter_bootloader = true;
		sys_reset();
	}
}