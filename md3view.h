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


/*! 
md3view.h - the header file holding the platform independent viewer interface.
the structure MD3ViewState holds all of the runtime global data needed by the viewer
holds most viewer interaction constants and mode enumerations. 
this should be the only file included by system dependent code, but that rule is flexible.
*/

#ifndef _MD3_VIEW_H_
#define _MD3_VIEW_H_

//this is the caption name & the name in the about box
#define FILENAME "MD3View v1.6"

/*! 
enumeration of various display modes that the ogl rendering engine supports. line refers
to wire frame, fill to flat shaded polygons, and textured is self explanatory. 
*/
enum DrawMode {
	DRAW_LINE,
	DRAW_FILL,
	DRAW_TEXTURE
};

/*! 
texture modes supported by the engine, translates loosely to TEX_FAST being GL_NEAREST texture
fileter with fastest perspective correction. used mostly but he software driver. unfiltered is GL_NEAREST
with full perspective correction and filtered uses bilinear filetering.
*/
enum TextureMode {
	TEX_FAST,
	TEX_UNFILTERED,
	TEX_FILTERED
};

/*!
MD3ViewState - main global structure that holds all the global variables need by the viewer at runtime.
is used for everythign and everwhere. 
*/

typedef struct
{	
	float			xPos, yPos;
	float			zPos;
	float			rotAngleX, rotAngleY;	
	float			rotAngleZ;
	double			animSpeed;
	double			timeStamp1;
	float           frameFrac;
	bool            hasAnimation;
	bool			interpolate;
	bool			done;
	bool            animate;
	bool			bUseAlpha;
	bool			bBBox;
	
/*	MD3GL		*	glmdl;
	MD3			*	model;
	unsigned int*	frames;
*/
	NodeDictionary  textureRes;                 // map of texture names to TextureGL data, stores texture resources
	unsigned int    topTextureBind;             // simple index to keep track of texture bindings

	NodeSequence    modelList;                   // list of gl_meshes loaded
	gl_model       *baseModel;                  // base model of a tag hierarchy

	DrawMode		drawMode;
	TextureMode     texMode;

	GLenum          faceSide;

	char			basepath[128];

#ifdef WIN32
	HWND			hwnd;
	HDC				hdc;
	HGLRC			glrc;
#endif

	int				iLODLevel;
	int				iSkinNumber;
	bool			bAxisView;

	byte			_R,_G,_B;	// clear colours

	// these 2 just for printing stats info...
	//
	bool			bAnimCFGLoaded;
	bool			bAnimIsMultiPlayerFormat;

	double			dFOV;

} MDViewState;

const double ANIM_SLOWER = 1.3;
const double ANIM_FASTER = 0.9;
const float MOUSE_ROT_SCALE  = 0.5f;
const float MOUSE_ZPOS_SCALE = 0.1f;
const float MOUSE_XPOS_SCALE = 0.1f;
const float MOUSE_YPOS_SCALE = 0.1f;


extern MDViewState mdview;


#ifdef WIN32
#include <windows.h>
/*!
platform independent key enumeration to drag functions
*/
enum mkey_enum {
	KEY_LBUTTON = MK_LBUTTON,
	KEY_RBUTTON = MK_RBUTTON,
	KEY_MBUTTON = MK_MBUTTON	
};
#endif

/*! initializes the mdview structures, should be called at startup before the gui is created */
void init_mdview(const char*);

/*! initializes the gl structures should be called immiedietly after opengl context has been created */
bool init_mdview_gl();

/*! called when window is resized to update gl matrices, actually defined in md3gl.cpp */
void set_windowSize( int x, int y );

/*! called to render the main viewer screen */
void render_mdview();

/*! commands to handle mouse dragging, uses key_flags defines above */
void start_drag( mkey_enum keyFlags, int x, int y );
bool drag(  mkey_enum keyFlags, int x, int y );
void end_drag(  mkey_enum keyFlags, int x, int y );

/*! 
loads a model into the file, called by a file open dialog box 
*/
bool loadmdl( char *filename );

/*
writes the current mesh frame to a raw file
*/
void write_baseModelToRaw( char *fname );

/*
frees data for shutdown, call this only as the last thing before exiting or bad thigns will happen
*/
void shutdown_mdviewdata();

/*
frees all mdviewdata, and effectively resets the data state back to init
*/
void free_mdviewdata();


/*! rewinds the animation, called by menu or key callbacks */
void rewindAnim();

void FrameAdvanceAnim(int iStep);	// frame up or down (I generally call this for the '<' & '>' keys)
void SetLODLevel(int iLOD);
void SetFaceSkin(int iSkin);

void animation_loop();

/*
frees a model that has been loaded before by loadmdl_totag into the modelptr slot
*/
void freemdl_fromtag( GLMODEL_DBLPTR modelptr );

/*
loads a model into the slot pointed to by dblptr
*/
void loadmdl_totag( char *fullName, GLMODEL_DBLPTR dblptr );

/*
loads a new costum skin and sets all the models to it
*/
void importNewSkin( char *fullName );
bool ParseSequenceLockFile(LPCSTR psFilename);

#define sRAVEN "\n\n(This version heavily customised by Ste Cork @/4 Raven Software)"

#define ABOUT_TEXT          "\n\n " FILENAME "\t\n\n" \
							"A q3test model viewer\t\n\n" \
				            "started by Sander 'FireStorm' van Rossen\t\n" \
							"& Matthew 'pagan' Baranowski\t\n\n" \
							"a Mental Vortex production\t\n\n" \
							"For more information go to: \t\n" \
							"mvwebsite.hypermart.net\t\n\n" \
							"SOURCE CODE IS FREELY AVAILABLE!!!\n\n" \
							sRAVEN

extern int giTagMenuSubtractValue_Torso;
extern int giTagMenuSubtractValue_Head;
extern int giTagMenuSubtractValue_Weapon;
extern int giTagMenuSubtractValue_Barrel;
extern int giTagMenuSubtractValue_Barrel2;
extern int giTagMenuSubtractValue_Barrel3;
extern int giTagMenuSubtractValue_Barrel4;
extern gl_model* pLastLoadedModel;
							

#endif
