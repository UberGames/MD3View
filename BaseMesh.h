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


#ifndef _BASEMESH_H_
#define _BASEMESH_H_

#include "mdr.h"	// for MAX_QPATH

/**START OLD STRUCTURES**/

//These structures are exactly the same in the md3 file structure.
//It is very likely that these structures will be replaced with a
//very different internal structure.
/*
typedef struct 
{
	//start guess
	float				Mins[3];
	float				Maxs[3];
	float				Position[3];
	float				Scale;
	//end guess
	char				Creator[16];		//i think this is the "creator" name..
											//but i'm only guessing.
} BoneFrame_t;
*/
typedef struct 
{
	vec3_t		bounds[2];	
	vec3_t		localOrigin;
	float		radius;
	char		name[16];	// creator
} md3BoundFrame_t;



// as with everything else in this source, this is a mess. I've replaced the original guesswork name/unknown stuff,
//	but couldn't be arsed converting either the original MD3 code, or the new MDR code to use the other's labels, so...
//
typedef struct 
{
/*	char				Name[12];			//name of 'tag' as it's usually called in the md3 files
											//try to see it as a sub-mesh/seperate mesh-part
	char				unknown[52];		//normally filled with zeros, but there is an exception 
											//where it's filled with other numbers...
											//it would be logical if it was part of name, because 
											//then name would have 64 chars.
*/
	union
	{
		char				Name[MAX_QPATH];
		char				name[MAX_QPATH];	// tag name
	};

	union
	{
	//unverified:
	Vec3				Position;			//relative position of tag
	vec3_t		origin;
	};

	union
	{	
	Mat3x3				Matrix;				//3x3 rotation matrix
	vec3_t		axis[3];
	};

} Tag;
typedef Tag*				TagFrame;
/**END OLD STRUCTURES**/

typedef Tag md3Tag_t;


#endif