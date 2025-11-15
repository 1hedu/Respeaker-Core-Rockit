/* 
@file led_switch_handler.c

@brief

This file contains functions related to the led and switch interactions.
Pressing switches updates LEDs and general synthesizer settings.
This file contains the functions needed to tell the i/o expander which LED
to light and what setting to change based on a switch press.

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
#include "io.h"
#include "spi.h"
#include "save_recall.h"
#include "lfo.h"
#include "drone_loop.h"
#include "midi.h"
#include "calculate_pitch.h"


//inline function prototypes
void 
inline tact_cs_enable(void);
void 
inline tact_cs_disable(void);
void 
inline mux_1_on(void);
void 
inline mux_2_on(void);
void 
inline mux_3_on(void);
void 
inline mux_1_off(void);
void 
inline mux_2_off(void);
void 
inline mux_3_off(void);

//Local Variable Definitions

static unsigned char uc_state = READ;
static unsigned char uc_pressed_btn_index;//This holds the number of the button that has been pressed.
static unsigned char uc_led_lfo_sync_state;//This holds the state of the led sync led.
static unsigned char uc_led_lfo_dest_state;//This stores the state of the lfo destination led.
static unsigned char uc_led_filt_type_state;//This stores the state of the filter type led.
static unsigned char uc_led_lfo_sel_state;//This stores the state of the lfo select led.
static unsigned char uc_drone_loop_state;
static unsigned char uc_patch_number = 0;
static unsigned char uc_ioexp_address;//The address of the io expander.
static unsigned char uc_ioexp_reg_address;//The register address inside the io expander.
static unsigned char uc_led_buffer_1 = 0;//These buffers store the values for each MUX
static unsigned char uc_led_buffer_2 = 0;//These buffers store the values for each MUX
static unsigned char uc_led_buffer_3 = 0;//These buffers store the values for each MUX
static unsigned char uc_paraphonic_mode_display_timer = 0; // PARAPHONIC: mode display hold timer

// PARAPHONIC: Helper to set display timer from outside this file
void paraphonic_set_display_timer(unsigned char value) {
    uc_paraphonic_mode_display_timer = value;
}
static unsigned char uc_button_hold_down_counter;//Count how long a button is held down
static unsigned char uc_new_midi_channel_number;
static unsigned char uc_blink_flag;


const unsigned char AUC_DIGIT_LUT[16] = {DIGIT0, DIGIT1, DIGIT2, DIGIT3, DIGIT4, DIGIT5, DIGIT6, DIGIT7,
										DIGIT8, DIGIT9, DIGITA, DIGITB, DIGITC, DIGITD, DIGITE, DIGITF};

/*
@brief This function handles the i/o expanders as in updating LEDs and handling button presses.
When a button is pressed, we have to determine which one and take the appropriate action.

@param This function takes no parameters and returns no vals.
*/
void
led_switch_handler(g_setting *p_global_setting)
{
	//If the interrupt occurred and the switch has been debounced, we can load up the address
	//to perform a read on the i/o expander that caused the interrupt.
	//First, we perform the read.  After the read, we determine which i/o expander has been triggered.
	//Sometimes, we have to write to more than one i/o expander.  If so, we have a third state of WRITE_2.
    switch (uc_state)
    {
	    case READ:
		
		    g_uc_spi_enable_port = SWITCH_CS_PORT;
			g_uc_spi_enable_pin = SWITCH_CS_ENABLE;
		 
			
			uc_ioexp_address = IOEXPANDER_U15_READ;
			
			
			uc_ioexp_reg_address = IOEXPANDER_REG_INTCAP;

	        //We send the last byte as all zeroes as a dummy write.  While the zeroes are being
			//transmitted, the i/o expander will be transmitting the contents of the interrupt
			//capture register.	
			
			//Enable the tact switch i/o expander.
			tact_cs_enable();
			
			send_spi_three_bytes(uc_ioexp_address, 
			                     uc_ioexp_reg_address, 
								 0x00);

			if(!g_uc_midi_channel_change_flag)
			{
				uc_state = SET_LEDS;
			}
			else
			{
				uc_state = CHANGE_MIDI_CHANNEL;
			}				

        break;
		
		case SET_LEDS:
		
			 //Figure out which button was pressed by
			 //reading the SPI register
			 uc_pressed_btn_index = SPDR;
		
		
				/*Now that we know which button was pressed, 
				we can turn on the appropriate LED.
				There is a case statement for each tact switch.
				We also update the appropriate setting.*/
				switch(uc_pressed_btn_index)
				{

				    case TACT_LFO_SYNC:
       					
						uc_led_lfo_sync_state ^= 0x01;

						p_global_setting->auc_synth_params[LFO_SYNC] ^= 0x01;

						set_lfo_sync_led();

						uc_state = READ;
						clear_button_press_flag();

					break;

					case TACT_LFO_DEST:

						/*If the LFO was modifying the amplitude, then we need to set the 
						synth_params value to it's maximum so that it doesn't get stuck at a low value*/
						if(AMPLITUDE == auc_lfo_dest_decode[LFO_1][p_global_setting->auc_synth_params[LFO_1_DEST]]
						|| AMPLITUDE == auc_lfo_dest_decode[LFO_2][p_global_setting->auc_synth_params[LFO_2_DEST]])
						{
							p_global_setting->auc_synth_params[AMPLITUDE] = 255;
						}
						else if(PITCH_SHIFT == auc_lfo_dest_decode[LFO_1][p_global_setting->auc_synth_params[LFO_1_DEST]])
						{
							p_global_setting->auc_synth_params[PITCH_SHIFT] = ZERO_PITCH_BEND;
						}

					  	/*Increment the state of the led and loop back around if necessary*/
					  	if(++uc_led_lfo_dest_state > LFO_DEST_6)
					 	{				
							  uc_led_lfo_dest_state = LFO_DEST_1;
			          	}

						if(1 == uc_led_lfo_sel_state)
						{
							p_global_setting->auc_synth_params[LFO_2_DEST] = uc_led_lfo_dest_state;
							
							// If LFO amount is 0, set the new destination to its AD knob value
							// This prevents parameters getting stuck
							if(p_global_setting->auc_synth_params[LFO_2_AMOUNT] == 0)
							{
								unsigned char new_dest = auc_lfo_dest_decode[LFO_2][uc_led_lfo_dest_state];
								// Set parameter source to AD so knob takes control
								p_global_setting->auc_parameter_source[new_dest] = SOURCE_AD;
							}
						}
						else
						{
							p_global_setting->auc_synth_params[LFO_1_DEST] = uc_led_lfo_dest_state;
							
							// If LFO amount is 0, set the new destination to its AD knob value  
							// This prevents parameters getting stuck
							if(p_global_setting->auc_synth_params[LFO_1_AMOUNT] == 0)
							{
								unsigned char new_dest = auc_lfo_dest_decode[LFO_1][uc_led_lfo_dest_state];
								// Set parameter source to AD so knob takes control
								p_global_setting->auc_parameter_source[new_dest] = SOURCE_AD;
							}
						}

						set_lfo_dest_leds();
						uc_state = READ;
						clear_button_press_flag();

					break;

					case TACT_FILTER_TYPE:

						if(++uc_led_filt_type_state > LED_HIGH_PASS)
						{
						 	uc_led_filt_type_state = LED_LOW_PASS;
						}

						p_global_setting->auc_synth_params[FILTER_TYPE] = uc_led_filt_type_state;
          				
						set_filter_type_leds();
						uc_state = READ;
						clear_button_press_flag();

					break;

					case TACT_LFO_SEL:

					    uc_led_lfo_sel_state ^= 0x01;
						
						p_global_setting->auc_synth_params[LFO_SEL] = uc_led_lfo_sel_state;

						set_lfo_sel_leds();

						//Since we are changing LFOs, we need to update all associated LEDs for the
						//LFO Destination and for the LFO Waveshape
						uc_state = LFO_SELECT_CHANGE;
						
					break;

					case TACT_SELECT:

						if(PIND & BUTTON_PRESS_MASK)/*Check to see if the button is not being held down*/
						{
							/*If it's not being held down, then we increment the patch number*/

							if(++uc_patch_number > MAX_PATCH_NUMBER)
							{
								uc_patch_number = 0;
							}

							uc_button_hold_down_counter = 0;
							uc_led_buffer_1 = to_digit(uc_patch_number);
							uc_state = READ;
							clear_button_press_flag();
						}
						else if(uc_button_hold_down_counter == 10)
						{
							g_uc_midi_channel_change_flag = TRUE;
							uc_blink_flag = TRUE;
							uc_button_hold_down_counter = 0;
							uc_state = READ;
						}
						else
						{
							uc_button_hold_down_counter++;
							uc_state = READ;
							clear_button_press_flag();
						}						

					break;

					case TACT_SAVE:
	
						save_patch(uc_patch_number, p_global_setting);

						uc_state = READ;
						clear_button_press_flag();

					break;

					case TACT_RECALL:
				
						recall_patch(uc_patch_number, p_global_setting);

						uc_state = READ;
						clear_button_press_flag();
						
					break;

					case TACT_DRONELOOP:

						switch (uc_drone_loop_state)
						{
						
							case OFF:

								g_uc_drone_flag = TRUE;
								uc_drone_loop_state = DRONE;
								set_drone_loop_led(1);

							break;	
							
							case DRONE:

								g_uc_drone_flag = FALSE;
								g_uc_loop_flag = TRUE;
								uc_drone_loop_state = LOOP;
								initialize_loop(p_global_setting);
								
							break;
							
							case LOOP:

								g_uc_loop_flag = FALSE;
								uc_drone_loop_state = OFF;
								g_uc_key_press_flag = 0;
								set_drone_loop_led(0);
							
							break;
							
						}				

						uc_state = READ;
						clear_button_press_flag();
						
					break;

					default:
						
						//If something goofy happened, we need to make sure, we have a way out.
						//Clear the external interrupt flag and reinitialize state machine.
						uc_state = READ;
						clear_button_press_flag();

					break;

					}
				
			break;

			//When we change LFOs, we have to update the LEDs for the
			//lfo destination.  This basically duplicates what is above
			//except that nothing is incremented.

			case LFO_SELECT_CHANGE:

				if(LFO_2 == uc_led_lfo_sel_state)
				{
					uc_led_lfo_dest_state = p_global_setting->auc_synth_params[LFO_2_DEST];	
				}
				else
				{
					uc_led_lfo_dest_state = p_global_setting->auc_synth_params[LFO_1_DEST];
				}

			
			  	set_lfo_dest_leds();
				uc_state = READ;
				clear_button_press_flag();

			break;
			
			case CHANGE_MIDI_CHANNEL:
			
				//Figure out which button was pressed by
				//reading the SPI register
				uc_pressed_btn_index = SPDR;
				uc_new_midi_channel_number = get_midi_channel_number();
				
				switch(uc_pressed_btn_index)
				{
					case TACT_SAVE:
					
						
						if(uc_new_midi_channel_number-- == 0)
						{
							uc_new_midi_channel_number = 14;
						}
						set_midi_channel_number(uc_new_midi_channel_number);
						
					
					break;
				
					case TACT_RECALL:
				
						if(uc_new_midi_channel_number++ >= 14)
						{
							uc_new_midi_channel_number = 0;
						}
						
						set_midi_channel_number(uc_new_midi_channel_number);
				
					break;
				
					case TACT_SELECT:
				
						if(PIND & BUTTON_PRESS_MASK)/*Check to see if the button is not being held down*/
						{
							uc_button_hold_down_counter = 0;
						}
						else if(uc_button_hold_down_counter == 10)
						{
							uc_button_hold_down_counter = 0;
							g_uc_midi_channel_change_flag = FALSE;
							uc_blink_flag = FALSE;
						}
						else
						{
							uc_button_hold_down_counter++;
						}
				
					break;
					
					default:
					
						uc_state = READ;
						clear_button_press_flag();
						
					break;
				}					
				
				
				uc_state = READ;
				clear_button_press_flag();
				handle_led_display(uc_new_midi_channel_number);
			
			break;

			default:

			break;
        }
	
 

}
/*This function handles what's shown on the led display. If it receives a 16 or higher, it shows the 
current patch number. Otherwise, it shows the number it's sent for 5 seconds. If the MIDI channel select
is active, the display blinks.*/
void
handle_led_display(unsigned char uc_message)
{
	static unsigned int un_timer;
	static unsigned char uc_blink_state;
	unsigned char uc_temp;
	
	// PARAPHONIC: Countdown mode display timer
	if(uc_paraphonic_mode_display_timer > 0)
	{
		uc_paraphonic_mode_display_timer--;
		return; // Keep current display (mode number)
	}
	
	if(uc_blink_flag == TRUE && uc_blink_state == TRUE)
	{
		mux_1_off();
	}
	
	if(un_timer > 0)
	{
		un_timer--;
	}
	else
	{
		if(g_uc_midi_channel_change_flag == FALSE)
		{
			set_led_display(get_current_patch_number());
			uc_blink_flag = FALSE;
		}
		else
		{
			uc_temp = get_midi_channel_number() + 1;
			set_led_display(uc_temp);
			
			if(uc_blink_state == FALSE)
			{
				uc_blink_state = TRUE;
				mux_1_off();
				un_timer = 200;
			}
			else
			{
				uc_blink_state = FALSE;
				un_timer = 200;
			}
		}			
	}
	
	if(uc_message <= 15 && g_uc_midi_channel_change_flag == FALSE)
	{
		set_led_display(uc_message);
		un_timer = 1000;
	}
}

void
set_lfo_sel_led_state(unsigned char uc_new_state)
{
	uc_led_lfo_sel_state = uc_new_state;

}

void
set_lfo_dest_led_state(unsigned char uc_new_state)
{
	uc_led_lfo_dest_state = uc_new_state;
}

void
set_lfo_sync_led_state(unsigned char uc_new_state)
{
	uc_led_lfo_sync_state = uc_new_state;
}

void
set_filter_type_led_state(unsigned char uc_new_state)
{
	uc_led_filt_type_state = uc_new_state;
}

void
set_led_display(unsigned char uc_new_state)
{
	uc_led_buffer_1 = to_digit(uc_new_state);
}

unsigned char
get_current_patch_number(void)
{
	return uc_patch_number;
}

void
set_lfo_sel_leds(void)
{
	uc_led_buffer_3 |= LFO_1_SEL_MASK | LFO_2_SEL_MASK;

	if(LFO_2 == uc_led_lfo_sel_state)
	{
		uc_led_buffer_3 ^= LFO_2_SEL_MASK;
	}
	else
	{
		uc_led_buffer_3 ^= LFO_1_SEL_MASK;
	}
}

void
set_lfo_sync_led(void)
{
	if(LFO_SYNC_ON == uc_led_lfo_sync_state)
	{
		uc_led_buffer_3 &= ~LFO_SYNC_MASK;
	}
	else
	{
		uc_led_buffer_3 |= LFO_SYNC_MASK;
	}
}

void
set_lfo_dest_leds(void)
{
	/*Clear all the LEDS*/
	
	uc_led_buffer_2 |= LFO_DEST_1_MASK | LFO_DEST_3_MASK | LFO_DEST_5_MASK |
						LFO_DEST_2_MASK | LFO_DEST_6_MASK | LFO_DEST_4_MASK;	


  	switch(uc_led_lfo_dest_state)
  	{		
	    case LFO_DEST_1:
		
			uc_led_buffer_2 ^= LFO_DEST_1_MASK;

		break;

		case LFO_DEST_2:

			uc_led_buffer_2 ^= LFO_DEST_2_MASK;

		break;

		case LFO_DEST_3:

			uc_led_buffer_2 ^= LFO_DEST_3_MASK;

		break;

		case LFO_DEST_4:
		
			uc_led_buffer_2 ^= LFO_DEST_4_MASK;
		
		break;

		case LFO_DEST_5:

			uc_led_buffer_2 ^= LFO_DEST_5_MASK;

		break;

		case LFO_DEST_6:

			uc_led_buffer_2 ^= LFO_DEST_6_MASK;

		break;

		default:

		break;
 	}

}


void
set_filter_type_leds(void)
{
	/*Clear all the LEDS*/
	
	uc_led_buffer_3 |= LOW_PASS_MASK | BAND_PASS_MASK | HIGH_PASS_MASK;	


  	switch(uc_led_filt_type_state)
  	{	
		case LOW_PASS:
		
			uc_led_buffer_3 ^= LOW_PASS_MASK;			

		break;

		case BAND_PASS:

			uc_led_buffer_3 ^= BAND_PASS_MASK;		

		break;

		case HIGH_PASS:

			uc_led_buffer_3 ^= HIGH_PASS_MASK;

		break;

		default:
		break;

	}

}

void
set_drone_loop_led(unsigned char uc_led_state)
{
	if(LFO_SYNC_ON == uc_led_state)
	{
		uc_led_buffer_3 &= ~DRONE_LOOP_MASK;
	}
	else
	{
		uc_led_buffer_3 |= DRONE_LOOP_MASK;
	}
}



/*
@brief This function handles the LED multiplexing. There are three sets of LEDs including the LED display.
Timing for switching between led sets is handled in the main routine.

@param This function takes no parameters and returns no values.
*/
void
led_mux_handler(void)
{
	static unsigned char 	uc_led_mux_state;
	static unsigned char	uc_led_mux_timer;

	uc_led_mux_timer--;

	if(0 == uc_led_mux_timer)
	{
		uc_led_mux_timer = LED_MUX_TIMER_RELOAD;

		uc_led_mux_state++;

		if(uc_led_mux_state > LED_MUX_3)
		{
			uc_led_mux_state = LED_MUX_1;
		}

		switch(uc_led_mux_state)
		{
			case LED_MUX_1:
			
				mux_3_off();				

				//Clear PORTC except for PC0 which is unrelated to the LEDs
				PORTC &= 0x01;

				PORTC |= uc_led_buffer_1; 

				mux_1_on();

			break;

			case LED_MUX_2:

				mux_1_off();

				//Clear PORTC except for PC0 which is unrelated to the LEDs
				PORTC &= 0x01;

				PORTC |= uc_led_buffer_2;
				
				mux_2_on();

			break;

			case LED_MUX_3:

				mux_2_off();

				//Clear PORTC except for PC0 which is unrelated to the LEDs
				PORTC &= 0x01;

				PORTC |= uc_led_buffer_3; 

				mux_3_on();

			break;

			default:

			break;

		}
	}
}


/*
@brief This function configures the i/o expander.
The master interrupt enable must be set for this routine to work.

@param This function takes no parameters and returns no vals.
*/
void
initialize_ioexpander()
{
    //Configure the SPI for transmission.
	//The interrupt enable is not set for this configuration step. We just wait
	//for each transmission. We still know that it's done by the interrupt flag being set.
	//The flag just doesn't trigger an interrupt service routine.
    SPCR = (1<<SPE)|(1<<MSTR);

	/*Now we configure the i/o expander*/
    

	tact_cs_enable();
	

	spi_simple_transmit(IOEXPANDER_U15_WRITE);//transmit the address for U15
	spi_simple_transmit(0x03);//transmit the register address for interrupt compare
	spi_simple_transmit(0xFF);//transmit val for interrupt compare val, anything other than this will cause an interrupt
	spi_simple_transmit(0xFF);//transmit val for interrupt control register, compare present val to val in interrupt compare register
	spi_simple_transmit(0x08);//transmit val for i/o control register - sequential operation enabled, hardware address enable, interrupt active low
	spi_simple_transmit(0xFF);//transmit val for pull-up resistor register - all pull-up registers
	spi_simple_transmit(0x00);//transmit val for interrupt flag register - this write is ignored
	spi_simple_transmit(0x00);//transmit val for interrupt capure register - this write is ignored also
	spi_simple_transmit(0xFF);//transmit val for gpio register - val of i/o - won't matter because all are inputs
	spi_simple_transmit(0xFF);//transmit val for output latch - ignored because all inputs
	spi_simple_transmit(0xFF);//transmit val for i/o dir = all inputs
	spi_simple_transmit(0x00);//transmit val for input polarity
	
	tact_cs_disable();

    //Enable U15 interrupt now that we've gotten everything else configured
	tact_cs_enable();

    spi_simple_transmit(IOEXPANDER_U15_WRITE);//transmit the address for U15
	spi_simple_transmit(0x02);//transmit the register address for interrupt compare
	spi_simple_transmit(0xFF);//enable the interrupts
	
	tact_cs_disable();	
  
}

void
initialize_leds(void)
{
	mux_1_off();
	mux_2_off();
	mux_3_off();	

	set_lfo_sync_led_state(LFO_SYNC_OFF);
	set_lfo_dest_led_state(LFO_DEST_1);
	set_lfo_sel_led_state(LFO_SEL_1);
	set_filter_type_led_state(LED_LOW_PASS);
	set_lfo_sync_led();
	set_lfo_sel_leds();
	set_filter_type_leds();
	set_lfo_dest_leds();
	set_filter_type_leds();
	set_drone_loop_led(OFF);
	uc_led_buffer_1 = to_digit(0);
}


unsigned char
to_digit(unsigned char uc_digit)
{

	return AUC_DIGIT_LUT[uc_digit];


}

//inline functions
void inline tact_cs_enable(void)
{
	PORTB &= ~0x10;
}

void inline tact_cs_disable(void)
{
	PORTB |= 0x10;
}

void inline mux_1_on(void)
{
	PORTC &= ~0x01;
}

void inline mux_2_on(void)
{
	PORTA &= ~0x40;
}

void inline mux_3_on(void)
{
	PORTA &= ~0x80;
}

void inline mux_1_off(void)
{
	PORTC |= 0x01;
}

void inline mux_2_off(void)
{
	PORTA |= 0x40;
}

void inline mux_3_off(void)
{
	PORTA |= 0x80;
}

/*
 * PARAPHONIC: Hold the display for mode feedback
 * Call this when mode changes to show mode number briefly
 */
void paraphonic_hold_mode_display(unsigned char mode_number, unsigned char hold_time)
{
	// Set display to show mode number (0, 1, 2, 3, or 4)
	set_led_display(DIGIT0 + mode_number);
	
	// Set timer to hold this display
	// handle_led_display() will count this down
	paraphonic_set_display_timer(hold_time);
}

