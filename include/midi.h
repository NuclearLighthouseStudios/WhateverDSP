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
	unsigned char command;
	unsigned char channel;
	int length;
	union
	{
		unsigned char raw[2];
		midi_note_message note;
		midi_cc_message cc;
		midi_pitchbend_message bitchbend;
	} data;
} midi_message;

typedef enum
{
	NOTE_ON = 0x09,
	NOTE_OFF = 0x08,
	CONTROL_CHANGE = 0x0B,
} midi_command;

extern void midi_init(void);

extern midi_message *midi_get_message(void);

#endif