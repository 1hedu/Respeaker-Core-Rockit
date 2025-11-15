/*
@file drone_loop.c

@brief This module handles the looping sequencer application. It can function in drone mode, which is just note on with
knob control over some functions, like pitch and amplitude. Or, it can function in loop mode which is like drone mode except
that it tracks knob movements and plays them back.


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
#include "filter.h"
#include "io.h"
#include "spi.h"
#include "led_switch_handler.h"
#include "lfo.h"


#define MAX_STEP					16 //The number of steps in the sequence - minus 1, remember we start at 0.
#define NUMBER_OF_INCREMENTS		32 //The number of increments between sequence steps.
#define HALF_NUMBER_OF_INCREMENTS	16
#define SHIFT_AMOUNT				3 //8 - LOG_2_NUMBER_OF_INCREMENTS = 8 - 5 = 3

unsigned char uc_current_loop_step;//This stores which step we are currently in.
unsigned char uc_next_loop_step;//The next loop step. Used for calculating the incrementer values.

unsigned char 	uc_current_increment,//The current increment between steps
				uc_next_increment,//The next increment
				uc_loop_timer,//This timer controls the length of the loop.
				auc_stored_params[MAX_STEP][NUMBER_OF_LOOP_KNOBS],
				auc_parameter_loop_end[NUMBER_OF_LOOP_KNOBS],
				auc_active_lfo_parameter_temp_storage[NUMBER_OF_LFOS];//Keeps track of where a knob loop starts				 

/*
@brief This function handles the drone/loop function. 

@param It takes the global setting structure as input

@return It doesn't return anything. It sets parameters based on points in the loop.
	Those parameters are used all over the rest of the code to make noises.
*/
void
drone_loop(g_setting *p_global_setting)
{
	static signed int 	asn_incrementer[NUMBER_OF_LOOP_KNOBS],//This array holds the incrementer values. Can be positive or negative
					  	asn_adder[NUMBER_OF_LOOP_KNOBS];//This array holds the adder values.

	signed int 	sn_adder_temp;//Temp variable for calculating adder addition.

	unsigned char 	uc_current_value,//temporary storage for calculating increments
					uc_next_value;//temporary storage for calculating increments
				
	unsigned char uc_parameter_source,//Temporary storage of the source of a parameter
				  uc_index = 0,//This keeps track of which knob we're working on.
				  uc_storage_loop_step,//This is where we'll store new values.
				  uc_lfo_1_destination,
				  uc_lfo_2_destination;//The LFO destination parameters. We must avoid conflicts with writes to the active parameter array


	/*If the drone flag is set, the pitch is controlled with the ADSR Attack knob and the level 
	is controlled by the ADSR Sustain knob*/
	p_global_setting->uc_adsr_multiplier = p_global_setting->auc_synth_params[ADSR_SUSTAIN];
	
	//If the arpeggiator is running, it will determine the note frequency
	if(p_global_setting->auc_synth_params[ARPEGGIATOR_MODE] == 0)
	{
		p_global_setting->auc_midi_note_index[OSC_1] = p_global_setting->auc_synth_params[ADSR_ATTACK] >> 1;
	}	

	if(g_uc_loop_flag == TRUE)
	{
		/*
		Loop Mode	
		1.uc_loop_timer is decremented each time through.  The length of the timer determines the length of the loop.
		2.There are a NUM_OF_LOOP_STEPS which is a power of 2 number of increments between each loop step.  
		3.This means each parameter is incremented 
		  asn_incrementer[uc_parameter_number] = (signed int)(uc_next_loop_value - uc_last_loop_value)<<(8-LOG_NUM_OF_INCREMENTS)
		  each time through.
		  This sets up an adder where the increment is some binary point value with 8 digits above the point and 
		  8 digits below the point.
			xxxx xxxx . yyyy yyyy
		  The x part is what's added to the current value 
		  The adder is
		  asn_adder[uc_parameter_number], one entry per parameter

		4.On the last time through the loop, no math is done, the synth_param value is made equal to the uc_next_loop_value
		  to account for any math errors.
		*/


		/*
		Decrement the timer. 
		If the timer is greater than 0, check the knobs for changes.
		When it reaches 0, we run the routine to increment each of the pot values
		*/
		if(uc_loop_timer-- > 0)
		{
			/*Figure out where we'll store new values.  We'll quantize to the nearest step;*/
			uc_storage_loop_step = uc_current_loop_step;

			/*If we've incremented more than halfway to the next step, quantize to the next step*/
			if(uc_current_increment < HALF_NUMBER_OF_INCREMENTS)
			{
				uc_storage_loop_step++;
				
				/*If we're at the end of the loop, the next step is 0, wrap around to the beginning*/
				if(MAX_STEP == uc_storage_loop_step)
				{
					uc_storage_loop_step = 0;
				}
			}
		
			/*Check the parameter source array for changes. If a pot has moved, it's flag will be 
			set. Loop through all parameters that can be saved and affected by the loop routine.*/
			for(uc_index = 0; uc_index < NUMBER_OF_LOOP_KNOBS; uc_index++)
			{
				
				
					uc_parameter_source = p_global_setting->auc_parameter_source[uc_index];
				
					/*Check for the pot source flag. If the pot has moved since the last time it was checked
					in the read_ad routine, this flag will be set*/
					if(SOURCE_AD == uc_parameter_source)
					{
						/*If the parameter was modified, store the new value*/
						auc_stored_params[uc_storage_loop_step][uc_index] = p_global_setting->auc_ad_values[uc_index];

						/*Clear the incrementer to stop the incrementing*/
						asn_incrementer[uc_index] = 0;

						p_global_setting->auc_synth_params[uc_index] = p_global_setting->auc_ad_values[uc_index];
				
						if(auc_parameter_loop_end[uc_index] == uc_current_loop_step)
						{
							/*If the parameter was modified, clear the flag*/
							p_global_setting->auc_parameter_source[uc_index] = SOURCE_LOOP;
						}
					}	
			}
		}
		else/*The loop timer has expired and it's time to increment the parameters*/
		{
			/*Reset the loop timer with the length set by the ADSR Release knob*/
			uc_loop_timer = p_global_setting->auc_ad_values[ADSR_RELEASE];

			uc_current_increment = uc_next_increment;

			if(0 == uc_current_increment)
			{
				
				uc_current_loop_step = uc_next_loop_step;

				uc_next_loop_step++;

				/*Increment the loop step. If we've reached the end of the loop, reset and start over*/
				if(MAX_STEP == uc_next_loop_step)
				{
			 		uc_next_loop_step = 0;
				}
					
				/*Calculate the incrementer values for the next step
				 And set all present parameters equal to the new step to complete the transition.*/
				for(uc_index = 0; uc_index < NUMBER_OF_LOOP_KNOBS; uc_index++)
				{
					
					uc_current_value = auc_stored_params[uc_current_loop_step][uc_index];

					//p_global_setting->auc_synth_params[uc_index] = uc_current_value;

					uc_next_value = auc_stored_params[uc_next_loop_step][uc_index];					

					/*If the value needs to change, calculate the increment. If not, set the 
					increment to 0, so we know we won't have to do any calculating*/
					if(uc_next_value != uc_current_value &&
						p_global_setting->auc_parameter_source[uc_index] != SOURCE_AD)
					{
						/*Calculate the incrementer value*/
						asn_incrementer[uc_index] =  uc_next_value - uc_current_value;

						/*Shift the incrementer value*/
						asn_incrementer[uc_index] <<= SHIFT_AMOUNT;//(8-LOG_NUM_OF_INCREMENTS)
					}
					else
					{
						/*Set the increment to 0 to avoid unnecessary calculations.*/
						asn_incrementer[uc_index] = 0;
					}

					asn_adder[uc_index] = 0;
				}
			}
			else/*Do the incrementing between steps*/
			{
				uc_lfo_1_destination = auc_lfo_dest_decode[LFO_1][p_global_setting->auc_synth_params[LFO_1_DEST]];
				uc_lfo_2_destination = auc_lfo_dest_decode[LFO_2][p_global_setting->auc_synth_params[LFO_2_DEST]];

				/*Loop through each knob parameter, incrementing each adder variable with the incrementer value*/
				for(uc_index = 0; uc_index < NUMBER_OF_LOOP_KNOBS; uc_index++)
				{
				
					if(uc_index != OSC_1_WAVESHAPE && uc_index != OSC_2_WAVESHAPE)
					{

						/*If the incrementer value is not 0, then increment the pot value*/
						if(0 != asn_incrementer[uc_index])
						{
						
							asn_adder[uc_index] += asn_incrementer[uc_index];
	
							sn_adder_temp = auc_stored_params[uc_current_loop_step][uc_index] + (asn_adder[uc_index] >> 8);

							/*No values below 0 or above 255!*/
							if(sn_adder_temp < 0)
							{
								p_global_setting->auc_synth_params[uc_index] = 0;
							}
							else if(sn_adder_temp > 255)
							{
								p_global_setting->auc_synth_params[uc_index] = 255;
							}

							
							/*If the LFO is working on a parameter, loading the incremented value into the synth params
							 will work against the function of the LFO, so we don't load it.  If it is the LFO active
							 parameter, then we put it in a storage location so the LFO can get at it and we won't have
							 any fights*/
							if(uc_index != uc_lfo_1_destination
							   && uc_index != uc_lfo_2_destination)
							{
								p_global_setting->auc_synth_params[uc_index] = sn_adder_temp;
							}
							else if(uc_index == uc_lfo_1_destination)
							{
								auc_active_lfo_parameter_temp_storage[LFO_1] = sn_adder_temp;
							}
							else if(uc_index == uc_lfo_2_destination)
							{
								auc_active_lfo_parameter_temp_storage[LFO_2] = sn_adder_temp;
							}
						}
					}
					else
					{
						p_global_setting->auc_synth_params[uc_index] = p_global_setting->auc_ad_values[uc_index];	
					}
				}//end for loop
			}

			/*Increment the current increment. If we've reached the maximum, then it's time
			to go to the next step.*/
			if(NUMBER_OF_INCREMENTS == ++uc_next_increment)
			{
				uc_next_increment = 0;//Start over.
			}	
		}
	}
}

/*
@brief initialize_loop clears out the parameters in the loop storage arrays. Starts everything with a clean slate. 

@param It takes the global setting structure as input

@return It doesn't return anything.
*/
void
initialize_loop(g_setting *p_global_setting)
{
	uc_current_increment = 0;
	uc_current_loop_step = 0;
	uc_next_loop_step = 1;
	uc_next_increment = 0;//The next increment
	uc_loop_timer = p_global_setting->auc_ad_values[ADSR_RELEASE];//This timer controls the length of the loop.
	
	for (int i = 0; i < MAX_STEP; i++)
	{
		for(int j = 0; j < NUMBER_OF_LOOP_KNOBS; j++)
		{
			auc_stored_params[i][j] = p_global_setting->auc_ad_values[j];

		}
	}

	for(int k = 0; k < NUMBER_OF_LOOP_KNOBS; k++)
	{
		p_global_setting->auc_parameter_source[k] = SOURCE_LOOP;
		auc_parameter_loop_end[k] = 0;
	}

					 
}

/*
@brief set_parameter_loop_end keeps track of where the the loop starts for each knob. Each knob will
loop around for an entire loop length regardless of where the entire loop has its beginning and end.
This ends up making the loop come back around to the point where the knob was last touched. Make sense?

@param It takes the global setting structure as input

@return It doesn't return anything.
*/
void
set_parameter_loop_end(unsigned char uc_knob_index)
{
	unsigned char uc_last_step;


	if(uc_current_loop_step != 0)
	{	
		uc_last_step = uc_current_loop_step - 1;
	}
	else
	{
		uc_last_step = MAX_STEP - 1;
	}


	if(uc_knob_index <= NUMBER_OF_LOOP_KNOBS)
	{
		auc_parameter_loop_end[uc_knob_index] = uc_current_loop_step;
	}
}


/*
@brief get_current_loop_step does what it says for external functions to know where
we are in the loop.

@param No input.

@return It returns a loop step.
*/
unsigned char
get_current_loop_step(void)
{
	return uc_current_loop_step;

}

/*
@brief get_loop_stored_parameter is accessed by the lfo to know what value to use for the LFO
since the parameter may be changing through the loop.

@param What parameter do you want to know about?

@return It returns a value of the parameter at this point in the loop.
*/
unsigned char
get_loop_stored_parameter(unsigned char uc_lfo_index)
{
	return auc_active_lfo_parameter_temp_storage[uc_lfo_index];
}
