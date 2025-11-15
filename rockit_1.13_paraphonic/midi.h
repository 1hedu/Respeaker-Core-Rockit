/*
	This file is part of Rockit.

    Rockit is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Rockit is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Rockit.  If not, see <http://www.gnu.org/licenses/>
*/

#ifndef MIDI_H
#define MIDI_H

#define DEFAULT_MIDI_CHANNEL_NUMBER 0

//locations in the active notes array
#define MIDI_NOTE_NUMBER	0
#define MIDI_NOTE_VELOCITY	1

#define MIDI_RESET_VALUE	255	//The value that tells us the position in the array is inactive

enum			// Steps in our little midi message receiving state machine.
{
	GET_NOTE_ON_DATA_BYTE_ONE=0,
	GET_NOTE_OFF_DATA_BYTE_ONE,
	GET_PROGRAM_CHANGE_DATA_BYTE,
	GET_CONTROL_CHANGE_CONTROLLER_NUM,
	GET_CONTROL_CHANGE_VALUE,
	GET_NOTE_ON_DATA_BYTE_TWO,
	GET_NOTE_OFF_DATA_BYTE_TWO,
	GET_PITCH_WHEEL_DATA_LSB,
	GET_PITCH_WHEEL_DATA_MSB,
	PASS_THE_MESSAGE_ON_BYTE_ONE,
	PASS_THE_MESSAGE_ON_BYTE_TWO,
	IGNORE_ME,
};

enum			// Steps in our little midi message transmitting state machine.
{
	READY_FOR_NEW_MESSAGE=0,
	NOTE_ON_DATA_BYTE_ONE,
	NOTE_OFF_DATA_BYTE_ONE,
	NOTE_ON_DATA_BYTE_TWO,
	NOTE_OFF_DATA_BYTE_TWO,
	PROGRAM_CHANGE_DATA_BYTE,
	CONTROL_CHANGE_DATA_BYTE_ONE,
	CONTROL_CHANGE_DATA_BYTE_TWO,
};

enum			// Types of MIDI messages we're getting.
{
	MESSAGE_TYPE_NULL=0,
	MESSAGE_TYPE_NOTE_ON,
	MESSAGE_TYPE_NOTE_OFF,
	MESSAGE_TYPE_PROGRAM_CHANGE,
	MESSAGE_TYPE_CONTROL_CHANGE,
	MESSAGE_TYPE_MIDI_START,
	MESSAGE_TYPE_MIDI_STOP,
	MESSAGE_TYPE_PITCH_WHEEL,
};

typedef struct					// Make a structure with these elements and call it a MIDI_MESSAGE.
{
	unsigned char
		uc_message_type;
	unsigned char
		uc_data_byte_one;
	unsigned char
		uc_data_byte_two;
}  MIDI_MESSAGE;

void 
midi_init(void);

unsigned char
midi_get_active_note_number(unsigned char uc_note_index);

unsigned char
midi_get_active_note_velocity(unsigned char uc_note_index);

unsigned char
midi_get_number_of_active_notes(void);

void 
get_midi_message_from_incoming_fifo(MIDI_MESSAGE *mm_the_message);

void 
put_midi_message_in_outgoing_fifo(unsigned char uc_the_message, 
								  unsigned char uc_the_data_byte_one, 
								  unsigned char uc_the_data_byte_two);
void 
handle_incoming_midi_byte(unsigned char uc_the_byte);

void
midi_interpret_incoming_message(MIDI_MESSAGE *mm_the_message, g_setting *p_global_setting);

unsigned char
pop_outgoing_midi_byte(void);

void
set_midi_channel_number(unsigned char uc_new_midi_channel_number);

unsigned char
get_midi_channel_number(void);

#define LENGTH_OF_ACTIVE_NOTE_ARRAY 12	//number of allowed active notes

#define MIDI_CHANNEL_NUMBER		0	//the default midi channel is midi channel 0

#define MIDI_CONTROLLER_0_INDEX 	2 //MIDI controller 0 is shifted up by two to make room for Mod Wheel

#define	MIDI_MESSAGE_INCOMING_FIFO_SIZE		12		// How many 4 byte messages can we queue?  The ATMEGA644 has 4k of RAM (a ton) but careful going nuts with this fifo on smaller parts (Atmega164p has 1k).
#define	MIDI_MESSAGE_OUTGOING_FIFO_SIZE		12		// How many 4 byte messages can we queue?  The ATMEGA644 has 4k of RAM (a ton) but careful going nuts with this fifo on smaller parts (Atmega164p has 1k).

extern MIDI_MESSAGE
	g_midi_message_incoming_fifo[MIDI_MESSAGE_INCOMING_FIFO_SIZE];		// Let the rest of the application know about our array of MIDI_MESSAGE structures.
extern MIDI_MESSAGE
	g_midi_message_outgoing_fifo[MIDI_MESSAGE_OUTGOING_FIFO_SIZE];		// Let the rest of the application know about our array of MIDI_MESSAGE structures.

extern unsigned char
	g_uc_midi_messages_in_incoming_fifo,			// How many messages in the rx queue?
	g_uc_midi_messages_in_outgoing_fifo;			// How many messages in the tx queue?

// Status Message Masks, Nybbles, Bytes:
//--------------------------------------

// Bytes:
#define		MIDI_TIMING_CLOCK			0xF8			// 248 (byte value)
#define		MIDI_REAL_TIME_START		0xFA			// 250 (byte value)
#define		MIDI_REAL_TIME_STOP			0xFC			// 252 (byte value)

// Bitmasks:
#define		MIDI_NOTE_ON_MASK			0x90			// IE, if you mask off the first nybble in a NOTE_ON message, it's always 1001.  These are first nybbles of the Status message, and are followed by the channel number.
#define		MIDI_NOTE_OFF_MASK			0x80			// 1000 (binary mask)
#define		MIDI_PROGRAM_CHANGE_MASK	0xC0			// 1100 (binary mask) 
#define		MIDI_PITCH_WHEEL_MASK		0xE0			// 1110 (binary mask)
#define		MIDI_CONTROL_CHANGE_MASK	0xB0			// 1011 (binary mask)
#define		MIDI_ALL_NOTE_OFF			0x7B			// 123 decimal

// Other stuff:
//--------------------------------------

#define		MIDI_GENERIC_VELOCITY		64				// When something isn't velocity sensitive or we don't care, this is value velocity is set to by the MIDI spec.


#endif /*MIDI_H*/
