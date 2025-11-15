/* 
@file read_ad.c

@brief

This file contains the ad reading functions. We have to step through each knob and each of the two multiplexers.
We read them one by one and compare that to the old value to determine if we need to update a parameter. There
are plenty of variables about when to actually update a parameter. Things like loading patches and such cause
parameters to be read from different places and to be updated or not. Check it out!

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
#include "io.h"
#include "read_ad.h"
#include "lfo.h"
#include "midi.h"
#include "led_switch_handler.h"
#include "drone_loop.h"


/*
@brief This function handles reading all the pots and updating the appropriate value in the appropriate place.

@param It takes the global setting structure as input

@return It doesn't return anything. It sets values in the global synth array function.
*/
void 
read_ad(volatile g_setting *p_global_setting)
{
	//local variables
	static unsigned char uc_ad_index = 0;//index to keep track of where we are in cycling through the AD channels and pots
	unsigned char uc_internal_ad_index;
	unsigned char uc_temp1 = 0;
	unsigned char uc_stored_value,
				uc_midi_status_byte;

	/*
		-read the AD,
		-compare the result to the value in the array
		-if the values don't match, a knob was turned
		-change the value in the parameter array
		-start the next AD read
	*/

	
	
	//read the AD
	uc_temp1 = ADCH;

	/*We need to load the stored value to compare the new reading to.
	  Because the LFOs share knobs, the reference needs to be linked to the appropriate
	  stored value if the knob being read is one of the LFO knobs*/
	if(uc_ad_index != LFO_RATE &&
	   uc_ad_index != LFO_AMOUNT &&
	   uc_ad_index != LFO_SHAPE)
	{
		uc_stored_value = p_global_setting->auc_ad_values[uc_ad_index];
		uc_internal_ad_index = uc_ad_index;
	}
	else if(uc_ad_index == LFO_RATE)
	{
		uc_stored_value = p_global_setting->auc_ad_values[LFO_RATE];

		if(p_global_setting->auc_synth_params[LFO_SEL] == LFO_1)
		{
			uc_internal_ad_index = LFO_1_RATE;
		}
		else
		{
			uc_internal_ad_index = LFO_2_RATE;
		}
	}
	else if(uc_ad_index == LFO_AMOUNT)
	{
		uc_stored_value = p_global_setting->auc_ad_values[LFO_AMOUNT];
		
		if(p_global_setting->auc_synth_params[LFO_SEL] == LFO_1)
		{
			uc_internal_ad_index = LFO_1_AMOUNT;
		}
		else
		{
			uc_internal_ad_index = LFO_2_AMOUNT;
		}

	}
	else
	{
		uc_stored_value = p_global_setting->auc_ad_values[LFO_SHAPE];
		
		if(p_global_setting->auc_synth_params[LFO_SEL] == LFO_1)
		{
			uc_internal_ad_index = LFO_1_WAVESHAPE;
		}
		else
		{
			uc_internal_ad_index = LFO_2_WAVESHAPE;
		}
	}

	//test the result, if it's not the same as the stored value,
	//update the value in the array and update the parameter array                                               
	if(((uc_stored_value < 254) && uc_temp1 > uc_stored_value + 1)
		|| ((uc_stored_value > 1) && uc_temp1 < uc_stored_value - 1))
	{
	
		/*Store the new value in the ad reading array*/
		p_global_setting->auc_ad_values[uc_internal_ad_index] = uc_temp1;

		/*If the index is the LFO rate or amount, we have to store the value in the 
		generic location to use it for comparison to avoid having the lfo parameters
		jump to the pot location when switching between LFOs*/
		if(	uc_ad_index == LFO_RATE   ||
			uc_ad_index == LFO_AMOUNT ||
			uc_ad_index == LFO_SHAPE)
		{
			p_global_setting->auc_ad_values[uc_ad_index] = uc_temp1;
			
		}
		
		/*If the drone or loop is active, we shift the ADSR knobs to their second purpose*/
		if(g_uc_drone_flag == TRUE || g_uc_loop_flag == TRUE)
		{
			if(uc_internal_ad_index == ADSR_DECAY)
			{
				uc_internal_ad_index = ARPEGGIATOR_MODE;
			}
		}

		/*If a waveshape pot was used, then we need to indicate the position on the led display*/
		if(uc_ad_index == LFO_SHAPE || uc_ad_index == OSC_1_WAVESHAPE || uc_ad_index == OSC_2_WAVESHAPE
		|| uc_internal_ad_index == ARPEGGIATOR_MODE)
		{
		
				handle_led_display(uc_temp1 >> 4);		
		}

		
		/*If the LFO is operating on a parameter, don't modify it here!
		Also, if the looping function is active, we don't want to change it here*/
		if((uc_internal_ad_index != auc_lfo_dest_decode[LFO_1][p_global_setting->auc_synth_params[LFO_1_DEST]]
			|| p_global_setting->auc_synth_params[LFO_1_AMOUNT] == 0)
			&& (uc_internal_ad_index != auc_lfo_dest_decode[LFO_2][p_global_setting->auc_synth_params[LFO_2_DEST]]
			|| p_global_setting->auc_synth_params[LFO_2_AMOUNT] == 0))
		{
			p_global_setting->auc_synth_params[uc_internal_ad_index] = uc_temp1;
		}
		
		
		
		//If the loop function is active
		if(g_uc_loop_flag == TRUE && p_global_setting->auc_parameter_source[uc_internal_ad_index] == SOURCE_LOOP)
		{
			set_parameter_loop_end(uc_internal_ad_index);
		}
	
		/*set the flag in the modified flag array to know that the pot value is what we want and not 
		the value loaded from the eeprom for a loaded patch
		or a value transmitted by MIDI*/
		p_global_setting->auc_parameter_source[uc_internal_ad_index] = SOURCE_AD;
		
		/*Since a parameter changed, put the change in the outgoing MIDI FIFO to be transmitted
		as a MIDI message*/
		uc_temp1 >>= 1;
		uc_internal_ad_index = uc_internal_ad_index + MIDI_CONTROLLER_0_INDEX;//MIDI controller numbers are shifted up
		uc_midi_status_byte = MESSAGE_TYPE_CONTROL_CHANGE | get_midi_channel_number();
		put_midi_message_in_outgoing_fifo(	uc_midi_status_byte, 
									  		uc_internal_ad_index, 
									  		uc_temp1);	
	}
	

	uc_ad_index++;	

	//if we've cycled through all the knobs
	if(uc_ad_index == NUMBER_OF_KNOBS)
	{
		uc_ad_index = 0;
	}

	

	/*There are 16 knobs connected to the multiplexers (knobs 0-15) and
	two knobs which are attached to independent inputs (knobs 16 and 17).  If we're 
	reading the multiplexed knobs, we have to set the multiplexer control signal and then
	cycle through all the knobs.  Then, at the end, we read the last two knobs*/
	if(uc_ad_index < NUMBER_OF_MUX_KNOBS)
	{
		//set the analog multiplexer
		//since the 16 pots are multiplexed down to four - divide the index by four
		uc_temp1 = uc_ad_index >> 2;

		set_pot_mux_sel(uc_temp1);

		//set the ADC input
		uc_temp1 = uc_ad_index%4;//cycle through AD 0 to 3 by modding the index (1 mod 4 = 1) (5 mod 4 = 1)
		ADMUX = 0x20 | uc_temp1;//the two here keeps the mux left adjusted, and we set which input we're reading
	}
	else
	{	
		/*We read knobs 16 and 17 which are connected to inputs 4 and 5, so subtract 12*/
		uc_temp1 = uc_ad_index - 12;
		ADMUX = 0x20 | uc_temp1;
	}

	
	
	
	//start the next conversion
	SET_BIT(ADCSRA, ADSC);
	
}

void
set_pot_mux_sel(unsigned char uc_index)
{
	
	switch (uc_index)
	{
		case 0:
			
			CLEAR_BIT(PORTB, PB0);
			CLEAR_BIT(PORTB, PB1);

		break;

		case 1:

			SET_BIT(PORTB, PB0);

		break;

		case 2:

			CLEAR_BIT(PORTB, PB0);
			SET_BIT(PORTB, PB1);

		break;

		case 3:

			SET_BIT(PORTB, PB0);

		break;

		default:

		break;
	}

}


/*

*/

void
initialize_pots(g_setting *p_global_setting)
{
	unsigned char 	uc_ad_index,
					uc_temp1,
					uc_ad_reading;

	for(uc_ad_index = 0; uc_ad_index < NUMBER_OF_KNOBS; uc_ad_index++)
	{
		
		/*There are 16 knobs connected to the multiplexers (knobs 0-15) and
		two knobs which are attached to independent inputs (knobs 16 and 17).  If we're 
		reading the multiplexed knobs, we have to set the multiplexer control signal and then
		cycle through all the knobs.  Then, at the end, we read the last two knobs*/
		if(uc_ad_index < NUMBER_OF_MUX_KNOBS)
		{
			//set the analog multiplexer
			//since the 16 pots are multiplexed down to four - divide the index by four
			uc_temp1 = uc_ad_index >> 2;

			set_pot_mux_sel(uc_temp1);

			//set the ADC input
			uc_temp1 = uc_ad_index%4;//cycle through AD 0 to 3 by modding the index (1 mod 4 = 1) (5 mod 4 = 1)
			ADMUX = 0x20 | uc_temp1;//the two here keeps the mux left adjusted, and we set which input we're reading
		}
		else
		{	
			/*We read knobs 16 and 17 which are connected to inputs 4 and 5, so subtract 12*/
			uc_temp1 = uc_ad_index - 12;
			ADMUX = 0x20 | uc_temp1;
		}

		//start the next conversion
		SET_BIT(ADCSRA, ADSC);

		while(CHECK_BIT(ADCSRA, ADSC));

		uc_ad_reading = ADCH;

		p_global_setting->auc_ad_values[uc_ad_index] = uc_ad_reading;
		p_global_setting->auc_synth_params[uc_ad_index] = uc_ad_reading;


	}


}

