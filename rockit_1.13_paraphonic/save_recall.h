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

#ifndef SAVE_RECALL_H
#define SAVE_RECALL_H


//function prototypes
void
save_patch(unsigned char uc_patch_number, g_setting *p_global_setting);

void
recall_patch(unsigned char uc_patch_number, g_setting *p_global_setting);

#endif /*SAVE_RECALL_H*/
