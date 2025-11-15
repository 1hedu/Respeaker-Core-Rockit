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

#ifndef WAVETABLES_H
#define WAVETABLES_H


extern const unsigned char G_AUC_SIN_LUT[256];
// Alias to match engine's expected symbol
#define G_AUC_SINE_SIMPLE_WAVETABLE_LUT G_AUC_SIN_LUT 
extern const unsigned char G_AUC_TRIANGLE_SIMPLE_WAVETABLE_LUT [256];
extern const unsigned char G_AUC_TRIANGLE_WAVETABLE_LUT [32] [256];
extern const unsigned char G_AUC_RAMP_SIMPLE_WAVETABLE_LUT [256];
extern const unsigned char G_AUC_RAMP_WAVETABLE_LUT [32] [256];
extern const unsigned char G_AUC_SQUARE_SIMPLE_WAVETABLE_LUT [256];
extern const unsigned char G_AUC_SQUARE_WAVETABLE_LUT [32] [256];
extern const unsigned char G_AUC_PARABOLIC_SIMPLE_WAVETABLE_LUT [256];
extern const unsigned char G_AUC_PARABOLIC_WAVETABLE_LUT [32] [256];
extern const unsigned char G_AUC_HARDSYNC_1_SIMPLE_WAVETABLE_LUT [128];
extern const unsigned char G_AUC_HARDSYNC_1_WAVETABLE_LUT [16] [128];
extern const unsigned char G_AUC_HARDSYNC_2_SIMPLE_WAVETABLE_LUT [128];
extern const unsigned char G_AUC_HARDSYNC_2_WAVETABLE_LUT [16] [128];
extern const unsigned char G_AUC_MULT1_WAVETABLE_LUT [16] [128];
extern const unsigned char G_AUC_MULT2_WAVETABLE_LUT [16] [128];


#endif /*WAVETABLES_H*/
