#include <libwdsp.h>

/**
 * 
 * This is a skeleton WhateverDSP project.
 * 
 * You have the following constants available:
 * 
 * 1. BLOCK_SIZE
 *    The size of the audio buffer (the size of chunk audio is processed in).
 *    Use this when doing loops to generate samples.
 * 
 * 2. BUFFER_SIZE
 *    The size of the DAC/ADC audio buffer (the size that's used to get audio
 *    blocks from the DAC/ADC). It should be a multiple of BLOCK_SIZE.
 * 
 * 3. SAMPLE_RATE
 *    The sample rate the device is set to.
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
 */
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

/**
 * The idle loop (executed every ~1ms or so).
 * Do any MIDI note handling and other non-audio priority processing here.
 */
void wdsp_idle(void)
{

}
