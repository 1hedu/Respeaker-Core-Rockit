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

#ifndef CALCULATE_PITCH_H
#define CALCULATE_PITCH_H


#define NUM_PITCH_SHIFT_INCREMENTS		128
#define LOG_NUM_PITCH_SHIFT_INCREMENTS	7

#define ZERO_PITCH_BEND 64

//Function prototypes
void
calculate_pitch(g_setting *p_global_setting);


#endif //CALCULATE_PITCH_H
