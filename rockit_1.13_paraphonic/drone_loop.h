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

#ifndef DRONE_LOOP_H
#define DRONE_LOOP_H


void
drone_loop(g_setting *p_global_setting);

unsigned char
get_current_loop_step(void);

void
set_parameter_loop_end(unsigned char uc_knob_index);

void
initialize_loop(g_setting *p_global_setting);

unsigned char
get_loop_stored_parameter(unsigned char uc_lfo_index);


#endif /*DRONE_LOOP_H*/
