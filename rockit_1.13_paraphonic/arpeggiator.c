/*
@file arpeggiator.c

@brief This module contains the arpeggiator function. It generates arpeggiator patterns based on
MIDI notes played.

@ Created by Matt Heins, HackMe Electronics, 2012
This file is part of Rockit.

    Sprockit is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Sprockit is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Sprockit.  If not, see <http://www.gnu.org/licenses/>
*/

#include "eight_bit_synth_main.h"
#include "arpeggiator.h"
#include "midi.h"
#include "led_switch_handler.h"

static unsigned char uc_arpeggiator_current_active_note;		//The note that the arpeggiator is currently playing.
static unsigned int un_arpeggiator_counter;		//This stores the larger timing increment
static unsigned char uc_arpeggiator_current_step;	//The step in the arpeggiator we are currently on.

const signed char AUC_ARPEGGIATOR_PATTERNS[16][8] = 
{
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0},
	{0,1,2,3,4,5,6,7},
	{0,-1,-2,-3,-4,-5,-6,-7},
	{0,2,4,6,8,10,12,14},
	{0,4,7,12,4,7,12,16},
	{0,3,7,11,3,7,11,12},
	{0,-2,-4,-6,-8,-10,-12,-14},
	{0,5,2,6,5,8,6,10},
	{0,-5,-2,-6,-5,-8,-6,-10},
	{0,6,2,7,6,9,7,11},
	{0,-6,-2,-7,-6,-9,-7,-11},
	{0,4,7,11,4,7,11,12},
	{0,1,-1,2,-2,3,-3,0},
	{0,4,7,12,7,4,0,12},
	{0,3,7,11,7,3,0,11},			
};

void
arpeggiator_reset_current_active_note(void)
{
	uc_arpeggiator_current_active_note = 0;
}

void
arpeggiator_reset_current_step(void)
{
	uc_arpeggiator_current_step = 0;
}

void
initialize_arpeggiator(void)
{
	p_global_setting->auc_synth_params[ARPEGGIATOR_MODE] = 0;
	p_global_setting->auc_synth_params[ARPEGGIATOR_SPEED] = 127;
	p_global_setting->auc_synth_params[ARPEGGIATOR_LENGTH] =  4;
	p_global_setting->auc_synth_params[ARPEGGIATOR_GATE] = 127;
	uc_arpeggiator_current_step = 0;
	un_arpeggiator_counter = 0;
}


/*
@brief This function generates the arpeggiator pattern. It is called by the main routine if
the arpeggiator function is activated. It has multiple modes.

@param It takes the global setting structure as input. The global setting structure contains tells the
arpeggiator which mode to be in.

@return It doesn't return anything. It sets the active note and sets various flags depending on which mode
it's in.

The MIDI routine has to check to see if the arpeggiator is turned on. 

The arpeggiator will have the following parameters:
p_global_setting->auc_synth_params[ARPEGGIATOR_MODE] - This is the main mode parameter. 0 is off.
p_global_setting->auc_synth_params[ARPEGGIATOR_SPEED] - This is how fast the arpeggiator will play back.
p_global_setting->auc_synth_params[ARPEGGIATOR_LENGTH] - This is how many notes will be played.
p_global_setting->auc_synth_params[ARPEGGIATOR_GATE] - This how much of the note period is note on.

The stored arpeggiator patterns will have for each step:
a transposition - how many half-steps up or down from the original is the note


How it's gonna work - 
We have to keep track of notes held and the sequence they were hit in. The arpeggiator cycles through this 
sequence of notes performing the necessary transpositions. Once it reaches the end of the sequence, it repeats.  

*/
void
arpeggiator(g_setting *p_global_setting)
{
	
	unsigned char	uc_number_of_active_notes,	//The number of keys presently being held.
					uc_current_note_velocity,	//The stored velocity of the current note
					uc_current_note_number,		//The stored midi note number of the current note
					uc_arpeggiator_length,		//The number of notes in the arpeggiator sequence
					uc_arpeggiator_mode;		//The current mode of the arpeggiator
					
	unsigned int	un_current_gate_length,		//The on time of the current note
					un_current_note_length;		//The total time for the current note
					
	signed char		sc_current_transposition;	//The current transposition
					
				
	/*We have to know how many active notes there are and keep track of which one we are currently using.
	If we are in drone mode, then the number of notes is 1 and that note is determined by the ADSR attack knob.*/
	
	if(g_uc_drone_flag == TRUE || g_uc_loop_flag == TRUE)
	{
		uc_number_of_active_notes = 1;
	}
	else
	{
		uc_number_of_active_notes = midi_get_number_of_active_notes();
	}		
		
	/*Get our parameters*/
	un_current_gate_length = p_global_setting->auc_synth_params[ARPEGGIATOR_GATE];
	uc_arpeggiator_mode = p_global_setting->auc_synth_params[ARPEGGIATOR_MODE] >> 4;//only 16 patterns
	sc_current_transposition = AUC_ARPEGGIATOR_PATTERNS[uc_arpeggiator_mode][uc_arpeggiator_current_step];
	uc_arpeggiator_length = p_global_setting->auc_synth_params[ARPEGGIATOR_LENGTH];
	
	/*If drone is active or loop is active, then we use the ADSR release knob as the speed setting for the arpeggiator.
	But, not if the parameter is being set externally.*/
	if((g_uc_drone_flag == TRUE || g_uc_loop_flag == TRUE) && (p_global_setting->auc_parameter_source[ARPEGGIATOR_SPEED] != SOURCE_EXTERNAL))
	{
		un_current_note_length = 255 - p_global_setting->auc_ad_values[ADSR_RELEASE];
		p_global_setting->auc_synth_params[ARPEGGIATOR_SPEED] = un_current_note_length;
	}	
	else
	{
		un_current_note_length = p_global_setting->auc_synth_params[ARPEGGIATOR_SPEED];
		/*Calculate the gate turn off point. The arpeggiator gate is a percentage of time that the note is on.
		This allows for envelopes to be running.*/
		un_current_gate_length = un_current_gate_length*un_current_note_length;
		un_current_gate_length = un_current_gate_length >> 7;
			
	}
	
	un_current_note_length = un_current_note_length << 1;
	
		
	
	/*If the number of active notes is 0, we don't run the arpeggiator and we reset it.*/
	if(uc_number_of_active_notes == 0)
	{
		uc_arpeggiator_current_active_note = 0;
		un_arpeggiator_counter = 0;
		uc_arpeggiator_current_step = 0;
	}
	else/*We run the routine*/
	{
		/*If the counter has reached the end, we go to the next step*/
		if (un_arpeggiator_counter >=  un_current_note_length)
		{	/*Reset the counter*/
			un_arpeggiator_counter = 0;
			
			if(g_uc_loop_flag == FALSE && g_uc_drone_flag == FALSE)
			{
				g_uc_adsr_midi_sync_flag = 1;
				g_uc_filter_envelope_sync_flag = 1;
			}	
			
				
			/*Increment the arpeggiator step*/
			uc_arpeggiator_current_step++;
				
			/*If it went past the end of the pattern, bring it back to 0*/
			if(uc_arpeggiator_current_step == uc_arpeggiator_length)
			{
				uc_arpeggiator_current_step = 0;
			}
			
			if(g_uc_drone_flag == TRUE || g_uc_loop_flag == TRUE)
			{
				uc_number_of_active_notes = 1;
			}
			else
			{
				uc_number_of_active_notes = midi_get_number_of_active_notes();
			}
				
			/*Increment the active note.*/
			uc_arpeggiator_current_active_note++;
				
			/*If the active note number is the same as the number of active notes, loop around*/
			if(uc_arpeggiator_current_active_note >= uc_number_of_active_notes)
			{
				uc_arpeggiator_current_active_note = 0;
			}
				
			/*Get the note info*/
			if(g_uc_drone_flag == FALSE && g_uc_loop_flag == FALSE)
			{
				uc_current_note_number = midi_get_active_note_number(uc_arpeggiator_current_active_note);
				uc_current_note_velocity =	midi_get_active_note_velocity(uc_arpeggiator_current_active_note);
			}
			else
			{
				uc_current_note_number = p_global_setting->auc_synth_params[ADSR_ATTACK]>>1;
				uc_current_note_velocity =	127;
			}				
				
			/*Transpose that thing*/
			if(uc_current_note_number + sc_current_transposition > 127)
			{
				uc_current_note_number = 127;
			}
			else if(uc_current_note_number + sc_current_transposition < 0)
			{
				uc_current_note_number = 0;
			}
			else
			{
				uc_current_note_number = uc_current_note_number + sc_current_transposition;
			}
				
			p_global_setting->auc_midi_note_index[OSC_1] = uc_current_note_number;
			p_global_setting->uc_note_velocity = uc_current_note_velocity;
		}

		/*Increment the counter*/
		un_arpeggiator_counter++;
		
		if(un_arpeggiator_counter > ARPEGGIATOR_COUNTER_MAX)
		{
			un_arpeggiator_counter = 0;
		}
		
	
		///*If the current counter number is below the gate length, the note is on*/
		if((un_arpeggiator_counter < un_current_gate_length) || g_uc_loop_flag == TRUE || g_uc_drone_flag == TRUE)
		{
			g_uc_key_press_flag = 1;//Simulate a key press.	
			g_uc_note_on_flag = 1;			
		}
		else
		{
			g_uc_key_press_flag = 0;//Simulate turning a key press off.
			g_uc_note_on_flag = 0;
		}
		
		
			
	}
}