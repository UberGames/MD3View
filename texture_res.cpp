/*
Copyright (C) 2010 Matthew Baranowski, Sander van Rossen, Raven software & Id Software, Inc.

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

#include "system.h"
#include "ndictionary.h"
#include "md3gl.h"
#include "md3view.h"
#include "targa.h"
#include "png/png.h"
#include "jpeg_interface.h"
#include "oddbits.h"


#pragma warning(disable : 4786)  //identifier was truncated 

#include <list>
//#include <map>
//#include <string>
//#include <set>
//#include <vector>
using namespace std;

#pragma warning(disable : 4786)  //identifier was truncated 



static void *RI_malloc(int size)
{
	void *pv = malloc(size);
	if (pv==NULL)
	{
		ErrorBox(va("malloc(): Failed to alloc %d bytes, exiting...\n(tell me -Ste)",size));
		exit(1);
	}
	memset(pv,0,size);
	return pv;
}



// fake cvar stuff
//
typedef struct
{
	int integer;
} cvar_t;

typedef list < void* > ptrArray_t;
ptrArray_t CvarList;
static cvar_t *InitCvar(int iInitialValue)
{
	cvar_t *p = (cvar_t*) RI_malloc(sizeof(*p));

	p->integer = iInitialValue;

	CvarList.push_back(p);

	return p;
}

// fake GL var stuff...
//
typedef enum {
	TC_NONE,
	TC_S3TC,
	TC_S3TC_DXT
} textureCompression_t;

typedef struct
{
	GLint maxTextureSize;
	textureCompression_t textureCompression;

} glConfig_t;

glConfig_t glConfig;

// called once only, and after GL is up and running...
//
void OnceOnly_GLVarsInit(void)
{
	memset(&glConfig,0,sizeof(glConfig));

	GLint temp;

	glGetIntegerv( GL_MAX_TEXTURE_SIZE, &temp );
	glConfig.maxTextureSize = temp;

	// stubbed or broken drivers may have reported 0...
	if ( glConfig.maxTextureSize <= 0 ) 
	{
		glConfig.maxTextureSize = 0;
	}

	glConfig.textureCompression = TC_NONE;
}


// cvars
cvar_t	*r_simpleMipMaps,
		*r_roundImagesDown,
		*r_picmipmin,
		*r_picmip,
		*r_texturebits,
		*r_ext_compressed_lightmaps,
		*r_colorMipLevels;

void FakeCvars_OnceOnlyInit(void)
{
	r_simpleMipMaps		= InitCvar(1);
	r_roundImagesDown	= InitCvar(1);
	r_picmipmin			= InitCvar(16);
	r_picmip			= InitCvar(0);
	r_texturebits		= InitCvar(32);
	r_ext_compressed_lightmaps = InitCvar(0);
	r_colorMipLevels	= InitCvar(0);	// debug only
}


static int *gpiReMipScratchBuffer = NULL;
int giReMipScratchBufferSize = 0;

void FakeCvars_Shutdown(void)
{
	for (ptrArray_t::iterator it = CvarList.begin(); it != CvarList.end(); ++it)
	{
		free(*it);
	}

	SAFEFREE(gpiReMipScratchBuffer);
	giReMipScratchBufferSize = 0;
}





/*
================
R_MipMap2

Operates in place, quartering the size of the texture
Proper linear filter
================
*/
static void R_MipMap2( unsigned *in, int inWidth, int inHeight ) {
	int			i, j, k;
	byte		*outpix;
	int			inWidthMask, inHeightMask;
	int			total;
	int			outWidth, outHeight;
	unsigned	*temp;

	outWidth = inWidth >> 1;
	outHeight = inHeight >> 1;
	temp = (unsigned int *) RI_malloc( outWidth * outHeight * 4 );

	inWidthMask = inWidth - 1;
	inHeightMask = inHeight - 1;

	for ( i = 0 ; i < outHeight ; i++ ) {
		for ( j = 0 ; j < outWidth ; j++ ) {
			outpix = (byte *) ( temp + i * outWidth + j );
			for ( k = 0 ; k < 4 ; k++ ) {
				total = 
					1 * ((byte *)&in[ ((i*2-1)&inHeightMask)*inWidth + ((j*2-1)&inWidthMask) ])[k] +
					2 * ((byte *)&in[ ((i*2-1)&inHeightMask)*inWidth + ((j*2)&inWidthMask) ])[k] +
					2 * ((byte *)&in[ ((i*2-1)&inHeightMask)*inWidth + ((j*2+1)&inWidthMask) ])[k] +
					1 * ((byte *)&in[ ((i*2-1)&inHeightMask)*inWidth + ((j*2+2)&inWidthMask) ])[k] +

					2 * ((byte *)&in[ ((i*2)&inHeightMask)*inWidth + ((j*2-1)&inWidthMask) ])[k] +
					4 * ((byte *)&in[ ((i*2)&inHeightMask)*inWidth + ((j*2)&inWidthMask) ])[k] +
					4 * ((byte *)&in[ ((i*2)&inHeightMask)*inWidth + ((j*2+1)&inWidthMask) ])[k] +
					2 * ((byte *)&in[ ((i*2)&inHeightMask)*inWidth + ((j*2+2)&inWidthMask) ])[k] +

					2 * ((byte *)&in[ ((i*2+1)&inHeightMask)*inWidth + ((j*2-1)&inWidthMask) ])[k] +
					4 * ((byte *)&in[ ((i*2+1)&inHeightMask)*inWidth + ((j*2)&inWidthMask) ])[k] +
					4 * ((byte *)&in[ ((i*2+1)&inHeightMask)*inWidth + ((j*2+1)&inWidthMask) ])[k] +
					2 * ((byte *)&in[ ((i*2+1)&inHeightMask)*inWidth + ((j*2+2)&inWidthMask) ])[k] +

					1 * ((byte *)&in[ ((i*2+2)&inHeightMask)*inWidth + ((j*2-1)&inWidthMask) ])[k] +
					2 * ((byte *)&in[ ((i*2+2)&inHeightMask)*inWidth + ((j*2)&inWidthMask) ])[k] +
					2 * ((byte *)&in[ ((i*2+2)&inHeightMask)*inWidth + ((j*2+1)&inWidthMask) ])[k] +
					1 * ((byte *)&in[ ((i*2+2)&inHeightMask)*inWidth + ((j*2+2)&inWidthMask) ])[k];
				outpix[k] = total / 36;
			}
		}
	}

	memcpy( in, temp, outWidth * outHeight * 4 );
	free( temp );
}

/*
================
R_MipMap

Operates in place, quartering the size of the texture
================
*/
static void R_MipMap (byte *in, int width, int height) {
	int		i, j;
	byte	*out;
	int		row;

	if ( !r_simpleMipMaps->integer ) {
		R_MipMap2( (unsigned *)in, width, height );
		return;
	}

	if ( width == 1 && height == 1 ) {
		return;
	}

	row = width * 4;
	out = in;
	width >>= 1;
	height >>= 1;

	if ( width == 0 || height == 0 ) {
		width += height;	// get largest
		for (i=0 ; i<width ; i++, out+=4, in+=8 ) {
			out[0] = ( in[0] + in[4] )>>1;
			out[1] = ( in[1] + in[5] )>>1;
			out[2] = ( in[2] + in[6] )>>1;
			out[3] = ( in[3] + in[7] )>>1;
		}
		return;
	}

	for (i=0 ; i<height ; i++, in+=row) {
		for (j=0 ; j<width ; j++, out+=4, in+=8) {
			out[0] = (in[0] + in[4] + in[row+0] + in[row+4])>>2;
			out[1] = (in[1] + in[5] + in[row+1] + in[row+5])>>2;
			out[2] = (in[2] + in[6] + in[row+2] + in[row+6])>>2;
			out[3] = (in[3] + in[7] + in[row+3] + in[row+7])>>2;
		}
	}
}


/*
================
ResampleTexture

Used to resample images in a more general than quartering fashion.

This will only be filtered properly if the resampled size
is greater than half the original size.

If a larger shrinking is needed, use the mipmap function 
before or after.
================
*/
static void ResampleTexture( unsigned *in, int inwidth, int inheight, unsigned *out,  
							int outwidth, int outheight ) {
	int		i, j;
	unsigned	*inrow, *inrow2;
	unsigned	frac, fracstep;
	unsigned	p1[1024], p2[1024];
	byte		*pix1, *pix2, *pix3, *pix4;

	fracstep = inwidth*0x10000/outwidth;

	frac = fracstep>>2;
	for ( i=0 ; i<outwidth ; i++ ) {
		p1[i] = 4*(frac>>16);
		frac += fracstep;
	}
	frac = 3*(fracstep>>2);
	for ( i=0 ; i<outwidth ; i++ ) {
		p2[i] = 4*(frac>>16);
		frac += fracstep;
	}

	for (i=0 ; i<outheight ; i++, out += outwidth) {
		inrow = in + inwidth*(int)((i+0.25)*inheight/outheight);
		inrow2 = in + inwidth*(int)((i+0.75)*inheight/outheight);
		frac = fracstep >> 1;
		for (j=0 ; j<outwidth ; j++) {
			pix1 = (byte *)inrow + p1[j];
			pix2 = (byte *)inrow + p2[j];
			pix3 = (byte *)inrow2 + p1[j];
			pix4 = (byte *)inrow2 + p2[j];
			((byte *)(out+j))[0] = (pix1[0] + pix2[0] + pix3[0] + pix4[0])>>2;
			((byte *)(out+j))[1] = (pix1[1] + pix2[1] + pix3[1] + pix4[1])>>2;
			((byte *)(out+j))[2] = (pix1[2] + pix2[2] + pix3[2] + pix4[2])>>2;
			((byte *)(out+j))[3] = (pix1[3] + pix2[3] + pix3[3] + pix4[3])>>2;
		}
	}
}



/*
==================
R_BlendOverTexture

Apply a color blend over a set of pixels
==================
*/
static void R_BlendOverTexture( byte *data, int pixelCount, byte blend[4] ) {
	int		i;
	int		inverseAlpha;
	int		premult[3];

	inverseAlpha = 255 - blend[3];
	premult[0] = blend[0] * blend[3];
	premult[1] = blend[1] * blend[3];
	premult[2] = blend[2] * blend[3];

	for ( i = 0 ; i < pixelCount ; i++, data+=4 ) {
		data[0] = ( data[0] * inverseAlpha + premult[0] ) >> 9;
		data[1] = ( data[1] * inverseAlpha + premult[1] ) >> 9;
		data[2] = ( data[2] * inverseAlpha + premult[2] ) >> 9;
	}
}

byte	mipBlendColors[16][4] = {
	{0,0,0,0},
	{255,0,0,128},
	{0,255,0,128},
	{0,0,255,128},
	{255,0,0,128},
	{0,255,0,128},
	{0,0,255,128},
	{255,0,0,128},
	{0,255,0,128},
	{0,0,255,128},
	{255,0,0,128},
	{0,255,0,128},
	{0,0,255,128},
	{255,0,0,128},
	{0,255,0,128},
	{0,0,255,128},
};



static void Upload32( unsigned *data, 
						  int width, int height, 
						  qboolean mipmap, 
						  qboolean picmip, 
						  qboolean isLightmap, 
						  int *format, 
						  int *pUploadWidth, int *pUploadHeight )
{
	int			samples;
	unsigned	*scaledBuffer = NULL;
	unsigned	*resampledBuffer = NULL;
	int			scaled_width, scaled_height;
	int			i, c;
	byte		*scan;
	GLenum		internalFormat = GL_RGB;
	float		rMax = 0, gMax = 0, bMax = 0;

	//
	// convert to exact power of 2 sizes
	//
	for (scaled_width = 1 ; scaled_width < width ; scaled_width<<=1)
		;
	for (scaled_height = 1 ; scaled_height < height ; scaled_height<<=1)
		;
	if ( r_roundImagesDown->integer && scaled_width > width )
		scaled_width >>= 1;
	if ( r_roundImagesDown->integer && scaled_height > height )
		scaled_height >>= 1;

	if ( scaled_width != width || scaled_height != height ) {
		resampledBuffer = (unsigned int *) RI_malloc( scaled_width * scaled_height * 4 );
		ResampleTexture (data, width, height, resampledBuffer, scaled_width, scaled_height);
		data = resampledBuffer;
		width = scaled_width;
		height = scaled_height;
	}

	//
	// perform optional picmip operation
	//
	if ( picmip ) {
		if (r_picmipmin->integer < scaled_width && r_picmipmin->integer < scaled_height) {	//only do it if it would be smaller
//			scaled_width >>= r_picmip->integer;
//			scaled_height >>= r_picmip->integer;
			int count = r_picmip->integer;
			// mip down and clamp to minimum picmipsize
			while ( count &&
				(scaled_width > r_picmipmin->integer
				|| scaled_height > r_picmipmin->integer) ) {
				scaled_width >>= 1;
				scaled_height >>= 1;
				count--;
			}
		}
	}

	//
	// clamp to minimum size
	//
	if (scaled_width < 1) {
		scaled_width = 1;
	}
	if (scaled_height < 1) {
		scaled_height = 1;
	}

	//
	// clamp to the current upper OpenGL limit
	// scale both axis down equally so we don't have to
	// deal with a half mip resampling
	//
	while ( scaled_width > glConfig.maxTextureSize
		|| scaled_height > glConfig.maxTextureSize ) {
		scaled_width >>= 1;
		scaled_height >>= 1;
	}

	scaledBuffer = (unsigned int *) RI_malloc( sizeof( unsigned ) * scaled_width * scaled_height );

	//
	// scan the texture for each channel's max values
	// and verify if the alpha channel is being used or not
	//
	c = width*height;
	scan = ((byte *)data);
	samples = 3;
	for ( i = 0; i < c; i++ )
	{
		if ( scan[i*4+0] > rMax )
		{
			rMax = scan[i*4+0];
		}
		if ( scan[i*4+1] > gMax )
		{
			gMax = scan[i*4+1];
		}
		if ( scan[i*4+2] > bMax )
		{
			bMax = scan[i*4+2];
		}
		if ( scan[i*4 + 3] != 255 ) 
		{
			samples = 4;
		}
	}

	// select proper internal format
	if ( samples == 3 )
	{
		// Set the default format, then override as necessary
		internalFormat = 3;// GL allows you to specify the number of color components in the texture like this

		if ( (glConfig.textureCompression != TC_NONE && !isLightmap) || 
			 (glConfig.textureCompression != TC_NONE && isLightmap && r_ext_compressed_lightmaps->integer) )
		{
/*modviewrem			if ( glConfig.textureCompression == TC_S3TC )
			{
				internalFormat = GL_RGB4_S3TC;
			}
			else if ( glConfig.textureCompression == TC_S3TC_DXT )
			{
				// Compress purely color - no alpha
				internalFormat = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
			}
*/
		}
		else if ( isLightmap /*modviewrem && r_texturebitslm->integer > 0 */)
		{
			/*modviewrem
			// Allow different bit depth when we are a lightmap
			if ( r_texturebitslm->integer == 16 )
			{
				internalFormat = GL_RGB5;
			}
			else if ( r_texturebitslm->integer == 32 )
			{
				internalFormat = GL_RGB8;
			}
			*/
		}
		else if ( r_texturebits->integer == 16 )
		{
			internalFormat = GL_RGB5;
		}
		else if ( r_texturebits->integer == 32 )
		{
			internalFormat = GL_RGB8;
		}
	}
	else if ( samples == 4 )
	{
		// Set the default format, then override as necessary
		internalFormat = 4;// GL allows you to specify the number of color components in the texture like this

/*modviewrem
		if ( glConfig.textureCompression == TC_S3TC_DXT && !isLightmap)
		{
			// Compress both alpha and colour
			internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
		}
		else */	if ( r_texturebits->integer == 16 )
		{
			internalFormat = GL_RGBA4;
		}
		else if ( r_texturebits->integer == 32 )
		{
			internalFormat = GL_RGBA8;
		}
	}

	// copy or resample data as appropriate for first MIP level
	if ( ( scaled_width == width ) && 
		( scaled_height == height ) ) {
		if (!mipmap)
		{
			glTexImage2D (GL_TEXTURE_2D, 0, internalFormat, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
			*pUploadWidth = scaled_width;
			*pUploadHeight = scaled_height;
			*format = internalFormat;

			goto done;
		}
		memcpy (scaledBuffer, data, width*height*4);
	}
	else
	{
		// use the normal mip-mapping function to go down from here
		while ( width > scaled_width || height > scaled_height ) {
			R_MipMap( (byte *)data, width, height );
			width >>= 1;
			height >>= 1;
			if ( width < 1 ) {
				width = 1;
			}
			if ( height < 1 ) {
				height = 1;
			}
		}
		memcpy( scaledBuffer, data, width * height * 4 );
	}

	//modviewrem R_LightScaleTexture (scaledBuffer, scaled_width, scaled_height, !mipmap );

	*pUploadWidth = scaled_width;
	*pUploadHeight = scaled_height;
	*format = internalFormat;

	glTexImage2D (GL_TEXTURE_2D, 0, internalFormat, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaledBuffer );

	if (mipmap)
	{
		int		miplevel;

		miplevel = 0;
		while (scaled_width > 1 || scaled_height > 1)
		{
			R_MipMap( (byte *)scaledBuffer, scaled_width, scaled_height );
			scaled_width >>= 1;
			scaled_height >>= 1;
			if (scaled_width < 1)
				scaled_width = 1;
			if (scaled_height < 1)
				scaled_height = 1;
			miplevel++;


			if ( r_colorMipLevels->integer ) {
				R_BlendOverTexture( (byte *)scaledBuffer, scaled_width * scaled_height, mipBlendColors[miplevel] );
			}

			glTexImage2D (GL_TEXTURE_2D, miplevel, internalFormat, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaledBuffer );
		}
	}
done:


#if 0	//modviewrem	
	if (mipmap)
	{
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, /*modviewrem gl_filter_min*/ GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, /*modviewrem gl_filter_max*/ GL_NEAREST);
/*modviewrem
		if(r_ext_texture_filter_anisotropic->integer && glConfig.textureFilterAnisotropicAvailable) {
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 2.0f);
		}
*/
	}
	else
	{
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	}
#else
	setTextureToFilter();
#endif

//modviewrem	GL_CheckErrors();

	if ( scaledBuffer != 0 )
		free( scaledBuffer );
	if ( resampledBuffer != 0 )
		free( resampledBuffer );
}




// subroutinised so I can add picmip stuff, called from both texture loader and textures-refresh...
//
static void Texture_BindAndUpload(TextureGL *pTexture)
{
//	pTexture->gluiBind = pTexture->gluiDesiredBind;
	glBindTexture( GL_TEXTURE_2D, pTexture->Bind );
	setTextureToFilter();
	
	// upload the image to opengl
/*	
	glTexImage2D (	GL_TEXTURE_2D,		// GLenum target,
					0,					// GLint level,		(LOD)
					GL_RGBA8,			// GLint components / internal dest format
					pTexture->iWidth,	// GLsizei width,
					pTexture->iHeight,	// GLsizei height,
					0,					// GLint border,
					GL_RGBA,			// GLenum format (src)
					GL_UNSIGNED_BYTE,	// GLenum type,
					pTexture->pPixels	// const GLvoid *pixels
					);
*/
// newcode
	int iFormat, iUploadWidth, iUploadHeight;

	// since mipping overwrites original buffer data, I'm going to have to do all ops on a scratchpad in this app...
	//
	int iThisScratchPadSizeNeeded = pTexture->Width * pTexture->Height * 4;
	if (iThisScratchPadSizeNeeded > giReMipScratchBufferSize)
	{
		giReMipScratchBufferSize = iThisScratchPadSizeNeeded;

		SAFEFREE(gpiReMipScratchBuffer);
		gpiReMipScratchBuffer = (int *) RI_malloc(iThisScratchPadSizeNeeded);
	}
	assert(gpiReMipScratchBuffer);
	memcpy(gpiReMipScratchBuffer, pTexture->Data, iThisScratchPadSizeNeeded);		
	

				Upload32(	(unsigned int *)gpiReMipScratchBuffer,//pTexture->pPixels,	// unsigned *data
							pTexture->Width,	// int width
							pTexture->Height,	// int height
							qtrue,				// qboolean mipmap
							qtrue,				// qboolean picmip
							qfalse,				// qboolean isLightmap
							&iFormat,			// int *format
							&iUploadWidth,		// int *pUploadWidth
							&iUploadHeight		// int *pUploadHeight
							);
}



int TextureList_GetMip(void)
{
	return r_picmip->integer;
}
void TextureList_ReMip(int iMIPLevel)
{
	StartWait();	//CWaitCursor wait;

	r_picmip->integer = iMIPLevel;

//	OutputDebugString(va("Setting picmip to %d\n",iMIPLevel));

//	string strJunk;	
	
	for (NodePosition pos=mdview.textureRes->first() ; pos!= NULL ; pos=mdview.textureRes->after(pos)) 
	{
		TextureGL *pTexture = (TextureGL *)pos->element();		

		if (pTexture->Bind)
		{
//			assert(pTexture->pPixels);			

			if (pTexture->Data)
			{
				// now bind to opengl texture...
				// 
				Texture_BindAndUpload(pTexture);	

				// check errors from texture binding
/*
				{
					static bool bDone = false;
					//if (!bDone)
					{
						bDone = true;

						GLint iValue,iValue2;
						glGetIntegerv(GL_ATTRIB_STACK_DEPTH,&iValue);
						glGetIntegerv(GL_MAX_ATTRIB_STACK_DEPTH,&iValue2);

						OutputDebugString(va("iValue %d, iValue2 %d\n",iValue,iValue2));

						strJunk += va("iValue %d, iValue2 %d\n",iValue,iValue2);																						
					}
				}
*/
				GLenum error = glGetError();
				if (error && error != GL_STACK_OVERFLOW	/* fucking stupid ATI cards report this for no reason sometimes */ ) 
				{
					static bool bKeepGettingErrors = true;
					assert(0);

					if (bKeepGettingErrors)
					{
						bKeepGettingErrors = GetYesNo(va("Error: TextureList_ReMip()::glGetError = %d\n\nContinue receiving GL errors here?", error));
//						InfoBox(strJunk.c_str());
					}
					//return;
					RestoreWait();
				}
			}
			else
			{
				//pTexture->gluiBind = 0;
			}
		}
	}

//	InfoBox(strJunk.c_str());
	EndWait();
}



/*
bind current texture to filtered mode
*/
void bindTextureFiltered()
{
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glHint( GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST );
}

/*
bind current texture to unfiltered mode
*/
void bindTextureUnFiltered()
{
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glHint( GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST );
}


/*
bind current to fastest texture rendering
*/
void bindTextureFast()
{
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glHint( GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST );
}


/*
chooses the specific filter to be applied based on current mode
*/
void setTextureToFilter()
{
	switch (mdview.texMode) {
		case TEX_FAST: bindTextureFast(); break;
		case TEX_UNFILTERED: bindTextureUnFiltered(); break;
		case TEX_FILTERED: bindTextureFiltered(); break;
	};
}


/*
sets texture parameters to the current texMode
*/
void setTextureFilter()
{
	NodePosition pos;
	TextureGL *texture;
	for (pos=mdview.textureRes->first() ; pos!= NULL ; pos=mdview.textureRes->after(pos)) {
		texture = (TextureGL *)pos->element();
		glBindTexture( GL_TEXTURE_2D, texture->Bind );
		setTextureToFilter();
	}
}

/*
searches for the file first in the texturePath, 
then in each descending directory of basepath
*/
bool searchForPath( LPCSTR texturePath, char *foundPath )
{
	char buffer1[512];
	char buffer1JPG[512];
	char buffer1PNG[512];
	char buffer2[512];

	char *texname	= &buffer1[0];
	char *texnameJPG= &buffer1JPG[0];
	char *texnamePNG= &buffer1PNG[0];
	char *texpath	= &buffer2[0];

	strcpy( texpath, (const char *)mdview.basepath );
	strcpy( texname, Filename_WithoutExt(texturePath) );
	strlwr( texname );
	strcat( texname, ".tga");

//========= JPG
	strcpy( texnameJPG, Filename_WithoutExt(texname));
	strlwr( texnameJPG );
	strcat( texnameJPG,".jpg");	
//=========

//========= PNG
	strcpy( texnamePNG, Filename_WithoutExt(texname));
	strlwr( texnamePNG );	
	strcat( texnamePNG,".png");	
//=========

	if (!file_exists( texname ) && !file_exists( texnameJPG ) && !file_exists( texnamePNG))
	{
		int k = strlen(texpath);
		do {
			do	{
				k--;
			} while ( (k>0) && (texpath[k]!='\\') && (texpath[k]!='/') );
			texpath[k]=0;
			if (k>0) 
			{
				strcpy( texname, Filename_WithoutExt(texpath) );
				strcat( texname, "/" );
				strcat( texname, Filename_WithoutExt(texturePath) );
				strcat( texname, ".tga" );
//========= JPG
				strcpy( texnameJPG, Filename_WithoutExt(texname));
				strlwr( texnameJPG );
				strcat( texnameJPG, ".jpg" );
//=========
//========= PNG
				strcpy( texnamePNG, Filename_WithoutExt(texname));
				strlwr( texnamePNG );
				strcat( texnamePNG, ".png" );
//=========
			}
		} while ( (k>0) && (!file_exists(texname) && !file_exists(texnameJPG) && !file_exists(texnamePNG)) );

		if (k<=0) {
			Debug("texture \"%s\" not found",texturePath);
			return false;
		}
	}

	// I hate this fucking inconsistant with or without extension logic that changes every project
	if (file_exists(texnamePNG))
	{
		strcpy( foundPath, texnamePNG );
	}
	else
	if (file_exists(texnameJPG))
	{
		strcpy( foundPath, texnameJPG );
	}
	else
	{
		strcpy( foundPath, texname );
	}
	return true;
}

void LoadTexture(char *foundPath, TextureGL *texture)
{		
	strlwr(foundPath);
	if (strstr(foundPath,".png"))
	{
		LoadPNG32( foundPath, &texture->Data, (int *)&texture->Width, (int *)&texture->Height, (int *)&texture->_Type );
	}
	else
	if (strstr(foundPath,".jpg"))
	{
		LoadJPG( foundPath, &texture->Data, (int*) &texture->Width, (int*) &texture->Height );
	}
	else
	{
		loadTGA( foundPath, &texture->Data, &texture->Width, &texture->Height, &texture->_Type );
	}
}


/*
damn smart and efficient texture loader, 
first searches for an existing teature name is parent directories
then checks if texture has already been loaded,
otherwise it loads it up, enters in textureRes manager and returns the new binding
*/
bool gbIgnoreTextureLoad = false;
GLuint loadTexture( LPCSTR texturepath )
{
	if (gbIgnoreTextureLoad || !strlen(texturepath))
	{
		return (GLuint)0;
	}

	char foundPath[512];
	GLenum error;

	// look for the actual file
	if (!searchForPath( texturepath, foundPath )) {
		// if file doesn't exist just exit
		return (GLuint)0;
	}

	// check errors from before
	error = glGetError();
#ifdef DEBUG_PARANOID
	if (error) {
		Debug("before loadTexture::glGetError = %d", error);
		return (GLuint)0;
	}
#endif


	// see if it has been loaded into texture resource manager
	TextureGL *texture = (TextureGL *)mdview.textureRes->find(foundPath);
	
	if (texture) {
		// if so just return the binding
		texture->numUsed++;
		return texture->Bind;
	} else {
		// otherwise must load a new resource
		texture = NULL;
		texture = new TextureGL;
		texture->Data = NULL;

		LoadTexture(foundPath,texture);

		// now bind to opengl texture, 
		if ( texture->Data != NULL ) {					

			// allocate texture path string
			texture->File = new char[strlen(foundPath)+1];
			strcpy( texture->File, foundPath );
			texture->numUsed = 1;

			// TODO: use glGenTextures perhaps
			texture->Bind = ++mdview.topTextureBind;
#if 0
/*								
			// bind and set texture parameters
			glBindTexture( GL_TEXTURE_2D, texture->Bind );				
			setTextureToFilter();

			// upload the image to opengl

			glTexImage2D (	GL_TEXTURE_2D,		// GLenum target,
							0,					// GLint level,		(LOD)
							GL_RGBA8,			// GLint components / internal dest format
							texture->Width,		// GLsizei width,
							texture->Height,	// GLsizei height,
							0,					// GLint border,
							GL_RGBA,			// GLenum format (src)
							GL_UNSIGNED_BYTE,	// GLenum type,
							texture->Data 		// const GLvoid *pixels
							);

*/
#else
			Texture_BindAndUpload(texture);
#endif

			// check errors from texture binding
			error = glGetError();
			if (error) {
				Debug("loadTexture::glGetError = %d", error);
				return (GLuint)0;
			}

			// insert texture keyed by file name
			mdview.textureRes->insert( (Object)texture, (Object)texture->File );
		}
		else {		
			delete texture;
			return (GLuint)0;
		}

		// return sucessfully loaded texture binding
		return texture->Bind;
	}	
}

/* 
reloads all the texture in the texture manager
*/
void refreshTextureRes()
{
	StartWait();

	NodePosition loc, next;
	TextureGL *texture;
	GLuint error;

	for (loc = mdview.textureRes->first() ; loc!=NULL ; loc=next) {

		next = mdview.textureRes->after(loc);
		texture = (TextureGL *)loc->element();		

		// free texture data
		delete [] texture->Data; texture->Data = NULL;		
		
		char foundPath[512];
		if (searchForPath( texture->File, foundPath )) 
		{
			LoadTexture(foundPath,texture);
			Texture_BindAndUpload(texture);

			// check errors from texture binding
			error = glGetError();
			if (error) {
				Debug("refreshTextureRes::glGetError = %d", error);
				EndWait();
				return;
			}
		}
		else
		{
			RestoreWait();
		}
	}				

	EndWait();
}

/*
frees a texture resource, called whenever a mesh is being freed. a texture is freed only if nothing is using it anymore
*/
void freeTexture( GLuint bind )
{
	NodePosition loc, next;
	TextureGL *texture;

	if (bind == 0) return;

	for (loc = mdview.textureRes->first() ; loc!=NULL ; loc=next) {
		next = mdview.textureRes->after(loc);
		texture = (TextureGL *)loc->element();		

#ifdef DEBUG_PARANOID
		if (texture == NULL) Debug("freeTexture: texture element is null");
		if ( texture->Data == NULL ) Debug("freeTexture: texture Data is null");
#endif

		if (bind == texture->Bind) {
			texture->numUsed--;
			if (texture->numUsed == 0) {
				((NodeSequence)(mdview.textureRes))->remove( loc );
				delete [] texture->Data; texture->Data = NULL;				
				delete [] texture->File;
				delete texture; texture = NULL;				
			}
			return;
		}
	}
}

/*
deletes the entire texture resource manager with all its textures at once, usually called when reseting viewer state
*/
void freeTextureRes()
{
	NodePosition loc, next;
	TextureGL *texture;

	for (loc = mdview.textureRes->first() ; loc!=NULL ; loc=next) {

		next = mdview.textureRes->after(loc);
		texture = (TextureGL *)loc->element();		

#ifdef DEBUG_PARANOID
		if (texture == NULL) Debug("freeTextureRes: texture element is null");
		if ( texture->Data == NULL ) Debug("freeTextureRes: texture Data is null");
#endif
		
		((NodeSequence)(mdview.textureRes))->remove( loc );
		
		delete [] texture->Data; texture->Data = NULL;		
		delete [] texture->File;
		delete texture; texture = NULL;				
	}			

	mdview.topTextureBind = 0;
}
