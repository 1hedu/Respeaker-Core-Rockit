/*
@file calculate_pitch.c

@brief This module contains the functions for calculating the oscillator pitch.  It takes 
the information from MIDI like note on info and pitch bends and updates the global
note information.

@ Created by Matt Heins, HackMe Electronics, 2011
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
#include "calculate_pitch.h"
#include "lfo.h"
#include "rockit_paraphonic.h"

//This array contains the note frequencies corresponding to MIDI note values.
const unsigned int AUN_FREQ_LUT[128] = 
{8,9,9,10,10,11,12,12,13,14,15,15,16,17,18,19,21,22,23,24,26,28,29,31,33,35,37,39,41,44,46,49,52,55,58,62,
65,69,73,78,82,87,92,98,104,110,117,123,131,139,147,156,165,175,185,196,208,220,233,247,262,277,294,311,
330,349,370,392,415,440,466,494,523,554,587,622,659,698,740,784,831,880,932,988,1047,1109,1175,1245,1319,
1397,1480,1568,1661,1760,1865,1976,2093,2217,2349,2489,2637,2794,2960,3136,3322,3520,3729,3951,4186,4435,
4699,4978,5274,5588,5920,6272,6645,7040,7459,7902,8372,8870,9397,9956,10548,11175,11840,12544};

const unsigned int AUN_PORTAMENTO_LUT[8] = {32,64,128,256,512,1024,2048,4096};
	
/*
@function: calculate_pitch

@brief: The purpose of this function is to calculate the pitch of the oscillators.  This can be as straightforward
as taking the MIDI note number received from the MIDI process and looking up the frequency in the look-up table.
It can get a little more complicated as well.  Complications can come from two sources: MIDI pitch bends, and
the LFO.  Early versions of this code just skipped from one note frequency to the next.  The result is steps in 
the frequency rather than a smooth pitch bend.  Later versions remedied this problem with a bit of math.

*/

void
calculate_pitch(g_setting *p_global_setting)
{

	unsigned char 
		uc_osc2_detune,
		uc_pitch_shift,
		uc_portamento,
		uc_log_number_of_pitch_shift_increments,
		uc_temp;
		
	static unsigned char
		uc_old_midi_note_number,
		uc_old_pitch_shift,
		uc_pitch_shift_increment_counter_osc1,
		uc_pitch_shift_increment_counter_osc2,
		uc_midi_note_number_osc1_shifted,
		uc_midi_note_number_osc2_shifted,
		uc_pitch_shift_incrementer_osc1,
		uc_pitch_shift_incrementer_osc2;

	static signed int
		sn_pitch_shift_increment_osc1,
		sn_pitch_shift_increment_osc2;

	static unsigned int
		un_old_oscillator_frequency_osc1 = 0,
		un_old_oscillator_frequency_osc2 = 0;

	unsigned int		
		un_target_oscillator_frequency_osc1,
		un_target_oscillator_frequency_osc2,
		un_number_of_pitch_shift_increments;

	/*Avoid the uninitialized condition when the synth starts up*/
	if(p_global_setting->aun_note_frequency[OSC_1] == 0)
	{
		p_global_setting->aun_note_frequency[OSC_1] = AUN_FREQ_LUT[p_global_setting->auc_midi_note_index[OSC_1]];
	}

	/*Calculate oscillator 2's note value based on oscillator 1's note value and the oscillator detune*/
	/*PARAPHONIC: Skip this in paraphonic modes - OSC_2 has its own note!*/
	/*PARAPHONIC: Also skip if sub-osc mode is on - OSC_2 is set to -12 semitones*/
	if(paraphonic_get_mode() == MODE_MONOPHONIC && !paraphonic_get_subosc_mode())
	{
		uc_osc2_detune = (p_global_setting->auc_synth_params[OSC_DETUNE])>>3;

		if(uc_osc2_detune > 16)
		{
			uc_osc2_detune -= 16;
			p_global_setting->auc_midi_note_index[OSC_2] = p_global_setting->auc_midi_note_index[OSC_1] + uc_osc2_detune;

			/*Make sure we're not trying to reference a table that doesn't exist*/
			if(p_global_setting->auc_midi_note_index[OSC_2] > 127)
			{
				p_global_setting->auc_midi_note_index[OSC_2] = 127;
			}
		}
		else
		{
			uc_osc2_detune = 16 - uc_osc2_detune;

			if(uc_osc2_detune < p_global_setting->auc_midi_note_index[OSC_1])
			{
				p_global_setting->auc_midi_note_index[OSC_2] = p_global_setting->auc_midi_note_index[OSC_1] - uc_osc2_detune;
			}
			else
			{
				p_global_setting->auc_midi_note_index[OSC_2] = 0;
			}
		}
	}
	/*PARAPHONIC: In para modes, OSC_2 note is set by voice allocation, don't touch it!*/
	
	uc_portamento = p_global_setting->auc_synth_params[PORTAMENTO];

	/*This OSC_PITCH_SHIFT parameter is like a non-physical knob.
	It can be mucked with by the LFO or a MIDI pitch bend.
	We divide by two because there are only 127 MIDI note frequencies.*/

	uc_pitch_shift = p_global_setting->auc_synth_params[PITCH_SHIFT];
	
	if(uc_pitch_shift < ZERO_PITCH_BEND)
	{
		uc_temp =  ZERO_PITCH_BEND - uc_pitch_shift;
		uc_temp  = uc_temp>>2;
		uc_pitch_shift = ZERO_PITCH_BEND - uc_temp;
	}
	else if(uc_pitch_shift > ZERO_PITCH_BEND)
	{
		uc_temp =  uc_pitch_shift - ZERO_PITCH_BEND;
		uc_temp  = uc_temp>>2;
		uc_pitch_shift = ZERO_PITCH_BEND + uc_temp;
	}
	
	/*Diagnostic*/
	//	set_led_display(uc_pitch_shift>>3);
	
	/*If LFO 1's destination is the pitch, then we need to calculate its effect.
	Otherwise, calculating pitch is as easy as looking it up in the frequency table*/
	
	if((((auc_lfo_dest_decode[LFO_1][p_global_setting->auc_synth_params[LFO_1_DEST]]) == PITCH_SHIFT)
		&& (1 == g_uc_note_on_flag))
		|| (uc_pitch_shift != ZERO_PITCH_BEND)
		|| (uc_portamento != 0))
	{
		
		/*If this parameter has changed, we need to recalculate our pitch shifting increment.
		If it hasn't changed, we continue with our incrementing or if we have finished incrementing,
		we stay at the shifted frequency.
		We also need to recalculate if the user has pressed a different note.*/

		if(uc_old_pitch_shift != uc_pitch_shift ||
		   uc_old_midi_note_number != p_global_setting->auc_midi_note_index[OSC_1])
		{

			/*Store the new pitch shift and midi note number in the old
			pitch shift variable so when we come around again, we can look for change.*/
	
			uc_old_pitch_shift = uc_pitch_shift;
			uc_old_midi_note_number = p_global_setting->auc_midi_note_index[OSC_1];
			

			/*The pitch bend can be up or down, positive or negative.
			0 to 63 is a negative pitch bend.
			64 is a zero pitch bend.
			65 to 127 is a positive pitch bend.*/
			
			if(uc_pitch_shift == ZERO_PITCH_BEND)
			{
				uc_midi_note_number_osc1_shifted = p_global_setting->auc_midi_note_index[OSC_1];
				uc_midi_note_number_osc2_shifted = p_global_setting->auc_midi_note_index[OSC_2];
				
			}
			else if(uc_pitch_shift > ZERO_PITCH_BEND)
			{
				uc_pitch_shift -= ZERO_PITCH_BEND;
				uc_midi_note_number_osc1_shifted = p_global_setting->auc_midi_note_index[OSC_1] + uc_pitch_shift;

				if(uc_midi_note_number_osc1_shifted > 127)
				{
					uc_midi_note_number_osc1_shifted = 127;	
				}
		
				uc_midi_note_number_osc2_shifted = p_global_setting->auc_midi_note_index[OSC_2] + uc_pitch_shift;

				if(uc_midi_note_number_osc2_shifted > 127)
				{
					uc_midi_note_number_osc2_shifted = 127;	
				}

			}
			else//If it's a negative pitch bend...
			{
				uc_pitch_shift = ZERO_PITCH_BEND - uc_pitch_shift;

				if(p_global_setting->auc_midi_note_index[OSC_1] > uc_pitch_shift)
				{
					uc_midi_note_number_osc1_shifted = p_global_setting->auc_midi_note_index[OSC_1] - uc_pitch_shift;
				
				}
				else
				{
					uc_midi_note_number_osc1_shifted = 0;
				}

				if(p_global_setting->auc_midi_note_index[OSC_2] > uc_pitch_shift)
				{
					uc_midi_note_number_osc2_shifted = p_global_setting->auc_midi_note_index[OSC_2] - uc_pitch_shift;
				
				}
				else
				{
					uc_midi_note_number_osc2_shifted = 0;
				}

			}

			/*Ok. Now we know the target note frequency.
			We've been storing and using the current note frequency.
			We need to get from here to there in a linear fashion.
			We can calculate an increment and add it to the current frequency until
			we get to the target frequency.*/

			/*If the LFO is pitch shifting or a pitch bend.
			Get the current oscillator frequency.  It gets set by the MIDI routine or was set by a previous
			run through this routine.
			Now, it's the old one.*/

			if(uc_portamento == 0)
			{
				un_old_oscillator_frequency_osc1 = p_global_setting->aun_note_frequency[OSC_1];
				un_old_oscillator_frequency_osc2 = p_global_setting->aun_note_frequency[OSC_2]; 
			}
			/*Otherwise, portamento is on and we don't want to jump to the new note frequency*/


			/*Get the target new oscillator frequency.*/

			un_target_oscillator_frequency_osc1 = AUN_FREQ_LUT[uc_midi_note_number_osc1_shifted];
			un_target_oscillator_frequency_osc2 = AUN_FREQ_LUT[uc_midi_note_number_osc2_shifted];

			//The pitch shift can be positive or negative.  We can either segregate the cases or use
			//signed math. Let's used signed math.
			//First, calculate the difference between where we are and where we want to go.
			
			sn_pitch_shift_increment_osc1 = un_target_oscillator_frequency_osc1 
											- un_old_oscillator_frequency_osc1;

			sn_pitch_shift_increment_osc2 = un_target_oscillator_frequency_osc2
											- un_old_oscillator_frequency_osc2;

			/*Now, divide it by the number of steps we are taking to get to the new frequency
			The increment may be less than the standard number of incrementing steps between one
			frequency and the next. If it is, it will end up being 0 and it won't increment and
			we'll get the sound of steps.  So, we make sure that if it is 0, then we change it to
			1 or -1 and set the appropriate number of increments to get to the new frequency.

			We also allow for the portamento. To make it slower, we have more increments. To speed it up,
			we have less increments.
			*/
			if(uc_portamento == 0)
			{
				un_number_of_pitch_shift_increments = NUM_PITCH_SHIFT_INCREMENTS;
				uc_log_number_of_pitch_shift_increments = LOG_NUM_PITCH_SHIFT_INCREMENTS;
			}
			else
			{
				uc_portamento = uc_portamento >> 5;
				un_number_of_pitch_shift_increments = AUN_PORTAMENTO_LUT[uc_portamento];
				uc_log_number_of_pitch_shift_increments = uc_portamento + 4;

			}

			//If the pitch shift increment for oscillator 1 is positive
			if(sn_pitch_shift_increment_osc1 >= 0)
			{
				//If it's greater than the number of increments, then dividing it by that number
				//won't make the increment 0
				if(	sn_pitch_shift_increment_osc1 >= un_number_of_pitch_shift_increments)
				{
					sn_pitch_shift_increment_osc1 >>= uc_log_number_of_pitch_shift_increments;
					uc_pitch_shift_incrementer_osc1 = un_number_of_pitch_shift_increments;
				}
				else//if the increment would end up being 0, make it 1 and set for the appropriate number of increments
				{
					uc_pitch_shift_incrementer_osc1 = sn_pitch_shift_increment_osc1;
					sn_pitch_shift_increment_osc1 = 1;
				}
			}
			else//the pitch shift is negative
			{

				//If it's greater than the number of increments, then dividing it by that number
				//won't make the increment 0
				if(	sn_pitch_shift_increment_osc1 <= -un_number_of_pitch_shift_increments)
				{
					sn_pitch_shift_increment_osc1 >>= uc_log_number_of_pitch_shift_increments;
					uc_pitch_shift_incrementer_osc1 = un_number_of_pitch_shift_increments;
				}
				else//if the increment would end up being 0, make it -1 and set for the appropriate number of increments
				{
					uc_pitch_shift_incrementer_osc1 = -sn_pitch_shift_increment_osc1;
					sn_pitch_shift_increment_osc1 = -1;
				}

			}

			//If the pitch shift increment for oscillator 2 is positive
			if(sn_pitch_shift_increment_osc2 >= 0)
			{
				//If it's greater than the number of increments, then dividing it by that number
				//won't make the increment 0
				if(	sn_pitch_shift_increment_osc2 >= un_number_of_pitch_shift_increments)
				{
					sn_pitch_shift_increment_osc2 >>= uc_log_number_of_pitch_shift_increments;
					uc_pitch_shift_incrementer_osc2 = un_number_of_pitch_shift_increments;
				}
				else//if the increment would end up being 0, make it 1 and set for the appropriate number of increments
				{
					uc_pitch_shift_incrementer_osc2 = sn_pitch_shift_increment_osc1;
					sn_pitch_shift_increment_osc2 = 1;
				}
			}
			else//the pitch shift is negative
			{

				//If it's greater than the number of increments, then dividing it by that number
				//won't make the increment 0
				if(	sn_pitch_shift_increment_osc2 <= -un_number_of_pitch_shift_increments)
				{
					sn_pitch_shift_increment_osc2 >>= uc_log_number_of_pitch_shift_increments;
					uc_pitch_shift_incrementer_osc2 = un_number_of_pitch_shift_increments;
				}
				else//if the increment would end up being 0, make it -1 and set for the appropriate number of increments
				{
					uc_pitch_shift_incrementer_osc2 = -sn_pitch_shift_increment_osc1;
					sn_pitch_shift_increment_osc2 = -1;
				}
			}
		
			//Reset the counter to count the number of steps to get to the new frequency
			
			uc_pitch_shift_increment_counter_osc1 = 0;
			uc_pitch_shift_increment_counter_osc2 = 0;

			
		}//End of if the pitch shift changed

	
		//Handle the incrementing of the pitch to get to the new note frequency

		//Increment for the appropriate number of steps to get to the new frequency for oscillator 1
		if(uc_pitch_shift_increment_counter_osc1 < uc_pitch_shift_incrementer_osc1)
		{
			uc_pitch_shift_increment_counter_osc1++;
			p_global_setting->aun_note_frequency[OSC_1] += sn_pitch_shift_increment_osc1;
		}
		else
		{
			p_global_setting->aun_note_frequency[OSC_1] = AUN_FREQ_LUT[uc_midi_note_number_osc1_shifted];
		}

		un_old_oscillator_frequency_osc1 = p_global_setting->aun_note_frequency[OSC_1];

		//Increment for the appropriate number of steps to get to the new frequency for oscillator 2
		if(uc_pitch_shift_increment_counter_osc2 < uc_pitch_shift_incrementer_osc2)
		{
			uc_pitch_shift_increment_counter_osc2++;
			p_global_setting->aun_note_frequency[OSC_2] += sn_pitch_shift_increment_osc2;	
											
		}
		else
		{
			p_global_setting->aun_note_frequency[OSC_2] = AUN_FREQ_LUT[uc_midi_note_number_osc2_shifted];
		}

		un_old_oscillator_frequency_osc2 = p_global_setting->aun_note_frequency[OSC_2];
		
	}
	else
	{

		//If there is no pitch bend, then we just use the base note number for oscillator
		//and the detuned number for oscillator 2
		p_global_setting->aun_note_frequency[OSC_1] = AUN_FREQ_LUT[p_global_setting->auc_midi_note_index[OSC_1]];
		p_global_setting->aun_note_frequency[OSC_2] = AUN_FREQ_LUT[p_global_setting->auc_midi_note_index[OSC_2]];

		//Clear out the pitch shifting variables to make sure that it calculates them
		//should it be necessary. 
		sn_pitch_shift_increment_osc1 = 0;
		sn_pitch_shift_increment_osc2 = 0;
		uc_pitch_shift_increment_counter_osc1 = 0;
		uc_pitch_shift_increment_counter_osc2 = 0;
		uc_old_pitch_shift = 0;

	}
}
