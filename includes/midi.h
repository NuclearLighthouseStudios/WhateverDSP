#ifndef __MIDI_H
#define __MIDI_H

typedef struct
{
	unsigned char note;
	unsigned char velocity;

} midi_note_message;

typedef struct
{
	unsigned char param;
	unsigned char value;

} midi_cc_message;

typedef struct
{
	unsigned char lsb;
	unsigned char msb;

} midi_pitchbend_message;

typedef struct
{
	unsigned char pressure;

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
	int length;
	union
	{
		midi_note_message note;
		midi_cc_message control;
		midi_pitchbend_message pitchbend;
		midi_aftertouch_message aftertouch;
	} data;
} midi_message;

extern void midi_init(void);

extern void midi_receive(unsigned char data);
extern void midi_transmit(void);

extern midi_message *midi_get_message(void);
extern void midi_send_message(midi_message message);

#endif