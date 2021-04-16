#include <stdbool.h>
#include <stdlib.h>
#include <math.h>

#include <libwdsp.h>

bool trigger_state = false;
int last_note = -1;

const int scale[] = {0, 3, 5, 7, 10};

#define STEPS 4

int step = 0;
int melody[STEPS];

void wdsp_init(unsigned long int _sample_rate)
{
	midi_message message;

	message.command = CONTROL_CHANGE;
	message.channel = 0;
	message.length = 2;
	message.data.control.param = 64;
	message.data.note.velocity = 128;

	midi_send_message(message);
}

void wdsp_process(float in_buffer[][2], float out_buffer[][2], unsigned long int nBlocks)
{
	bool trigger = false;

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

	trigger |= io_digital_in(BUTTON_1);

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
		message.length = 2;
		message.data.note.note = last_note;
		message.data.note.velocity = 40+rand()%40;

		midi_send_message(message);
	}
}
