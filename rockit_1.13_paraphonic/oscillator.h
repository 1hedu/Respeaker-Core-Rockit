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

#ifndef OSCILLATOR_H
#define OSCILLATOR_H


#define MORPH_1_TIME_PERIOD 15
#define MORPH_2_TIME_PERIOD	10
#define PHASE_SHIFT_TIMER_2	50



//function prototype
unsigned char 
oscillator(unsigned char uc_waveshape, unsigned int un_sample_reference, unsigned char uc_frequency);

unsigned char 
linear_interpolate(unsigned char uc_reference, unsigned char uc_sample_1, unsigned char uc_sample_2);

unsigned char 
blend_wavetable(const unsigned char *p_uc_byte_address, unsigned char uc_note_number_modulus);


#endif /*OSCILLATOR_H*/
