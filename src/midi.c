#include <stdbool.h>
#include <string.h>

#include "midi.h"

#include "midi_phy.h"

#define RXQ_LENGTH 16
#define MIDI_DATA_MAXLEN 2

static midi_message __attribute__((section(".ccmram"))) midi_rx_queue[RXQ_LENGTH];
static int __attribute__((section(".ccmram"))) rxq_read_pos = 0;
static int __attribute__((section(".ccmram"))) rxq_write_pos = 0;

static midi_command __attribute__((section(".ccmram"))) midi_current_command = 0;
static unsigned int __attribute__((section(".ccmram"))) midi_current_channel;
static unsigned char __attribute__((section(".ccmram"))) midi_data_buffer[MIDI_DATA_MAXLEN];
static unsigned int __attribute__((section(".ccmram"))) midi_data_length;

#define TXQ_LENGTH 16
#define TX_BUFFER_LENGTH 8

static midi_message __attribute__((section(".ccmram"))) midi_tx_queue[TXQ_LENGTH];
static int __attribute__((section(".ccmram"))) txq_read_pos = 0;
static int __attribute__((section(".ccmram"))) txq_write_pos = 0;

static unsigned char midi_tx_buffer[TX_BUFFER_LENGTH];

static inline int get_message_length(midi_command command)
{
	switch (command)
	{
	case PROGRAM_CHANGE:
	case AFTERTOUCH:
	default:
		return 1;

	case NOTE_ON:
	case NOTE_OFF:
	case POLY_AFTERTOUCH:
	case CONTROL_CHANGE:
	case PITCHBEND:
		return 2;
	}
}

void midi_receive(unsigned char data)
{
	if (data & 0x80)
	{
		midi_current_command = (data & 0xf0) >> 0x04;
		midi_current_channel = data & 0x0f;
		midi_data_length = 0;
	}
	else
	{
		// Ignore all system common and realtime messages,
		// also ignore data when we don't have a valid command yet
		if ((midi_current_command == 0x0f) || (midi_current_command == 0x00))
			return;

		if (midi_data_length < MIDI_DATA_MAXLEN)
		{
			midi_data_buffer[midi_data_length] = data;
			midi_data_length++;
		}

		int data_len = get_message_length(midi_current_command);

		if (midi_data_length == data_len)
		{
			midi_message *message = &(midi_rx_queue[rxq_write_pos]);

			message->length = midi_data_length;
			message->command = midi_current_command;
			message->channel = midi_current_channel;
			memcpy(&(message->data), midi_data_buffer, midi_data_length);

			midi_data_length = 0;

			rxq_write_pos++;
			rxq_write_pos %= RXQ_LENGTH;

			if (rxq_read_pos == rxq_write_pos)
			{
				rxq_read_pos++;
				rxq_read_pos %= RXQ_LENGTH;
			}
		}
	}
}

void midi_transmit(void)
{
	if (txq_read_pos == txq_write_pos)
		return;

	if (!midi_phy_can_transmit())
		return;

	midi_message *message = &(midi_tx_queue[txq_read_pos]);

	txq_read_pos++;
	txq_read_pos %= TXQ_LENGTH;

	midi_tx_buffer[0] = (message->command << 0x04) | (message->channel & 0x0f);

	memcpy(&midi_tx_buffer[1], &(message->data), message->length);

	midi_phy_transmit(midi_tx_buffer, message->length + 1);
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

void midi_send_message(midi_message tx_message)
{
	midi_message *message = &(midi_tx_queue[txq_write_pos]);

	message->length = tx_message.length;
	message->command = tx_message.command;
	message->channel = tx_message.channel;
	memcpy(&(message->data), &(tx_message.data), tx_message.length);

	txq_write_pos++;
	txq_write_pos %= TXQ_LENGTH;

	if (txq_read_pos == txq_write_pos)
	{
		txq_read_pos++;
		txq_read_pos %= TXQ_LENGTH;
	}

	midi_transmit();
}

void midi_init(void)
{
	midi_phy_init();
}