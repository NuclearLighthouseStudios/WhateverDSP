#ifndef __MIDI_H
#define __MIDI_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#define MIDI_MAX_INTERFACES 8

typedef struct
{
	uint8_t note;
	uint8_t velocity;

} midi_note_message;

typedef struct
{
	uint8_t param;
	uint8_t value;

} midi_cc_message;

typedef struct
{
	uint8_t lsb;
	uint8_t msb;

} midi_pitchbend_message;

typedef struct
{
	uint8_t pressure;

} midi_aftertouch_message;

typedef enum
{
	NOTE_OFF = 0b1000,
	NOTE_ON = 0b1001,
	POLY_AFTERTOUCH = 0b1010,
	CONTROL_CHANGE = 0b1011,
	PROGRAM_CHANGE = 0b1100,
	AFTERTOUCH = 0b1101,
	PITCHBEND = 0b1110,
} midi_command;

typedef struct
{
	midi_command command;
	int channel;
	uint32_t interface_mask;
	size_t length;
	union
	{
		midi_note_message note;
		midi_cc_message control;
		midi_pitchbend_message pitchbend;
		midi_aftertouch_message aftertouch;
	} data;
} midi_message;


typedef void (*midi_transmit_func)(midi_message *message);
typedef bool (*midi_can_transmit_func)(void);

typedef struct
{
	int interface_num;

	midi_transmit_func transmit;
	midi_can_transmit_func can_transmit;
} midi_interface;


extern void midi_init(void);

extern int midi_add_interface(midi_transmit_func transmit, midi_can_transmit_func can_transmit);

extern void midi_receive(midi_message *message);
extern void midi_transmit(void);

extern midi_message *midi_get_message(void);
extern void midi_send_message(midi_message message);

#endif