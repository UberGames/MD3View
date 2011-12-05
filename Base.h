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


#ifndef _BASE_H_
#define _BASE_H_

#define INT32	int
#define INT16	short
#define INT8	char

#define UINT32	unsigned int
#define UINT16	unsigned short
#define UINT8	unsigned char

#define SINT32	signed int
#define SINT16	signed short
#define SINT8	signed char

typedef float	Vec3[3];
typedef float	Vec2[2];
typedef Vec3	Mat3x3[3];

typedef INT32	TriVec[3];	//vertex 1,2,3 of TriVec
typedef Vec2	TexVec;		//Texture U/V coordinates of vertex 

#endif