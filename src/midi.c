#include <stdbool.h>
#include <string.h>
#include <stddef.h>

#include "core.h"
#include "board.h"

#include "midi.h"

#define RXQ_LENGTH 16
static midi_message __CCMRAM midi_rx_queue[RXQ_LENGTH];
static int __CCMRAM rxq_read_pos = 0;
static int __CCMRAM rxq_write_pos = 0;

#define TXQ_LENGTH 16
static midi_message __CCMRAM midi_tx_queue[TXQ_LENGTH];
static int __CCMRAM txq_read_pos = 0;
static int __CCMRAM txq_write_pos = 0;

#define MIDI_MAX_INTERFACES 4
static midi_interface __CCMRAM interfaces[MIDI_MAX_INTERFACES];
static int __CCMRAM num_interfaces = 0;


void midi_receive(midi_message *message)
{
	memcpy(&(midi_rx_queue[rxq_write_pos]), message, sizeof(*message));

	rxq_write_pos++;
	rxq_write_pos %= RXQ_LENGTH;

	if (rxq_read_pos == rxq_write_pos)
	{
		rxq_read_pos++;
		rxq_read_pos %= RXQ_LENGTH;
	}
}

void midi_transmit(void)
{
	if (txq_read_pos == txq_write_pos)
		return;

	midi_message *message = &(midi_tx_queue[txq_read_pos]);

	for (int i = 0; i < num_interfaces; i++)
	{
		if (((message->interface_mask & (0b1 << i)) == 0) && (!interfaces[i].can_transmit()))
			return;
	}

	txq_read_pos++;
	txq_read_pos %= TXQ_LENGTH;

	for (int i = 0; i < num_interfaces; i++)
	{
		if ((message->interface_mask & (0b1 << i)) == 0)
			interfaces[i].transmit(message);
	}
}

midi_message *midi_get_message(void)
{
	if (rxq_read_pos == rxq_write_pos)
		return NULL;

	midi_message *message = &(midi_rx_queue[rxq_read_pos]);

	rxq_read_pos++;
	rxq_read_pos %= RXQ_LENGTH;

	return message;
}

void midi_send_message(midi_message *tx_message)
{
	midi_message *message = &(midi_tx_queue[txq_write_pos]);

	message->command = tx_message->command;
	message->channel = tx_message->channel;

	size_t length = midi_get_message_length(tx_message->command);
	memcpy(&(message->data), &(tx_message->data), length);

	txq_write_pos++;
	txq_write_pos %= TXQ_LENGTH;

	if (txq_read_pos == txq_write_pos)
	{
		txq_read_pos++;
		txq_read_pos %= TXQ_LENGTH;
	}

	midi_transmit();
}

size_t midi_get_message_length(midi_command command)
{
	if ((command & 0xf0) != 0xf0)
		command &= 0xf0;

	switch (command)
	{
		case PROGRAM_CHANGE:
		case AFTERTOUCH:
		case SYSEX:
		case TIME_CODE:
		case SONG_SELECT:
			return 1;

		case NOTE_ON:
		case NOTE_OFF:
		case POLY_AFTERTOUCH:
		case CONTROL_CHANGE:
		case PITCHBEND:
		case SONG_POSITION:
			return 2;

		default:
			return 0;
	}
}

int midi_add_interface(midi_transmit_func transmit, midi_can_transmit_func can_transmit)
{
	interfaces[num_interfaces].interface_num = num_interfaces;
	interfaces[num_interfaces].transmit = transmit;
	interfaces[num_interfaces].can_transmit = can_transmit;
	num_interfaces++;

	return num_interfaces - 1;
}

void midi_init(void)
{
	return;
}