/* 
@file midi.c

@brief

This file contains functions relating to MIDI handling.
Many shouts out to Todd Michael Bailey for the foundations of these MIDI functions.
I had to change many things to suit it to my style and needs, but I still owe you a beer.

Fundamentally, we need to send and receive MIDI messages.  We have a stack for 
receiving and transmitting messages.

@ Based heavily on code created by Todd Michael Bailey, narrat1ve.com.
Modified by Matt Heins, HackMe Electronics, 2011
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

#include "eight_bit_synth_main.h"
#include "io.h"
#include "midi.h"
#include "led_switch_handler.h"
#include "lfo.h"
#include "rockit_paraphonic.h"

MIDI_MESSAGE
	g_midi_message_incoming_fifo[MIDI_MESSAGE_INCOMING_FIFO_SIZE];		// Make an array of MIDI_MESSAGE structures.
MIDI_MESSAGE
	g_midi_message_outgoing_fifo[MIDI_MESSAGE_OUTGOING_FIFO_SIZE];		// Make an array of MIDI_MESSAGE structures.

static unsigned char auc_midi_active_notes[LENGTH_OF_ACTIVE_NOTE_ARRAY][2];		//Array of note numbers of active notes

unsigned char
	g_uc_midi_messages_in_incoming_fifo,		// How many messages in the rx queue?
	g_uc_midi_messages_in_outgoing_fifo;		// How many messages in the tx queue?
	
	
static unsigned char
	uc_midi_number_active_notes,
	uc_midi_channel_number;				// How many notes are active?;			
	

static unsigned char
	uc_midi_incoming_fifo_write_pointer,	// Where is our next write going in the fifo?
	uc_midi_incoming_fifo_read_pointer,	// Where is our next read coming from in the fifo?
	uc_midi_incoming_message_state;		// Keeps track of the state out MIDI message receiving routine is in.

static unsigned char
	uc_midi_outgoing_fifo_write_pointer,	// Where is our next write going in the fifo?
	uc_midi_outgoing_fifo_read_pointer,	// Where is our next read coming from in the fifo?
	uc_midi_outgoing_message_state;		// Keeps track of the state out MIDI message receiving routine is in.


static void init_midi_incoming_fifo(void)
// Initialize the MIDI receive fifo to empty.
{
	g_uc_midi_messages_in_incoming_fifo=0;		// No messages in FIFO yet.
	uc_midi_incoming_fifo_write_pointer=0;		// Next write is to 0.
	uc_midi_incoming_fifo_read_pointer=0;		// Next read is at 0.
}

static void init_midi_outgoing_fifo(void)
// Initialize the MIDI transmit fifo to empty.
{
	g_uc_midi_messages_in_outgoing_fifo=0;		// No messages in FIFO yet.
	uc_midi_outgoing_fifo_write_pointer=0;		// Next write is to 0.
	uc_midi_outgoing_fifo_read_pointer=0;		// Next read is at 0.
}

//Initialize the active notes array
static void 
init_midi_active_notes(void)
{
	uc_midi_number_active_notes = 0;
}

void midi_init(void)
{
	uc_midi_channel_number = DEFAULT_MIDI_CHANNEL_NUMBER;
	uc_midi_incoming_message_state=IGNORE_ME;					// Reset the midi message gathering state machine -- We need a status byte first.
	uc_midi_outgoing_message_state=READY_FOR_NEW_MESSAGE;		// Output state machine ready to begin sending bytes.
	init_midi_incoming_fifo();								// Set up the receiving buffer.
	init_midi_outgoing_fifo();								// Set up xmit buffer.
	init_midi_active_notes();								//Set up the active notes buffer
}

//void midi_add_active_note(unsigned char uc_note_number, unsigned char uc_note_velocity)
//@brief This function adds an active note to the active note array

//@param It takes the midi note number and the played note velocity

//@return Nada.
static void
midi_add_active_note(unsigned char uc_note_number, unsigned char uc_note_velocity)
{
	/*We can only hold so many notes. If it's too many, ignore.*/
	if(uc_midi_number_active_notes != LENGTH_OF_ACTIVE_NOTE_ARRAY)
	{	
		/*Add a note to the active notes array*/
		auc_midi_active_notes[uc_midi_number_active_notes][MIDI_NOTE_NUMBER] = uc_note_number;
		auc_midi_active_notes[uc_midi_number_active_notes][MIDI_NOTE_VELOCITY] = uc_note_velocity;
	
		/*Increment the number of active notes*/
		uc_midi_number_active_notes++;
	}		
}

//void midi_remove_active_note(unsigned char uc_note_number, unsigned char uc_note_velocity)
//@brief This function removes an active note from the active note array

//@param It takes the midi note number and the played note velocity

//@return Nada.
static void
midi_remove_active_note(unsigned char uc_note_number)
{
	unsigned char uc_length = LENGTH_OF_ACTIVE_NOTE_ARRAY - 1;
	unsigned char uc_search_index;
	unsigned char uc_reorder_index;
	
	/*Find this active note to remove. If too many note are held, they won't be in the array.*/
	for(uc_search_index = 0; uc_search_index != uc_length; uc_search_index++)
	{
		/*If we find a match for the passed note number, we take that note out and
		compress the array so there are no empty spaces. If we never find one, the routine exits
		and does nothing.*/
		if (auc_midi_active_notes[uc_search_index][MIDI_NOTE_NUMBER] == uc_note_number)
		{
			auc_midi_active_notes[uc_search_index][MIDI_NOTE_NUMBER] = 255;
			auc_midi_active_notes[uc_search_index][MIDI_NOTE_VELOCITY] = 0;
			
			/*Decrement the number of active notes*/
			uc_midi_number_active_notes--;
			
			/*If it's not the last note in the array, we have to move the other notes up
			to fill in the empty location*/
			if(uc_search_index != uc_midi_number_active_notes)
			{
				for(uc_reorder_index = uc_search_index; uc_reorder_index < uc_midi_number_active_notes; uc_reorder_index++)
				{
					auc_midi_active_notes[uc_reorder_index][MIDI_NOTE_NUMBER] = auc_midi_active_notes[uc_reorder_index + 1][MIDI_NOTE_NUMBER];
					auc_midi_active_notes[uc_reorder_index][MIDI_NOTE_VELOCITY] = auc_midi_active_notes[uc_reorder_index + 1][MIDI_NOTE_VELOCITY];
				}	
			}
						
			/*Turn on the last held note, if it wasn't the last*/
			if(uc_midi_number_active_notes > 0)
			{
				p_global_setting->auc_midi_note_index[OSC_1] = auc_midi_active_notes[uc_midi_number_active_notes - 1][MIDI_NOTE_NUMBER];
				p_global_setting->uc_note_velocity = auc_midi_active_notes[uc_midi_number_active_notes - 1][MIDI_NOTE_VELOCITY];
			}
			
			auc_midi_active_notes[uc_midi_number_active_notes][MIDI_NOTE_NUMBER] = 255;
			auc_midi_active_notes[uc_midi_number_active_notes][MIDI_NOTE_VELOCITY] = 0;
		}
	}					
}

//void midi_get_active_note_number(unsigned char uc_note_index)
//@brief This function returns a midi number for an active note 

//@param It takes a note index.

//@return It returns a midi note number.
unsigned char
midi_get_active_note_number(unsigned char uc_note_index)
{
	return auc_midi_active_notes[uc_note_index][MIDI_NOTE_NUMBER];
}

//void midi_get_active_note_velocity(unsigned char uc_note_index)
//@brief This function returns a midi number for an active note 

//@param It takes a note index.

//@return It returns a midi note velocity.
unsigned char
midi_get_active_note_velocity(unsigned char uc_note_index)
{
	return auc_midi_active_notes[uc_note_index][MIDI_NOTE_VELOCITY];
}

//unsigned char midi_get_number_of_active_notes(void)
//@brief This function returns the number of currently active notes - i.e., the number of keys being held

//@param It takes no parameter.

//@return It returns the number of currently active midi notes.
unsigned char
midi_get_number_of_active_notes(void)
{
	return uc_midi_number_active_notes;
}

//void get_midi_messageFromIncomingFifo(MIDI_MESSAGE *the_message)
//@brief This function pulls a message from the incoming midi message stack.

//@param It takes a pointer to a midi message and loads data into the MIDI MESSAGE from the stack.

//@return

void 
get_midi_message_from_incoming_fifo(MIDI_MESSAGE *mm_the_message)
// Returns an entire 3-byte midi message if there are any in the fifo.
// If there are no messages in the fifo, do nothing.
{
	if(g_uc_midi_messages_in_incoming_fifo>0)			// Any messages in the fifo?
	{
		(*mm_the_message).uc_message_type = g_midi_message_incoming_fifo[uc_midi_incoming_fifo_read_pointer].uc_message_type;		// Get the message at the current read pointer.
		(*mm_the_message).uc_data_byte_one = g_midi_message_incoming_fifo[uc_midi_incoming_fifo_read_pointer].uc_data_byte_one;
		(*mm_the_message).uc_data_byte_two = g_midi_message_incoming_fifo[uc_midi_incoming_fifo_read_pointer].uc_data_byte_two;	

		uc_midi_incoming_fifo_read_pointer++;			// read from the next element next time
		if(uc_midi_incoming_fifo_read_pointer >= MIDI_MESSAGE_INCOMING_FIFO_SIZE)	// handle wrapping at the end
		{
			uc_midi_incoming_fifo_read_pointer = 0;
		}

		g_uc_midi_messages_in_incoming_fifo--;		// One less message in the fifo.
	}
}

static void 
put_midi_message_in_incoming_fifo(MIDI_MESSAGE *mm_the_message)
// If there is room in the fifo, put a MIDI message into it.
// If the fifo is full, don't do anything.
{
	if(g_uc_midi_messages_in_incoming_fifo < MIDI_MESSAGE_INCOMING_FIFO_SIZE)		// Have room in the fifo?
	{
		g_midi_message_incoming_fifo[uc_midi_incoming_fifo_write_pointer].uc_message_type = (*mm_the_message).uc_message_type;	// Transfer the contents of the pointer we've passed into this function to the fifo, at the write pointer.
		g_midi_message_incoming_fifo[uc_midi_incoming_fifo_write_pointer].uc_data_byte_one = (*mm_the_message).uc_data_byte_one;
		g_midi_message_incoming_fifo[uc_midi_incoming_fifo_write_pointer].uc_data_byte_two = (*mm_the_message).uc_data_byte_two;
	
		uc_midi_incoming_fifo_write_pointer++;			// write to the next element next time
		if(uc_midi_incoming_fifo_write_pointer >= MIDI_MESSAGE_INCOMING_FIFO_SIZE)	// handle wrapping at the end
		{
			uc_midi_incoming_fifo_write_pointer = 0;
		}
		
		g_uc_midi_messages_in_incoming_fifo++;								// One more message in the fifo.
	}
}

static void 
get_midi_message_from_outgoing_fifo(MIDI_MESSAGE *mm_the_message)
// Returns the data the synth put into the output fifo.  This is generalized data and is turned into the correct midi bytes by the output handler.
// If there are no messages in the fifo, do nothing.
{
	if(g_uc_midi_messages_in_outgoing_fifo>0)			// Any messages in the fifo?
	{
		(*mm_the_message).uc_message_type = g_midi_message_outgoing_fifo[uc_midi_outgoing_fifo_read_pointer].uc_message_type;		// Get the message at the current read pointer.
		(*mm_the_message).uc_data_byte_one = g_midi_message_outgoing_fifo[uc_midi_outgoing_fifo_read_pointer].uc_data_byte_one;
		(*mm_the_message).uc_data_byte_two = g_midi_message_outgoing_fifo[uc_midi_outgoing_fifo_read_pointer].uc_data_byte_two;	
		

		uc_midi_outgoing_fifo_read_pointer++;										// read from the next element next time
		if(uc_midi_outgoing_fifo_read_pointer >= MIDI_MESSAGE_OUTGOING_FIFO_SIZE)	// handle wrapping at the end
		{
			uc_midi_outgoing_fifo_read_pointer = 0;
		}

		g_uc_midi_messages_in_outgoing_fifo--;		// One less message in the fifo.
	}
}

void 
put_midi_message_in_outgoing_fifo(unsigned char uc_the_message, 
								  unsigned char uc_the_data_byte_one, 
								  unsigned char uc_the_data_byte_two)
// If there is room in the fifo, put a MIDI message into it.  Again, this is the synth's idea of a midi message and must be interpreted by the midi output handler before it makes sense to real instruments.
// The format for passing in variables is slightly different as well (we use variables and not a pointer, as this makes it easier to use in the sampler routines). 
// If the fifo is full, don't do anything.
{
	if(g_uc_midi_messages_in_outgoing_fifo<MIDI_MESSAGE_OUTGOING_FIFO_SIZE)		// Have room in the fifo?
	{
		g_midi_message_outgoing_fifo[uc_midi_outgoing_fifo_write_pointer].uc_message_type=uc_the_message;
		g_midi_message_outgoing_fifo[uc_midi_outgoing_fifo_write_pointer].uc_data_byte_one=uc_the_data_byte_one;
		g_midi_message_outgoing_fifo[uc_midi_outgoing_fifo_write_pointer].uc_data_byte_two=uc_the_data_byte_two;
	
		uc_midi_outgoing_fifo_write_pointer++;			// write to the next element next time
		if(uc_midi_outgoing_fifo_write_pointer>=MIDI_MESSAGE_OUTGOING_FIFO_SIZE)	// handle wrapping at the end
		{
			uc_midi_outgoing_fifo_write_pointer=0;
		}
		
		g_uc_midi_messages_in_outgoing_fifo++;								// One more message in the fifo.
	}
}

void 
handle_incoming_midi_byte(unsigned char uc_the_byte)
// In this routine we sort out the bytes coming in over the UART and decide what to do.  It is state-machine based.
// This function allows for us to either act on received messages OR just toss them out and keep the MIDI state updated.  
// We want to do this when some other routine must occupy the keyboard for
// more than a MIDI byte time.
// NOTE:  We don't (yet) account for all the types of MIDI messages that exist in the world -- 
// A lot of messages will get tossed out as of now.
// This function is fed incoming midi bytes from the UART.  
// First, we check to see if the byte is a status byte.  If it is, we reset the state machine based on the 
// type of status byte.  If the byte wasn't a status byte, we plug it into the state machine to see what we should do with the data.
// So, for instance, if we get a NOTE_ON status byte, we keep the NOTE_ON context for data bytes until we get a new STATUS. 
// This allows for expansion to handle different types of status messages, and makes sure we can handle "Running Status" style NOTE messages.
// Real time messages don't mung up the channel message state machine (they don't break running status states) 
// but system common messages DO break running status.
// According to the MIDI spec, any voice / channel message should allow for running status, but it mostly seems to pertain to NOTE_ONs.
{
	static unsigned char	// Use this to store the first data byte of a midi message while we get the second byte.
		uc_first_data_byte,
		uc_not_our_message;//store the midi channel to transmit
	unsigned char ucTemp;

	MIDI_MESSAGE
		mm_the_message;
	
	if(uc_the_byte&0x80)// First Check to if this byte is a status message.  Unimplemented status bytes should fall through.
	{
/*
		// Check now to see if this is a system message which is applicable to all MIDI channels.
		// For now we only handle these Real Time messages: Timing Clock, Start, and Stop.  Real time messages shouldn't reset the state machine.
		// @@@ When we implement these realtime messages we will need to implement a MIDI_CHANNEL_NUMBER==MIDI_CHANNEL_ALL idea.

		if(uc_the_byte==MIDI_TIMING_CLOCK)
		{
//			UpdateMidiClock();				// @@@ Unimplemented.		
		}
		else if(uc_the_byte==MIDI_REAL_TIME_START)
		{
			// Queue midi message
//			mm_the_message.uc_message_type=MESSAGE_TYPE_MIDI_START;		// What kind of message is this?
//			mm_the_message.uc_data_byte_one=0;							// No databytes.
//			mm_the_message.uc_data_byte_two=0;							// And what velocity?

//			put_midi_message_in_incoming_fifo(&mm_the_message);			// Send that to the fifo.
		}
		else if(uc_the_byte==MIDI_REAL_TIME_STOP)
		{
			// Queue midi message
//			mm_the_message.uc_message_type=MESSAGE_TYPE_MIDI_STOP;		// What kind of message is this?
//			mm_the_message.uc_data_byte_one=0;							// No databytes.
//			mm_the_message.uc_data_byte_two=0;							// And what velocity?

//			put_midi_message_in_incoming_fifo(&mm_the_message);			// Send that to the fifo.
		}		

		// Not a system message we care about.  Channel / Voice Message on our channel?
*/
		if((uc_the_byte&0x0F) == uc_midi_channel_number)		// Are you talking a valid Channel?  Now see if it's a command we understand.  
		{

			ucTemp = uc_the_byte & 0xF0; //Get the first nibble to check for status byte

			switch (ucTemp)
			{

				case MIDI_NOTE_ON_MASK:			// Is the byte a NOTE_ON status byte?  Two Data bytes.
			
				uc_midi_incoming_message_state=GET_NOTE_ON_DATA_BYTE_ONE;		// Cool.  We're starting a new NOTE_ON state.
			
				break;

				case MIDI_NOTE_OFF_MASK:			// Is the byte a NOTE_OFF status byte?
			
				uc_midi_incoming_message_state=GET_NOTE_OFF_DATA_BYTE_ONE;	// We're starting NOTE_OFFs.  2 data bytes.
	
				break;

				case MIDI_PROGRAM_CHANGE_MASK:	// Program change started.  One data byte.
			
				uc_midi_incoming_message_state=GET_PROGRAM_CHANGE_DATA_BYTE;	// One data byte.  Running status applies here, too, in theory.  The CPS-101 doesn't send bytes this way, but something out there might.

				break;

				case MIDI_PITCH_WHEEL_MASK:		// Getting Pitch Wheel Data.  Pitch wheel is two data bytes, 2 data bytes, LSB then MSB.  0x2000 is no pitch change (bytes would be 0x00, 0x40 respectively).
			
				uc_midi_incoming_message_state=GET_PITCH_WHEEL_DATA_LSB;		// LSB then MSB.  AFAICT running status applies. 
	
				break;

				case MIDI_CONTROL_CHANGE_MASK:	// Control Changes (low res) have 2 data bytes -- the controller (or control) number, then the 7-bit value.
			
				uc_midi_incoming_message_state=GET_CONTROL_CHANGE_CONTROLLER_NUM;		// Control number, then value.  AFAICT running status applies. 

				break;

				default: // We don't understand this status byte, so drop out of running status.
						 //Right now this will happen if we get aftertouch info on a valid channel.
					
				uc_midi_incoming_message_state=IGNORE_ME;
			
				break;
			}
		
		}	
		else
		{
			uc_not_our_message = uc_the_byte; //Store the channel number of a message not meant for us.
			uc_midi_incoming_message_state = PASS_THE_MESSAGE_ON_BYTE_ONE;		// Message is for a different channel, or otherwise unloved.  Ignore non-status messages until we get a status byte pertinent to us.
		}
	}
	else
	// The byte we got wasn't a status byte.  Fall through to the state machine that handles data bytes.
	{

		switch(uc_midi_incoming_message_state)
		{
			case GET_NOTE_ON_DATA_BYTE_ONE:				// Get the note value for the NOTE_ON.
			if(uc_the_byte>127)								// SHOULD NEVER  HAPPEN.  If the note value is out of range (a status byte), ignore it and wait for another status.
			{
				uc_midi_incoming_message_state=IGNORE_ME;
			}
			else
			{
				uc_first_data_byte=uc_the_byte;							// Got a note on, got a valid note -- now we need to get the velocity.
				uc_midi_incoming_message_state=GET_NOTE_ON_DATA_BYTE_TWO;
			}
			break;

			case GET_NOTE_ON_DATA_BYTE_TWO:					// Check velocity, and make the hardware do a note on, note off, or bug out if there's an error.
			if(uc_the_byte==0)									// This "note on" is really a "note off".
			{
				// Queue midi message
				mm_the_message.uc_message_type=MESSAGE_TYPE_NOTE_OFF;		// What kind of message is this?
				mm_the_message.uc_data_byte_one=uc_first_data_byte;				// For what note?
				mm_the_message.uc_data_byte_two=uc_the_byte;						// And what velocity?

				put_midi_message_in_incoming_fifo(&mm_the_message);			// Send that to the fifo.
	
				uc_midi_incoming_message_state=GET_NOTE_ON_DATA_BYTE_ONE;	// And continue dealing with NOTE_ONs until we're told otherwise.
			}
			else if(uc_the_byte>127)
			{
				uc_midi_incoming_message_state=IGNORE_ME;					// Something got messed up.  The velocity value is invalid.  Wait for a new status.		
			}
			else											// Real note on, real value.
			{
				// Queue midi message
				mm_the_message.uc_message_type=MESSAGE_TYPE_NOTE_ON;	// What kind of message is this?
				mm_the_message.uc_data_byte_one=uc_first_data_byte;			// For what note?
				mm_the_message.uc_data_byte_two=uc_the_byte;					// And what velocity?

				put_midi_message_in_incoming_fifo(&mm_the_message);			// Send that to the fifo.

				uc_midi_incoming_message_state=GET_NOTE_ON_DATA_BYTE_ONE; // And continue dealing with NOTE_ONs until we're told otherwise.
			}
			break;

			case GET_NOTE_OFF_DATA_BYTE_ONE:			// Get the note value to turn off, check validity.
			if(uc_the_byte>127)								// If the note value is out of range, ignore and wait for new status. 
			{
				uc_midi_incoming_message_state=IGNORE_ME;
			}
			else
			{
				uc_first_data_byte=uc_the_byte;								// Got a note off for a valid note.  Get Velocity, like we care.
				uc_midi_incoming_message_state=GET_NOTE_OFF_DATA_BYTE_TWO;
			}		
			break;

			case GET_NOTE_OFF_DATA_BYTE_TWO:			// Get a valid velocity and turn the note off.
			if(uc_the_byte>127)								// If the note value is out of range, ignore and wait for new status. 
			{
				uc_midi_incoming_message_state=IGNORE_ME;
			}
			else
			{
				// Queue midi message
				mm_the_message.uc_message_type=MESSAGE_TYPE_NOTE_OFF;		// What kind of message is this?
				mm_the_message.uc_data_byte_one=uc_first_data_byte;				// For what note?
				mm_the_message.uc_data_byte_two=uc_the_byte;						// And what velocity?

				put_midi_message_in_incoming_fifo(&mm_the_message);			// Send that to the fifo.

				uc_midi_incoming_message_state=GET_NOTE_OFF_DATA_BYTE_ONE;	// And continue dealing with NOTE_OFFs until we're told otherwise.
			}		
			break;
		
			case GET_PROGRAM_CHANGE_DATA_BYTE:			// We got a request for program change.  Check validity and deal with it.
			if(uc_the_byte>127)								// If the note value is out of range, ignore and wait for new status. 
			{
				uc_midi_incoming_message_state=IGNORE_ME;
			}
			else
			{
				// Queue midi message
				mm_the_message.uc_message_type=MESSAGE_TYPE_PROGRAM_CHANGE;		// A program change...
				mm_the_message.uc_data_byte_one=uc_the_byte;							// ...To this program
				mm_the_message.uc_data_byte_two=0;								// And no second data byte.
				
				put_midi_message_in_incoming_fifo(&mm_the_message);			// Send that to the fifo.

				uc_midi_incoming_message_state=GET_PROGRAM_CHANGE_DATA_BYTE;	// AFAICT, theoretically, program changes are subject to running status.
			}		
			break;

			case GET_CONTROL_CHANGE_CONTROLLER_NUM:			// Get the controller number, check validity.
			if(uc_the_byte>127)									// If the value is out of range, ignore and wait for new status. 
			{
				uc_midi_incoming_message_state=IGNORE_ME;
			}
			else
			{
				uc_first_data_byte=uc_the_byte;								// Got a valid CC number.  Get the value next.
				uc_midi_incoming_message_state=GET_CONTROL_CHANGE_VALUE;
			}		
			break;

			case GET_CONTROL_CHANGE_VALUE:				// Get a valid value and queue it.
			if(uc_the_byte>127)								// If the value is out of range, ignore and wait for new status. 
			{
				uc_midi_incoming_message_state=IGNORE_ME;
			}
			else
			{
				// Queue midi message
				mm_the_message.uc_message_type=MESSAGE_TYPE_CONTROL_CHANGE;		// What kind of message is this?
				mm_the_message.uc_data_byte_one=uc_first_data_byte;					// This is the CC number.
				mm_the_message.uc_data_byte_two=uc_the_byte;							// And the value.

				put_midi_message_in_incoming_fifo(&mm_the_message);						// Send that to yr fifo.

				uc_midi_incoming_message_state=GET_CONTROL_CHANGE_CONTROLLER_NUM;		// Unlikely to see running status here, but I guess it's possible.
			}		
			break;

			case GET_PITCH_WHEEL_DATA_LSB:				// Began wanking on pitch wheel, check validity.
			if(uc_the_byte>127)								// If the note value is out of range, ignore and wait for new status. 
			{
				uc_midi_incoming_message_state=IGNORE_ME;
			}
			else
			{
				uc_first_data_byte=uc_the_byte;								// Got an LSB for the pitch wheel, now get the _important_ byte.
				uc_midi_incoming_message_state=GET_PITCH_WHEEL_DATA_MSB;
			}		
			break;

			case GET_PITCH_WHEEL_DATA_MSB:				// Get a valid MSB and queue.
			if(uc_the_byte>127)								// If the note value is out of range, ignore and wait for new status. 
			{
				uc_midi_incoming_message_state=IGNORE_ME;
			}
			else
			{
				// Queue midi message
				mm_the_message.uc_message_type=MESSAGE_TYPE_PITCH_WHEEL;	// What kind of message is this?
				mm_the_message.uc_data_byte_one=uc_first_data_byte;				// LSB
				mm_the_message.uc_data_byte_two=uc_the_byte;						// MSB

				put_midi_message_in_incoming_fifo(&mm_the_message);					// Send that to the fifo.

				uc_midi_incoming_message_state=GET_PITCH_WHEEL_DATA_LSB;			// And continue dealing with Pitch Wheel wanking until we're told otherwise.
			}		
			break;
			
			case PASS_THE_MESSAGE_ON_BYTE_ONE:
			if(uc_the_byte>127)								// If the note value is out of range, ignore and wait for new status. 
			{
				uc_midi_incoming_message_state=IGNORE_ME;
			}
			else
			{
				uc_first_data_byte=uc_the_byte;								//Get the first byte.
				uc_midi_incoming_message_state=PASS_THE_MESSAGE_ON_BYTE_TWO;
			}
			break;
			
			case PASS_THE_MESSAGE_ON_BYTE_TWO:
			if(uc_the_byte>127)								// If the note value is out of range, ignore and wait for new status. 
			{
				uc_midi_incoming_message_state=IGNORE_ME;
			}
			else
			{	//Put the message in the outgoing fifo to pass it on.
				put_midi_message_in_outgoing_fifo(uc_not_our_message, 
												uc_first_data_byte, 
												uc_the_byte);
			}
			break;

			case IGNORE_ME:
				//Compiler happiness.
			break;

			default:
			uc_midi_incoming_message_state=IGNORE_ME;			// @@@ Should never happen.		
			break;
		}	
	}
}

void
midi_interpret_incoming_message(MIDI_MESSAGE *mm_the_message, g_setting *p_global_setting)
{
	unsigned char uc_message_type;
	unsigned char uc_data_byte_one;
	unsigned char uc_data_byte_two;
	

	uc_message_type = mm_the_message->uc_message_type;
	uc_data_byte_one = mm_the_message->uc_data_byte_one;
	uc_data_byte_two = mm_the_message->uc_data_byte_two;

	switch(uc_message_type)
	{
		case MESSAGE_TYPE_NOTE_ON:

				// PARAPHONIC: Envelope flags now handled in paraphonic_note_on()
				// Don't set them here - paraphonic code decides when to trigger
				
				/*Add a note to the active notes array*/
				midi_add_active_note(uc_data_byte_one, uc_data_byte_two);
				
				/*If the arpeggiator is inactive, we play the note. Otherwise, we don't
				want to interrupt the arpeggiator*/
				if(p_global_setting->auc_synth_params[ARPEGGIATOR_MODE] == 0)
				{
					// PARAPHONIC: Route to voice allocation system
					paraphonic_note_on(uc_data_byte_one, uc_data_byte_two, p_global_setting);
					// Note: g_uc_key_press_flag is set by paraphonic_allocate_voices()
				}					
							
		break;

		case MESSAGE_TYPE_NOTE_OFF:

				/*Remove a note from the active notes array*/
				midi_remove_active_note(uc_data_byte_one);
				
				/*If there are no more active notes, then release the key press flag*/
				// PARAPHONIC: Handle note off
				paraphonic_note_off(uc_data_byte_one, p_global_setting);

						
				

				//if(uc_data_byte_one == p_global_setting->auc_midi_note_index[OSC_1])
				//	g_uc_key_press_flag = 0;			

		break;

		case MESSAGE_TYPE_CONTROL_CHANGE:

			// PARAPHONIC: Handle Mono/Juno toggle via CC 102
			if(uc_data_byte_one == 102)
			{
				paraphonic_handle_mode_cc(uc_data_byte_two, p_global_setting);
				break; // Don't process this CC further
			}

			// PARAPHONIC: Handle enable/disable via CC 103
			if(uc_data_byte_one == 103)
			{
				paraphonic_handle_enable_cc(uc_data_byte_two, p_global_setting);
				break; // Don't process this CC further
			}

			// PARAPHONIC: Handle paraphonic mode selection via CC 104
			if(uc_data_byte_one == 104)
			{
				paraphonic_handle_paraphonic_mode_cc(uc_data_byte_two, p_global_setting);
				break; // Don't process this CC further
			}
			
			// PARAPHONIC: Handle EF-101D enable/disable via CC 105
			if(uc_data_byte_one == 105)
			{
				paraphonic_handle_ef101d_cc(uc_data_byte_two, p_global_setting);
				break; // Don't process this CC further
			}


			/*I'm making an allowance for mod wheels which is typically sent on channel 1.
			So, I'm starting my numbering for midi transmitting and receiving at controller #2.
			The MIDI channels are contiguous from there on up. The Mod Wheel is sent to 
			the active LFO's LFO amount*/
			if(uc_data_byte_one == MIDI_ALL_NOTE_OFF)
			{
				/*Zero out the active notes array*/
				init_midi_active_notes();
				
				/*If there are no more active notes, then release the key press flag*/
				if(uc_midi_number_active_notes == 0)
				{
					g_uc_key_press_flag = 0;
				}
			}
			else if(uc_data_byte_one > 2)
			{
				uc_data_byte_one = uc_data_byte_one - MIDI_CONTROLLER_0_INDEX;
			}
			else
			{
				if(p_global_setting->auc_synth_params[LFO_SEL] == LFO_1)
				{
					uc_data_byte_one = LFO_1_AMOUNT;
				}
				else
				{
					uc_data_byte_one = LFO_2_AMOUNT;
				} 
			}

			if(uc_data_byte_one != PITCH_SHIFT)
			{
				p_global_setting->auc_external_params[uc_data_byte_one] = uc_data_byte_two << 1;
			}
						
			p_global_setting->auc_parameter_source[uc_data_byte_one] = SOURCE_EXTERNAL;
			
			if((uc_data_byte_one != auc_lfo_dest_decode[LFO_1][p_global_setting->auc_synth_params[LFO_1_DEST]])
			&& (uc_data_byte_one != auc_lfo_dest_decode[LFO_2][p_global_setting->auc_synth_params[LFO_2_DEST]]))
			{
				p_global_setting->auc_synth_params[uc_data_byte_one] = uc_data_byte_two << 1;
				
			}	

		//	set_led_display(p_global_setting->auc_parameter_source[uc_data_byte_one]);//diagnostic 

		break;
		
		case MESSAGE_TYPE_PITCH_WHEEL:
		
			/*This is all rough and stuff just taking the MSB, but I try to smooth the shifting in
			the calc pitch routine. Over there, I have 255 levels, so shift it up by one to make everybody happy*/
			p_global_setting->auc_synth_params[PITCH_SHIFT] = uc_data_byte_two;
		
		break;

		default:

		break;

	}

}

unsigned char 
midi_tx_buffer_not_empty(void)
{
	if(g_uc_midi_messages_in_outgoing_fifo||(uc_midi_outgoing_message_state!=READY_FOR_NEW_MESSAGE))		// Got something to say?
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

unsigned char 
pop_outgoing_midi_byte(void)
// This looks through our outgoing midi message fifo and pops the message bytes off one by one.
// It is smart enough to throw out bytes if it can use running status and make NOTE_OFFs into NOTE_ONs with a velocity of 0.
// It is the caller's responsibility to make sure there are messages in the outgoing FIFO before calling this.
// It is generally not as flexible as the midi input handler since it never has to worry about the synth doing and sending certain things.
// NOTE:  this stack doesn't include handling for real-time events which would happen OUTSIDE of running status.
// NOTE:  this stack sends generic velocity data.
// NOTE:  this stack always sends a NOTE_ON with a velocity of zero when it wants to turn a NOTE_OFF.  It never sends a NOTE_OFF byte.  AFAICT, this is how all commercial synths do it.
{
	static MIDI_MESSAGE
		mm_the_message;

	unsigned char
		uc_the_byte,
		uc_message_type_without_channel;

	static unsigned char
		uc_last_status_byte;		// Used to calculate running status.

	switch(uc_midi_outgoing_message_state)
	{
		case READY_FOR_NEW_MESSAGE:						// Finished popping off the last message.
		get_midi_message_from_outgoing_fifo(&mm_the_message);	// Get the next one.
		uc_message_type_without_channel = mm_the_message.uc_message_type & 0xF0;

		switch(uc_message_type_without_channel)					// What's the new status byte.
		{
			case MESSAGE_TYPE_NOTE_ON:
			uc_the_byte=mm_the_message.uc_message_type;	// Note on, current channel (status messages are 4 MSbs signifying a message type, followed by 4 signifying the channel number)
			if(uc_last_status_byte==uc_the_byte)								// Same status byte as last time?
			{
				uc_the_byte=mm_the_message.uc_data_byte_one;						// Set up to return the first data byte (the note number) and skip sending the status byte.
				uc_midi_outgoing_message_state=NOTE_ON_DATA_BYTE_TWO;		// Skip to second data byte next time.
			}	
			else
			{
				uc_last_status_byte=uc_the_byte;								// Update current running status.
				uc_midi_outgoing_message_state=NOTE_ON_DATA_BYTE_ONE;		// Next time send the data byte.

			}
			break;

			case MESSAGE_TYPE_NOTE_OFF:

			uc_the_byte=mm_the_message.uc_message_type;		// Note off, current channel -- THIS IS THE SAME STATUS BYTE AS NOTE ON, MIND.
			
			if(uc_last_status_byte==uc_the_byte)									// Same status byte as last time (this method keeps running status whether the application passes note_ons or note_offs)
			{
				uc_the_byte=mm_the_message.uc_data_byte_one;						// Set up to return the first data byte (the note number)
				uc_midi_outgoing_message_state=NOTE_OFF_DATA_BYTE_TWO;	// Skip to second data byte next time.
			}	
			else
			{
				uc_last_status_byte=uc_the_byte;								// Update current running status.
				uc_midi_outgoing_message_state=NOTE_OFF_DATA_BYTE_ONE;	// Next time send the data byte.

			}
			break;

			case MESSAGE_TYPE_PROGRAM_CHANGE:
			uc_the_byte=mm_the_message.uc_message_type;	// Program change, current channel.
			if(uc_last_status_byte==uc_the_byte)										// Same status byte as last time?
			{
				uc_the_byte=mm_the_message.uc_data_byte_one;						// Set up to return the first data byte (skip the status byte)
				uc_midi_outgoing_message_state=READY_FOR_NEW_MESSAGE;		// Restart state machine (PC messages only have one data byte)
			}	
			else
			{
				uc_last_status_byte=uc_the_byte;								// Update current running status.
				uc_midi_outgoing_message_state=PROGRAM_CHANGE_DATA_BYTE;	// Next time send the data byte.

			}
			break;

			case MESSAGE_TYPE_CONTROL_CHANGE:
			uc_the_byte=mm_the_message.uc_message_type;	// CC, current channel (status messages are 4 MSbs signifying a message type, followed by 4 signifying the channel number)
			if(uc_last_status_byte==uc_the_byte)										// Same status byte as last time?
			{
				uc_the_byte=mm_the_message.uc_data_byte_one;								// Set up to return the first data byte (the note number) and skip sending the status byte.
				uc_midi_outgoing_message_state=CONTROL_CHANGE_DATA_BYTE_TWO;		// Skip to second data byte next time.
			}	
			else
			{
				uc_last_status_byte=uc_the_byte;										// Update current running status.
				uc_midi_outgoing_message_state=CONTROL_CHANGE_DATA_BYTE_ONE;		// Next time send the data byte.

			}
			break;

			default:
			uc_the_byte=0;		// Make compiler happy.
			break;
		}
	return(uc_the_byte);		// Send out our byte.
	break;
	
	case NOTE_ON_DATA_BYTE_ONE:
	uc_midi_outgoing_message_state=NOTE_ON_DATA_BYTE_TWO;		// Skip to second data byte next time.
	return(mm_the_message.uc_data_byte_one);						// Return the first data byte.
	break;

	case NOTE_OFF_DATA_BYTE_ONE:
	uc_midi_outgoing_message_state=NOTE_OFF_DATA_BYTE_TWO;	// Skip to second data byte next time.
	return(mm_the_message.uc_data_byte_one);						// Return the first data byte.
	break;

	case NOTE_ON_DATA_BYTE_TWO:
	uc_midi_outgoing_message_state=READY_FOR_NEW_MESSAGE;		// Start state machine over.
	return(MIDI_GENERIC_VELOCITY);						// Return generic "note on" velocity.
	break;

	case NOTE_OFF_DATA_BYTE_TWO:
	uc_midi_outgoing_message_state=READY_FOR_NEW_MESSAGE;		// Start state machine over.
	return(0);											// Return a velocity of 0 (this means a note off)
	break;

	case PROGRAM_CHANGE_DATA_BYTE:
	uc_midi_outgoing_message_state=READY_FOR_NEW_MESSAGE;		// Start state machine over.
	return(mm_the_message.uc_data_byte_one);						// Return the first (only) data byte.
	break;

	case CONTROL_CHANGE_DATA_BYTE_ONE:
	uc_midi_outgoing_message_state=CONTROL_CHANGE_DATA_BYTE_TWO;		// Skip to second data byte next time.
	return(mm_the_message.uc_data_byte_one);								// Return the first data byte.
	break;	

	case CONTROL_CHANGE_DATA_BYTE_TWO:
	uc_midi_outgoing_message_state=READY_FOR_NEW_MESSAGE;		// Start state machine over.
	return(mm_the_message.uc_data_byte_two);						// Return the second data byte.
	break;

	default:
	return(0);
	break;
	}
}

void
set_midi_channel_number(unsigned char uc_new_midi_channel_number)
{
	/*Only 15 MIDI Channels. Limit the wraparound or errors*/
	if(uc_new_midi_channel_number < 16)
	{
		uc_midi_channel_number = uc_new_midi_channel_number;
	}
	else
	{
		uc_midi_channel_number = 15;
	}		
}

unsigned char
get_midi_channel_number(void)
{
	return uc_midi_channel_number;
}
