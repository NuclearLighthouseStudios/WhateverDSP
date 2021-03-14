#include <stdbool.h>
#include <stdlib.h>
#include <math.h>

#include <libwdsp.h>

#define NUMVOICES 16
#define BASENOTE 21

// SO-666, based on https://github.com/50m30n3/SO-666, used with permission

struct voice
{
	float *string;
	float lp_last;
	int note;
	int stringpos;
	int stringlength;
	bool active;
	bool sustained;
};

struct voice voices[NUMVOICES];
int currvoice = 0;

unsigned long int sample_rate;

float lpval, lplast;
float hpval, hplast;
float feedback;

bool sustain = false;

/* greets to wrl */
static inline float shape_tanh(const float x)
{
	/* greets to aleksey vaneev */
	const float ax = fabsf(x);
	const float x2 = x * x;

	return (x * (2.45550750702956f + 2.45550750702956f * ax + (0.893229853513558f + 0.821226666969744f * ax) * x2) /
			(2.44506634652299f + (2.44506634652299f + x2) * fabsf(x + 0.814642734961073f * x * ax)));
}

void wdsp_init(unsigned long int _sample_rate)
{
	feedback = 0;
	sample_rate = _sample_rate;

	for (int voice = 0; voice < NUMVOICES; voice++)
	{
		float min_freq = 440.0 * powf(2.0f, (BASENOTE - 69) / 12.0f);
		int length = ceilf((float)sample_rate / min_freq);
		voices[voice].string = malloc(length * sizeof(float));

		for (int i = 0; i < length; i++)
		{
			voices[voice].string[i] = 0.0f;
		}

		voices[voice].stringlength = length;
		voices[voice].stringpos = 0;
		voices[voice].active = false;
		voices[voice].lp_last = 0.0f;
	}

	lpval = lplast = 0.0f;
	hpval = hplast = 0.0f;
}

void wdsp_process(float in_buffer[][2], float out_buffer[][2], unsigned long int nBlocks)
{
	io_digital_out(MUTE, io_digital_in(BUTTON_2));

	float volume = io_analog_in(POT_4);
	float cutoff = powf((io_analog_in(POT_1) * 127.0f + 50.0f) / 200.0f, 5.0f);
	float reso = io_analog_in(POT_2);
	feedback = 0.01 + powf(io_analog_in(POT_3), 4.0) * 0.9;

	midi_message *message = midi_get_message();

	if (message != NULL)
	{
		if (message->command == NOTE_ON)
		{
			int note = message->data.note.note;

			if (note >= BASENOTE)
			{
				int active_count = 0;
				while ((voices[currvoice].active) && (active_count < NUMVOICES))
				{
					currvoice++;
					active_count++;
					currvoice %= NUMVOICES;
				}

				voices[currvoice].note = note;
				voices[currvoice].active = true;
				float freq = 440.0 * powf(2.0f, (note - 69) / 12.0f);
				voices[currvoice].stringlength = roundf((float)sample_rate / freq);

				for (int i = 0; i < voices[currvoice].stringlength; i++)
					voices[currvoice].string[i] = 0.0f;

				currvoice++;
				currvoice %= NUMVOICES;
			}
		}
		else if (message->command == NOTE_OFF)
		{
			int note = message->data.note.note;

			for (int i = 0; i < NUMVOICES; i++)
			{
				if (voices[i].note == note)
				{
					if (sustain)
						voices[i].sustained = true;
					else
						voices[i].active = false;
				}
			}
		}
		else if (message->command == CONTROL_CHANGE)
		{
			if (message->data.cc.param == 64)
			{
				sustain = message->data.cc.value > 10;

				if (!sustain)
				{
					for (int i = 0; i < NUMVOICES; i++)
					{
						if (voices[i].sustained)
						{
							voices[i].active = false;
							voices[i].sustained = false;
						}
					}
				}
			}
		}
	}

	for (int i = 0; i < nBlocks; i++)
	{
		float l_samp = in_buffer[i][0];
		float r_samp = in_buffer[i][1];

		float exciter = (l_samp + r_samp) / 2.0f;
		float noise = (((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f) * 0.001f + (l_samp + r_samp) / 2.0f;
		float sample = 0;

		for (int voice = 0; voice < NUMVOICES; voice++)
		{
			int pos = voices[voice].stringpos;

			voices[voice].string[pos] = voices[voice].string[pos] * 0.9f + voices[voice].lp_last * 0.1f;
			voices[voice].lp_last = voices[voice].string[pos];

			voices[voice].string[pos] = shape_tanh(voices[voice].string[pos]) * 0.99f;

			sample += voices[voice].string[pos];
		}

		hpval += (sample - hplast) * 0.0001f;
		hplast += hpval;
		hpval *= 0.96f;
		sample -= hplast;

		float fmod = shape_tanh(lplast);
		lpval += (sample - lplast) * cutoff * (1.0f - fmod * fmod * 0.9f);
		lplast += lpval;
		lpval *= reso;
		sample = lplast;

		for (int voice = 0; voice < NUMVOICES; voice++)
		{
			if (voices[voice].active)
				voices[voice].string[voices[voice].stringpos] += (sample + noise) * feedback + exciter;

			voices[voice].stringpos++;
			if (voices[voice].stringpos >= voices[voice].stringlength)
				voices[voice].stringpos = 0;
		}

		sample = shape_tanh(sample) * volume;

		out_buffer[i][0] = sample;
		out_buffer[i][1] = sample;
	}

	io_digital_out(LED_1, fabs(out_buffer[0][0]) >= 0.5);
}
