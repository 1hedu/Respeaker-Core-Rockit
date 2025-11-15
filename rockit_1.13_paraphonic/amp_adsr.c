/*
@file amp_adsr.c

@brief This module contains the adsr (attack, decay, sustain, release) amplitude envelope generator.
The routine is based on a timer that controls how long it takes to get from 
one phase of the envelope to the next.  It's nothing fancy, just set the timer, wait for the
timer to run out and increment the multiplier.  Once it gets to the target value, it moves to the
next stage. Sustain just holds the value as long as the note is being played.

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
#include "amp_adsr.h"
#include "io.h"

static unsigned char uc_adsr_timer = 0;
static unsigned char uc_state = 0;
static unsigned char uc_sustain_level = 127;
static unsigned char uc_velocity = 127;


/*
@brief This function sets the overall amplitude of the synth. 
		It blends together amplitude data from the LFO, MIDI, and the ADSR envelope.
@param It takes the global setting structure as input

@return It doesn't return anything. It sets the amplitude by setting the output 
		which is a PWM output for the transconductance amplifier based voltage-controlled
		amplifier.
*/
void
set_amplitude(g_setting *p_global_setting)
{
	unsigned int un_amplitude_temp;
	
	un_amplitude_temp = p_global_setting->uc_adsr_multiplier;//From the ADSR
	un_amplitude_temp *= p_global_setting->auc_synth_params[AMPLITUDE];//This parameter comes from the LFO or Drone Mode.

	un_amplitude_temp >>= 8;

	/*The ADSR has a minimum value which is not zero. This is an artifact of the 
	transconductance amplifier and the way that it works. Two Base-Emitter junction drops
	if you really want to know. Check out the datasheet for serious details.*/
	if(un_amplitude_temp < ADSR_MIN_VALUE)
	{
		un_amplitude_temp = ADSR_MIN_VALUE;
	}

	/*Set the PWM duty cycle which sets the amplitude of the voltage-controlled amplifier.*/
	OCR1AL = (unsigned char)un_amplitude_temp;

}


/*
@brief The adsr function calculates the amplitude envelope for the ADSR function. It's 
based around a central timer which is counting down. The number of countdown cycles
determines how long each stage of the envelope takes. 
There are different ways to handle the ADSR. This method does not start from zero with 
every new note. It's hard with an analog ADSR to avoid pops and clicks with sudden large
changes in amplitude. You're free to try to fix that problem.

@param It takes the global setting structure.

@return It returns nothing, just setting the adsr multiplier variable in the global structure.
*/
void 
adsr(g_setting *p_global_setting)
{
	
	unsigned int un_sustain_calc;
	unsigned int un_velocity_calc;

	if(g_uc_adsr_midi_sync_flag == 1)
	{
		uc_state = ATTACK;
		g_uc_adsr_midi_sync_flag = 0;	
		
		//Calculate the maximum based on the MIDI note velocity as long as we're not in drone mode
		if(!g_uc_drone_flag && !g_uc_loop_flag)
		{
			un_velocity_calc = p_global_setting->uc_note_velocity << 1;
			un_velocity_calc *= (NUMBER_OF_ADSR_STEPS - ADSR_MIN_VALUE);
			uc_velocity = un_velocity_calc >> 8;
			uc_velocity += ADSR_MIN_VALUE;
		}
		else
		{
			uc_velocity = 255;
		}
			
	}


	//if the key is released, move directly to release
	//if the key has bee pressed, turn on the note on Flag and the sequence will begin with attack
	if(0 == g_uc_key_press_flag)
	{
		uc_state = RELEASE;
	}
	else
	{
		g_uc_note_on_flag = 1;//turn on the output
	}

	if(uc_adsr_timer > 0)
	{
		uc_adsr_timer--;		

	}
	else
	{
		/*Four state for the ADSR envelope (Obviously!)
		Attack is an upward progression.
		Decay brings the amplitude down to the sustain level.
		Release comes in when the key is released and takes the amplitude down to zero.
		If you want to know more, wikipedia awaits your questioning mind.
		I broke up sections of the knob travel to make much longer and much shorter lengths of time
		possible with only 8 bits. That's what all the if/else statements in each phase are about.
		*/
		switch(uc_state)
		{

			case ATTACK:
			
				if(p_global_setting->uc_adsr_multiplier < uc_velocity)
				{	
					
					if(p_global_setting->auc_synth_params[ADSR_ATTACK] < 64)
					{
						uc_adsr_timer = p_global_setting->auc_synth_params[ADSR_ATTACK]>>3;
						
						if(p_global_setting->uc_adsr_multiplier < 251)
						{
							p_global_setting->uc_adsr_multiplier += 4;
						}
						else
						{
							p_global_setting->uc_adsr_multiplier = NUMBER_OF_ADSR_STEPS;
						}
					}
					else if(p_global_setting->auc_synth_params[ADSR_ATTACK] < 192)
					{
						uc_adsr_timer = p_global_setting->auc_synth_params[ADSR_ATTACK]>>2;
						
						if(p_global_setting->uc_adsr_multiplier < 253)
						{
							p_global_setting->uc_adsr_multiplier += 2;
						}
						else
						{
							p_global_setting->uc_adsr_multiplier = NUMBER_OF_ADSR_STEPS;
						}
					}
					else
					{
						uc_adsr_timer = p_global_setting->auc_synth_params[ADSR_ATTACK]>>1;
						
						if(p_global_setting->uc_adsr_multiplier < 253)
						{
							p_global_setting->uc_adsr_multiplier += 1;
						}
						else
						{
							p_global_setting->uc_adsr_multiplier = NUMBER_OF_ADSR_STEPS;
						}
					}
				}
				else
				{
					uc_state = DECAY;
			
					uc_adsr_timer = p_global_setting->auc_synth_params[ADSR_DECAY]>>2;
					
					/*The minimum is not zero because the VCA has a 1.3V zero point, so we have to shift
					the knob value to be between the VCA zero point and the maximum*/
					un_sustain_calc = uc_velocity * p_global_setting->auc_synth_params[ADSR_SUSTAIN];

					un_sustain_calc >>= 8;

					un_sustain_calc *= (NUMBER_OF_ADSR_STEPS - SUSTAIN_MIN_VALUE);
					un_sustain_calc >>= 8;
					uc_sustain_level = un_sustain_calc + SUSTAIN_MIN_VALUE;
				}
				
			break;

			case DECAY:


				if(p_global_setting->uc_adsr_multiplier > uc_sustain_level)
				{
					

					if(p_global_setting->auc_synth_params[ADSR_DECAY] < 48)
					{
						uc_adsr_timer = p_global_setting->auc_synth_params[ADSR_DECAY] >> 3;
						if(p_global_setting->uc_adsr_multiplier > 4)
						{
							p_global_setting->uc_adsr_multiplier -= 4;
						}
						else
						{
							p_global_setting->uc_adsr_multiplier = 0;
						}
					}
					else if(p_global_setting->auc_synth_params[ADSR_DECAY] < 96)
					{	
						uc_adsr_timer = p_global_setting->auc_synth_params[ADSR_DECAY] >> 2;
						if(p_global_setting->uc_adsr_multiplier > 2)
						{
							p_global_setting->uc_adsr_multiplier -= 2;
						}
						else
						{
							p_global_setting->uc_adsr_multiplier = 0;
						}
					}
					else
					{
						uc_adsr_timer = p_global_setting->auc_synth_params[ADSR_DECAY];
						p_global_setting->uc_adsr_multiplier--;
					}

				}
				else
				{
					uc_state = SUSTAIN;

				}	

			break;

			case SUSTAIN:

			break;

			case RELEASE:

				//if the note got pressed again, start over
				if(1 == g_uc_key_press_flag)
				{
					
					uc_state = ATTACK;
					uc_adsr_timer = p_global_setting->auc_synth_params[ADSR_ATTACK];
				}
				else//otherwise keep going with the release
				{
					/*There is a non-zero minimum caused by the analog voltage-controlled
					  amplifier*/
					if(p_global_setting->uc_adsr_multiplier > ADSR_MIN_VALUE)
					{
						p_global_setting->uc_adsr_multiplier--;
	
						uc_adsr_timer = p_global_setting->auc_synth_params[ADSR_RELEASE]>>2;

					}
					else
					{
						//when the envelope has been completed, return to attack and clear the note on Flag
						uc_state = ATTACK;
	
						uc_adsr_timer = p_global_setting->auc_synth_params[ADSR_ATTACK];

						g_uc_note_on_flag = 0;//end of that note

					}
				}
			break;
			
			default:

			break;
		}
	}

}

/*
@brief get_adsr_state does like it says and returns the current adsr state in the case
that an external function would like to know.

@param Nada.

@return Returns a number which corresponds to a ADSR state. 0 = Attack, etc.
*/
unsigned char
get_adsr_state(void)
{
	return uc_state;
}

/*
@brief set_adsr_state allows an external function to set the ADSR state.  

@param A number corrresponding to an adsr state.

@return Nothing.
*/
void
set_adsr_state(unsigned char uc_adsr_state)
{
	uc_state = uc_adsr_state;
}
