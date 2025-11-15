/*
@file spi.c

@brief This function initializes the hardware.


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
#include "interrupt.h"
#include "led_switch_handler.h"
#include "sys_init.h"
#include "uart.h"
#include "midi.h"
#include "arpeggiator.h"


/*
sys_init()
Configures the microcontroller
Setting up ports, timers and interrupts
External crystal oscillator frequency is 18.432MHz chosen because this is an even multiple of 48000
*/
void sys_init(void)
{
	unsigned int temp;

	//PORTA Setup
	DDRA = 0xC0; //PORTA Data Direction: 7,6 Outputs, 5,4,3,2,1,0 Inputs
	PORTA = 0xC0;

	//PORTB Setup
	DDRB = 0xFF;//Port B Data Direction:  All outputs
	PORTB = 0xFF;//1 = LED off; 

	//PORTC Setup
	DDRC = 0xFF;//Port C Data Direction: All Outputs
	PORTC = 0xFF;//Initialize all high

	//PORTD Setup
	DDRD =  0x7E;//PORTD Data Direction:  6,5,4,3,2,1 Outputs, 7,0 Input
	PORTD = 0X7F;//Initialize all high

	/*
	Timer2 Setup - 8 bit timer
	Timer 2 supplies the sample timing
	The clock is divided by 8, 18.432MHz/8 = 2.304MHz  
	Output sample frequency = 2.304MHz/(256-TCNT0)
	For Example:
	18.432MHz/48000 = 384, 384/8 = 48
	Sample frequency is stored in a global variable (un16sampleFrequency) to make it easy to change
	*/
 
	temp = 19660800/SAMPLE_FREQUENCY;//clock cycles per sample, un16sampleFrequency is set in the main 8BitSynth.c file
	temp /= 8;//divided down by 8, or the value in TCCR0B if you wanted to change it

	TCCR2B = 0x02;//clk/8 = 18.432MHz/8 = 2.304MHz
	OCR2A = (temp - 1);//set the timer output compare value - when the counter gets to this number
	      //it triggers an interrupt and the timer is reset 
	      // - 1 because the counter starts at 0!!!
	TCCR2A = 0x02;//waveform generation bits are set to normal mode - no ports are triggered
	TIMSK2 = 0x02;//Enable Timer0 Output Compare Interrupt

	/*
	Timer1 Setup - 16 bit timer
	PWM Generator
	Output A is the voltage-controlled amplifier control voltage
	Output B is the main audio output
	*/
	
	TCCR1A = (1<<COM1A1)|(1<<COM1B1)|(1<<WGM10);//Set for fast PWM,8bit, set bit at bottom, clear when counter equals compare value
	TCCR1B = (1<<WGM12)|(1<<CS10);//Set for fast PWM, no prescaler
	OCR1BL = 0;//initially set the compare to 0
	OCR1AL = 0;

	/*
	Timer0 Setup - 8 bit timer
	This generates the slow interrupt for events in the main loop
	*/
	TCCR0B = (1<<CS01)|(1<<CS00);//clk/64 = 19.6608MHz/64 = 307,200
	OCR0A = 95;// /96 = 3200Hz
	TCCR0A = 0x02; //waveform generation bits are set to normal mode - no ports are triggered
	TIMSK0 = 0x02;//enable timer output compare interrupt
	
	/*
	Configure Analog to Digital Converter - Used for reading the pots
	The AD is left-justified down to 8 bits - who needs 10 bits? Well you can if you want, but that's your
	problem
	The AD interrupt is used to set a flag to read the result of the conversion
	We can't wait for the conversion to end because we need to spit out samples
	The AD needs to be prescaled to run at a maximum of 200kHz
	*/
	ADMUX = 0x20;//left-justify the result - 8 bit resolution, AD source is ADC0
	ADCSRA = (1<<ADEN)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0);//ADC Enable, Prescale 128 
	//set the multiplexer for pots 0 and 1 here
 
	/*
	Configure I/O Expanders
	*/
  
	sei();//Interrupts need to be enabled for the configuration of the i/o expanders.
	initialize_ioexpander();//This routine is in the led_switch_handler.c file
	initialize_leds();

    /*Now that the i/o expander is configured, 
	we can turn on the external interrupt in the micro.
	The interrupt it on Port D7, which we configure with a Pin Change Interrupt
	*/
//	PCICR = 1<<PCIE3;//Enable Pin Change Interrupt Group 3
//	PCMSK3 = 1<<PCINT31;//Enable Port D7, Pin Change Interrupt 31
 
	/*
	Configure SPI
	The SPI is interrupt driven, meaning we use the interrupt to know about the end of transmission.
	*/
	SPCR = (1<<SPE)|(1<<MSTR);//|(1<<SPR1);//|(1<<SPR0);//interrupt enable, spi enable, MSB data order, fosc/4 speed

	SPSR = (1<<SPI2X);

	/*Initialize the UART for MIDI and initialize the MIDI state machine*/
	uart_init();
	midi_init();
	initialize_arpeggiator();

}
