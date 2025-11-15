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

#ifndef READ_AD_H
#define READ_AD_H
		
//function prototype
void 
read_ad(volatile g_setting *p_global_setting);

void 
set_pot_mux_sel(unsigned char uc_index);

void
display_knob_position(unsigned char uc_value);

void
initialize_pots(g_setting *p_global_setting); 

#endif /*READ_AD_H*/
