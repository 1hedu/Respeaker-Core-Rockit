/*
@file interrupt_routines.c

@brief This module contains the interrupt routines.  These routines
handles things like timer interrupts, switch press interrupts, 
and spi transmission complete interrupts.


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
#include "oscillator.h"
#include "interrupt.h"
#include "interrupt_routines.h"
#include "midi.h"
#include "uart.h"
#include "spi.h"
#include "midi.h"
#include "uart.h"



/*
@brief This interrupt service routine handles Timer 2 - 8 bit timer compare match interrupts.
This interrupt sets a flag which is checked in the main routine.  Setting this flag tells the main
process to handle one of the auxiliary tasks.

@param This routine takes no parameters and returns no value.
*/

ISR(TIMER2_COMPA_vect)
{
	static unsigned char uc_last_sample = 127;
	static unsigned char uc_output = 127;

	unsigned char 	uc_temp1, 
					uc_temp2,
					uc_sample;
	unsigned int 	un_temp1,
					un_temp2;

	signed int		sn_low_pass_filter_calc;


	OCR1BL = uc_output;

	

	//if the NoteOnFlag is set, we get a sample ready for output
	if(1 == g_uc_note_on_flag)
	{													
		//If the sample reference is over the maximum, then subtract the maximum
		//so that it wraps around
		if(p_global_setting->aun_sample_reference[OSC_1] >= SAMPLE_MAX)
		{
			p_global_setting->aun_sample_reference[OSC_1] -= SAMPLE_MAX;
		}

		if(p_global_setting->aun_sample_reference[OSC_2] >= SAMPLE_MAX)
		{
			p_global_setting->aun_sample_reference[OSC_2] -= SAMPLE_MAX;
		}
		
		//Get the sample value based on the waveshape, sample reference, 
		//and the frequency index.	
		uc_temp1 = oscillator(p_global_setting->auc_synth_params[OSC_1_WAVESHAPE],
							p_global_setting->aun_sample_reference[OSC_1], 
							p_global_setting->auc_midi_note_index[OSC_1]);
		uc_temp2 = oscillator(p_global_setting->auc_synth_params[OSC_2_WAVESHAPE],
							p_global_setting->aun_sample_reference[OSC_2], 
							p_global_setting->auc_midi_note_index[OSC_2]);			

		//mix the oscillators, by scaling each and adding them together
		//the oscillator mix is controlled by the oscillator mix pot		
		//there is an option to create a routine to run on the slow interrupt to precalculate the mix variables
		//you could easily add more oscillators and add levels for each oscillator here
		un_temp1 = uc_temp1*(255 - p_global_setting->auc_synth_params[OSC_MIX]);
		
		//scale the other oscillator
		//using XOR for subtraction from 255 saves 19 clock cycles every time around
		un_temp2 = uc_temp2*(p_global_setting->auc_synth_params[OSC_MIX]);
	
		un_temp1 += un_temp2;

		//we scaled them by the oscillator mix, now add them together
		uc_sample = un_temp1>>8;		
	
		g_uc_sample_request_flag = 0;


		//low pass filter

		sn_low_pass_filter_calc = uc_sample - uc_last_sample;
		sn_low_pass_filter_calc >>= 2;
		sn_low_pass_filter_calc += uc_last_sample;

		uc_output = sn_low_pass_filter_calc;

		uc_last_sample = uc_output;

		//update the sampleReference which is used to tell where we are in the oscillator cycle
		p_global_setting->aun_sample_reference[OSC_1] += p_global_setting->aun_note_frequency[OSC_1];
		p_global_setting->aun_sample_reference[OSC_2] += p_global_setting->aun_note_frequency[OSC_2];
	
	}//end if statement
	else
	{
		uc_output = 0;	
		p_global_setting->aun_sample_reference[OSC_1] = 0;
		p_global_setting->aun_sample_reference[OSC_2] = 0;
	}

	
	

}

/*
@brief 

@param This routine takes no parameters and returns no value.
*/
ISR(TIMER0_COMPA_vect)
{
	g_uc_slow_interrupt_flag = 1;

}

/*
@brief This interrupt service routine handles the Analog to Digital Converter conversion complete interrupts.
When the the AD finishes a conversion, a flag is set and the main routine will start another conversion on
a different pin, cycling through all the AD pins.

@param This routine takes no parameters and returns no value.
*/
ISR(ADC_vect)
{
	g_uc_ad_ready_flag = TRUE;
}

/*
@brief This interrupt service routine handles the SPI transmission interrupt.
We have a buffer for SPI transmissions because all transmissions are at least two bytes and
the AVR has no built-in hardware buffer. When the SPI transmission is configured by the transmitting
routine, it loads the buffer and points the pointer to the right member for either two or three byte
transmission.  We know the transmission is complete when the point points to a null value which is the
end of the buffer.

@param This routine takes no parameters and returns no value.
*/
ISR(SPI_STC_vect)
{
	//If the buffer is empty, set the ready flag for SPI
	if(g_p_uc_spi_tx_buffer >= (g_p_uc_spi_tx_buffer_end))
	{
	  //Raise all the chip select lines
	  PORTB |= 0x1C;
	  g_uc_spi_ready_flag = 1;

    }
	//If the buffer is not empty, transmit the next byte.
	else
	{
	  //Transmit the next byte.
	  SPDR = *g_p_uc_spi_tx_buffer;
	}

    //Increment the pointer.
	g_uc_spi_tx_buffer_index++;
	g_p_uc_spi_tx_buffer = &g_auc_spi_tx_buffer[g_uc_spi_tx_buffer_index];

}



