#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <math.h>

#include "config.h"

#include "board.h"
#include "core.h"

#include "system.h"
#include "debug.h"
#include "usb.h"
#include "usb_phy.h"
#include "usb_config.h"
#include "usb_uac.h"

#include "audio_usb.h"

#include "conf/audio_usb.h"


#if OUTPUT_ENABLED == true
// These variables keep track of the USB IN endpoint's status
// The direction of the endpoints is always specified from the hostm
// so we use IN endpoints to send data INto the computer.
static usb_in_endpoint __CCMRAM *audio_in_ep;

static uint8_t __CCMRAM in_alt_setting = 0;
static bool __CCMRAM in_active = false;
#endif

#if INPUT_ENABLED == true
// Same thing here for the OUT endpoint and it's accompanying SYNCH IN endpoint.
// We'll use the synch endpoint to send our effective sample rate to the host
// so it always knows how many samples to send us per USB frame.
static usb_out_endpoint __CCMRAM *audio_out_ep;
static usb_in_endpoint __CCMRAM *synch_in_ep;

static uint8_t __CCMRAM out_alt_setting = 0;
static bool __CCMRAM out_active = false;
#endif

#if OUTPUT_ENABLED == true
// These are the descriptors for the terminals used out USB audio output.
// They consist of an audio input terminal which describes how we receive
// audio from the audio subsystem and an USB terminal which describes
// how we send data to the computer.
static usb_audio_input_terminal_descriptor __CCMRAM audio_input_terminal = USB_AUDIO_INPUT_TERMINAL_DESCRIPTOR_INIT(1, 0x0201, NUM_USB_CHANNELS, USB_CHANNEL_CONFIG);
static usb_audio_output_terminal_descriptor __CCMRAM usb_output_terminal = USB_AUDIO_OUTPUT_TERMINAL_DESCRIPTOR_INIT(2, 0x0101, 1);
#endif

#if INPUT_ENABLED == true
// These are the descriptors for the terminals used out USB audio input.
// Same as for the output terminals, but this time there is a USB input terminal
// and an audio output terminal.
static usb_audio_input_terminal_descriptor __CCMRAM usb_input_terminal = USB_AUDIO_INPUT_TERMINAL_DESCRIPTOR_INIT(3, 0x0101, NUM_USB_CHANNELS, USB_CHANNEL_CONFIG);
static usb_audio_output_terminal_descriptor __CCMRAM audio_output_terminal = USB_AUDIO_OUTPUT_TERMINAL_DESCRIPTOR_INIT(4, 0x0301, 1);
#endif

#if OUTPUT_ENABLED == true
// These are all the descriptors used for the audio output interface.
// There is a zero bandwidth variant of the interface with no endpoints that
// the computer can switch to when it wants to stop audio transmission.
static usb_interface_descriptor __CCMRAM in_interface_desc_zb = USB_INTERFACE_DESCRIPTOR_INIT(0, 0x01, 0x02, 0x00);
static usb_interface_descriptor __CCMRAM in_interface_desc = USB_INTERFACE_DESCRIPTOR_INIT_ALT(1, 1, 0x01, 0x02, 0x00);

// These descriptors describe the format we use for audio output and which
// of the above terminals this interface is connected to.
static usb_audio_interface_descriptor __CCMRAM in_audio_interface_desc = USB_AUDIO_INTERFACE_DESCRIPTOR_INIT(2, 0, FORMAT_TAG);
static usb_audio_format_i_descriptor __CCMRAM in_audio_format_desc = USB_AUDIO_FORMAT_I_DESCRIPTOR_INIT(NUM_USB_CHANNELS, SUBFRAME_SIZE, BIT_RESOLUTION, SAMPLE_RATE);

// This is our actual data endpoint descriptor.
// We're going to set this up later once we actually know the address of our endpoint.
static usb_endpoint_descriptor __CCMRAM in_endpoint_desc;
static usb_audio_endpoint_descriptor __CCMRAM in_audio_endpoint_desc = USB_AUDIO_ENDPOINT_DESCRIPTOR_INIT();
#endif

#if INPUT_ENABLED == true
// Same as with the output interface with have a zero bandwith and an active
// version of our interface. This time the active version has two endpoints
// associated with it, one for audio data and one for synch information.
static usb_interface_descriptor __CCMRAM out_interface_desc_zb = USB_INTERFACE_DESCRIPTOR_INIT(0, 0x01, 0x02, 0x00);
static usb_interface_descriptor __CCMRAM out_interface_desc = USB_INTERFACE_DESCRIPTOR_INIT_ALT(1, 2, 0x01, 0x02, 0x00);

// Just as above, this sets up the connection to the input terminals and format.
static usb_audio_interface_descriptor __CCMRAM out_audio_interface_desc = USB_AUDIO_INTERFACE_DESCRIPTOR_INIT(3, 0, FORMAT_TAG);
static usb_audio_format_i_descriptor __CCMRAM out_audio_format_desc = USB_AUDIO_FORMAT_I_DESCRIPTOR_INIT(NUM_USB_CHANNELS, SUBFRAME_SIZE, BIT_RESOLUTION, SAMPLE_RATE);

// Our audio data output endpoint. Again to be filled in later.
static usb_endpoint_descriptor __CCMRAM out_endpoint_desc;
static usb_audio_endpoint_descriptor __CCMRAM out_audio_endpoint_desc = USB_AUDIO_ENDPOINT_DESCRIPTOR_INIT();

// This is the descriptor for the synch endpoint.
// We're going to initialize this in the init function.
static usb_endpoint_descriptor __CCMRAM synch_endpoint_desc;
#endif

#if OUTPUT_ENABLED == true
// This is the double buffer used for the data we transmit to the host/computer.
// One side of the buffer is used to write samples into while the other side
// is used by the USB system to transmit the data.
static uint8_t __CCMRAM tx_buf[2][OUT_BUF_SIZE + 4];
// This variable keeps track of which side of the buffer we're currently writing to
static uint32_t __CCMRAM tx_active_buf = 0;
static size_t __CCMRAM tx_buf_length = 0;
#endif

#if INPUT_ENABLED == true
// These are the buffers used for USB audio reception.
// This first buffer is for the data we receive over USB
static uint8_t __CCMRAM rx_buf[FRAME_SIZE];

// This second buffer is our circular receive buffer.
// It stores the samples after they're been transferred from the rx_buf ready
// to be picked up by the audio subsystem.
static SAMPLE_TYPE __CCMRAM in_buf[NUM_USB_CHANNELS][IN_BUF_SIZE];
static uint32_t __CCMRAM in_read_pos = 0;
static uint32_t __CCMRAM in_write_pos = 0;
static bool __CCMRAM in_filled = false;

// These variables keep track of the average number of audio samples that happen
// during one USB frame and the average input buffer fill level to calculate
// the effective sync sample rate that gets reported to the host.
static uint32_t __CCMRAM num_samples = 0;
static uint32_t __CCMRAM num_frames = 0;
static uint32_t __CCMRAM in_buf_fill = 0;

static uint32_t __CCMRAM sync_sample_rate;
#endif

/*******************************************************************************
 * This function gets called by the USB_Phy layer at the end of each USB frame
 * which happens every millisecond.
 * It schedules the transmission/reception of the next audio frame in addition
 * to recalculating the effective sample rate in relation to the USB start of
 * frame clock to report back to the host via the sync endpoint.
 ******************************************************************************/
static void eof_callback(void)
{
#if INPUT_ENABLED == true
	// Only start input transfers and send sync data when the required endpoint
	// has been activated and we're not set to the zero bandwidth alt setting.
	if ((out_active) && (out_alt_setting != 0))
	{
		num_frames++;

		// This part recalculates the effective sample rate compared to the USB start of frame interval
		// so we can report the exactly number of samples we want to receive per USB frame.
		// The "servo" term is added to slowly steer the input buffer fill level towards a target level and keep it there.
		if (num_frames >= (1 << SYNC_INTERVAL))
		{
			float servo = (((float)in_buf_fill / (float)num_frames) - (float)IN_BUF_TARGET) * SYNC_SERVO_AMOUNT;
			sync_sample_rate = ((float)num_samples / (float)num_frames - servo) * (float)(1 << 14);

			num_frames = 0;
			num_samples = 0;
			in_buf_fill = 0;
		}

		usb_transmit((uint8_t *)&sync_sample_rate, 3, synch_in_ep);
		usb_receive((uint8_t *)(rx_buf), FRAME_SIZE, audio_out_ep);
	}
#endif

#if OUTPUT_ENABLED == true
	// Only transmit data when the endpoint has been enabled and we're not in
	// zero bandwidth mode.
	if ((in_active) && (in_alt_setting != 0))
	{
		// In case we have more data than we can transmit, only transmit as much as we can
		size_t tx_size = tx_buf_length < FRAME_SIZE ? tx_buf_length : FRAME_SIZE;

		usb_transmit(tx_buf[tx_active_buf], tx_size, audio_in_ep);

		// If we were not able to fit the amount of buffered audio data in a single
		// USB frame we carry over the remainder to the next frame.
		if (tx_size < tx_buf_length)
		{
			tx_buf_length -= tx_size;
			memcpy(tx_buf[!tx_active_buf], tx_buf[tx_active_buf] + tx_size, tx_buf_length);
		}
		else
		{
			tx_buf_length = 0;
		}

		// The transmission is double buffered so the USB system can use one
		// buffer for transmission while we fill up the next one.
		// Here we switch which buffer we write audio data into.
		tx_active_buf = !tx_active_buf;
	}
#endif
}

/*******************************************************************************
 * This is the receive callback that gets called by the USB system each time
 * we receive a complete USB audio frame.
 * It transfers the received samples into the input ring buffer.
 ******************************************************************************/
#if INPUT_ENABLED == true
static void rx_callback(usb_out_endpoint *ep, uint8_t *buf, size_t count)
{
	// Calculate the current fill level of the input buffer
	int32_t fill = in_write_pos - in_read_pos;
	if (fill < 0)
		fill += IN_BUF_SIZE;

	// If we're at more than twice the minimum fill level, start the playback
	if (fill >= IN_BUF_TARGET * 2)
		in_filled = true;

	// Add the current fill level to the running average calculation for the
	// "servo" term used to calculate the effective sample rate for sync.
	in_buf_fill += fill;

	for (int i = 0; i < count; i += SUBFRAME_SIZE * NUM_USB_CHANNELS)
	{
		// Unpack the subframes and put them into the receive buffer channel by channel
		for (int c = 0; c < NUM_USB_CHANNELS; c++)
		{
			in_buf[c][in_write_pos] = *((SAMPLE_TYPE *)&buf[i + c * SUBFRAME_SIZE]);
		}

		in_write_pos = (in_write_pos + 1) % IN_BUF_SIZE;

		// If we get into the situation where the buffer overflows, just
		// overwrite samples. Not much we can do.
		if (in_write_pos == in_read_pos)
			in_read_pos = (in_read_pos + 1) % IN_BUF_SIZE;
	}
}
#endif

/*******************************************************************************
 * These functions get called on activation or deactivation of the input and
 * output endpoints.
 ******************************************************************************/
#if OUTPUT_ENABLED == true
static void in_start(usb_in_endpoint *ep)
{
	in_active = true;
}

static void in_stop(usb_in_endpoint *ep)
{
	in_active = false;
	in_alt_setting = 0;
}
#endif

#if INPUT_ENABLED == true
static void out_start(usb_out_endpoint *ep)
{
	out_active = true;
}

static void out_stop(usb_out_endpoint *ep)
{
	out_active = false;
	out_alt_setting = 0;
}
#endif


/*******************************************************************************
 * This function takes audio samples from an array of BLOCK_SIZE sized buffers
 * and transfers NUM_USB_CHANNELS of them into the USB transmit buffer
 * after converting them into the proper USB sample format.
 ******************************************************************************/
void audio_usb_out(float *buffer[BLOCK_SIZE])
{
#if OUTPUT_ENABLED == true
	if ((!in_active) || (in_alt_setting == 0))
		return;

	SAMPLE_TYPE sample;

	for (int i = 0; i < BLOCK_SIZE; i++)
	{
		// Check if we still have enough space in the output buffer
		if (tx_buf_length + NUM_USB_CHANNELS * SUBFRAME_SIZE > OUT_BUF_SIZE)
			return;

		for (int c = 0; c < NUM_USB_CHANNELS; c++)
		{
		#ifdef SCALER
			// This part is for converting 0.16 and 0.24 fixed point samples into floats.
			// Would be nice to optimize this using ARMs built in CVCT instructions in the future.
			sample = (SAMPLE_TYPE)(buffer[c][i] * SCALER) >> (sizeof(SAMPLE_TYPE) * 8 - BIT_RESOLUTION);
		#else
			// When we're dealing with float samples we don't need to do anything.
			sample = buffer[c][i];
		#endif

			// Write the sample into the buffer and advance the buffer by
			// the required amount of bytes.
			*((SAMPLE_TYPE *)&tx_buf[tx_active_buf][tx_buf_length]) = sample;
			tx_buf_length += SUBFRAME_SIZE;
		}
	}
#endif
}

/*******************************************************************************
 * This function pulls audio samples from the circular receive buffer and
 * transfers them into the the specified array of BLOCK_SIZE sized
 * audio buffers after converting them back from the USB sample format.
 * There need to be at least NUM_USB_CHANNELS buffers to write into.
 ******************************************************************************/
void audio_usb_in(float *buffer[BLOCK_SIZE])
{
#if INPUT_ENABLED == true
	if ((!out_active) || (out_alt_setting == 0))
		return;

	num_samples += BLOCK_SIZE;

	if (!in_filled)
		return;

	for (int i = 0; i < BLOCK_SIZE; i++)
	{
		// If the buffer runs empty stop USB audio playback.
		// This value will get reset by the rx_callback function once the
		// buffer has reached a suitable level again.
		if (in_read_pos == in_write_pos)
		{
			in_filled = false;
			break;
		}

		for (int c = 0; c < NUM_USB_CHANNELS; c++)
		{
		#ifdef SCALER
			// Convert float samples to fixed point 0.16 or 0.24 samples.
			// Same as with the out function, this could use to be optimized
			// using ARMs built in fixed point float conversions
			buffer[c][i] += (in_buf[c][in_read_pos] << (sizeof(SAMPLE_TYPE) * 8 - BIT_RESOLUTION)) / (float)SCALER;
		#else
			// Don't need to do anything for float samples.
			buffer[c][i] += in_buf[c][in_read_pos];
		#endif
		}

		// This is a ring buffer, so we just increment the position and loop around
		in_read_pos = (in_read_pos + 1) % IN_BUF_SIZE;
	}
#endif
}

/*******************************************************************************
 * These functions handle USB SETUP requests on the audio interfaces.
 * The only function we implement is GET_INTERFACE and SET_INTERFACE.
 * These set and get the current interface alt setting used to switch between
 * active and zero bandwidth mode.
 ******************************************************************************/
#if OUTPUT_ENABLED == true
static bool handle_in_setup(usb_setup_packet *packet, usb_in_endpoint *in_ep, usb_out_endpoint *out_ep)
{
	switch (packet->bRequest)
	{
		// GET_INTERFACE
		case 10:
		{
			size_t size = sizeof(in_alt_setting);

			if (size > packet->wLength)
				size = packet->wLength;

			usb_transmit((uint8_t *)&in_alt_setting, size, in_ep);
		}
		break;

		// SET_INTERFACE
		case 11:
		{
			in_alt_setting = packet->wValue & 0xff;

			// Cancel all active transmissions when we're switched into zero bandwith mode
			if (in_alt_setting == 0)
				usb_cancel_transmit(audio_in_ep);
		}
		break;

		default:
			error("Unknown request %d\n", packet->bRequest);
			return false;
	}

	return true;
}
#endif

#if INPUT_ENABLED == true
static bool handle_out_setup(usb_setup_packet *packet, usb_in_endpoint *in_ep, usb_out_endpoint *out_ep)
{
	switch (packet->bRequest)
	{
		// GET_INTERFACE
		case 10:
		{
			size_t size = sizeof(out_alt_setting);

			if (size > packet->wLength)
				size = packet->wLength;

			usb_transmit((uint8_t *)&out_alt_setting, size, in_ep);
		}
		break;

		// SET_INTERFACE
		case 11:
		{
			out_alt_setting = packet->wValue & 0xff;

			// When we get switched out of zero bandwidth mode reset all buffers
			// and initialize the effective sync sample rate to out actual sample
			// rate so we have a value to report before we can calculate something
			// more accurate.
			if (out_alt_setting != 0)
			{
				sync_sample_rate = ((float)SAMPLE_RATE / 1000.0f) * (float)(1 << 14);
				num_samples = 0;
				num_frames = 0;
				in_read_pos = 0;
				in_write_pos = 0;
			}
			else
			{
				// If we're switched into zero bandwidth mode, cancel any active
				// receive commands.
				usb_cancel_receive(audio_out_ep);
			}
		}
		break;

		default:
			error("Unknown request %d\n", packet->bRequest);
			return false;
	}

	return true;
}
#endif

void audio_usb_init(void)
{
#if OUTPUT_ENABLED == true
	// This is the USB IN endpoint which whe use to send data INto the computer.
	audio_in_ep = usb_add_in_ep(EP_TYPE_ISOCHRONOUS, FRAME_SIZE, FRAME_SIZE, &in_start, &in_stop);
#endif

#if INPUT_ENABLED == true
	// This is the USB OUT endpoint, we use this to receive data that comes OUT of the computer
	audio_out_ep = usb_add_out_ep(EP_TYPE_ISOCHRONOUS, FRAME_SIZE, &out_start, &out_stop);
	usb_set_rx_callback(audio_out_ep, &rx_callback);

	// This is the USB SYNCH IN endpoint, we use it to send synchronisation data
	// (our current effective sample rate) INto the computer.
	synch_in_ep = usb_add_in_ep(EP_TYPE_ISOCHRONOUS, 8, 8, NULL, NULL);
#endif

#if OUTPUT_ENABLED == true
	// When USB output is enabled set up the output terminal descriptors.
	// These specify the connections between our actual physical jacks
	// and the USB connection. Each one gets added to both the list of
	// main USB configuration descriptors and the USB Audio Class header.
	usb_config_add_descriptor((usb_descriptor *)&audio_input_terminal);
	usb_uac_add_terminal((usb_descriptor *)&audio_input_terminal);

	usb_config_add_descriptor((usb_descriptor *)&usb_output_terminal);
	usb_uac_add_terminal((usb_descriptor *)&usb_output_terminal);
#endif

#if INPUT_ENABLED == true
	// Same thing as above, but for the input terminals.
	usb_config_add_descriptor((usb_descriptor *)&usb_input_terminal);
	usb_uac_add_terminal((usb_descriptor *)&usb_input_terminal);

	usb_config_add_descriptor((usb_descriptor *)&audio_output_terminal);
	usb_uac_add_terminal((usb_descriptor *)&audio_output_terminal);
#endif

#if (OUTPUT_ENABLED == true) || (INPUT_ENABLED == true)
	// This interface string is the name of the interface set in the
	// configuration that gets reported to the operating system.
	int interface_string = usb_config_add_string(INTERFACE_NAME);
#endif

#if OUTPUT_ENABLED == true
	// This part sets up the output interface on the IN endpoint.
	// Each USB audio interface is actually two interfaces that can be switched
	// between using their alt-settings. The first one has no endpoints and is
	// called a zero bandwidth interface. The host switches to that interface
	// When it's not using the audio interface.
	in_interface_desc_zb.iInterface = interface_string;
	usb_config_add_interface(&in_interface_desc_zb, &handle_in_setup);

	// Our second interface is the actual "active" interface which has the
	// IN endpoint associated with it and handled the actual transmission of
	// audio samples.
	in_interface_desc.iInterface = interface_string;
	usb_config_add_interface(&in_interface_desc, &handle_in_setup);
	usb_uac_add_interface(&in_interface_desc);

	// Each interface also has an associated format descriptor which informs
	// the host about which sample format and sample rate the interface supports.
	usb_config_add_descriptor((usb_descriptor *)&in_audio_interface_desc);
	usb_config_add_descriptor((usb_descriptor *)&in_audio_format_desc);

	// Here we add the actual endpoint to the interface.
	in_endpoint_desc = USB_ENDPOINT_DESCRIPTOR_INIT_AUDIO(audio_in_ep->epnum | 0x80, 0b01, 0b00, 0, audio_in_ep->max_packet_size);
	usb_config_add_descriptor((usb_descriptor *)&in_endpoint_desc);
	usb_config_add_descriptor((usb_descriptor *)&in_audio_endpoint_desc);
#endif

#if INPUT_ENABLED == true
	// Same as abobe the input part of the interface also has a
	// zero bandwidth setting interface…
	out_interface_desc_zb.iInterface = interface_string;
	usb_config_add_interface(&out_interface_desc_zb, &handle_out_setup);

	// and an active interface that actually transmits the data.
	out_interface_desc.iInterface = interface_string;
	usb_config_add_interface(&out_interface_desc, &handle_out_setup);
	usb_uac_add_interface(&out_interface_desc);

	usb_config_add_descriptor((usb_descriptor *)&out_audio_interface_desc);
	usb_config_add_descriptor((usb_descriptor *)&out_audio_format_desc);

	out_endpoint_desc = USB_ENDPOINT_DESCRIPTOR_INIT_AUDIO(audio_out_ep->epnum, 0b01, 0b00, synch_in_ep->epnum | 0x80, audio_out_ep->max_packet_size);
	usb_config_add_descriptor((usb_descriptor *)&out_endpoint_desc);
	usb_config_add_descriptor((usb_descriptor *)&out_audio_endpoint_desc);

	// In addition to the data endpoint this interface also has a synch endpoint
	// to inform the host about out effective sample rate
	synch_endpoint_desc = USB_ENDPOINT_DESCRIPTOR_INIT_SYNCH(synch_in_ep->epnum | 0x80, 0b00, 0b01, SYNC_INTERVAL, synch_in_ep->max_packet_size);
	usb_config_add_descriptor((usb_descriptor *)&synch_endpoint_desc);
#endif

	// This sets up the End Of Frame callback with the USB_Phy subsystem so
	// we can get notified about when we need to start our transmissions.
	usb_phy_add_eof_callback(&eof_callback);
}