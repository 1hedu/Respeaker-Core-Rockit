/*
@file save_recall.c

@brief This module contains functions for saving and recalling patches.  We save the patch
		by storing the auc_synth_params array and recall them by loading the same array from 
		the eeprom.


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
#include "led_switch_handler.h"
#include "save_recall.h"
#include "io.h"
#include "eeprom.h"
#include "interrupt.h"
#include "calculate_pitch.h"
#include "lfo.h"


static unsigned int un_memory_address;

/*
@brief This function stores a patch in the eeprom by storing byte by byte the auc_synth_params array

@param It takes a patch number so that it knows which location to use.

@return Void
*/
void
save_patch(unsigned char uc_patch_number, g_setting *p_global_setting)
{
	unsigned char uc_data;
  
	//Figure out the first memory location based on the patch number.
	//The patches are stored back to back in the EEPROM, so one patch
	//uses as many bytes of storage as there are parameters, but of course
	//it starts at 0.
	un_memory_address = (uc_patch_number*NUMBER_OF_PARAMETERS);

	//Write the bytes of the auc_synth_params array one by one into the EEPROM
	for(unsigned char i = 0; i < NUMBER_OF_PARAMETERS; i++)
	{
		
		/*We have to store all the parameters from the "knob" based parameters, including
		the LFO parameters which are imaginary shared knobs first, and then we have to 
		store all the parameters which are switch based.*/
		if(i < NUMBER_OF_KNOB_PARAMETERS)
		{
			/*If the knob has been moved, we'll get the value from the AD readings.
			If it hasn't been moved, we'll get the value from the loaded params array*/
			if(p_global_setting->auc_parameter_source[i] == SOURCE_AD)
			{
				uc_data = p_global_setting->auc_ad_values[i];
			}
			else
			{
				uc_data = p_global_setting->auc_external_params[i];
			}
		}
		else
		{
			uc_data = p_global_setting->auc_synth_params[i];
		}

		eeprom_write_byte ((uint8_t *) un_memory_address, uc_data);

		//Increment the memory address for the next write
		un_memory_address++;
	}

}


/*
@brief This function loads a patch from the EEPROM into the auc_synth_params array byte by byte. Then,
		it sets all the LEDs to the correct state based on the loaded values. If the patch is empty,
		then the recall is ignored.

@param It takes a patch number so that it knows which location to load from.

@return
*/
void
recall_patch(unsigned char uc_patch_number, g_setting *p_global_setting)
{
	unsigned char uc_data;
	unsigned char uc_lfo_sel;

	/*If the patch is all zeroes, then it is not a legitimate patch and we should ignore the load*/
	unsigned char uc_patch_not_null_flag = 0;


	/*Figure out the first memory location based on the patch number.
	The patches are stored back to back in the EEPROM, so one patch
	uses as many bytes of storage as there are parameters, but of course
	it starts at 0.*/
	un_memory_address = (uc_patch_number*NUMBER_OF_PARAMETERS);

	/*Read the bytes of the EEPROM into the auc_synth_params array one by one*/
	for(unsigned char i = 0; i < NUMBER_OF_PARAMETERS; i++)
	{
		/*We have to load all the parameters into the loaded params array*/
		if(i < NUMBER_OF_PARAMETERS)
		{
			//Get the byte from the eeprom
			uc_data = eeprom_read_byte((uint8_t *) un_memory_address);

			// Load the data into the external_params array
			p_global_setting->auc_external_params[i] = uc_data;
		
		}

		/*If the patch is non-zero, set the flag so we'll know the patch is good*/
		if(uc_data != 0)
		{
			uc_patch_not_null_flag = 1;
		}
		

		/*Increment the memory address*/
		un_memory_address++;
	}

	/*If the patch was all zeroes, then it is not legitimate and we should ignore the load.
	If it was legitimate, we load the values into the synth params array and set all the 
	leds based on the loaded values.*/
	if(uc_patch_not_null_flag == 1)
	{

		/*Load the into the synth_params array to make them generally active*/
		for(unsigned char j = 0; j < NUMBER_OF_PARAMETERS; j++)
		{
			p_global_setting->auc_synth_params[j] = p_global_setting->auc_external_params[j];
		

			/*Clear the modified value flag bit vector. This tells the rest of the synth
			to use these values instead of values from the pots. */
			p_global_setting->auc_parameter_source[j] = SOURCE_EXTERNAL;
		}
		
		// Fix center-point parameters if they're at invalid values (from empty patches)
		if(p_global_setting->auc_synth_params[PITCH_SHIFT] == 0)
		{
			p_global_setting->auc_synth_params[PITCH_SHIFT] = ZERO_PITCH_BEND;
			p_global_setting->auc_external_params[PITCH_SHIFT] = ZERO_PITCH_BEND;
		}
		
		// Force minimal LFO amounts to 0 (prevents beating from amount=1)
		if(p_global_setting->auc_synth_params[LFO_1_AMOUNT] <= 1)
		{
			p_global_setting->auc_synth_params[LFO_1_AMOUNT] = 0;
			p_global_setting->auc_external_params[LFO_1_AMOUNT] = 0;
		}
		if(p_global_setting->auc_synth_params[LFO_2_AMOUNT] <= 1)
		{
			p_global_setting->auc_synth_params[LFO_2_AMOUNT] = 0;
			p_global_setting->auc_external_params[LFO_2_AMOUNT] = 0;
		}
		
		// Fix LFO parameters that cause issues when at 0
		// If LFOs have amount=0, let knobs control the destination parameters immediately
		if(p_global_setting->auc_synth_params[LFO_1_AMOUNT] == 0)
		{
			unsigned char dest = auc_lfo_dest_decode[LFO_1][p_global_setting->auc_synth_params[LFO_1_DEST]];
			p_global_setting->auc_parameter_source[dest] = SOURCE_AD;
		}
		if(p_global_setting->auc_synth_params[LFO_2_AMOUNT] == 0)
		{
			unsigned char dest = auc_lfo_dest_decode[LFO_2][p_global_setting->auc_synth_params[LFO_2_DEST]];
			p_global_setting->auc_parameter_source[dest] = SOURCE_AD;
		}
		
		// Button-controlled parameters should always be switchable
		// Don't lock them to SOURCE_EXTERNAL or buttons won't work
		p_global_setting->auc_parameter_source[LFO_SEL] = SOURCE_AD;
		p_global_setting->auc_parameter_source[LFO_1_DEST] = SOURCE_AD;
		p_global_setting->auc_parameter_source[LFO_2_DEST] = SOURCE_AD;
		p_global_setting->auc_parameter_source[LFO_SYNC] = SOURCE_AD;
		p_global_setting->auc_parameter_source[FILTER_TYPE] = SOURCE_AD;

		uc_lfo_sel = p_global_setting->auc_synth_params[LFO_SEL];

		set_lfo_sel_led_state(uc_lfo_sel);

		set_lfo_sel_leds();

		if(uc_lfo_sel == 0)
		{
			set_lfo_dest_led_state(p_global_setting->auc_synth_params[LFO_1_DEST]);
		}
		else
		{
			set_lfo_dest_led_state(p_global_setting->auc_synth_params[LFO_2_DEST]);

		}

		set_lfo_dest_leds();

		set_lfo_sync_led_state(p_global_setting->auc_synth_params[LFO_SYNC]);

		set_lfo_sync_led();

		set_filter_type_led_state(p_global_setting->auc_synth_params[FILTER_TYPE]);

		set_filter_type_leds();

	}

}

