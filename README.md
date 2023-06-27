# WhateverDSP

¯\\\_(ツ)\_/¯ *It's just like, whatever!*

![image](https://user-images.githubusercontent.com/55932282/128610385-a0652180-9c79-4f24-bbf6-792533ffc454.png)

WhateverDSP is an open C/C++ framework for high quality audio processing on embedded devices. Right now, it targets two boards:

- Version 1 of the WhateverDSP Development board (shown above)
- The [FXSDP](https://github.com/NuclearLighthouseStudios/FXDSP) guitar pedal

Both of these run on the STM32F405 processor, but support for other processors is viable as the framework itself is CPU agnostic.

WDSP supports low latency (<1ms) 32bit float audio processing, analog and digital GPIO, MIDI via both the classic serial interface and USB in addition to full duplex class compliant USB audio at up to 48khz in both 24 bit and 32 bit float sample formats.

All code was written from scratch with low latency and performance in mind.

## Getting started

[To get started, please see the Getting started page in our wiki](https://github.com/NuclearLighthouseStudios/WhateverDSP/wiki/Getting-started).

## Example code

Here is what that a simple volume control that scales the audio level using a potentiometer looks like:

```c
#include <libwdsp.h>

void wdsp_process(float *in_buffer[], float *out_buffer[])
{
	float volume = io_analog_in(POT_1);

	for (int i = 0; i < BLOCK_SIZE; i++)
	{
		float l_sample = in_buffer[0][i];
		float r_sample = in_buffer[1][i];

		out_buffer[0][i] = l_sample * volume;
		out_buffer[1][i] = r_sample * volume;
	}
}

```

Want to learn more? [We're working on the documentation in the Github wiki](https://github.com/NuclearLighthouseStudios/WhateverDSP/wiki). 

In the meantime, why not check out the [examples repository](https://github.com/NuclearLighthouseStudios/WhateverDSP-Examples) or [come chat with us on Discord](https://github.com/NuclearLighthouseStudios/WhateverDSP#chat).

## Chat

[You can chat with us on Discord.](https://discord.gg/WDsFnartXb)

## V1 Development board

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

## FXDSP guitar pedal

FXDSP is a programmable DSP platform in a familiar guitar pedal form factor. It has interchangeable user interface PCBs so it can easily be tailored to each effects needs.

[Read more about FXDSP here](https://github.com/NuclearLighthouseStudios/FXDSP).

## License

The WhateverDSP library is licensed under GNU Lesser General Public License.
