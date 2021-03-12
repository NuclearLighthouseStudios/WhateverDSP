#include <stdbool.h>
#include <math.h>

#include "stm32f4xx.h"

#include "audio.h"
#include "io.h"
#include "filters.h"

#include "dsp.h"

struct filter hpf[2];
struct filter lpf[2];

void dsp_init()
{
	hpf[0].cutoff = 0;
	hpf[0].resonance = 0.1;
	lpf[0].cutoff = 1;
	lpf[0].resonance = 0.1;

	hpf[1].cutoff = 0;
	hpf[1].resonance = 0.1;
	lpf[1].cutoff = 1;
	lpf[1].resonance = 0.1;
}

// This is just a little filter example thing
void dsp_process(float in_buffer[2][BUFSIZE], float out_buffer[2][BUFSIZE])
{
	io_digital_out(LED_1, fabs(in_buffer[0][0]) >= 0.5);
	io_digital_out(MUTE, io_digital_in(BUTTON_2));

	bool bypass = io_digital_in(BUTTON_1);

	float lpc = io_analog_in(POT_1);
	float hpc = io_analog_in(POT_2);
	float res = io_analog_in(POT_3);
	float vol = io_analog_in(POT_4);

	hpf[0].cutoff = hpc;
	hpf[1].cutoff = hpc;

	lpf[0].cutoff = lpc;
	lpf[1].cutoff = lpc;
	lpf[0].resonance = res*0.9f;
	lpf[1].resonance = res*0.9f;

	for (int i = 0; i < BUFSIZE; i++)
	{
		float l_samp = in_buffer[0][i];
		float r_samp = in_buffer[1][i];

		if (!bypass)
		{
			l_samp = filter_lp_iir(l_samp, &lpf[0]);
			r_samp = filter_lp_iir(r_samp, &lpf[1]);

			l_samp = filter_hp_iir(l_samp, &hpf[0]);
			r_samp = filter_hp_iir(r_samp, &hpf[1]);
		}

		out_buffer[0][i] = l_samp * vol;
		out_buffer[1][i] = r_samp * vol;
	}
}
