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


/****************/
/* !!WARNING!!  */
/* UNDER HEAVY  */
/* CONSTRUCTION */
/****************/

//SVR: lol

#include "system.h"


/*
special map names:
	$lightmap
	*white
	$whiteimage
	$fromblack

types of waves:
	"wave","sin"			,float,float,float,float
	"wave","inversesawtooth",float,float,float,float
	"wave","sawtooth"		,float,float,float,float
	"wave","square"			,float,float,float,float
	"wave","noise"			,float,float,float,float
	"wave","triangle"		,float,float,float,float

}
*/

void	clean( char*& line )
{
	unsigned int i;
	if ( line[ 0 ] != 0 )
	{
		i = 0;while ( ( i < strlen( line ) ) && ( line[ i ] != '/' ) ) i++;
		if ( ( line[ i ] == '/' ) && ( line[ i + 1 ] == '/' )) line[ i ] = 0;
		if ( line[ 0 ] != 0 )
		{
			for ( i = 0 ; i < strlen( line ) ; i++ )
				if ( line[ i ] ==  9 ) line[ i ] = 32; else
				if ( line[ i ] == 10 ) line[ i ] = 32;
			
			i = strlen( line ) - 1 ;
			while ( line[ i ] == 32 ) i--;
			line[ i + 1 ] = 0;

			if ( line[ 0 ] != 0 )
			{
				while ( line[ 0 ] == 32 ) line++;

				if ( line[ 0 ] == 0 )
					line = NULL;
			} else
				line = NULL;
		} else
			line = NULL;
	} else
		line = NULL;
}

char*	getstring( char*& line )
{
	if ( line != NULL )
	{
		unsigned int i;
		char*	retptr = line;
		i = 0;while ( ( line[ i ] != 32 ) && ( line[ i ] != 0 ) ) i++;
		if ( line[ i ] != 0 ) 
		{
			line[ i ] = 0;
			line = &line[ i + 1 ];
			if ( line[ 0 ] == 0 )
				line = NULL; 
			else while ( line[ 0 ] == 32 ) line++;
		} else
			line = NULL;
		return retptr;
	} else
		return NULL;
}

void Error (char *error, ...);
void Debug (char *error, ...);

/*START UNFINISHED BIT*/

class RenderPass
{
public:
	//tcMod - texcoords
	float		rp_rotate,rp_scalex,rp_scaley,rp_scrollx,rp_scrolly,
				//i don't know what the values a-d are for.
				rp_turba,rp_turbb,rp_turbc,rp_turbd,
				//i don't know what the values a-d are for.
				rp_stretchsina,rp_stretchsinb,rp_stretchsinc,rp_stretchsind;
	//various gl functions	
	GLenum		blend_sfactor, blend_dfactor,alpha_func,depth_func;
	GLclampf	alpha_ref;

	//misc commands
	bool	rp_detail,rp_depthwrite,rp_clamptexcoords;

	void	AddParam( char* Type, char* Var );
};

class Surface
{
public:
	//surfaceparam
	bool    sp_trans      ,sp_metalsteps,sp_nolightmap,sp_nodraw    ,sp_noimpact,
			sp_nonsolid   ,sp_nomarks   ,sp_nodrop    ,sp_nodamage  ,sp_playerclip,
			sp_structural ,sp_slick     ,sp_origin    ,sp_areaportal,sp_fog,
			sp_lightfilter,sp_water     ,sp_slime     ,sp_sky       ,sp_lava;
	//cull
	enum	{cull_none, cull_disable, cull_twosided, cull_backsided, cull_back} cull;

	//misc commands
	bool	sf_portal,sf_fogonly,sf_nomipmaps,sf_polygonOffset,
			sf_lightning,sf_backsided,sf_qer_nocarve;
	float	sf_light,sf_tesssize,sf_sort,sf_qer_trans,sf_q3map_backsplash,
			sf_q3map_surfacelight;
	char	*sf_sky,*sf_q3map_lightimage,*sf_qer_editorimage;


	void	AddParam( char* Type, char* Var );
	void	AddPass( RenderPass*& Pass );
};

Surface*	CreateSurface( char* Name )
{
	return new Surface;
}

void	RenderPass::AddParam( char* Type, char* Var )
{
	char*	Token;
	if ( stricmp( Type , "tcMod" ) == 0 )
	{
		if ( Var == NULL )
			Error( "token expected after surfaceparam, but none found");

		Token = getstring( Var );
		if ( Var == NULL )
			Error( "unexpected end after token: %s" , Token );
		
		if ( stricmp( Token , "rotate" ) == 0 )
		{
			Token = getstring( Var );
			if ( Var != NULL )
				Error( "%s followed by unexpected token: %s" , Token, Var );

			//TODO: add check if it's really a number
			rp_rotate = (float)atof(Token);
		} else
		if ( stricmp( Token , "scale" ) == 0 )
		{
			Token = getstring( Var );
			if ( Var == NULL )
				Error( "unexpected end after token: %s" , Token );

			//TODO: add check if it's really a number
			rp_scalex = (float)atof(Token);

			Token = getstring( Var );
			if ( Var != NULL )
				Error( "%s followed by unexpected token: %s" , Token, Var );

			//TODO: add check if it's really a number
			rp_scaley = (float)atof(Token);

		} else
		if ( stricmp( Token , "scroll" ) == 0 )
		{
			Token = getstring( Var );
			if ( Var == NULL )
				Error( "unexpected end after token: %s" , Token );

			//TODO: add check if it's really a number
			rp_scrollx = (float)atof(Token);

			Token = getstring( Var );
			//there is this "tcmod scroll" with four!! paramters..
			//i have no idear what the other 2 are for.
//			if ( Var != NULL )
//				Error( "%s followed by unexpected token: %s" , Token, Var );

			//TODO: add check if it's really a number
			rp_scrolly = (float)atof(Token);

		} else
		if ( stricmp( Token , "turb" ) == 0 )
		{
			Token = getstring( Var );
			if ( Var == NULL )
				Error( "unexpected end after token: %s" , Token );

			//TODO: add check if it's really a number
			rp_turba = (float)atof(Token);

			Token = getstring( Var );
			if ( Var == NULL )
				Error( "unexpected end after token: %s" , Token );

			//TODO: add check if it's really a number
			rp_turbb = (float)atof(Token);

			Token = getstring( Var );
			if ( Var == NULL )
				Error( "unexpected end after token: %s" , Token );

			//TODO: add check if it's really a number
			rp_turbc = (float)atof(Token);

			Token = getstring( Var );
			if ( Var != NULL )
				Error( "%s followed by unexpected token: %s" , Token, Var );

			//TODO: add check if it's really a number
			rp_turbd = (float)atof(Token);
		} else
		if ( stricmp( Token , "stretch" ) == 0 )
		{
			Token = getstring( Var );
			if ( Var == NULL )
				Error( "unexpected end after token: %s" , Token );

			if ( stricmp( Type , "sin" ) != 0 )
				Error( "unknown tcmod stretch function: %s" , Token );

			//TODO: add check if it's really a number
			rp_stretchsina = (float)atof(Token);

			Token = getstring( Var );
			if ( Var == NULL )
				Error( "unexpected end after token: %s" , Token );

			//TODO: add check if it's really a number
			rp_stretchsinb = (float)atof(Token);

			Token = getstring( Var );
			if ( Var == NULL )
				Error( "unexpected end after token: %s" , Token );

			//TODO: add check if it's really a number
			rp_stretchsinc = (float)atof(Token);

			Token = getstring( Var );
			if ( Var != NULL )
				Error( "%s followed by unexpected token: %s" , Token, Var );

			//TODO: add check if it's really a number
			rp_stretchsind = (float)atof(Token);
		} else
			Error("unknown tcmod command found %s:",Token);
	} else
	if ( stricmp( Type , "blendfunc" ) == 0 )
	{
		if ( Var == NULL )
			Error( "token expected after blendfunc, but none found");

		Token = getstring( Var );
		if ( Var == NULL )
			Error( "unexpected end after token: %s" , Token );

		if ( stricmp( Token , "gl_zero"					) == 0 ) blend_sfactor=GL_ZERO;else
		if ( stricmp( Token , "gl_one"					) == 0 ) blend_sfactor=GL_ONE;else
		if ( stricmp( Token , "gl_dst_color"			) == 0 ) blend_sfactor=GL_DST_COLOR;else
		if ( stricmp( Token , "gl_one_minus_dst_color"	) == 0 ) blend_sfactor=GL_ONE_MINUS_DST_COLOR;else
		if ( stricmp( Token , "gl_src_alpha"			) == 0 ) blend_sfactor=GL_SRC_ALPHA;else
		if ( stricmp( Token , "gl_one_minus_src_alpha"	) == 0 ) blend_sfactor=GL_ONE_MINUS_SRC_ALPHA;else
		if ( stricmp( Token , "gl_dst_alpha"			) == 0 ) blend_sfactor=GL_DST_ALPHA;else
		if ( stricmp( Token , "gl_one_minus_dst_alpha"	) == 0 ) blend_sfactor=GL_ONE_MINUS_DST_ALPHA;else
		if ( stricmp( Token , "gl_src_alpha_saturate"	) == 0 ) blend_sfactor=GL_SRC_ALPHA_SATURATE;
		else Error( "unknown blendfunc parameter found: %s ", Token );

		Token = getstring( Var );
		if ( Var != NULL )
			Error( "%s followed by unexpected token: %s" , Token, Var );

		if ( stricmp( Token , "gl_zero"					) == 0 ) blend_dfactor=GL_ZERO;else
		if ( stricmp( Token , "gl_one"					) == 0 ) blend_dfactor=GL_ONE;else
		if ( stricmp( Token , "gl_src_color"			) == 0 ) blend_dfactor=GL_SRC_COLOR;else
		if ( stricmp( Token , "gl_one_minus_dst_color"	) == 0 ) blend_dfactor=GL_ONE_MINUS_SRC_COLOR;else
		if ( stricmp( Token , "gl_src_alpha"			) == 0 ) blend_dfactor=GL_SRC_ALPHA;else
		if ( stricmp( Token , "gl_one_minus_src_alpha"	) == 0 ) blend_dfactor=GL_ONE_MINUS_SRC_ALPHA;else
		if ( stricmp( Token , "gl_dst_alpha"			) == 0 ) blend_dfactor=GL_DST_ALPHA;else
		if ( stricmp( Token , "gl_one_minus_dst_alpha"	) == 0 ) blend_dfactor=GL_ONE_MINUS_DST_ALPHA;
		else Error( "unknown blendfunc parameter found: %s ", Token );
	} else
	if ( stricmp( Type , "alphafunc" ) == 0 )
	{
		if ( Var == NULL )
			Error( "token expected after alphafunc, but none found");

		Token = getstring( Var );
		if ( Var != NULL )
			Error( "%s followed by unexpected token: %s" , Token, Var );

		if ( stricmp( Token , "gt0"	 ) == 0 ) {
			alpha_func = GL_GREATER;alpha_ref = 0;
		} else
		if ( stricmp( Token , "lt128") == 0 ) {
			alpha_func = GL_LESS;alpha_ref = 128;
		} else
		if ( stricmp( Token , "ge128") == 0 ) {
			alpha_func = GL_GEQUAL;alpha_ref = 128;
		}
		else Error( "unknown alphafunc parameter found: %s ", Token );
	} else
	if ( stricmp( Type , "depthfunc" ) == 0 )
	{
		if ( Var == NULL )
			Error( "token expected after depthfunc, but none found");
		
		Token = getstring( Var );
		if ( Var != NULL )
			Error( "%s followed by unexpected token: %s" , Token, Var );

		if ( stricmp( Token , "equal"	 ) == 0 ) { 
			depth_func = GL_EQUAL;
		}
		else Error( "unknown depthfunc parameter found: %s ", Token );
	} else
	if ( stricmp( Type , "detail" ) == 0 )
	{
		if ( Var != NULL )
			Error( "detail followed by unexpected token: %s" , Var );

		rp_detail = true;
	} else
	if ( stricmp( Type , "depthwrite" ) == 0 )
	{
		if ( Var != NULL )
			Error( "depthwrite followed by unexpected token: %s" , Var );

		rp_depthwrite = true;
	} else
	if ( stricmp( Type , "clamptexcoords" ) == 0 )
	{
		if ( Var != NULL )
			Error( "clamptexcoords followed by unexpected token: %s" , Var );

		rp_clamptexcoords = true;
	} else
	if ( stricmp( Type , "tcgen" ) == 0 )
	{
		if ( Var == NULL )
			Error( "token expected after tcgen, but none found");

		Token = getstring( Var );
		if ( Var != NULL )
			Error( "%s followed by unexpected token: %s" , Token, Var );

		if ( stricmp( Token , "environment" ) != 0 )
			Error( "unknown tcgen command found: %s ", Token );

		//TODO: add functionality
		// Create enviroment map
	} else
	if ( stricmp( Type , "map" ) == 0 )
	{
		if ( Var == NULL )
			Error( "token expected after map, but none found");
		
		Token = getstring( Var );
		if ( Var != NULL )
			Error( "%s followed by unexpected token: %s" , Token, Var );

		//TODO: add functionality
		// load texture map
	} else
	if ( stricmp( Type , "alphamap" ) == 0 )
	{
		if ( Var == NULL )
			Error( "token expected after alphamap, but none found");

		Token = getstring( Var );
		if ( Var != NULL )
			Error( "%s followed by unexpected token: %s" , Token, Var );

		//TODO: add functionality
		// load texture alphamap
	} else
	if ( stricmp( Type , "animmap" ) == 0 )
	{
		if ( Var == NULL )
			Error( "token expected after animmap, but none found");

		Token = getstring( Var );
		if ( Var == NULL )
			Error( "unexpected end after token: %s" , Token );

		// Token == unknown number

		while ( ( Token = getstring( Var ) ) != NULL )
		{
			//TODO: add functionality
			// load texture animmap
		}
	} else
	if ( stricmp( Type , "rgbgen" ) == 0 )
	{
		if ( Var == NULL )
			Error( "token expected after rgbgen, but none found");
		/*
			{"rgbGen","wave","sin",float,float,float,float}
			{"rgbGen","wave","inversesawtooth",float,float,float,float}
			{"rgbGen","wave","sawtooth",float,float,float,float}
			{"rgbGen","wave","square",float,float,float,float}
			{"rgbGen","wave","noise",float,float,float,float}
			{"rgbGen","wave","triangle",float,float,float,float}
			{"rgbGen","identity"}
			{"rgbGen","entity"}
			{"rgbGen","exactvertex"}
			{"rgbGen","vertex"}
			{"rgbGen","identitylighting"}
			{"rgbGen","lightingDiffuse"}
			{"rgbGen","lightingSpecular"}
			{"rgbGen","oneminusentity"}
		*/
	} else
	if ( stricmp( Type , "alphagen" ) == 0 )
	{
		if ( Var == NULL )
			Error( "token expected after alphagen, but none found");
		/*
			{"colorGen","wave","sin",float,float,float,float}
			{"colorGen","wave","inversesawtooth",float,float,float,float}
			{"colorGen","wave","sawtooth",float,float,float,float}
			{"colorGen","wave","square",float,float,float,float}
			{"colorGen","wave","noise",float,float,float,float}
			{"colorGen","wave","triangle",float,float,float,float}
			{"colorGen","identity"}
			{"colorGen","entity"}
			{"colorGen","exactvertex"}
			{"colorGen","vertex"}
			{"colorGen","identitylighting"}
			{"colorGen","lightingDiffuse"}
			{"colorGen","lightingSpecular"}
			{"colorGen","oneminusentity"}
		*/
	} else
	if ( stricmp( Type , "colorgen" ) == 0 )
	{
		if ( Var == NULL )
			Error( "token expected after colorgen, but none found");
		/*
			{"alphaGen","wave","sin",float,float,float,float}
			{"alphaGen","wave","inversesawtooth",float,float,float,float}
			{"alphaGen","wave","sawtooth",float,float,float,float}
			{"alphaGen","wave","square",float,float,float,float}
			{"alphaGen","wave","noise",float,float,float,float}
			{"alphaGen","wave","triangle",float,float,float,float}
			{"alphaGen","identity"}
			{"alphaGen","entity"}
			{"alphaGen","exactvertex"}
			{"alphaGen","vertex"}
			{"alphaGen","identitylighting"}
			{"alphaGen","lightingDiffuse"}
			{"alphaGen","lightingSpecular"}
			{"alphaGen","oneminusentity"}
		*/
	} else
		Error("unknown command found: %s", Type );
}

void	Surface::AddParam( char* Type, char* Var )
{
	char*	Token;
	if ( stricmp( Type , "surfaceparam" ) == 0 )
	{
		if ( Var == NULL )
			Error( "token expected after surfaceparam, but none found");

		Token = getstring( Var );
		if ( Var != NULL )
			Error( "%s followed by unexpected token: %s" , Token, Var );

		if ( stricmp( Token , "lightfilter" ) == 0 ) sp_lightfilter = true;else
		if ( stricmp( Token , "playerclip"  ) == 0 ) sp_playerclip  = true;else
		if ( stricmp( Token , "structural"  ) == 0 ) sp_structural  = true;else
		if ( stricmp( Token , "areaportal"  ) == 0 ) sp_areaportal  = true;else
		if ( stricmp( Token , "metalsteps"  ) == 0 ) sp_metalsteps  = true;else
		if ( stricmp( Token , "nolightmap"  ) == 0 ) sp_nolightmap  = true;else
		if ( stricmp( Token , "noimpact"    ) == 0 ) sp_noimpact	 = true;else
		if ( stricmp( Token , "nodamage"    ) == 0 ) sp_nodamage    = true;else
		if ( stricmp( Token , "nonsolid"    ) == 0 ) sp_nonsolid    = true;else
		if ( stricmp( Token , "nomarks"     ) == 0 ) sp_nomarks     = true;else
		if ( stricmp( Token , "origin"      ) == 0 ) sp_origin      = true;else
		if ( stricmp( Token , "nodrop"      ) == 0 ) sp_nodrop      = true;else
		if ( stricmp( Token , "nodraw"      ) == 0 ) sp_nodraw      = true;else
		if ( stricmp( Token , "water"       ) == 0 ) sp_water       = true;else
		if ( stricmp( Token , "slime"       ) == 0 ) sp_slime       = true;else
		if ( stricmp( Token , "slick"       ) == 0 ) sp_slick       = true;else
		if ( stricmp( Token , "trans"       ) == 0 ) sp_trans       = true;else
		if ( stricmp( Token , "lava"        ) == 0 ) sp_lava        = true;else
		if ( stricmp( Token , "sky"         ) == 0 ) sp_sky         = true;else
		if ( stricmp( Token , "fog"         ) == 0 ) sp_fog         = true;
		else Error( "unknown surfaceparam variable: %s" , Token );

	} else
	if ( stricmp( Type , "cull" ) == 0 )
	{
		if ( Var == NULL )
			Error( "token expected after cull, but none found");

		Token = getstring( Var );
		if ( Var != NULL )
			Error( "%s followed by unexpected token: %s" , Token, Var );

		if ( stricmp( Token , "none"		) == 0 ) cull = cull_none     ;else
		if ( stricmp( Token , "disable"		) == 0 ) cull = cull_disable  ;else
		if ( stricmp( Token , "twosided"	) == 0 ) cull = cull_twosided ;else
		if ( stricmp( Token , "backsided"   ) == 0 ) cull = cull_backsided;else
		if ( stricmp( Token , "back"        ) == 0 ) cull = cull_back     ;
		else Error( "unknown cull variable: %s" , Token );
		
	} else
	if ( stricmp( Type , "light" ) == 0 )
	{
		if ( Var == NULL )
			Error( "token expected after light, but none found");

		Token = getstring( Var );
		if ( Var != NULL )
			Error( "%s followed by unexpected token: %s" , Token, Var );

		//TODO: add check if it's really a number
		sf_light = (float)atof(Token);
	} else
	if ( stricmp( Type , "tesssize" ) == 0 )
	{
		if ( Var == NULL )
			Error( "token expected after tesssize, but none found");

		Token = getstring( Var );
		if ( Var != NULL )
			Error( "%s followed by unexpected token: %s" , Token, Var );

		//TODO: add check if it's really a number
		sf_tesssize = (float)atof(Token);
	} else
	if ( stricmp( Type , "sort" ) == 0 )
	{
		if ( Var == NULL )
			Error( "token expected after sort, but none found");

		Token = getstring( Var );
		if ( Var != NULL )
			Error( "%s followed by unexpected token: %s" , Token, Var );

		//TODO: add check if it's really a number
		sf_sort = (float)atof(Token);
	} else
	if ( stricmp( Type , "qer_trans" ) == 0 )
	{
		if ( Var == NULL )
			Error( "token expected after qer_trans, but none found");

		Token = getstring( Var );
		if ( Var != NULL )
			Error( "%s followed by unexpected token: %s" , Token, Var );

		//TODO: add check if it's really a number
		sf_qer_trans = (float)atof(Token);
	} else
	if ( stricmp( Type , "q3map_backsplash" ) == 0 )
	{
		if ( Var == NULL )
			Error( "token expected after q3map_backsplash, but none found");

		Token = getstring( Var );
		if ( Var != NULL )
			Error( "%s followed by unexpected token: %s" , Token, Var );

		//TODO: add check if it's really a number
		sf_q3map_backsplash = (float)atof(Token);
	} else
	if ( stricmp( Type , "q3map_surfacelight" ) == 0 )
	{
		if ( Var == NULL )
			Error( "token expected after q3map_surfacelight, but none found");

		Token = getstring( Var );
		if ( Var != NULL )
			Error( "%s followed by unexpected token: %s" , Token, Var );

		//TODO: add check if it's really a number
		sf_q3map_surfacelight = (float)atof(Token);
	} else
	if ( stricmp( Type , "sky" ) == 0 )
	{
		if ( Var == NULL )
			Error( "token expected after sky, but none found");

		Token = getstring( Var );
		if ( Var != NULL )
			Error( "%s followed by unexpected token: %s" , Token, Var );

		//TODO: add check if it's really a number
		sf_sky = new char[ strlen( Token ) ];
		strcpy( sf_sky , Token );
	} else
	if ( stricmp( Type , "qer_editorimage" ) == 0 )
	{
		if ( Var == NULL )
			Error( "token expected after qer_editorimage, but none found");

		Token = getstring( Var );
		if ( Var != NULL )
			Error( "%s followed by unexpected token: %s" , Token, Var );

		//TODO: add check if it's really a number
		sf_qer_editorimage = new char[ strlen( Token ) ];
		strcpy( sf_qer_editorimage , Token );
	} else
	if ( stricmp( Type , "q3map_lightimage" ) == 0 )
	{
		if ( Var == NULL )
			Error( "token expected after q3map_lightimage, but none found");

		Token = getstring( Var );
		if ( Var != NULL )
			Error( "%s followed by unexpected token: %s" , Token, Var );

		//TODO: add check if it's really a number
		sf_q3map_lightimage = new char[ strlen( Token ) ];
		strcpy( sf_q3map_lightimage , Token );
	} else
	if ( stricmp( Type , "cloudparms" ) == 0 )
	{
		//currently ignores cloudparms
		/*	
			{"cloudparms",int,"full"}
			{"cloudparms",int,"half"}
			{"cloudparms",int}
		*/
	} else
	if ( stricmp( Type , "skyparms" ) == 0 )
	{
		//currently ignores skyparms
		/*	
			{"skyparms",char*,"-","-"}
			{"skyparms",int,"full","-"}
			{"skyparms",int,"full","-"}
			{"skyparms","-",int,"-"}
			{"skyparms","full",int,"-"}
			{"skyparms","half",int,"-"}
		*/
	} else
	if ( stricmp( Type , "deformVertexes" ) == 0 )
	{
		//currently ignores deformVertexes
		/*	
			{"deformVertexes","wave",int,"sin",float,float,float,float}
			{"deformVertexes","autosprite"}
			{"deformVertexes","autosprite2"}
			{"deformVertexes","projectshadow"}
			{"deformVertexes","bulge",float,float,float}
			{"deformVertexes","bulge",float,float,float}
		*/
	} else
	if ( stricmp( Type , "fogparms" ) == 0 )
	{
		//currently ignores fogparms
		//{"fogparms",float,float,float,float,float}
	} else
	if ( stricmp( Type , "q3map_sun" ) == 0 )
	{
		//currently ignores q3map_sun
		//{"q3map_sun",float,float,float,int,int,int}
	} else
	if ( stricmp( Type , "fogGen" ) == 0 )
	{
		//currently ignores fogGen
		//{"fogGen","sin",float,float,float,float}
	} else
	if ( stricmp( Type , "blendMap" ) == 0 )
	{
		//currently ignores blendMap
		//{"blendMap",char*,char*}
	} else
	if ( Var == NULL )
	{
		if ( stricmp( Type , "portal"		 ) == 0 ) sf_portal			= true;else
		if ( stricmp( Type , "fogonly"		 ) == 0 ) sf_fogonly		= true;else
		if ( stricmp( Type , "nomipmaps"	 ) == 0 ) sf_nomipmaps		= true;else
		if ( stricmp( Type , "polygonOffset" ) == 0 ) sf_polygonOffset	= true;else
		if ( stricmp( Type , "lightning"	 ) == 0 ) sf_lightning		= true;else
		if ( stricmp( Type , "backsided"	 ) == 0 ) sf_backsided		= true;
		if ( stricmp( Type , "qer_nocarve"	 ) == 0 ) sf_qer_nocarve	= true;
		else Error( "unknown token: %s" , Type );

	} else
		Error( "unexpected token found: %s" , Type );
}

void	Surface::AddPass( RenderPass*& Pass )
{

}

/*END UNFINISHED BIT*/

char*	getline( FILE*& F )
{
	char	line[4096];
	char*	lineptr = NULL;
	
	while ( ( lineptr == NULL ) && ( !feof( F ) ) )
	{
		fgets( line , sizeof( line ) , F );
		lineptr = line;
		clean( lineptr );
	}
	return lineptr;
}

bool	LoadScript( char* filename )
{
	FILE*	F;
	if ( ( F = fopen( filename , "rt" ) ) != NULL ) 
	{
		char*	lineptr;
		while ( !feof( F ) )
		{
			lineptr = getline( F );
			if ( lineptr != NULL )
			{
				char*	Token = getstring( lineptr );
				if ( ( stricmp( Token , "{" ) == 0 ) ||
					 ( stricmp( Token , "}" ) == 0 ) )
				{
					Debug("surface expected, got: %s",Token);
					return false;
				} else
				{
					if ( lineptr != NULL ) {
						Debug("surface name followed by unexpected token: %s",lineptr);
						return false;
					}

					Surface*	newSurface;
					if ( ( newSurface = CreateSurface( Token ) ) != NULL )
					{
						lineptr = getline( F );
						if ( lineptr == NULL ) {
							Error("unexpected end of file"); return false;
						}
						Token = getstring( lineptr );
						if ( stricmp( Token , "{" ) != 0 ) {
							Error("unexpected token found: %s", Token); return false;
						}
						if ( lineptr != NULL ) {
							Error("{ followed by unexpected token: %s", lineptr); 
							return false;
						}
						while ( ( ( Token = lineptr = getline( F ) ) != NULL ) &&
							    ( ( Token = getstring( lineptr ) ) != NULL ) &&
							    ( stricmp( Token , "}" ) != 0 ) )
						{
							if ( stricmp( Token , "{" ) != 0 )
							{
								newSurface->AddParam( Token , lineptr );
							} else
							{
								if ( lineptr != NULL ) {
									Error("{ followed by unexpected token: %s", lineptr); 
									return false;
								}
								RenderPass*	newPass = new RenderPass;
								while ( ( ( Token = lineptr = getline( F ) ) != NULL ) &&
										( ( Token = getstring( lineptr ) ) != NULL ) &&
										( stricmp( Token , "}" ) != 0 ) )
								{
									newPass->AddParam( Token , lineptr );
								}
								if ( Token == NULL ) {
									Error("unexpected end of file"); return false;
								}
								if ( lineptr != NULL ) {
									Error("} followed by unexpected token: %s ", lineptr); 
									return false;
								}
								newSurface->AddPass( newPass );
							}
						}
						if ( Token == NULL ) {
							Error("unexpected end of file"); return false;
						}
						if ( lineptr != NULL ) {
							Error("} followed by unexpected token: %s ", lineptr); 
							return false;
						}
					} else
						Error("failed to make surface");
				}
			}
		}
		fclose(F);
		return true;
	} else
		return false;
}