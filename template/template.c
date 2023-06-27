#include <libwdsp.h>

/**
 * This is a skeleton WhateverDSP project. It aims to give you an overview of
 * everything you can do to get started immediately without looking at external
 * resources.
 *
 * CONSTANTS:
 *
 * You have the following constants available for use anywhere in your code:
 *
 * 1. BLOCK_SIZE
 *    The size of the audio buffer (the size of chunk audio is processed in).
 *    Use this when doing loops to generate samples.
 *
 * 2. NUM_CHANNELS
 *    The number of channels you have available.
 *
 * 3. SAMPLE_RATE
 *    The sample rate the device is set to.
 *
 * PERIPHERALS:
 *
 * Quick refresher on how to read the peripherals:
 *
 * 1. Knobs
 *
 *   - Read a knob:
 *
 *     float pot_1 = io_analog_in(POT_1); // Output: 0..1
 *
 *   - Scale a knob:
 *
 *     float pot_1 = io_analog_in(POT_1) * 0.5f; // Output: 0..0.5
 *     float pot_2 = io_analog_in(POT_2) * 2.0f - 1.0f; // Output: -1..1
 *
 * 2. Buttons
 *
 *   - Trigger a button:
 *
 *     bool button_1 = io_digital_in(BUTTON_1); // Output: true if down
 *
 *   - How to use buttons as toggles instead of triggers:
 *
 *     // Declare variables outside loop
 *     bool toggle_trigger = false;
 *     bool toggled = false;
 *
 *     // In your loop (idle or process)
 *     if (io_digital_in(BUTTON_1) && !toggle_trigger) toggled = !toggled;
 *     toggle_trigger = io_digital_in(BUTTON_1);
 *
 */

/**
 * Put any initialisation code here.
 */
void wdsp_init(void)
{

}

/**
 * The main audio processing loop.
 * This function gets the audio data in the form of
 * in_buffer[NUM_CHANNELS][BLOCK_SIZE] and returns it in the same
 * format in out_buffer[NUM_CHANNELS][BLOCK_SIZE].
 */
void wdsp_process(float *in_buffer[], float *out_buffer[])
{
	// This example simply copies the input samples into the output buffer.
	for (int i = 0; i < BLOCK_SIZE; i++)
	{
		float l_samp = in_buffer[0][i];
		float r_samp = in_buffer[1][i];

		out_buffer[0][i] = l_samp;
		out_buffer[1][i] = r_samp;
	}
}

/**
 * The idle loop (executed every ~1ms or so).
 * Do any MIDI note handling and other non-audio priority processing here.
 * The ticks parameter contains the current time in milliseconds.
 */
void wdsp_idle(unsigned long int ticks)
{

}
