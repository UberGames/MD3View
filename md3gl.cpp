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

#include "system.h"
#include "ndictionary.h"
#include "md3gl.h"
#include "md3view.h"
#include "targa.h"
#include <math.h>
#include "animation.h"
#include "oddbits.h"
#include "text.h"
#include "matcomp.h"


/*
sets up an orthogonal projection matrix
*/
void set_windowSize( int x, int y )
{
    glMatrixMode(GL_PROJECTION); 
    glLoadIdentity();
	if (y > 0) gluPerspective( mdview.dFOV, (double)x/(double)y, NEAR_GL_PLANE, FAR_GL_PLANE );
	glViewport( 0, 0, x, y );
    
    glMatrixMode(GL_MODELVIEW);
}


/* 
initializes the opengl state for rendering
*/
void initialize_glstate()
{		
	glClearColor((float)1/((float)256/(float)mdview._R), (float)1/((float)256/(float)mdview._G), (float)1/((float)256/(float)mdview._B), 0.0f);
    glClearDepth(1.0);                    
    glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );	
    glEnable( GL_DEPTH_TEST ); 	
    glDepthFunc( GL_LEQUAL );
//GEFORCE    glShadeModel( GL_SMOOTH );
//	oglStateFlatTextured();

//	glFrontFace( mdview.faceSide );
  
	float mat_specular[] = { 1.0, 1.0, 1.0, 1.0 };
	float mat_ambient[] = { 0.9f, 0.9f, 0.9f, 1.0 };
	float mat_diffuse[] = { 1, 1, 1, 1 };
	float mat_shininess[] = { 20.0 };

	float light_position[] = { 55.0, -50.0, -5.0, 0.0 };

	float light2_position[] = {-50.0, 45.0, 15.0, 0.0 };
	float mat2_diffuse[] = { 0.5f, 0.5f, 1, 1 };

//GEFORCE	glShadeModel (GL_SMOOTH);

	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
	glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);

	glLightfv(GL_LIGHT0, GL_AMBIENT, mat_ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, mat2_diffuse);
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);

	glLightfv(GL_LIGHT1, GL_POSITION, light2_position);   
	glLightfv(GL_LIGHT1, GL_DIFFUSE, mat2_diffuse);
	glLightfv(GL_LIGHT1, GL_AMBIENT, mat_ambient);

	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_LIGHT1);
	glEnable( GL_LIGHTING );

	glEnable( GL_BLEND );

	glBlendFunc(GL_ONE, GL_ZERO);//bug fix

//GEFORCE	glEnable( GL_POLYGON_SMOOTH );

	oglStateFlatTextured(); 
}


bool bWireFrame = false;
bool bTexturesOn = true;	// needed for Joe's request, now that we can have both wire & textures at once

bool bIsWireFrame(void)
{
	return bWireFrame;
}

bool bIsTextured(void)
{
	return bTexturesOn;
}


void oglStateFlatTextured()
{
	glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	glEnable( GL_TEXTURE_2D );
	glDisable( GL_LIGHTING );
	bWireFrame = false;
	bTexturesOn = true;
}

void oglStateShadedTexturedAndWireFrame()
{
	oglStateFlatTextured();
	bWireFrame = true;
}

void oglStateShadedTextured()
{
	glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	glEnable( GL_TEXTURE_2D );
	glEnable( GL_LIGHTING );
	bWireFrame = false;
	bTexturesOn = true;
}

void oglStateShadedFlat()
{
	glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	glDisable( GL_TEXTURE_2D );
	glEnable( GL_LIGHTING );
	bWireFrame = false;
	bTexturesOn = true;
}

void oglStateWireframe()
{
	glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );	
	glDisable( GL_TEXTURE_2D );
	glDisable( GL_LIGHTING );
	bWireFrame = true;
	bTexturesOn = false;
}

/*
computes vector cross product
*/
void cross(Vec3 v1, Vec3 v2, Vec3 c)
{
	c[0] = v1[1]*v2[2] - v1[2]*v2[1];
	c[1] = v1[2]*v2[0] - v1[0]*v2[2];
	c[2] = v1[0]*v2[1] - v1[1]*v2[0];
}

/* 
makes normal unit length
*/
void normalize( Vec3 n )
{
	int i;
	float length = 0.0f;
	for (i=0 ; i< 3 ; i++) length += n[i]*n[i];
	length = (float)sqrt(length);
	if (length == 0) return;	
	for (i=0 ; i< 3 ; i++) n[i] /= length;	
}


/* --------------------------------------- new gl code ------------------------------------------------- */
// Copyright 1999-200 Raven Software
//
shaderCommands_t tess;


void RB_SurfaceAnim( md4Surface_t *surface, gl_model *pModel )	// pModel just for anim frame numbers
{
	int				i, j, k;
	float			frontlerp, backlerp;
	int				*triangles;
	int				indexes;
	int				baseIndex, baseVertex;
	int				numVerts;
	md4Vertex_t		*v;
	md4Bone_t		bones[MD4_MAX_BONES];
	md4Bone_t		*bonePtr, *bone;
	md4Header_t		*header;
	md4Frame_t		*frame;
	md4Frame_t		*oldFrame;
	md4CompFrame_t	*cframe;
	md4CompFrame_t	*coldFrame;

	int				frameSize;

//=====================
	tess.numIndexes = 0;
	tess.numVertexes = 0;	
/////////////////	tess.shader = shader;
/////////////////	tess.fogNum = fogNum;
/////////////////	tess.dlightBits = 0;		// will be OR'd in by surface functions
/////////////////	tess.xstages = shader->stages;
/////////////////	tess.numPasses = shader->numUnfoggedPasses;
/////////////////	tess.currentStageIteratorFunc = shader->optimalStageIteratorFunc;
//=====================

	int iFrame		= pModel->currentFrame;
	int iNextFrame	= GetNextFrame_MultiLocked(pModel, iFrame, 1);
		iNextFrame  = Model_EnsureCurrentFrameLegal(pModel,iNextFrame);

	// don't lerp if lerping off, or this is the only frame, or the last frame...
	//
	if (  !mdview.interpolate || pModel->iNumFrames == 0 || iNextFrame == pModel->iNumFrames
//		backEnd.currentEntity->e.oldframe == backEnd.currentEntity->e.frame 
		) 
	{
		backlerp	= 0;	// if backlerp is 0, lerping is off and frontlerp is never used
		frontlerp	= 1;
	} 
	else  
	{
		backlerp	= mdview.frameFrac;
		frontlerp	= 1.0f - backlerp;
	}
	header = (md4Header_t *)((byte *)surface + surface->ofsHeader);

	bool bCompressed = false;
	if (header->ofsFrames<0)
	{
		// compressed model...
		//
		bCompressed = true;

		frameSize	= (int)( &((md4CompFrame_t *)0)->bones[ header->numBones ] );
		cframe		= (md4CompFrame_t *)((byte *)header + (-header->ofsFrames) + iFrame		* frameSize );
		coldFrame	= (md4CompFrame_t *)((byte *)header + (-header->ofsFrames) + iNextFrame	* frameSize );
	}
	else
	{
		frameSize	= (int)( &((md4Frame_t *)0)->bones[ header->numBones ] );
		frame		= (md4Frame_t *)((byte *)header + header->ofsFrames + iFrame	 * frameSize );
		oldFrame	= (md4Frame_t *)((byte *)header + header->ofsFrames + iNextFrame * frameSize );
	}

//	RB_CheckOverflow( surface->numVerts, surface->numTriangles );

	triangles	= (int *) ((byte *)surface + surface->ofsTriangles);
	indexes		= surface->numTriangles * 3;
	baseIndex	= tess.numIndexes;
	baseVertex	= tess.numVertexes;
	for (j = 0 ; j < indexes ; j++) 
	{
		tess.indexes[baseIndex + j] = baseVertex + triangles[j];
	}
	tess.numIndexes += indexes;

	//
	// lerp all the needed bones
	//
	if ( !backlerp && !bCompressed) 
	{
		// no lerping needed
		bonePtr = frame->bones;
	} 
	else 
	{
		bonePtr = bones;
		if (bCompressed)
		{
			for ( i = 0 ; i < header->numBones ; i++ )
			{
				if ( !backlerp )
					MC_UnCompress(bonePtr[i].matrix,cframe->bones[i].Comp);
				else
				{
					md4Bone_t tbone[2];

					MC_UnCompress(tbone[0].matrix,cframe->bones[i].Comp);
					MC_UnCompress(tbone[1].matrix,coldFrame->bones[i].Comp);
					for ( j = 0 ; j < 12 ; j++ )
						((float *)&bonePtr[i])[j] = frontlerp * ((float *)&tbone[0])[j] + backlerp * ((float *)&tbone[1])[j];
				}
			}
		}
		else
		{
			for ( i = 0 ; i < header->numBones*12 ; i++ ) 
			{
				((float *)bonePtr)[i] = frontlerp * ((float *)frame->bones)[i] + backlerp * ((float *)oldFrame->bones)[i];
			}
		}
	}

	//
	// deform the vertexes by the lerped bones
	//
	numVerts = surface->numVerts;
	v = (md4Vertex_t *) ((byte *)surface + surface->ofsVerts);
	for ( j = 0; j < numVerts; j++ ) 
	{
		vec3_t	tempVert, tempNormal;
		md4Weight_t	*w;

		VectorClear( tempVert );
		VectorClear( tempNormal );
		w = v->weights;
		for ( k = 0 ; k < v->numWeights ; k++, w++ ) 
		{
			bone = bonePtr + w->boneIndex;
			
			tempVert[0] += w->boneWeight * ( DotProduct( bone->matrix[0], w->offset ) + bone->matrix[0][3] );
			tempVert[1] += w->boneWeight * ( DotProduct( bone->matrix[1], w->offset ) + bone->matrix[1][3] );
			tempVert[2] += w->boneWeight * ( DotProduct( bone->matrix[2], w->offset ) + bone->matrix[2][3] );
			
			tempNormal[0] += w->boneWeight * DotProduct( bone->matrix[0], v->normal );
			tempNormal[1] += w->boneWeight * DotProduct( bone->matrix[1], v->normal );
			tempNormal[2] += w->boneWeight * DotProduct( bone->matrix[2], v->normal );
		}

		tess.xyz[baseVertex + j][0] = tempVert[0];
		tess.xyz[baseVertex + j][1] = tempVert[1];
		tess.xyz[baseVertex + j][2] = tempVert[2];

		tess.normal[baseVertex + j][0] = tempNormal[0];
		tess.normal[baseVertex + j][1] = tempNormal[1];
		tess.normal[baseVertex + j][2] = tempNormal[2];

		tess.texCoords[baseVertex + j][0][0] = v->texCoords[0];
		tess.texCoords[baseVertex + j][0][1] = v->texCoords[1];

		v = (md4Vertex_t *)&v->weights[v->numWeights];
	}


//=============================
	// now draw it...
	//
	{
		glPushAttrib(GL_POLYGON_BIT);	// preserves GL_CULL_FACE && GL_CULL_FACE_MODE

		if (bIsWireFrame())
		{
			glEnable(GL_CULL_FACE);
		}

		for (int iPass=0; iPass<(bIsWireFrame()?2:1); iPass++)
		{
			if (bIsWireFrame())
			{
				if (!iPass)
				{
					glCullFace(GL_BACK);
					glColor3f(0.5,0.5,0.5);				
				}
				else
				{
					glCullFace(GL_FRONT);
					glColor3f( 1,1,1);				
				}
			}			
			
			glBegin( GL_TRIANGLES );
			{				
				for (int i=0; i<surface->numTriangles*3; i+=3)
				{
					glNormal3fv	 ( tess.normal	 [tess.indexes[i+0]] );

//					OutputDebugString(va("x,y = %f,%f\n",tess.texCoords[tess.indexes[i+0]][0][0],tess.texCoords[tess.indexes[i+0]][0][1]));					
					glTexCoord2fv( tess.texCoords[tess.indexes[i+0]][0] );	
					glVertex3fv	 ( tess.xyz		 [tess.indexes[i+0]] );

//					OutputDebugString(va("x,y = %f,%f\n",tess.texCoords[tess.indexes[i+1]][0][0],tess.texCoords[tess.indexes[i+1]][0][1]));
					glTexCoord2fv( tess.texCoords[tess.indexes[i+1]][0] );	
					glVertex3fv	 ( tess.xyz		 [tess.indexes[i+1]] );

//					OutputDebugString(va("x,y = %f,%f\n",tess.texCoords[tess.indexes[i+2]][0][0],tess.texCoords[tess.indexes[i+2]][0][1]));
					glTexCoord2fv( tess.texCoords[tess.indexes[i+2]][0] );	
					glVertex3fv	 ( tess.xyz		 [tess.indexes[i+2]] );					
				}				
			}
			glEnd();
		}

		glPopAttrib();
		glColor3f( 1,1,1);		
	}
//=============================

	tess.numVertexes += surface->numVerts;

	pModel->iRenderedTris += surface->numTriangles;
	pModel->iRenderedVerts+= surface->numVerts;
}

#if 1
shaderCommands_t* Freeze_Surface( md4Surface_t *surface, int iFrame )	// pModel just for anim frame numbers
{
	int				i, j, k;
	int				*triangles;
	int				indexes;
	int				baseIndex, baseVertex;
	int				numVerts;
	md4Vertex_t		*v;
	md4Bone_t		bones[MD4_MAX_BONES];
	md4Bone_t		*bonePtr, *bone;
	md4Header_t		*header;
	md4Frame_t		*frame;
	md4CompFrame_t	*cframe;

	int				frameSize;

//=====================
	tess.numIndexes = 0;
	tess.numVertexes = 0;	
//=====================

//	int iFrame		= pModel->currentFrame;

	header = (md4Header_t *)((byte *)surface + surface->ofsHeader);

	bool bCompressed = false;
	if (header->ofsFrames<0)
	{
		// compressed model...
		//
		bCompressed = true;

		frameSize	= (int)( &((md4CompFrame_t *)0)->bones[ header->numBones ] );
		cframe		= (md4CompFrame_t *)((byte *)header + (-header->ofsFrames) + iFrame * frameSize );		
	}
	else
	{
		frameSize	= (int)( &((md4Frame_t *)0)->bones[ header->numBones ] );
		frame		= (md4Frame_t *)((byte *)header + header->ofsFrames + iFrame	 * frameSize );		
	}

	triangles	= (int *) ((byte *)surface + surface->ofsTriangles);
	indexes		= surface->numTriangles * 3;
	baseIndex	= tess.numIndexes;
	baseVertex	= tess.numVertexes;
	for (j = 0 ; j < indexes ; j++) 
	{
		tess.indexes[baseIndex + j] = baseVertex + triangles[j];
	}
	tess.numIndexes += indexes;

	//
	// lerp all the needed bones
	//
	if ( !bCompressed) 
	{
		bonePtr = frame->bones;
	} 
	else 
	{
		bonePtr = bones;
		for ( i = 0 ; i < header->numBones ; i++ )
		{
			MC_UnCompress(bonePtr[i].matrix,cframe->bones[i].Comp);
		}
	}

	//
	// deform the vertexes by the lerped bones
	//
	numVerts = surface->numVerts;
	v = (md4Vertex_t *) ((byte *)surface + surface->ofsVerts);
	for ( j = 0; j < numVerts; j++ ) 
	{
		vec3_t	tempVert, tempNormal;
		md4Weight_t	*w;

		VectorClear( tempVert );
		VectorClear( tempNormal );
		w = v->weights;
		for ( k = 0 ; k < v->numWeights ; k++, w++ ) 
		{
			bone = bonePtr + w->boneIndex;
			
			tempVert[0] += w->boneWeight * ( DotProduct( bone->matrix[0], w->offset ) + bone->matrix[0][3] );
			tempVert[1] += w->boneWeight * ( DotProduct( bone->matrix[1], w->offset ) + bone->matrix[1][3] );
			tempVert[2] += w->boneWeight * ( DotProduct( bone->matrix[2], w->offset ) + bone->matrix[2][3] );
			
			tempNormal[0] += w->boneWeight * DotProduct( bone->matrix[0], v->normal );
			tempNormal[1] += w->boneWeight * DotProduct( bone->matrix[1], v->normal );
			tempNormal[2] += w->boneWeight * DotProduct( bone->matrix[2], v->normal );
		}

		tess.xyz[baseVertex + j][0] = tempVert[0];
		tess.xyz[baseVertex + j][1] = tempVert[1];
		tess.xyz[baseVertex + j][2] = tempVert[2];

		tess.normal[baseVertex + j][0] = tempNormal[0];
		tess.normal[baseVertex + j][1] = tempNormal[1];
		tess.normal[baseVertex + j][2] = tempNormal[2];

		tess.texCoords[baseVertex + j][0][0] = v->texCoords[0];
		tess.texCoords[baseVertex + j][0][1] = v->texCoords[1];

		v = (md4Vertex_t *)&v->weights[v->numWeights];
	}


//=============================
/*
	// now draw it...
	//
	{
		glBegin( GL_TRIANGLES );
		{				
			for (int i=0; i<surface->numTriangles*3; i+=3)
			{
				glNormal3fv	 ( tess.normal	 [tess.indexes[i+0]] );

//				OutputDebugString(va("x,y = %f,%f\n",tess.texCoords[tess.indexes[i+0]][0][0],tess.texCoords[tess.indexes[i+0]][0][1]));
				glTexCoord2fv( tess.texCoords[tess.indexes[i+0]][0] );
				glVertex3fv	 ( tess.xyz		 [tess.indexes[i+0]] );

//				OutputDebugString(va("x,y = %f,%f\n",tess.texCoords[tess.indexes[i+1]][0][0],tess.texCoords[tess.indexes[i+1]][0][1]));
				glTexCoord2fv( tess.texCoords[tess.indexes[i+1]][0] );
				glVertex3fv	 ( tess.xyz		 [tess.indexes[i+1]] );

//				OutputDebugString(va("x,y = %f,%f\n",tess.texCoords[tess.indexes[i+2]][0][0],tess.texCoords[tess.indexes[i+2]][0][1]));
				glTexCoord2fv( tess.texCoords[tess.indexes[i+2]][0] );
				glVertex3fv	 ( tess.xyz		 [tess.indexes[i+2]] );
			}				
		}
		glEnd();
	}
*/
//=============================

	tess.numVertexes += surface->numVerts;

	return &tess;
}
#endif


LPCSTR vtos(vec3_t v3)
{
	return va("%.3f %.3f %.3f",v3[0],v3[1],v3[2]);
}

LPCSTR v2tos(vec2_t v2)
{
	return va("%.3f %.3f",v2[0],v2[1]);
}


// this is the only actual (MD3) model draw code, the one that everything else calls...
//
void draw_gl_mesh( gl_mesh *mesh, Vec3 *vecs, gl_model* pModel )
{
	TriVec	*tris;	
	int tri_num, t, v;
	Vec3 v1, v2, normal;
	TexVec       *tvecs;

		tris	= mesh->triangles;
		tvecs   = mesh->textureCoord;		
		tri_num = mesh->iNumTriangles;				

		pModel->iRenderedTris += mesh->iNumTriangles;
		pModel->iRenderedVerts+= mesh->iNumVertices;

		glPushAttrib(GL_POLYGON_BIT);	// preserves GL_CULL_FACE && GL_CULL_FACE_MODE

		if (bIsWireFrame() && !bIsTextured())
		{
			glEnable(GL_CULL_FACE);
		}

		for (int iPass=0; iPass<((bIsWireFrame() && !bIsTextured())?2:1); iPass++)
		{
			if (bIsWireFrame() && !bIsTextured())
			{
				// standard wireframe...
				//
				if (!iPass)
				{
					glCullFace(GL_BACK);
					glColor3f(0.5,0.5,0.5);
				}
				else
				{
					glCullFace(GL_FRONT);
					glColor3f( 1,1,1);				
				}
			}

			glBegin( GL_TRIANGLES );			
			for (t=0 ; t<tri_num ; t++) 
			{						
				for (v=0 ; v<3 ; v++) {
					v1[v] = vecs[tris[t][1]][v] - vecs[tris[t][0]][v];
					v2[v] = vecs[tris[t][2]][v] - vecs[tris[t][0]][v];
				}
				cross( v1, v2, normal );
				normalize( normal );

				glNormal3fv( normal );
				glTexCoord2fv( tvecs[tris[t][0]] ); glVertex3fv( vecs[tris[t][0]] );
				glTexCoord2fv( tvecs[tris[t][1]] ); glVertex3fv( vecs[tris[t][1]] );
				glTexCoord2fv( tvecs[tris[t][2]] ); glVertex3fv( vecs[tris[t][2]] );
			}
			glEnd();
		}

		if (bIsWireFrame() && bIsTextured())
		{
			glDisable(GL_TEXTURE_2D);
			glDisable(GL_BLEND);
			glDisable(GL_LIGHTING);					

			glLineWidth(2);
			glColor3f(1,1,0);	// yellow

			for (t=0 ; t<tri_num ; t++)
			{	
				glBegin( GL_LINE_LOOP );
				glVertex3fv( vecs[tris[t][0]] );
				glVertex3fv( vecs[tris[t][1]] );
				glVertex3fv( vecs[tris[t][2]] );
				glEnd();				
			}			
			glLineWidth(1);

			glEnable(GL_TEXTURE_2D);
			glEnable(GL_BLEND);
		}

		if ( mdview.bBBox )
		{
			glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
			{
				glDisable(GL_TEXTURE_2D);
				glDisable(GL_LIGHTING);
				glDisable(GL_BLEND);


				// uber-hackery, I don't care about this program any more, so...
				//
				#define VectorSet(v, x, y, z)	((v)[0]=(x), (v)[1]=(y), (v)[2]=(z))
	/*
				glBegin( GL_LINE_LOOP );

				glVertex3fv( vecs[tris[t][0]] );

	typedef struct 
	{
		vec3_t		bounds[2];	
		vec3_t		localOrigin;
		float		radius;
		char		name[16];	// creator
	} md3BoundFrame_t;

				md3BoundFrame_t *pMD3BoundFrames;			 // 2d array of boundary frames size [iNumFrames][iNumTags]
	*/
				int iFrame = 0;

				vec3_t v3Corners[8];

				vec3_t &v3Mins = pModel->pMD3BoundFrames[ iFrame ].bounds[0];
				vec3_t &v3Maxs = pModel->pMD3BoundFrames[ iFrame ].bounds[1];

				
				VectorSet(v3Corners[0],v3Mins[0],v3Mins[1],v3Mins[2]);
				VectorSet(v3Corners[1],v3Maxs[0],v3Mins[1],v3Mins[2]);
				VectorSet(v3Corners[2],v3Maxs[0],v3Mins[1],v3Maxs[2]);
				VectorSet(v3Corners[3],v3Mins[0],v3Mins[1],v3Maxs[2]);
				VectorSet(v3Corners[4],v3Mins[0],v3Maxs[1],v3Maxs[2]);
				VectorSet(v3Corners[5],v3Maxs[0],v3Maxs[1],v3Maxs[2]);
				VectorSet(v3Corners[6],v3Maxs[0],v3Maxs[1],v3Mins[2]);
				VectorSet(v3Corners[7],v3Mins[0],v3Maxs[1],v3Mins[2]);

				glColor3f(0.8f,0.8f,0.8f);//1,1,1);
		/*				glBegin(GL_LINE_LOOP);
				{
					glVertex3fv(v3Corners[0]);
					glVertex3fv(v3Corners[1]);
					glVertex3fv(v3Corners[2]);
					glVertex3fv(v3Corners[3]);

					glVertex3fv(v3Corners[4]);
					glVertex3fv(v3Corners[5]);
					glVertex3fv(v3Corners[6]);
					glVertex3fv(v3Corners[7]);
				}
				glEnd();
		*/
				// new version, draw the above shape, but without 3 of the lines (and therefore without GL_LINE_LOOP)
				//	because otherwise the white lines fight with the coloured ones below...  (aesthetics)
				//
				glBegin(GL_LINES);
				{
					glVertex3fv(v3Corners[2]);
					glVertex3fv(v3Corners[3]);

					glVertex3fv(v3Corners[3]);
					glVertex3fv(v3Corners[4]);

					glVertex3fv(v3Corners[4]);
					glVertex3fv(v3Corners[5]);

					glVertex3fv(v3Corners[5]);
					glVertex3fv(v3Corners[6]);

					glVertex3fv(v3Corners[6]);
					glVertex3fv(v3Corners[7]);
				}
				glEnd();

				// draw the thicker lines for the 3 dimension axis...
				//
				glLineWidth(3);
				{
					// X...
					//
					glColor3f(1,0,0);
					glBegin(GL_LINES);
					{
						glVertex3fv(v3Corners[0]);
						glVertex3fv(v3Corners[1]);
					}
					glEnd();

					// Y...
					//
	//				glColor3f(0,1,0);
					glColor3f(0,0,1);	// err.... dunno, but this comes out the right colour now. Quake axis tilt?
					glBegin(GL_LINES);
					{
						glVertex3fv(v3Corners[1]);
						glVertex3fv(v3Corners[2]);
					}
					glEnd();

					// Z...
					//
	//				glColor3f(0,0,1);
					glColor3f(0,1,0);	// err.... dunno, but this comes out the right colour now. Quake axis tilt?
					glBegin(GL_LINES);
					{
						glVertex3fv(v3Corners[7]);
						glVertex3fv(v3Corners[0]);
					}
					glEnd();
				}
				glLineWidth(1);

				// now draw the coords...
				//
				for (int i=0; i<sizeof(v3Corners)/sizeof(v3Corners[0]); i++)
				{
					LPCSTR psCoordString = vtos(v3Corners[i]);
						
					Text_Display(psCoordString,v3Corners[i],200,70,0);//228/2,107/2,35/2);
				}

				// now draw the 3 dimensions (sizes)...
				//
				float fXDim = v3Maxs[0] - v3Mins[0];
				float fYDim = v3Maxs[1] - v3Mins[1];
				float fZDim = v3Maxs[2] - v3Mins[2];

				vec3_t v3Pos;		

				// X...
				//
				VectorAdd	(v3Corners[0],v3Corners[1],v3Pos);
				VectorScale	(v3Pos,0.5,v3Pos);
				Text_Display(va(" X = %.3f",fXDim),v3Pos,255,128,128);	// ruler-pink
				//
				// Y...
				//
				VectorAdd	(v3Corners[7],v3Corners[0],v3Pos);
				VectorScale	(v3Pos,0.5,v3Pos);
				Text_Display(va(" Y = %.3f",fYDim),v3Pos,255,128,128);
				//
				// Z...
				//
				VectorAdd	(v3Corners[1],v3Corners[2],v3Pos);
				VectorScale	(v3Pos,0.5,v3Pos);
				Text_Display(va(" Z = %.3f",fZDim),v3Pos,255,128,128);
			}
			glPopAttrib();
		}

		glPopAttrib();
		glColor3f( 1,1,1);		
}



void draw_interpolated_gl_mesh( gl_mesh *mesh, int i, gl_model* model)
{
	unsigned int sf=i, ef/*=i+1*/, v, vec_num, t;
	Vec3 *vecs1, *vecs2;
	Vec3 *vecsI;
	float frac=mdview.frameFrac, frac1=1.f-frac;

	ef = GetNextFrame_MultiLocked(model, sf, 1);
	ef = Model_EnsureCurrentFrameLegal(model,ef);

	// do not interpolate if this is the only frame, or the last frame
	if ((mesh->iNumMeshFrames < 2) || (ef == mesh->iNumMeshFrames)) 
	{
		draw_gl_mesh( mesh, mesh->meshFrames[i].pVerts, model );
		return;
	}
		
	// calculate an array of interpolated floating point vertices
	vecs1 = mesh->meshFrames[sf].pVerts;
	vecs2 = mesh->meshFrames[ef].pVerts;
	vecsI = mesh->iterMesh;
	vec_num = mesh->iNumVertices;
	
	for (t=0 ; t<vec_num ; t++) {					
		for (v=0 ; v<3 ; v++) {
			vecsI[t][v] = vecs1[t][v]*frac1 + vecs2[t][v]*frac;		
		}
	}

	draw_gl_mesh( mesh, vecsI, model );
}


/*
renders the current frame 
*/


void widget_Axis();

void draw_gl_model( gl_model *model )
{
	model->iRenderedTris = 0;
	model->iRenderedVerts= 0;

	if (model->modelType == MODTYPE_MD3)
	{			
		gl_model* pModel = NULL;

		switch (mdview.iLODLevel)
		{
			case 0:	pModel = model;				break;
			case 1: pModel = model->pModel_LOD1;break;
			case 2: pModel = model->pModel_LOD2;break;
		}

		if (pModel)
		{		
			pModel->iRenderedTris = 0;
			pModel->iRenderedVerts= 0;

			pModel->currentFrame = model->currentFrame;	// since MD3 LODs are seperate models, copy what's needed

			int mesh_num = pModel->iNumMeshes;

			for (int i=0; i<mesh_num; i++)
			{
				gl_mesh *pMesh =&pModel->pMeshes[i];
				int skin_num  = pMesh->iNumSkins;
				int frame_num = pMesh->iNumMeshFrames;
				int cur_skin  = pMesh->bindings[0];

				glColor3f( 1, 1, 1 );
				glBindTexture( GL_TEXTURE_2D, cur_skin );
				GLenum error = glGetError();						

				if (mdview.interpolate) 
				{
					draw_interpolated_gl_mesh( pMesh, pModel->currentFrame,pModel);
				}
				else 
				{
					draw_gl_mesh( pMesh, pMesh->meshFrames[pModel->currentFrame].pVerts, pModel );
				}						
			}
		}
	}
	else
	{
		if (model->modelType == MODTYPE_MDR)
		{
			md4LOD_t *pLOD = (md4LOD_t *) ((byte *)model->Q3ModelStuff.md4 + model->Q3ModelStuff.md4->ofsLODs);
			int iWhichLod = mdview.iLODLevel;

			if (iWhichLod >= model->Q3ModelStuff.md4->numLODs)
				iWhichLod  = model->Q3ModelStuff.md4->numLODs-1;				
			
			for ( int i=0; i<iWhichLod; i++)
			{
				pLOD = (md4LOD_t*)( (byte *)pLOD + pLOD->ofsEnd );
			}

			md4Surface_t * pSurface = (md4Surface_t *)( (byte *)pLOD + pLOD->ofsSurfaces );
			for ( int i = 0 ; i < pLOD->numSurfaces ; i++ )
			{
/*				if ( ent->e.customShader ) 
				{
					shader = cust_shader;
				} 
				else if ( ent->e.customSkin > 0 && ent->e.customSkin < tr.numSkins ) 
				{
					skin_t *skin;
					int		j;
					
					skin = R_GetSkinByHandle( ent->e.customSkin );
					
					// match the surface name to something in the skin file
					shader = tr.defaultShader;
					for ( j = 0 ; j < skin->numSurfaces ; j++ ) {
						// the names have both been lowercased
						if ( !strcmp( skin->surfaces[j]->name, surface->name ) ) {
							shader = skin->surfaces[j]->shader;
							break;
						}
					}
				} 
				else 
				{
					shader = R_GetShaderByHandle( surface->shaderIndex );
				}
*/

				glColor3f( 1, 1, 1 );
				glBindTexture( GL_TEXTURE_2D, pSurface->shaderIndex);
//				OutputDebugString(va("Binding surface %d\n",pSurface->shaderIndex));
				GLenum error = glGetError();
			
				RB_SurfaceAnim( pSurface, model );						

				pSurface = (md4Surface_t *)( (byte *)pSurface + pSurface->ofsEnd );
			}
		}
	}	
}


// this is currently not being called -slc
void draw_view()
{
	NodePosition pos;
	gl_model *model;

	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	glLoadIdentity	(); 
	glTranslatef	(	   mdview.xPos, mdview.yPos , mdview.zPos );
	glScalef	    (     (float)0.05 , (float)0.05, (float)0.05 );
	glRotatef		( mdview.rotAngleX, 1.0 ,  0.0 , 0.0 );
 	glRotatef		( mdview.rotAngleY, 0.0 ,  1.0 , 0.0 );	
	glRotatef		(    -90.f , 1.0 ,  0.0 , 0.0 );

	for (pos=mdview.modelList->first() ; pos!=NULL ; pos=mdview.modelList->after(pos)) {
		model = (gl_model *)pos->element();
		draw_gl_model( model );
	}
}


/* ------------------------------ new skeletal drawing code ---------------------------------------------- */


/*
creates a matrix froma  quaternion, accepts a ptr to 16 float 4x4 matrix array and a ptr to a 4 float quat array
*/

#define MAT( p, row, col ) (p)[((col)*3)+(row)]
#define MATGL( p, row, col ) (p)[((row)*3)+(col)]
#define X 0
#define Y 1
#define Z 2
#define W 3
#define DELTA 0.1
void matrix_from_quat( float *m, float *quat )
{
                   float wx, wy, wz, xx, yy, yz, xy, xz, zz, x2, y2, z2;
                   // calculate coefficients
                   x2 = quat[X] + quat[X]; 
				   y2 = quat[Y] + quat[Y]; 
                   z2 = quat[Z] + quat[Z];
                   xx = quat[X] * x2;   xy = quat[X] * y2;   xz = quat[X] * z2;
                   yy = quat[Y] * y2;   yz = quat[Y] * z2;   zz = quat[Z] * z2;
                   wx = quat[W] * x2;   wy = quat[W] * y2;   wz = quat[W] * z2;

                   MAT(m,0,0) = 1.f - (yy + zz);    MAT(m,0,1) = xy - wz;
                   MAT(m,0,2) = xz + wy;            //MAT(m,0,3) = 0.0;
                  
                   MAT(m,1,0) = xy + wz;            MAT(m,1,1) = 1.f - (xx + zz);
                   MAT(m,1,2) = yz - wx;            //MAT(m,1,3) = 0.0;

                   MAT(m,2,0) = xz - wy;            MAT(m,2,1) = yz + wx;
                   MAT(m,2,2) = 1.f - (xx + yy);    //MAT(m,2,3) = 0.f;

				   
                   //MAT(m,3,0) = 0;                  MAT(m,3,1) = 0;
                   //MAT(m,3,2) = 0;                  MAT(m,3,3) = 1.f;         
}

/*
creates a quaternion from a matrix, parameters like above but reversed
*/

void quat_from_matrix( float *quat, float *m )
{
  float  tr, s, q[4];
  int    i, j, k;
  int    nxt[3] = {1, 2, 0};  

  tr = MAT(m, 0, 0) + MAT(m, 1, 1) + MAT(m, 2, 2);

  // check the diagonal
  if (tr > 0.0) {
    s = (float)sqrt (tr + 1.f);
    quat[W] = s / 2.0f;
    s = 0.5f / s;
    quat[X] = (MAT(m,1,2) - MAT(m,2,1)) * s;
    quat[Y] = (MAT(m,2,0) - MAT(m,0,2)) * s;
    quat[Z] = (MAT(m,0,1) - MAT(m,1,0)) * s;
  } else {                
    // diagonal is negative
    i = 0;
    if (MAT(m,1,1) > MAT(m,0,0)) i = 1;
    if (MAT(m,2,2) > MAT(m,i,i)) i = 2;
    j = nxt[i];
    k = nxt[j];

    s = (float)sqrt ((MAT(m,i,i) - (MAT(m,j,j) + MAT(m,k,k))) + 1.0);
                       
    q[i] = s * (float)0.5;
                             
    if (s != 0.0f) s = 0.5f / s;

    q[3] = (MAT(m,j,k) - MAT(m,k,j)) * s;
    q[j] = (MAT(m,i,j) + MAT(m,j,i)) * s;
    q[k] = (MAT(m,i,k) + MAT(m,k,i)) * s;

    quat[X] = q[0];
    quat[Y] = q[1];
    quat[Z] = q[2];
    quat[W] = q[3];
  }
}


/*
interpolate quaternions along unit 4d sphere
*/

void quat_slerp(float *from, float *to, float t, float *res)
{
  float           to1[4];
  float        omega, cosom, sinom, scale0, scale1;

  // calc cosine
  cosom = from[X]*to[X] + 
		  from[Y]*to[Y] + 
		  from[Z]*to[Z] +
          from[W]*to[W];

  // adjust signs (if necessary)
  if ( cosom <0.0 ) { 
		  cosom = -cosom; 
		  to1[0] = - to[X];
          to1[1] = - to[Y];
          to1[2] = - to[Z];
          to1[3] = - to[W];
  } else  {
          to1[0] = to[X];
          to1[1] = to[Y];
          to1[2] = to[Z];
          to1[3] = to[W];
  }

 // calculate coefficients
 if ( (1.0 - cosom) > DELTA ) {
          // standard case (slerp)
          omega = (float)acos(cosom);
          sinom = (float)sin(omega);
          scale0 = (float)sin((1.0 - t) * omega) / sinom;
          scale1 = (float)sin(t * omega) / sinom;

  } else {        
      // "from" and "to" quaternions are very close 
      //  ... so we can do a linear interpolation
          scale0 = 1.0f - t;
          scale1 = t;
  }
  // calculate final values
  res[X] = scale0 * from[X] + scale1 * to1[0];
  res[Y] = scale0 * from[Y] + scale1 * to1[1];
  res[Z] = scale0 * from[Z] + scale1 * to1[2];
  res[W] = scale0 * from[W] + scale1 * to1[3];
}	


/* 
draws a node of the skeleton, setting up the transformation for its child nodes
*/
void draw_skeleton( gl_model *model )
{
	// draw this model
	draw_gl_model( model );

	// draw its children	
	Tag *tag, *tag2;
	float m[16], quat1[4], quat2[4], resQuat[4], fm[9], *matrix;
	//float m1[9], m2[9];
	gl_model *child;
	float *position; //, *matrix;
	Vec3 interPos;
	unsigned int j,v;
	float frac=mdview.frameFrac, frac1=1.f-frac;
	unsigned int sf=model->currentFrame, ef/*=model->currentFrame+1*/;
	ef = GetNextFrame_MultiLocked(model, sf, 1);
	ef = Model_EnsureCurrentFrameLegal(model,ef);

	bool doInterpolate = false;

	// check if we can do interpolation
	if ((model->iNumFrames > 1) && (ef != model->iNumFrames) && (mdview.interpolate)) {
		doInterpolate = true;
	}


	// draw all the attached child models
	for (j=0; j<model->iNumTags ; j++) 
	{			
		child = model->linkedModels[j];
//		if (!child) 
//			continue;	// do this later, I may want to draw tags anyway even if no model attached there

		tag = &model->tags[sf][j];		
		
		// if interpolating then calculate in between values...
		//
		if (doInterpolate) 
		{
			tag2 = &model->tags[ef][j];
			// interpolate position
			for (v=0 ; v<3 ; v++) interPos[v] = tag->Position[v]*frac1 + tag2->Position[v]*frac;
			position = interPos;

			// interpolate rotation matrix...
			//
			quat_from_matrix( quat1, &tag->Matrix[0][0] );
			quat_from_matrix( quat2, &tag2->Matrix[0][0] );
			quat_slerp( quat1, quat2, frac, resQuat );
			matrix_from_quat( fm, resQuat );
			matrix = fm;
			
			// quaternion code is column based, so use transposed matrix when spitting out to gl...
			//
			m[0] = MAT(matrix,0,0); m[4] = MAT(matrix,0,1); m[8] = MAT(matrix,0,2); m[12] = position[0];
			m[1] = MAT(matrix,1,0); m[5] = MAT(matrix,1,1); m[9] = MAT(matrix,1,2); m[13] = position[1];
			m[2] = MAT(matrix,2,0); m[6] = MAT(matrix,2,1); m[10]= MAT(matrix,2,2); m[14] = position[2];
			m[3] = 0;               m[7] = 0;               m[11]= 0;               m[15] = 1;				
		}
		else
		{
			// otherwise stay with last frame...
			//
			position = tag->Position;
			matrix = &tag->Matrix[0][0];

			m[0] = MATGL(matrix,0,0); m[4] = MATGL(matrix,0,1); m[8] = MATGL(matrix,0,2); m[12] = position[0];
			m[1] = MATGL(matrix,1,0); m[5] = MATGL(matrix,1,1); m[9] = MATGL(matrix,1,2); m[13] = position[1];
			m[2] = MATGL(matrix,2,0); m[6] = MATGL(matrix,2,1); m[10]= MATGL(matrix,2,2); m[14] = position[2];
			m[3] = 0;               m[7] = 0;               m[11]= 0;               m[15] = 1;	
		}

		// build transformation matrix		
		glPushMatrix();
		glMultMatrixf( m );

		// draw tags...
		//
		if (mdview.bAxisView)
		{
		#define TAG_LINE_LEN 10
			static Vec3 v3X		= { TAG_LINE_LEN,0,0};
			static Vec3 v3XNeg	= {-TAG_LINE_LEN,0,0};

			static Vec3 v3Y		= {0, TAG_LINE_LEN,0};
			static Vec3 v3YNeg	= {0,-TAG_LINE_LEN,0};
			
			static Vec3 v3Z		= {0,0, TAG_LINE_LEN};
			static Vec3 v3ZNeg	= {0,0,-TAG_LINE_LEN};

			glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
			{
				glColor3f(1,1,1);
				glDisable(GL_TEXTURE_2D);
				glDisable(GL_LIGHTING);
				glBegin(GL_LINES);
				{
					glVertex3fv(v3X);
					glVertex3fv(v3XNeg);

					glVertex3fv(v3Y);
					glVertex3fv(v3YNeg);

					glVertex3fv(v3Z);
					glVertex3fv(v3ZNeg);
				}
				glEnd();

		#define TAG_TEXT_COLOUR 255,0,255
				Text_Display( "X",	v3X,	TAG_TEXT_COLOUR);
				Text_Display("-X",	v3XNeg,	TAG_TEXT_COLOUR);
				Text_Display( "Y",	v3Y,	TAG_TEXT_COLOUR);
				Text_Display("-Y",	v3YNeg,	TAG_TEXT_COLOUR);
				Text_Display( "Z",	v3Z,	TAG_TEXT_COLOUR);
				Text_Display("-Z",	v3ZNeg,	TAG_TEXT_COLOUR);

				// now draw tag name (neat, eh?)
				//
				glColor3f(0.5,0.5,0.5);
				static Vec3 v3NamePos = {TAG_LINE_LEN,TAG_LINE_LEN,TAG_LINE_LEN};

				glLineStipple( 8, 0xAAAA);
				glEnable(GL_LINE_STIPPLE);

				glBegin(GL_LINES);
				{
					glVertex3f(0,0,0);
					glVertex3fv(v3NamePos);
				}
				glEnd();

				Text_Display( tag->name, v3NamePos, TAG_TEXT_COLOUR);
			}
			glPopAttrib();
		}


		if (child)
			draw_skeleton( child );
		glPopMatrix();
	}	
}

/*
draws the entire scene starting with mdview.baseModel
*/
void draw_viewSkeleton()
{	
	if (!mdview.baseModel) return;

	glClearColor((float)1/((float)256/(float)mdview._R), (float)1/((float)256/(float)mdview._G), (float)1/((float)256/(float)mdview._B), 0.0f);

	float r = (float)1/((float)256/(float)mdview._R);

	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	glLoadIdentity	(); 	
	glTranslatef	(	   mdview.xPos, mdview.yPos , mdview.zPos );
	glScalef	    (     (float)0.05 , (float)0.05, (float)0.05 );
	glRotatef		( mdview.rotAngleX, 1.0 ,  0.0 , 0.0 );
 	glRotatef		( mdview.rotAngleY, 0.0 ,  1.0 , 0.0 );	
	glRotatef		( mdview.rotAngleZ, 1.0 ,  0.0 , 0.0 );	
		
	if (mdview.bUseAlpha)
	{
		glEnable	(GL_BLEND);
		glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
	else
	{
		glDisable(GL_BLEND);
	}

	draw_skeleton( mdview.baseModel );

	glDisable(GL_BLEND);

	if (mdview.bAxisView)
	{
		// draw origin lines...
		//
	#define ORIGIN_LINE_LEN 20
		static Vec3 v3X		= { ORIGIN_LINE_LEN,0,0};
		static Vec3 v3XNeg	= {-ORIGIN_LINE_LEN,0,0};

		static Vec3 v3Y		= {0, ORIGIN_LINE_LEN,0};
		static Vec3 v3YNeg	= {0,-ORIGIN_LINE_LEN,0};
		
		static Vec3 v3Z		= {0,0, ORIGIN_LINE_LEN};
		static Vec3 v3ZNeg	= {0,0,-ORIGIN_LINE_LEN};

		glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
		{
			glColor3f(0,0,1);
			glDisable(GL_TEXTURE_2D);
			glDisable(GL_LIGHTING);
			glBegin(GL_LINES);
			{
				glVertex3fv(v3X);
				glVertex3fv(v3XNeg);

				glVertex3fv(v3Y);
				glVertex3fv(v3YNeg);

				glVertex3fv(v3Z);
				glVertex3fv(v3ZNeg);
			}
			glEnd();

	#define ORIGIN_TEXT_COLOUR 255,0,255
			Text_Display( "X",	v3X,	ORIGIN_TEXT_COLOUR);
			Text_Display("-X",	v3XNeg,	ORIGIN_TEXT_COLOUR);
			Text_Display( "Y",	v3Y,	ORIGIN_TEXT_COLOUR);
			Text_Display("-Y",	v3YNeg,	ORIGIN_TEXT_COLOUR);
			Text_Display( "Z",	v3Z,	ORIGIN_TEXT_COLOUR);
			Text_Display("-Z",	v3ZNeg,	ORIGIN_TEXT_COLOUR);
		}
		glPopAttrib();
	}
}





#include <string>
typedef set<string> Strings_t;
void R_ViewModelInfo( gl_model *model, char *sInfo )
{
	strcat(sInfo, va("Model: %s\n",model->sMDXFullPathname));

	switch (model->modelType)
	{
		case MODTYPE_MDR:
		{
			for (int iLOD = 0; iLOD <model->Q3ModelStuff.md4->numLODs; iLOD++)
			{
				strcat(sInfo, va("\nSurfaces(LOD %d/%d):\n\n",iLOD,model->Q3ModelStuff.md4->numLODs));

				md4LOD_t *pLOD = (md4LOD_t *) ((byte *)model->Q3ModelStuff.md4 + model->Q3ModelStuff.md4->ofsLODs);
				for ( int i=0; i<iLOD; i++)
				{
					pLOD = (md4LOD_t*)( (byte *)pLOD + pLOD->ofsEnd );
				}

				md4Surface_t * pSurface = (md4Surface_t *)( (byte *)pLOD + pLOD->ofsSurfaces );
				for ( int i = 0 ; i < pLOD->numSurfaces ; i++ )
				{
					strcat(sInfo, va("  %s = %s %s\n",pSurface->name,pSurface->shader,pSurface->shaderIndex?"":"( Not found!)"));							
					pSurface = (md4Surface_t *)( (byte *)pSurface + pSurface->ofsEnd );
				}
			}
		}	
		break;
	
		case MODTYPE_MD3:
		{				
			for (int iLOD=0; iLOD<3; iLOD++)
			{
				strcat(sInfo, va("\nSurfaces(LOD %d):\n\n",iLOD));

				gl_model* pModel = NULL;

				switch (iLOD)
				{
					case 0: pModel = model;					break;
					case 1: pModel = model->pModel_LOD1;	break;
					case 2: pModel = model->pModel_LOD2;	break;
				}

				if (pModel)
				{
					//strcat(sInfo, "Surfaces:\n");
					for (int i=0; i<(int)pModel->iNumMeshes; i++)
					{			
						gl_mesh *pMesh =&pModel->pMeshes[i];

						strcat(sInfo, va("  %s = %s %s\n",pMesh->sName, pMesh->sTextureName,pMesh->bindings[0]?"":"( Not found!)"));
					}
				}
			}
		}
		break;

		default:
		{
			ErrorBox("Code missing for new model type in R_ViewModelInfo()");
			assert(0);
		}
		break;
	}

	strcat(sInfo, "\n\n");

	// get detail on any attached child models...
	//
	for (UINT j=0; j<model->iNumTags ; j++) 
	{			
		gl_model *child = model->linkedModels[j];
		if (child)
		{
			R_ViewModelInfo( child, sInfo );			
		}
	}
}


// for a popup info box...
//
char *GetLoadedModelInfo(void)
{
	static char strInfo[10000];	// <g>

	strInfo[0]=0;

	if (!mdview.baseModel) 
	{
		strcpy(strInfo,"No model loaded!");
	}
	else
	{
		R_ViewModelInfo( mdview.baseModel, strInfo );
	}

	return strInfo;
}



// takes (eg)		"q:\quake\baseq3\models\players\seven\lower.md3"
//
//		and makes:	"q:\quake\baseq3\models\players\seven\lower_default.skin"
//
LPCSTR SkinName_FromPathedModelName(LPCSTR psModelPathName)
{
	static char sSkinName[1024];

	strcpy(sSkinName,psModelPathName);
	strlwr(sSkinName);

	char *psExt = strstr(sSkinName,".md");	

	if (psExt)
	{
		*psExt = 0;
		strcat(sSkinName,"_default.skin");
	}
	else
	{
		assert(0);
	}

	strlwr(sSkinName);
	return sSkinName;
}


// takes (eg) "q:\quake\baseq3\models\players\seven\lower_default.skin"
//        or  "q:\quake\baseq3\models\players\seven\lower.mdr"
//		  or  "lower_blue.skin"
//
// and makes "lower"
//
LPCSTR Model_BaseNameFromFilename(LPCSTR psFileName)
{
	static char sModelName[8][1024];
	static char tmpNodelName[1024];
	static int  iIndex = 0;

	iIndex = (++iIndex)&7;

	strcpy(tmpNodelName, psFileName);

	char *
	psModelName = strrchr(tmpNodelName,'\\');
	if (!psModelName)
	{
		psModelName = strrchr(tmpNodelName,'/');
	}

	if (psModelName)
	{
		strcpy(sModelName[iIndex],psModelName+1);
	}
	else
	{
		strcpy(sModelName[iIndex],tmpNodelName);
	}

	psModelName = strchr(sModelName[iIndex],'.');
	if (psModelName)
	{
		*psModelName  = 0;
	}

	psModelName = strchr(sModelName[iIndex],'_');
	if (psModelName)
	{
		*psModelName  = 0;
	}

	strlwr(sModelName[iIndex]);
	return sModelName[iIndex];
}


gl_model* R_FindModel( gl_model* model, LPCSTR psModelBaseName)	// eg "upper"
{
	gl_model* pReturnVal = NULL;

//	OutputDebugString(va("Scanning model \"%s\"\n",model->sMDXFullPathname));

	char sThisModelName[MAX_QPATH];
	strcpy(sThisModelName,Model_BaseNameFromFilename(model->sMD3BaseName));	// strip off .mdr/.md3 etc for easier compare
	 
	if (!strcmp(sThisModelName, psModelBaseName))
	{
		pReturnVal = model;
	}
	else
	{
		// try it's children...
		//
		for (UINT j=0; j<model->iNumTags ; j++) 
		{			
			gl_model *child = model->linkedModels[j];
			if (child)
			{
				gl_model* pFound = R_FindModel(child, psModelBaseName);

				if (pFound)
				{
					pReturnVal = pFound;
					break;
				}
			}
		}
	}

	return pReturnVal;
}


gl_model* Model_FromPathedSkinName(LPCSTR psSkinFileName)
{
	gl_model* pModel = NULL;

	if (!mdview.baseModel) 
	{
		ErrorBox("No models loaded!");		
	}
	else
	{	
		// note that I can't put this string convert in-line, because the recursive function it calls also
		//	uses the same string function internally, so names get overwritten!
		//
		char sModelBaseName[1024];
		strcpy(sModelBaseName,Model_BaseNameFromFilename(psSkinFileName));

		pModel = R_FindModel(mdview.baseModel,sModelBaseName);

		if (!pModel)
		{
			ErrorBox(va("Unable to find model to apply skin \"%s\" to!",psSkinFileName));
		}
	}

	return pModel;
}

// update, if psTextureName is NULL, surface is bound to zero (the famous GL white),
//	and if psSurfaceName is NULL, then no strcmp is performed (so you can put one texture on all surfaces)
//		   
bool Model_ApplyTextureToSurface(gl_model* model, LPCSTR psSurfaceName, LPCSTR psTextureName)
{
	bool bReturn = false;

	switch (model->modelType)
	{
		case MODTYPE_MD3:
		{			
			for (int iLOD = 0; iLOD<3; iLOD++)
			{
				// ID models actually append the LOD level onto the surface names as well. Doh!
				//
				char sSurfaceNameID[1024];
				strcpy(sSurfaceNameID,psSurfaceName?psSurfaceName:"");

				gl_model* pModel = NULL;
				switch (iLOD)
				{
					case 0: pModel = model;					break;
					case 1: pModel = model->pModel_LOD1;	break;
					case 2: pModel = model->pModel_LOD2;	break;
				}

				strcat(sSurfaceNameID,iLOD?va("_%d",iLOD):"");

				if (pModel)
				{
					int mesh_num = pModel->iNumMeshes;

					for (int i=0; i<mesh_num; i++)
					{			
						gl_mesh *pMesh =&pModel->pMeshes[i];

						if (!psSurfaceName || !strcmp(pMesh->sName,psSurfaceName) || !strcmp(pMesh->sName, sSurfaceNameID))							
						{
							// MD3s are a bit weird, ignore the local 'iNumSkins' count, and always assume 1 here...
							//					
							pMesh->bindings[0] = psTextureName?loadTexture(psTextureName):0;
							strcpy(pMesh->sTextureName,psTextureName?psTextureName:"");
							bReturn = true;
							if (psSurfaceName)
								break;	// else keep looping for ALL
						}
					}
				}
			}
		}
		break;

		case MODTYPE_MDR:
		{
			for (int iLOD=0; iLOD < model->Q3ModelStuff.md4->numLODs; iLOD++)
			{
				md4LOD_t *pLOD = (md4LOD_t *) ((byte *)model->Q3ModelStuff.md4 + model->Q3ModelStuff.md4->ofsLODs);
				
				for ( int i=0; i<iLOD; i++)
				{
					pLOD = (md4LOD_t*)( (byte *)pLOD + pLOD->ofsEnd );
				}

				md4Surface_t * pSurface = (md4Surface_t *)( (byte *)pLOD + pLOD->ofsSurfaces );
				for ( int i = 0 ; i < pLOD->numSurfaces ; i++ )
				{
					if (!psSurfaceName || !strcmp(pSurface->name,psSurfaceName))
					{
						strcpy(pSurface->shader,psTextureName?psTextureName:"");
						pSurface->shaderIndex = psTextureName?loadTexture(pSurface->shader):0;
						bReturn = true;
						if (psSurfaceName)
							break;
					}

					pSurface = (md4Surface_t *)( (byte *)pSurface + pSurface->ofsEnd );
				}
			}
		}
		break;

		default:
		{
			ErrorBox("Missing model-type case statement (Model_ApplyTextureToSurface)");			
			bReturn = true;		// so it doesn't try and report the skin error below
		}
		break;
	}

	if (!bReturn)
	{
		if (!psSurfaceName || !psTextureName)
		{
			// forget it, the model probably had no surfaces
		}
		else
		{
			// !psSurfaceName should already have been handled by above, so crash won't happen here...
			//
			ErrorBox(va("Unable to find surface name \"%s\" in model %s",psSurfaceName,model->sMDXFullPathname));
		}
	}
		
	return bReturn;
}



// if psSkinFilename==NULL, white-out the model
//
bool Model_ApplySkin(gl_model* model, LPCSTR psSkinFilename)
{		
	FILE *fpHandle = NULL;

	// default model to blank/white first, so that missing bits show up...
	//
	if (psSkinFilename)
	{
		fpHandle = fopen(psSkinFilename,"rt");		

		// white-out the model first if we do find the file, or we don't and it's not a default skin (which are optional)
		//
		if (fpHandle || (!strstr(String_ToLower(psSkinFilename),"_default.skin")))
			Model_ApplyTextureToSurface(model, NULL, NULL);

		if (fpHandle)
		{
			typedef struct
			{
				char sSurfaceName[100];
				char sTextureName[MAX_QPATH];
			} SKINITEM, *LPSKINITEM;

			SKINITEM SkinItems[1024]={0};
			int iSkinIndex = 0;

			char sLine[1024]={0};

			while (fgets(sLine, sizeof(sLine), fpHandle))
			{
				// lose CR...
				//
				char *p = strchr(sLine,'\n');
				if (p)
					*p=0;

				if (strlen(sLine))
				{					
					LPCSTR psComma = strchr(sLine,',');
					if (psComma)
					{
						int iCommaIndex = psComma - sLine;

						LPSKINITEM lpSkinItem = &SkinItems[iSkinIndex];

						// new bit to cope with faulty ID skin files, where they have spurious lines like "tag_torso,"
						//
						if (strnicmp(sLine,"tag_",4)==0 || !strlen(&sLine[iCommaIndex+1]))
						{
							// crap line, so ignore it...
						}
						else
						{
							strncpy(lpSkinItem->sSurfaceName,sLine,iCommaIndex);
							strcpy (lpSkinItem->sTextureName,&sLine[iCommaIndex+1]);
	
							char *psCR = strchr(lpSkinItem->sTextureName,'\n');
							if (psCR)
								*psCR = 0;
	
							iSkinIndex++;
						}
					}
					else
					{
						ErrorBox(va("Error parsing line \"%s\" in file \"%s\"!",sLine,psSkinFilename));
					}

					sLine[0]=0;
				}
			}

			// apply any entries found...
			//
			for (int i=0; i<iSkinIndex; i++)
			{
				LPSKINITEM lpSkinItem = &SkinItems[i];

				Model_ApplyTextureToSurface(model, lpSkinItem->sSurfaceName, lpSkinItem->sTextureName);
			}

			fclose(fpHandle);
		}
	}	

	return (!psSkinFilename)?true:!!fpHandle;
}



// called with filename from browser, so work out what to do with it...
//
bool ApplyNewSkin(LPCSTR psSkinFullPathName)
{
	bool bReturn = false;

	gl_model* pModel = Model_FromPathedSkinName(psSkinFullPathName);

	if (pModel)
	{
		if (!strnicmp(pModel->sMD3BaseName,"head.",5) && strstr(psSkinFullPathName,"head_"))
		{
			char sSkinBase[MAX_PATH];
			strcpy(sSkinBase,strchr(Filename_WithoutExt(Filename_WithoutPath(psSkinFullPathName)),'_')+1);
			if (strchr(sSkinBase,'-')) *strchr(sSkinBase,'-')=0;
			strcpy(pModel->sHeadSkinName,sSkinBase);
		}

		bReturn = Model_ApplySkin(pModel, psSkinFullPathName);
	}

	return bReturn;
}


// if there's a head model present then change it's skin according to:
//
// -1   frame down
//  0   reset to default
//  1	frame up
//
// (head frames are 6 talking frames, then two expression frames)
//
#define HEAD_SKIN_MAX 8
#define HEAD_SKIN_TALK_MAX 8
#define HEAD_SKIN_TALK_MIN 5
//		
// this is now also called every time any model is loaded (for safety reasons))
//
void SetFaceSkin(int iSkin)
{		
	gl_model* pModel = R_FindModel( mdview.baseModel, "head");	// eg "upper"

	if (pModel)
	{
		// don't try and apply skins to something that has no default skin to restore to...
		//
		if (file_exists(SkinName_FromPathedModelName(pModel->sMDXFullPathname)))
		{
			if (iSkin == 0)
				mdview.iSkinNumber = 0;
			else
			{
				mdview.iSkinNumber += iSkin;
				if (iSkin>0)
				{
					// cycling forward through speech...
					//
					if (mdview.iSkinNumber > HEAD_SKIN_TALK_MAX)
					{
						mdview.iSkinNumber = HEAD_SKIN_TALK_MIN;
					}
				}
				else
				{
					// resetting or cycling backwards (which access the higher frames)
					//
					if (mdview.iSkinNumber < 0)
					{
						mdview.iSkinNumber = HEAD_SKIN_MAX;
					}
				}

				// sanity...
				//
				if (mdview.iSkinNumber<0 || mdview.iSkinNumber>HEAD_SKIN_MAX)
				{
					mdview.iSkinNumber=0;
				}
			}

			char sSkinFileName[MAX_PATH];
			strcpy(sSkinFileName,pModel->sMDXFullPathname);		// default to fullpathname of model (windows-style backslashes)		
			*strrchr(sSkinFileName,'.')=0;					// lose ".mdx"
			strcat(sSkinFileName,va("_%s%s.skin",pModel->sHeadSkinName,!mdview.iSkinNumber?"":va("-%1d",mdview.iSkinNumber)));		
			
			if (!Model_ApplySkin(pModel, sSkinFileName))
			{
				// error applying skin, but errors would already be either reported or visible in model now, so do nowt
			}
		}
		else
		{
			if (iSkin)	// so it won't report on the default-set call
			{
				ErrorBox("This head model does not have a default skin file to restore to, so no expressions are available");
			}
		}
	}	
}

