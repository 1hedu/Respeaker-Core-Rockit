/*
@file uart.c

@brief This module handles the uart functionality which is the hardware layer for MIDI.
It initializes the hardware and handles the sending and receiving of packets.


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


#include "io.h"
#include "eight_bit_synth_main.h"
#include "uart.h"


void 
uart_init(void)
{

// This UART setup is for 31250 baud, 8 data bits, one stop bit, no parity, no flow control.
// The acutal frequency is a little bit off because I chose to use a frequency that made the 
// sample rate easier for calculation of oscillator samples.  It doesn't have to be exact.
// The way that asynchronous transmission works, there is about a 4% up or down window for frequency mismatch.
// Interrupts are disabled.

	PRR &= ~(1<<PRUSART0);					// Turn the USART power on.
	UCSR0A &= ~(1<<U2X0);					// Sets the USART to "normal rate"
	UCSR0B = ((1<<TXEN0)|(1<<RXEN0)); 		// Tx/Rx enable.  This overrides DDRs.  This turns interrupts off, too.
	UBRR0L = 38;  							// Value for normal rate 31.25k baud. Acutal rate is 31507.69 baud
											// which is within the allowable error for uarts.
	UCSR0C = ((1<<UCSZ00)|(1<<UCSZ01));		// No parity, one stop bit, 8 data bits.

	while(!uart_tx_buffer_empty())				// Waits until the transmit buffer is ready to move on.
	{
		;
	}

	while(uart_rx_buffer_has_byte())
	{
		uart_get_byte();
	}		
}

//This function returns a byte from the receive FIFO.
unsigned char
uart_get_byte(void)
{
	return UDR0;
}

//This function places a byte in the transmit buffer.
//It will be sent when the uart is unoccupied.
void
uart_transmit_byte(unsigned char uc_byte)
{
	UDR0 = uc_byte;
}

//This returns a 1 if the transmit buffer is empty.
unsigned char
uart_tx_buffer_empty(void)
{
	return UCSR0A&(1<<UDRE0);
}

//This returns a 1 if the receive buffer has a byte in it.
unsigned char
uart_rx_buffer_has_byte(void)
{
	return UCSR0A&(1<<RXC0);
}
