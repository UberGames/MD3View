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

#ifdef MODVIEW
#include "stdafx.h"
#endif

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <memory.h>	// for memcpy
//
#include "MatComp.h"


#define MC_MASK_X ((1<<(MC_BITS_X))-1)
#define MC_MASK_Y ((1<<(MC_BITS_Y))-1)
#define MC_MASK_Z ((1<<(MC_BITS_Z))-1)
#define MC_MASK_VECT ((1<<(MC_BITS_VECT))-1)

#define MC_SCALE_VECT (1.0f/(float)((1<<(MC_BITS_VECT-1))-2))

#define MC_POS_X (0)
#define MC_SHIFT_X (0)

#define MC_POS_Y ((((MC_BITS_X))/8))
#define MC_SHIFT_Y ((((MC_BITS_X)%8)))

#define MC_POS_Z ((((MC_BITS_X+MC_BITS_Y))/8))
#define MC_SHIFT_Z ((((MC_BITS_X+MC_BITS_Y)%8)))

#define MC_POS_V11 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z))/8))
#define MC_SHIFT_V11 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z)%8)))

#define MC_POS_V12 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT))/8))
#define MC_SHIFT_V12 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT)%8)))

#define MC_POS_V13 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*2))/8))
#define MC_SHIFT_V13 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*2)%8)))

#define MC_POS_V21 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*3))/8))
#define MC_SHIFT_V21 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*3)%8)))

#define MC_POS_V22 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*4))/8))
#define MC_SHIFT_V22 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*4)%8)))

#define MC_POS_V23 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*5))/8))
#define MC_SHIFT_V23 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*5)%8)))

#define MC_POS_V31 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*6))/8))
#define MC_SHIFT_V31 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*6)%8)))

#define MC_POS_V32 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*7))/8))
#define MC_SHIFT_V32 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*7)%8)))

#define MC_POS_V33 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*8))/8))
#define MC_SHIFT_V33 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*8)%8)))

void MC_Compress(const float mat[3][4],unsigned char * _comp)
{
	char comp[MC_COMP_BYTES*2];

	int i,val;
	for (i=0;i<MC_COMP_BYTES;i++)
		comp[i]=0;

	val=(int)(mat[0][3]/MC_SCALE_X);
	val+=1<<(MC_BITS_X-1);
	if (val>=(1<<MC_BITS_X))
		val=(1<<MC_BITS_X)-1;
	if (val<0)
		val=0;
	*(unsigned int *)(comp+MC_POS_X)|=((unsigned int)(val))<<MC_SHIFT_X;

	val=(int)(mat[1][3]/MC_SCALE_Y);
	val+=1<<(MC_BITS_Y-1);
	if (val>=(1<<MC_BITS_Y))
		val=(1<<MC_BITS_Y)-1;
	if (val<0)
		val=0;
	*(unsigned int *)(comp+MC_POS_Y)|=((unsigned int)(val))<<MC_SHIFT_Y;

	val=(int)(mat[2][3]/MC_SCALE_Z);
	val+=1<<(MC_BITS_Z-1);
	if (val>=(1<<MC_BITS_Z))
		val=(1<<MC_BITS_Z)-1;
	if (val<0)
		val=0;
	*(unsigned int *)(comp+MC_POS_Z)|=((unsigned int)(val))<<MC_SHIFT_Z;


	val=(int)(mat[0][0]/MC_SCALE_VECT);
	val+=1<<(MC_BITS_VECT-1);
	if (val>=(1<<MC_BITS_VECT))
		val=(1<<MC_BITS_VECT)-1;
	if (val<0)
		val=0;
	*(unsigned int *)(comp+MC_POS_V11)|=((unsigned int)(val))<<MC_SHIFT_V11;

	val=(int)(mat[0][1]/MC_SCALE_VECT);
	val+=1<<(MC_BITS_VECT-1);
	if (val>=(1<<MC_BITS_VECT))
		val=(1<<MC_BITS_VECT)-1;
	if (val<0)
		val=0;
	*(unsigned int *)(comp+MC_POS_V12)|=((unsigned int)(val))<<MC_SHIFT_V12;

	val=(int)(mat[0][2]/MC_SCALE_VECT);
	val+=1<<(MC_BITS_VECT-1);
	if (val>=(1<<MC_BITS_VECT))
		val=(1<<MC_BITS_VECT)-1;
	if (val<0)
		val=0;
	*(unsigned int *)(comp+MC_POS_V13)|=((unsigned int)(val))<<MC_SHIFT_V13;


	val=(int)(mat[1][0]/MC_SCALE_VECT);
	val+=1<<(MC_BITS_VECT-1);
	if (val>=(1<<MC_BITS_VECT))
		val=(1<<MC_BITS_VECT)-1;
	if (val<0)
		val=0;
	*(unsigned int *)(comp+MC_POS_V21)|=((unsigned int)(val))<<MC_SHIFT_V21;

	val=(int)(mat[1][1]/MC_SCALE_VECT);
	val+=1<<(MC_BITS_VECT-1);
	if (val>=(1<<MC_BITS_VECT))
		val=(1<<MC_BITS_VECT)-1;
	if (val<0)
		val=0;
	*(unsigned int *)(comp+MC_POS_V22)|=((unsigned int)(val))<<MC_SHIFT_V22;

	val=(int)(mat[1][2]/MC_SCALE_VECT);
	val+=1<<(MC_BITS_VECT-1);
	if (val>=(1<<MC_BITS_VECT))
		val=(1<<MC_BITS_VECT)-1;
	if (val<0)
		val=0;
	*(unsigned int *)(comp+MC_POS_V23)|=((unsigned int)(val))<<MC_SHIFT_V23;

	val=(int)(mat[2][0]/MC_SCALE_VECT);
	val+=1<<(MC_BITS_VECT-1);
	if (val>=(1<<MC_BITS_VECT))
		val=(1<<MC_BITS_VECT)-1;
	if (val<0)
		val=0;
	*(unsigned int *)(comp+MC_POS_V31)|=((unsigned int)(val))<<MC_SHIFT_V31;

	val=(int)(mat[2][1]/MC_SCALE_VECT);
	val+=1<<(MC_BITS_VECT-1);
	if (val>=(1<<MC_BITS_VECT))
		val=(1<<MC_BITS_VECT)-1;
	if (val<0)
		val=0;
	*(unsigned int *)(comp+MC_POS_V32)|=((unsigned int)(val))<<MC_SHIFT_V32;

	val=(int)(mat[2][2]/MC_SCALE_VECT);
	val+=1<<(MC_BITS_VECT-1);
	if (val>=(1<<MC_BITS_VECT))
		val=(1<<MC_BITS_VECT)-1;
	if (val<0)
		val=0;
	*(unsigned int *)(comp+MC_POS_V33)|=((unsigned int)(val))<<MC_SHIFT_V33;

	// I added this because the line above actually ORs data into an int at the 22 byte (from 0), and therefore technically
	//	is writing beyond the 24th byte of the output array. This *should** be harmless if the OR'd-in value doesn't change 
	//	those bytes, but BoundsChecker says that it's accessing undefined memory (which it does, sometimes). This is probably
	//	bad, so... 
	memcpy(_comp,comp,MC_COMP_BYTES);
}


void MC_UnCompress(float mat[3][4],const unsigned char * comp)
{
	int val;

	val=(int)((unsigned short *)(comp))[0];
	val-=1<<(MC_BITS_X-1);
	mat[0][3]=((float)(val))*MC_SCALE_X;

	val=(int)((unsigned short *)(comp))[1];
	val-=1<<(MC_BITS_Y-1);
	mat[1][3]=((float)(val))*MC_SCALE_Y;

	val=(int)((unsigned short *)(comp))[2];
	val-=1<<(MC_BITS_Z-1);
	mat[2][3]=((float)(val))*MC_SCALE_Z;

	val=(int)((unsigned short *)(comp))[3];
	val-=1<<(MC_BITS_VECT-1);
	mat[0][0]=((float)(val))*MC_SCALE_VECT;

	val=(int)((unsigned short *)(comp))[4];
	val-=1<<(MC_BITS_VECT-1);
	mat[0][1]=((float)(val))*MC_SCALE_VECT;

	val=(int)((unsigned short *)(comp))[5];
	val-=1<<(MC_BITS_VECT-1);
	mat[0][2]=((float)(val))*MC_SCALE_VECT;


	val=(int)((unsigned short *)(comp))[6];
	val-=1<<(MC_BITS_VECT-1);
	mat[1][0]=((float)(val))*MC_SCALE_VECT;

	val=(int)((unsigned short *)(comp))[7];
	val-=1<<(MC_BITS_VECT-1);
	mat[1][1]=((float)(val))*MC_SCALE_VECT;

	val=(int)((unsigned short *)(comp))[8];
	val-=1<<(MC_BITS_VECT-1);
	mat[1][2]=((float)(val))*MC_SCALE_VECT;


	val=(int)((unsigned short *)(comp))[9];
	val-=1<<(MC_BITS_VECT-1);
	mat[2][0]=((float)(val))*MC_SCALE_VECT;

	val=(int)((unsigned short *)(comp))[10];
	val-=1<<(MC_BITS_VECT-1);
	mat[2][1]=((float)(val))*MC_SCALE_VECT;

	val=(int)((unsigned short *)(comp))[11];
	val-=1<<(MC_BITS_VECT-1);
	mat[2][2]=((float)(val))*MC_SCALE_VECT;
}

