#ifndef __MIDI_H
#define __MIDI_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#define MIDI_MAX_INTERFACES 4

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

typedef struct
{
	uint8_t data;

} midi_sysex_message;

typedef struct
{
	uint8_t type : 4;
	uint8_t value : 4;

} midi_timecode_message;

typedef struct
{
	uint8_t lsb;
	uint8_t msb;

} midi_song_position_message;

typedef struct
{
	uint8_t song;

} midi_song_select_message;

typedef enum midi_command
{
	NOTE_OFF = 0b10000000,
	NOTE_ON = 0b10010000,
	POLY_AFTERTOUCH = 0b10100000,
	CONTROL_CHANGE = 0b10110000,
	PROGRAM_CHANGE = 0b11000000,
	AFTERTOUCH = 0b11010000,
	PITCHBEND = 0b11100000,
	SYSEX = 0b11110000,
	TIME_CODE = 0b11110001,
	SONG_POSITION = 0b11110010,
	SONG_SELECT = 0b11110011,
	TUNE_REQUEST = 0b11110110,
	SYSEX_END = 0b11110111,
	CLOCK = 0b11111000,
	START = 0b11111010,
	CONTINUE = 0b11111011,
	STOP = 0b11111100,
	ACTIVE_SENSE = 0b11111110,
	SYS_RESET = 0b11111111,
} __attribute__((packed)) midi_command;

typedef struct
{
	midi_command command;
	uint8_t channel;
	uint32_t interface_mask;
	union
	{
		midi_note_message note;
		midi_cc_message control;
		midi_pitchbend_message pitchbend;
		midi_aftertouch_message aftertouch;
		midi_sysex_message sysex;
		midi_timecode_message timecode;
		midi_song_position_message song_position;
		midi_song_select_message song_select;
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
extern void midi_send_message(midi_message *message);

extern size_t midi_get_message_length(midi_command command);

#endif