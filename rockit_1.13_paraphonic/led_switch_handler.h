/*
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

#ifndef LED_SWITCH_HANDLER_H
#define LED_SWITCH_HANDLER_H

//I/O Expander Handler State Machine States
#define READ    			0
#define SET_LEDS 			1
#define LFO_SELECT_CHANGE	2
#define CHANGE_MIDI_CHANNEL	3

//Drone Loop States
#define DRONE	1
#define LOOP 	2

//I/O Expander SPI constants
#define IOEXPANDER_U15_WRITE	0x44	//01000100
#define IOEXPANDER_U15_READ		0x45	//01000101
#define IOEXPANDER_REG_INTCAP	0x08
#define IOEXPANDER_REG_IODIR 	0x00	//i/o expander i/o direction register address - determines whether in or out
#define IOEXPANDER_REG_GPIO		0x09	//i/o expander i/o value register address -  determines whether high or low
#define IOEXPANDER_ALL_OUTPUTS	0x00	//values for the IODIR to tell it to make all i/o outputs
#define SWITCH_CS_PORT          PORTB   //The port for the switch chip select.
#define SWITCH_CS_ENABLE        0x10    //The pin for switch chip select enable.

//The number of each tact switch in terms of the input on the i/o expander.
//The input number shows up as a zero in the byte.
#define TACT_LFO_SYNC		0xFE
#define	TACT_LFO_DEST		0xFD
#define TACT_FILTER_TYPE    0xFB
#define TACT_LFO_SEL      	0xF7
#define TACT_SAVE   		0xEF
#define TACT_RECALL        	0xDF
#define TACT_SELECT      	0xBF
#define TACT_DRONELOOP     	0x7F

/*The mask for each LED, basically a 1 at whichever number matches the LED input*/
#define LFO_DEST_4_MASK		0x04
#define LFO_DEST_6_MASK		0x08
#define LFO_DEST_2_MASK		0x10
#define LFO_DEST_5_MASK		0x20
#define LFO_DEST_3_MASK		0x40
#define LFO_DEST_1_MASK		0x80

#define LFO_2_SEL_MASK		0x02
#define	BAND_PASS_MASK		0x04
#define HIGH_PASS_MASK		0x08
#define LOW_PASS_MASK		0x10
#define DRONE_LOOP_MASK		0x20
#define LFO_SYNC_MASK		0x40
#define LFO_1_SEL_MASK		0x80

//The states for the LFO Destination LEDs.
#define LFO_DEST_1   	0
#define LFO_DEST_2   	1
#define LFO_DEST_3   	2
#define LFO_DEST_4   	3
#define LFO_DEST_5 		4
#define LFO_DEST_6		5

#define LFO_SYNC_OFF	0
#define LFO_SYNC_ON		1


//The states for the LFO Select LEDs
#define LFO_SEL_1  		0
#define LFO_SEL_2  		1
#define LFO_SEL_1_2		2

//The states for the Filter Type LEDs
#define LED_LOW_PASS  0
#define	LED_BAND_PASS 1
#define LED_HIGH_PASS 2

//The states for LED MUXing
#define LED_MUX_1	0
#define LED_MUX_2	1
#define LED_MUX_3	2

//The states for the sample rate LEDs
#define LED_SAMPLE_RATE_NORMAL	0
#define LED_SAMPLE_RATE_LOFI	1
#define LED_SAMPLE_RATE_DIRT	2

//The maximum number of patches
#define MAX_PATCH_NUMBER 	15

//The LED multiplex timer reload value
#define LED_MUX_TIMER_RELOAD 	10

//The bit pattern for the 7 segment display
//      Digit	Hex	  abcdefgx
#define DIGIT0 0x02	//00000010 10000000
#define DIGIT1 0x9E //10011110 11110010
#define DIGIT2 0x24	//00100100 01001000
#define DIGIT3 0x0C //00001100 01100000
#define DIGIT4 0x98 //10011000 00110010
#define DIGIT5 0x48 //01001000 00100100
#define DIGIT6 0x40 //01000000 00000100
#define DIGIT7 0x1E //00011110 11110000
#define DIGIT8 0x00 //00000000
#define DIGIT9 0x18 //00011000 00110000
#define DIGITA 0x10	//00010000 00010000
#define DIGITB 0xC0	//11000000 00000110
#define DIGITC 0x62	//01100010 10001100
#define DIGITD 0x84	//10000100 01000010
#define DIGITE 0x60	//01100000 00001100
#define DIGITF 0x70	//01110000 00011100

//Function prototypes
void
led_switch_handler(g_setting *p_global_setting);

void 
initialize_ioexpander(void);

void
initialize_leds(void);

void
set_lfo_sync_led_state(unsigned char uc_new_state);

void
set_lfo_dest_led_state(unsigned char uc_new_state);

void
set_lfo_sel_led_state(unsigned char uc_new_state);

void
set_filter_type_led_state(unsigned char uc_new_state);



void
set_led_display(unsigned char uc_new_state);

void
handle_led_display(unsigned char uc_message);

unsigned char
get_current_patch_number(void);

void
set_lfo_sync_led(void);

void
set_lfo_sel_leds(void);

void
set_filter_type_leds(void);

void
set_lfo_dest_leds(void);

void
set_filter_type_leds(void);

void
set_drone_loop_led(unsigned char uc_led_state);

void
led_mux_handler(void);

unsigned char
to_digit(unsigned char uc_digit);


void
paraphonic_hold_mode_display(unsigned char mode_number, unsigned char hold_time);
#endif /*LED_SWITCH_HANDLER_H*/
