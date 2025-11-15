/*
@file spi.c

@brief This module contains functions for transmitting and receiving information over the
SPI bus. Initially, I used a bunch of i/o expanders but I ended up cutting down to just one.
Still, we have to ask that I/O expander what button was pressed when one is, and we have to send
data packets to the digital pots in the filter to let it know what frequency and how much resonance.


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
#include "spi.h"
#include "led_switch_handler.h"
#include "filter.h"

unsigned char
inline check_for_button_press();


//SPI Related Global Variables
volatile unsigned char g_auc_spi_tx_buffer[SPI_TX_BUF_LGTH];//This array provides a buffer for spi transmission.
volatile unsigned char *g_p_uc_spi_tx_buffer = &g_auc_spi_tx_buffer[0];//This pointer is used to access members 
                                                                       //of the spi tx buffer.
volatile unsigned char *g_p_uc_spi_tx_buffer_end = &g_auc_spi_tx_buffer[SPI_TX_BUF_LGTH - 1];//This pointer points to the memory 
                                                                                 //address directly after end of the spi tx buffer.
volatile unsigned char g_uc_spi_tx_buffer_index = 0;//This holds the index for spi tx buffer actions.
volatile unsigned int  g_uc_spi_enable_port;//This holds the memory mapped address of the enable port for the spi.
volatile unsigned char g_uc_spi_enable_pin;//This holds the number of the enable pin for the spi.
volatile unsigned int g_un_debounce_timer = 300;//This timer is used to debounce a switch press on an i/o expander.

static unsigned char uc_button_press_flag = 0;
static unsigned int un_debounce_timer;

/*
@brief This function is a state machine for the SPI function. Since both the buttons and the filter need to use it, 
this function acts as a traffic cop.

@param Nothing.

@return Nothing.
*/
void
spi(void)
{
	//If the debounce timer is running, we need to decrement it.
	if(un_debounce_timer > 0)
	{
	    un_debounce_timer--;
	}
	else
	{
		if(check_for_button_press() && !check_button_press_flag())
		{
			set_button_press_flag();
			set_debounce_timer(DEBOUNCE_TIMER_SET);
		}
	}

	if(CHECK_BIT(SPSR, SPIF))
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
	}

    //Increment the pointer.
	g_uc_spi_tx_buffer_index++;
	g_p_uc_spi_tx_buffer = &g_auc_spi_tx_buffer[g_uc_spi_tx_buffer_index];

	//If the SPI is currently in use, there is nothing to do here.
	if(1 == g_uc_spi_ready_flag)
	{   
		//If the debounce timer is zero, and an external interrupt has occurred, 
		//we read the i/o expander that triggered the interrupt.
		//Otherwise, we handle the filter.
		if((0 == un_debounce_timer) && 
		  check_button_press_flag())
		{
		    led_switch_handler(p_global_setting);
		}
		else
		{
			//Run the filter routine
	   		filter(p_global_setting);
		}
	}
}


//Performs a simple transmit of one byte on the SPI bus.
void spi_simple_transmit(unsigned char uc_data)
{
	g_p_uc_spi_tx_buffer = g_p_uc_spi_tx_buffer_end;	

	SPDR = uc_data;

	while(!(SPSR & (1<<SPIF)));

}

//Performs a simple read by transmitting a dummy variable.
//The dummy variable is necessary to read when the micro
//is the master in the SPI system.
unsigned char spi_simple_read(void)
{
	SPDR = 0x00;	

	while(!(SPSR & (1<<SPIF)));

	return SPDR;
}

/*
@brief This function transmits the first byte and loads the second into the
transmit buffer.  It points the transmit buffer pointer to the second member of the
spi transmit buffer so that the interrupt routine will transmit the second byte.

@param It takes two unsigned chars as bytes to transmit.

@return No value.
*/
void 
send_spi_two_bytes(unsigned char uc_byte_one, 
                   unsigned char uc_byte_two)
{
	//Clear the flag for SPI ready.
	//This flag gets set when the SPI finishes transmitting.
	g_uc_spi_ready_flag = 0;

	g_uc_spi_tx_buffer_index = 1;
	//Load the second byte in the buffer
	g_auc_spi_tx_buffer[g_uc_spi_tx_buffer_index] = uc_byte_two;

	//Point the buffer pointer
	g_p_uc_spi_tx_buffer = &g_auc_spi_tx_buffer[g_uc_spi_tx_buffer_index];	

	//Start the transmission of the first byte.
	SPDR = uc_byte_one;

	g_uc_spi_ready_flag = 0;

}


/*
@brief This function transmits the first byte and loads the second and third bytes into the
transmit buffer.  It points the transmit buffer pointer to the first member of the
spi transmit buffer so that the interrupt routine will transmit the second byte.

@param It takes three unsigned chars as bytes to transmit.

@return No value.
*/
void 
send_spi_three_bytes(unsigned char uc_byte_one, 
                     unsigned char uc_byte_two,
					 unsigned char uc_byte_three)
{
	//Clear the flag for SPI ready.
	//This flag gets set when the SPI finishes transmitting.
	g_uc_spi_ready_flag = 0;

    g_uc_spi_tx_buffer_index = 0;

    //Load the second byte into the buffer.    
    g_auc_spi_tx_buffer[g_uc_spi_tx_buffer_index] = uc_byte_two;

    //Load the second byte in the buffer
	g_auc_spi_tx_buffer[g_uc_spi_tx_buffer_index + 1] = uc_byte_three;

	//Point the buffer pointer
	g_p_uc_spi_tx_buffer = &g_auc_spi_tx_buffer[g_uc_spi_tx_buffer_index];	

	//Start the transmission of the first byte.
	SPDR = uc_byte_one;

	g_uc_spi_ready_flag = 0;

}

unsigned char
inline check_for_button_press()
{
	return !(PIND & BUTTON_PRESS_MASK);
}

void
set_button_press_flag()
{
	uc_button_press_flag = 1;
}

void
clear_button_press_flag()
{
	uc_button_press_flag = 0;
}

unsigned char
check_button_press_flag()
{
	return uc_button_press_flag;
}

void
set_debounce_timer(unsigned int un_timer_length)
{
	un_debounce_timer = un_timer_length;
}
