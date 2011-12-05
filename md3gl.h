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

#ifndef _MD3_GL_H_
#define _MD3_GL_H_

#include "mdr.h"

typedef struct
{
	unsigned char          *Data;              // actual texture bitmap
	unsigned int            Width, Height;     // size of TextureData
	GLenum                  _Type;              // type of texture ie.		(not actually used)
	GLuint       		    Bind;			   // id the texture is bound to
	char                   *File;              // allocated string storing path name
	int                     numUsed;
} TextureGL;

typedef struct
{			
	GLuint         *textureBinds;              // array of texture binds, one for each mesh
	int            topTextureBind;    
	Vec3		   *iterVerts;                // arrays of vertices for interpolation, one for each mesh	
	NodeDictionary textureRes;                 // map of texture names to TextureGL data, stores texture resources
} MD3GL;

typedef struct
{
	Vec3		*pVerts;
	short		*pNormalIndexes;
} FMeshFrame;
/*
run time model data structure
*/

struct gl_mesh
{
	char          sName[64];
	unsigned int  iNumVertices;                 // number of vertices in model
	unsigned int  iNumTriangles;                // number of triangles in gl_model
	unsigned int  iNumMeshFrames;               // same as global frame but convenient
	
	TriVec	     *triangles;                    // array of triangle vertex indices of size iNumTriangles
	TexVec       *textureCoord;                 // array of float s/t texture coordinates of size iNumVertices
	FMeshFrame   *meshFrames;                   // 2d array of size of [iNumFrames][vertexNum] stores mesh frame vertices
	Vec3		 *iterMesh;                     // buffer mesh frame used to store interpolated mesh of size vertexNum

	unsigned int  iNumSkins;                      // number of skins this model has
	GLuint       *bindings;                     // array of texture bindings of size skinNum;	

	char		sTextureName[MAX_QPATH];		// only bother storing first one
};

struct gl_model;
struct gl_model
{
	// common to any model format:
	//
	modelType_t		modelType;
	unsigned int   iNumFrames;                   // number of frames in gl_model
	unsigned int   iNumTags;	                 // number of tags in gl_model     	
	unsigned int   currentFrame;                 // current frame being displayed
	// tag info is duplicated here, even for MDR types, so rest of code works ok
	TagFrame      *tags;                         // 2d array of tags size [iNumFrames][iNumTags]
	gl_model     **linkedModels;                 // array of links of size iNumTags, each link corresponds to a tag
	gl_model      *parent;

	// some stuff for dealing with MD3 LODs (which are actually seperate models)...
	//
	gl_model	  *pModel_LOD0;	// for the LOD0 (default) model, this will be NULL, for other LODs, this will point at original LOD0 model)
	gl_model	  *pModel_LOD1;
	gl_model	  *pModel_LOD2;

	// other stuff...
	//
	unsigned int   iNumMeshes;                   // number of meshes in whole model
	NodePosition   modelListPosition;            // link to its mdview.modelList
	float          CTM_matrix[16];               // object transformation matrix for manipulating mesh
		
	gl_mesh       *pMeshes;                       // array of meshes of size meshNum
	md3BoundFrame_t *pMD3BoundFrames;			 // array of boundary frames size [iNumFrames]

	// this requires more work once the script parser is done
	bool          isBlended;                    
	GLenum        blendParam1, blendParam2;     	

	int			  iRenderedTris;
	int			  iRenderedVerts;

	char			sHeadSkinName[32];	// should be enough
	char			sMD3BaseName[1024];
	char			sMDXFullPathname[1024];
	model_t			Q3ModelStuff;
};


#define NEAR_GL_PLANE 0.1
#define FAR_GL_PLANE 512

void initialize_glstate();
bool loadmd3gl( MD3GL &data );
void rendermd3gl( MD3GL &mdl );
void freemd3gl( MD3GL &data );
void renderFrame();

bool bIsWireFrame(void);
bool bIsTextured(void);

void oglStateFlatTextured();
void oglStateShadedTexturedAndWireFrame();
void oglStateShadedTextured();
void oglStateShadedFlat();
void oglStateWireframe();


/*
damn smart and efficient texture loader, 
first searches for an existing teature name is parent directories
then checks if texture has already been loaded,
otherwise it loads it up, enters in textureRes manager and returns the new binding
*/
extern bool gbIgnoreTextureLoad;
GLuint loadTexture( LPCSTR texturepath );

/*
frees a texture resource, called whenever a mesh is being freed. a texture is freed only if nothing is using it anymore
*/
void freeTexture( GLuint bind );

/*
deletes the entire texture resource manager with all its textures, usually called at the end of the program
or when a whole new model is loaded
*/
void freeTextureRes();

/*
reloads all texture resources from disk
*/
void refreshTextureRes();

/*
chooses the specific filter to be applied based on current mode
*/
void setTextureToFilter();

/*
sets texture parameters to the current texMode
*/
void setTextureFilter();

/*
renders the current frame 
*/
void draw_view();
void draw_viewSkeleton();



typedef unsigned int glIndex_t;
typedef struct shaderCommands_s 
{
	glIndex_t	indexes[SHADER_MAX_INDEXES];
	vec4_t		xyz[SHADER_MAX_VERTEXES];
	vec4_t		normal[SHADER_MAX_VERTEXES];
	vec2_t		texCoords[SHADER_MAX_VERTEXES][2];
/*	color4ub_t	vertexColors[SHADER_MAX_VERTEXES];
	int			vertexDlightBits[SHADER_MAX_VERTEXES];

	stageVars_t	svars;

	color4ub_t	constantColor255[SHADER_MAX_VERTEXES];

	shader_t	*shader;
	int			fogNum;

	int			dlightBits;	// or together of all vertexDlightBits
*/
	int			numIndexes;

	int			numVertexes;
/*
	// info extracted from current shader
	int			numPasses;
	void		(*currentStageIteratorFunc)( void );
	shaderStage_t	**xstages;
*/
} shaderCommands_t;


LPCSTR SkinName_FromPathedModelName(LPCSTR psModelPathName);
bool Model_ApplySkin(gl_model* model, LPCSTR psSkinFilename);
bool ApplyNewSkin(LPCSTR psSkinFullPathName);
gl_model* R_FindModel( gl_model* model, LPCSTR psModelBaseName);
shaderCommands_t* Freeze_Surface( md4Surface_t *surface, int iFrame );

#endif
