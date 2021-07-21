#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <libwdsp.h>

bool trigger_state = false;
bool trigger = false;
int last_note = -1;

const int scale[] = { 0, 3, 5, 7, 10 };

#define STEPS 4

int step = 0;
int melody[STEPS];

float last_sust = -1;

int clock_count = 1;

void wdsp_process(float in_buffer[][2], float out_buffer[][2], unsigned long int nBlocks)
{
	for (int i = 0; i < nBlocks; i++)
	{
		float l_samp = in_buffer[i][0];
		float r_samp = in_buffer[i][1];

		if (fabs(l_samp) + fabs(r_samp) > 0.5)
		{
			trigger = true;
		}

		out_buffer[i][0] = 0;
		out_buffer[i][1] = 0;
	}
}

void wdsp_idle(void)
{
	float sustain = io_analog_in(POT_1);

	if (fabs(sustain - last_sust) > 1.0f / 127.0f)
	{
		last_sust = sustain;

		midi_message message;

		message.command = CONTROL_CHANGE;
		message.channel = 0;
		message.data.control.param = 64;
		message.data.control.value = sustain * 127.0f;

		midi_send_message(&message);
	}

	trigger |= io_digital_in(BUTTON_1);

	int divider = powf(2, floor(((1.0f - io_analog_in(POT_2)) * 1.1f) * 4 + 1));

	midi_message *rx_message;
	if ((rx_message = midi_get_message()) != NULL)
	{
		if (rx_message->command == CLOCK)
		{
			clock_count++;
		}
	}

	if (clock_count % divider == 0)
		trigger = true;

	if (trigger != trigger_state)
	{
		trigger_state = trigger;

		midi_message message;

		if (trigger)
		{
			if ((melody[step] == 0) || (rand() % 10 == 1))
			{
				int note = scale[rand() % 5];
				int octave = rand() % 5;
				melody[step] = 36 + octave * 12 + note;
			}

			last_note = melody[step];

			step++;
			step %= STEPS;

			message.command = NOTE_ON;
		}
		else
		{
			message.command = NOTE_OFF;
		}

		message.channel = 0;
		message.data.note.note = last_note;
		message.data.note.velocity = 40 + rand() % 40;

		midi_send_message(&message);
	}

	trigger = false;
}
