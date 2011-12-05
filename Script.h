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

#ifndef _RENDERPASS_H_
#define _RENDERPASS_H_

enum typeTextureEnum
{
	texture_normal, texture_alpha, texture_anim
};

class RenderPass
{
public:
	//tcMod - texcoords
	float			rp_rotate,rp_scalex,rp_scaley,rp_scrollx,rp_scrolly,
					//i don't know what the values a-d are for.
					rp_turba,rp_turbb,rp_turbc,rp_turbd,
					//i don't know what the values a-d are for.
					rp_stretchsina,rp_stretchsinb,rp_stretchsinc,rp_stretchsind;
	//various gl functions
	GLenum			blend_sfactor,blend_dfactor,alpha_func,depth_func;
	GLclampf		alpha_ref;

	//misc commands
	bool			rp_detail,rp_depthwrite,rp_clamptexcoords;

	NodeDictionary	AnimationFramesNames;
	NodeDictionary	AnimationFramesBuffers;
	int				AnimationFramesNum;
	char*			AlphaName;
	GLuint			AlphaBuffer;
	char*			TextureName;
	GLuint			TextureBuffer;

	typeTextureEnum	typeTexture;

	RenderPass();
	void			AddParam( char* Type, char* Var );
};

typedef RenderPass*	RenderPassPtr;

enum culltype {cull_normal,cull_none, cull_disable, cull_twosided, cull_backsided, cull_back};

class Surface
{
public:
	//surfaceparam
	bool			sp_trans      ,sp_metalsteps,sp_nolightmap,sp_nodraw    ,sp_noimpact,
					sp_nonsolid   ,sp_nomarks   ,sp_nodrop    ,sp_nodamage  ,sp_playerclip,
					sp_structural ,sp_slick     ,sp_origin    ,sp_areaportal,sp_fog,
					sp_lightfilter,sp_water     ,sp_slime     ,sp_sky       ,sp_lava,
					sf_light1;
	//cull
	culltype		cull;

	//misc commands
	bool			sf_portal,sf_fogonly,sf_nomipmaps,sf_polygonOffset,
					sf_lightning,sf_backsided,sf_qer_nocarve;
	float			sf_light,sf_tesssize,sf_sort,sf_qer_trans,sf_q3map_backsplash,
					sf_q3map_surfacelight;
	char			*sf_sky,*sf_q3map_lightimage,*sf_qer_editorimage,
					*sf_qer_lightimage;

	NodeDictionary	RenderPasses;
	UINT32			pass_num;

	Surface();
	~Surface();
	void			AddParam( char* Type, char* Var );
	void			AddPass	( RenderPass* Pass );
	void			LoadTextures();
	void			DestroyTextures();
};

typedef Surface*	SurfacePtr;

Surface*	LoadSurface( char* filename );
void		freeSurface( Surface* );

#endif //_RENDERPASS_H_