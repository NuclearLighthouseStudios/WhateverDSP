# WhateverDSP

¯\\\_(ツ)\_/¯ *It's just like, whatever!*

WhateverDSP ist an open C/C++ framework for high quality audio effects on embedded devices. It's still in pretty early development and right now only targets the accompanying WhateverDSP Development board and it's STM32F405 processor.

It supports low latency (<1ms) 32bit float audioprocessing , analog and digital GPIO, MIDI via both the classic serial interface and USB in addition to full duplex class compliant USB audio at up to 48khz in both 24 bit and 32 bit float sample formats.

All code was written from scratch with low latency and performance in mind.


## Getting started

You can include this library into your project as a git submodule and only need to reference it in your Makefile. A minimal example could look something like this:

```
# Makefile
TARGET = example
BOARD = wdsp-dev

include WhateverDSP/libwdsp.mk

```

Including the WhateverDSP makefile and setting the name of your output sets up everything you need to get you started. In this configuration it would automatically compile an example.c into a ready to flash binary once you invoke `make`. For more complex projects you can specify your own dependencies like usual.

Here is what that a simple example effect that just passes through audio would look like:

```
// example.c
#include <libwdsp.h>

void wdsp_process(float *in_buffer[BLOCK_SIZE], float *out_buffer[BLOCK_SIZE])
{
	for (int i = 0; i < BLOCK_SIZE; i++)
	{
		float l_samp = in_buffer[0][i];
		float r_samp = in_buffer[1][i];

		out_buffer[0][i] = l_samp;
		out_buffer[1][i] = r_samp;
	}
}

```

If you have `dfu-util` installed the resulting binary can be flashed onto the board by connecting it to your computer via USB, holding down the "Boot" button while powering it up and running `make dfu`.

Want to learn more? Tough luck, there is no documentation yet. But I'm working on it…  
In the meantime, why not check out the [examples repository](https://github.com/NuclearLighthouseStudios/WhateverDSP-Examples).

## Setting up VSCode

Running `make vscode` in your project directory sets up a workspace with all required setting to get you started with development and debugging in VSCode. It relies on the C/C+++ and Cortex-Debug extensions as well as OpenOCD and GDB to talk to the board.

Using a debugger like the STLinkV3-Mini you can now start and debug project directly on the hardware just by hitting F5.

## Required Software

### On Linux

* `arm-none-eabi-gcc` GCC toolchain
* `arm-none-eabi-newlib` C library
* `dfu-util` if you want to flash via USB
* `arm-none-eabi-gdb` and `openocd` for use with 

### On MacOS

* `brew install gcc-arm-embedded` For all the toolchainy bits
* `brew install dfu-util` for flashing via USB

### On Windows

* ¯\\\_(ツ)\_/¯

## Chat

We have an IRC channel on [libera.chat](https://libera.chat/) called `#wdsp`. 

## License

The WhateverDSP library is licensed under GNU Lesser General Public License.
