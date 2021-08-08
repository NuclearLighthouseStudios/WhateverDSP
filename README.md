# WhateverDSP

¯\\\_(ツ)\_/¯ *It's just like, whatever!*

![image](https://user-images.githubusercontent.com/55932282/128610385-a0652180-9c79-4f24-bbf6-792533ffc454.png)

WhateverDSP is an open C/C++ framework for high quality audio processing on embedded devices. It's still in pretty early development and right now only targets the accompanying WhateverDSP Development board and it's STM32F405 processor.

It supports low latency (<1ms) 32bit float audio processing, analog and digital GPIO, MIDI via both the classic serial interface and USB in addition to full duplex class compliant USB audio at up to 48khz in both 24 bit and 32 bit float sample formats.

All code was written from scratch with low latency and performance in mind.

## Getting started

[See our Getting started page](https://github.com/NuclearLighthouseStudios/WhateverDSP/wiki/Getting-started).

## Example code

Here is what that a simple example effect that just passes through audio would look like:

```c
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

Want to learn more? [We're working on the documentation in the Github wiki](https://github.com/NuclearLighthouseStudios/WhateverDSP/wiki). 

In the meantime, why not check out the [examples repository](https://github.com/NuclearLighthouseStudios/WhateverDSP-Examples) or [come chat with us on IRC](https://github.com/NuclearLighthouseStudios/WhateverDSP#chat).

## Chat

We have an IRC channel on [libera.chat](https://libera.chat/) called `#wdsp`. 

## Development board

The current version of the WhateverDSP dev board runs on 9V DC power and includes:

- STM32F405 MCU
- 2x TRS analog inputs
- 2x TRS analog outputs
- 4 potentiometers
- 2 buttons (+ boot and reset buttons)
- USB-C
- TRS MIDI input
- TRS MIDI output

Headers:  
- Debug header for realtime debugging via ST-Link V3
- I2C
- 3.3V
- Ground
- 4 analog inputs
- 2 analog outputs
- 3 digital inputs
- 3 digital outputs

The board will be released as open-source after some further testing. You will also be able to purchase them directly fully assembled. In the meantime, come hang on the IRC channel for updates!

## License

The WhateverDSP library is licensed under GNU Lesser General Public License.
