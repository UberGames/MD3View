/*
Copyright (C) 2010 Matthew Baranowski, Sander van Rossen & Raven software.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#ifndef __MATCOMP__
#define __MATCOMP__

#ifdef __cplusplus
extern "C"
{
#endif


#if (defined _MODVIEW || defined _CARCASS)
#include "special_defines.h"
#endif

#pragma optimize( "p", on )	// improve floating-point consistancy (makes release do bone-pooling as good as debug)


#define MC_BITS_X (16)
#define MC_BITS_Y (16)
#define MC_BITS_Z (16)
#define MC_BITS_VECT (16)

#define MC_SCALE_X (1.0f/64)
#define MC_SCALE_Y (1.0f/64)
#define MC_SCALE_Z (1.0f/64)


// pre-quat version
//#define MC_COMP_BYTES (((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*9)+7)/8)
//
// quat version, this is gay to do this but hard to change
#define MC_COMP_BYTES 14

void MC_Compress(const float mat[3][4],unsigned char * comp);
void MC_UnCompress(float mat[3][4],const unsigned char * comp);

#ifdef __cplusplus
}
#endif

#endif