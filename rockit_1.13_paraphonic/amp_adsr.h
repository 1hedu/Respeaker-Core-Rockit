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


#ifndef AMP_ADSR_H
#define AMP_ADSR_H

/*Definitions*/
//ADSR
#define NUMBER_OF_ADSR_STEPS	255 //number of steps in the adsr, is affected by the length of the slow interrupt
								//lower numbers here can make some interesting effects
#define ADSR_DIVIDER			8	//log of NUMBEROFADSRSTEPS base 2, basically 2 to what power 
#define ADSR_MIN_VALUE			47	//the minimum value of the ADSR, limited by the analog VCA
#define SUSTAIN_MIN_VALUE		47

//ADSR States
enum
{
	ATTACK = 0,
	DECAY,
	SUSTAIN,
	RELEASE
};


/*Function Prototypes*/
void
set_amplitude(g_setting *p_global_setting);

void 
adsr(g_setting *p_global_setting);

unsigned char
get_adsr_state(void);

void
set_adsr_state(unsigned char uc_adsr_state);

#endif /* AMP_ADSR_H */
