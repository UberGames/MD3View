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
#ifndef MDR_H
#define MDR_H

// a whole bunch of crap to make a big cut/paste from Trek work...


typedef int qboolean;
#define qfalse 0
#define qtrue 1
#define MAX_QPATH 64

// surface geometry should not exceed these limits
#define	SHADER_MAX_VERTEXES	1000
#define	SHADER_MAX_INDEXES	(6*SHADER_MAX_VERTEXES)


#define MD4_IDENT			(('5'<<24)+('M'<<16)+('D'<<8)+'R')
#define MD4_VERSION			2
#define	MD4_MAX_BONES		128

typedef float vec_t;
typedef vec_t vec2_t[2];
typedef vec_t vec3_t[3];
typedef vec_t vec4_t[4];
typedef vec_t vec5_t[5];

#define VectorClear(a)			((a)[0]=(a)[1]=(a)[2]=0)
#define VectorCopy(a,b)			((b)[0]=(a)[0],(b)[1]=(a)[1],(b)[2]=(a)[2])
#define DotProduct(x,y)			((x)[0]*(y)[0]+(x)[1]*(y)[1]+(x)[2]*(y)[2])
#define VectorSubtract(a,b,c)	((c)[0]=(a)[0]-(b)[0],(c)[1]=(a)[1]-(b)[1],(c)[2]=(a)[2]-(b)[2])
#define VectorAdd(a,b,c)		((c)[0]=(a)[0]+(b)[0],(c)[1]=(a)[1]+(b)[1],(c)[2]=(a)[2]+(b)[2])
#define	VectorScale(v, s, o)	((o)[0]=(v)[0]*((vec_t)s),(o)[1]=(v)[1]*((vec_t)s),(o)[2]=(v)[2]*((vec_t)s))
#define	VectorMA(v, s, b, o)	((o)[0]=(v)[0]+(b)[0]*(s),(o)[1]=(v)[1]+(b)[1]*(s),(o)[2]=(v)[2]+(b)[2]*(s))



typedef enum {
	MODTYPE_BAD,
	MODTYPE_MD3,
	MODTYPE_MDR,
} modelType_t;


typedef enum {
	SF_BAD, 
	SF_SKIP,				// ignore
	SF_FACE,
	SF_GRID,
	SF_TRIANGLES,
	SF_POLY,
	SF_MD3,
	SF_MD4,
	SF_FLARE,
	SF_ENTITY,				// beams, rails, lightning, etc that can be determined by entity
	SF_DISPLAY_LIST,

	SF_NUM_SURFACE_TYPES,
	SF_MAX = 0xffffffff			// ensures that sizeof( surfaceType_t ) == sizeof( int )
} surfaceType_t;

typedef struct {
	int			boneIndex;		// these are indexes into the boneReferences,
	float		   boneWeight;		// not the global per-frame bone list
	vec3_t		offset;
} md4Weight_t;

typedef struct {
	vec3_t		normal;
	vec2_t		texCoords;
	int			numWeights;
	md4Weight_t	weights[1];		// variable sized
} md4Vertex_t;

typedef struct {
	int			indexes[3];
} md4Triangle_t;

typedef struct {
	int			ident;

	char		name[MAX_QPATH];	// polyset name
	char		shader[MAX_QPATH];
	int			shaderIndex;		// for in-game use

	int			ofsHeader;			// this will be a negative number

	int			numVerts;
	int			ofsVerts;

	int			numTriangles;
	int			ofsTriangles;

	// Bone references are a set of ints representing all the bones
	// present in any vertex weights for this surface.  This is
	// needed because a model may have surfaces that need to be
	// drawn at different sort times, and we don't want to have
	// to re-interpolate all the bones for each surface.
	int			numBoneReferences;
	int			ofsBoneReferences;

	int			ofsEnd;				// next surface follows
} md4Surface_t;

typedef struct {
	float		matrix[3][4];
} md4Bone_t;

typedef struct {
	vec3_t		bounds[2];			// bounds of all surfaces of all LOD's for this frame
	vec3_t		localOrigin;		// midpoint of bounds, used for sphere cull
	float		radius;				// dist from localOrigin to corner
	char		name[16];
	md4Bone_t	bones[1];			// [numBones]
} md4Frame_t;

typedef struct {
	unsigned char Comp[24]; // MC_COMP_BYTES is in MatComp.h, but don't want to couple
} md4CompBone_t;

typedef struct {
	vec3_t		bounds[2];			// bounds of all surfaces of all LOD's for this frame
	vec3_t		localOrigin;		// midpoint of bounds, used for sphere cull
	float		radius;				// dist from localOrigin to corner
	md4CompBone_t	bones[1];			// [numBones]
} md4CompFrame_t;

typedef struct {
	int			numSurfaces;
	int			ofsSurfaces;		// first surface, others follow
	int			ofsEnd;				// next lod follows
} md4LOD_t;

typedef struct {
	int			boneIndex;
	char		name[32];
} md4Tag_t;

typedef struct {
	int			ident;
	int			version;

	char		name[MAX_QPATH];	// model name

	// frames and bones are shared by all levels of detail
	int			numFrames;
	int			numBones;
	int			ofsFrames;			// md4Frame_t[numFrames]		// this will be -ve for compressed models

	// each level of detail has completely separate sets of surfaces
	int			numLODs;
	int			ofsLODs;

	int			numTags;
	int			ofsTags;

	int			ofsEnd;				// end of file
} md4Header_t;

typedef struct model_s {
	char		name[MAX_QPATH];
	modelType_t	modelType;
	int			index;				// model = tr.models[model->index]
	
	int			dataSize;			// just for listing purposes
//	bmodel_t	*bmodel;			// only if type == MOD_BRUSH
//	md3Header_t	*md3[MD3_MAX_LODS];	// only if type == MOD_MESH
	md4Header_t	*md4;				// only if type == MOD_MD4

	int			 numLods;
} model_t;

#define LittleFloat(l) l
#define LittleLong(l) l
#define LL(x) x=LittleLong(x)


#endif	// #ifndef MDR_H

