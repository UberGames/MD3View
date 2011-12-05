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

#include "system.h"		// stuff needed for caller program, standar headers etc
#include "oddbits.h"
//
#include "matrix4.h"
#include "mdx_format.h"
#include "matcomp.h"
//
#include "g2export.h"

#define BASEDIRNAME "base" 
extern char	qdir[];//1024];
extern char gamedir[];//1024];		// q:\quake\baseef
extern LPCSTR Filename_QUAKEBASEONLY(LPCSTR psFullPathedName);



extern	LPCSTR	G2Exporter_Surface_GetName		(int iSurfaceIndex);
extern	LPCSTR	G2Exporter_Surface_GetShaderName(int iSurfaceIndex);
extern	int		G2Exporter_Surface_GetNumVerts	(int iSurfaceIndex, int iLOD);
extern	int		G2Exporter_Surface_GetNumTris	(int iSurfaceIndex, int iLOD);
extern	int		G2Exporter_Surface_GetTriIndex	(int iSurfaceIndex, int iTriangleIndex, int iTriVert, int iLOD);
extern	vec3_t*	G2Exporter_Surface_GetVertNormal(int iSurfaceIndex, int iVertIndex, int iLOD);
extern	vec3_t*	G2Exporter_Surface_GetVertCoords(int iSurfaceIndex, int iVertIndex, int iLOD);
extern	vec2_t*	G2Exporter_Surface_GetTexCoords	(int iSurfaceIndex, int iVertIndex, int iLOD);



static LPCSTR Filename_StripQuakeBase(LPCSTR psFullPathedName)
{
	static char sTemp[1024];	
	strcpy(sTemp,Filename_QUAKEBASEONLY(psFullPathedName));
	int iQuakeLen = strlen(sTemp);
	strcpy(sTemp,&psFullPathedName[iQuakeLen]);

	return &sTemp[0];
}

static void ForwardSlash(char *psLocalName)
{
	while (*psLocalName)
	{
		if (*psLocalName == '\\')
			*psLocalName =  '/';

		psLocalName++;
	}
}

// saves clogging the param stack...
//
int giNumLODs;
int giNumSurfaces;
int giNumTags;

static char *ExportGhoul2FromMD3_Main(FILE *fhGLM, /*FILE *fhGLA,*/ LPCSTR psFullPathedNameGLM)
{
	char *psErrMess = NULL;

	const int iMallocSize = 1024*1024*20;	// 20MB should be enough for one model... :-)
	byte *pbBuffer = (byte *) malloc(iMallocSize);
	if (pbBuffer)
	{
		// filename used in both files...
		//
		char sLocalNameGLA[MAX_PATH];
		strcpy(	sLocalNameGLA,Filename_WithoutExt(Filename_StripQuakeBase(psFullPathedNameGLM)));		
		ForwardSlash(sLocalNameGLA);

		// start writing...
		//
		byte *at = pbBuffer;

		// write GLM Header...
		//
		mdxmHeader_t *pMDXMHeader = (mdxmHeader_t *) at;
		at += sizeof(mdxmHeader_t);

		{// GLM brace-match for skipping...

					pMDXMHeader->ident			=	MDXM_IDENT;
					pMDXMHeader->version			=	MDXM_VERSION;					
			strcpy(	pMDXMHeader->name,Filename_StripQuakeBase(psFullPathedNameGLM));
					ForwardSlash(pMDXMHeader->name);
			strcpy(	pMDXMHeader->animName,sDEFAULT_GLA_NAME);//sLocalNameGLA);
					pMDXMHeader->animIndex		=	0;	// ingame use only
					pMDXMHeader->numBones		=	1;	// ... and a fake one at that.
					pMDXMHeader->numLODs			=	giNumLODs;
	//				pMDXMHeader->ofsLODs			=	?????????????????????????????????????????????
					pMDXMHeader->numSurfaces		=	giNumSurfaces + giNumTags;
	//				pMDXMHeader->ofsSurfHierarchy= 	?????????????????????????????????????????????
	//				pMDXMHeader->ofsEnd			=	?????????????????????????????????????????????



			// for hierarchiy purposes, I'm going to just write out the first surface as the parent, 
			//	then make every other surface a child of that one...
			//
			// G2 surfaces come from MD3 meshes first, then the MD3 tags (since G2 uses tag surfaces)
			//
			mdxmHierarchyOffsets_t *pHierarchyOffsets = (mdxmHierarchyOffsets_t *) at;
			at += sizeof(mdxmHierarchyOffsets_t) * pMDXMHeader->numSurfaces;

			pMDXMHeader->ofsSurfHierarchy = at - (byte *) pMDXMHeader;
			
			for (int iSurfaceIndex = 0; iSurfaceIndex < pMDXMHeader->numSurfaces; iSurfaceIndex++)
			{
				// Note:  bool bSurfaceIsTag == (iSurfaceIndex < giNumSurfaces)
				//
				mdxmSurfHierarchy_t *pSurfHierarchy = (mdxmSurfHierarchy_t *) at;
				//
				// store this offset...
				//
				pHierarchyOffsets->offsets[iSurfaceIndex] = (byte *)pSurfHierarchy - (byte *)pHierarchyOffsets;

				// fill in surf hierarchy struct...
				//
				strcpy(	pSurfHierarchy->name,	G2Exporter_Surface_GetName(iSurfaceIndex));
						pSurfHierarchy->flags		= 0;

				if ( iSurfaceIndex >= giNumSurfaces)
				{
						pSurfHierarchy->flags		|= G2SURFACEFLAG_ISBOLT;
				}
				if (!strnicmp(&pSurfHierarchy->name[strlen(pSurfHierarchy->name)-4],"_off",4))
				{
						pSurfHierarchy->flags		|= G2SURFACEFLAG_OFF;
				}
				strlwr(pSurfHierarchy->name);

				strcpy(	pSurfHierarchy->shader,	G2Exporter_Surface_GetShaderName(iSurfaceIndex));
						pSurfHierarchy->shaderIndex	= 0;	// ingame use only
						pSurfHierarchy->parentIndex = iSurfaceIndex?0:-1;
						pSurfHierarchy->numChildren = iSurfaceIndex?0:pMDXMHeader->numSurfaces-1;
				if (!iSurfaceIndex)
				{
					for (int i=0; i<pSurfHierarchy->numChildren; i++)
					{
						pSurfHierarchy->childIndexes[i] = i+1;
					}
				}

				int iThisHierarchySize = (int)( &((mdxmSurfHierarchy_t *)0)->childIndexes[ pSurfHierarchy->numChildren ] );

				at += iThisHierarchySize;
			}



			// write out LODs...
			//
			pMDXMHeader->ofsLODs = at - (byte *) pMDXMHeader;
			for (int iLODIndex = 0; iLODIndex < pMDXMHeader->numLODs; iLODIndex++)
			{
				mdxmLOD_t *pLOD = (mdxmLOD_t *) at;
				at += sizeof(mdxmLOD_t);

				mdxmLODSurfOffset_t *pLODSurfOffsets = (mdxmLODSurfOffset_t *) at;
				at += sizeof(mdxmLODSurfOffset_t) * pMDXMHeader->numSurfaces;

				for (int iSurfaceIndex = 0; iSurfaceIndex < pMDXMHeader->numSurfaces; iSurfaceIndex++)
				{
					mdxmSurface_t *pSurface = (mdxmSurface_t *) at;
					at += sizeof(mdxmSurface_t);
					//
					// store this offset...
					//
					pLODSurfOffsets->offsets[iSurfaceIndex] = (byte *)pSurface - (byte *) pLODSurfOffsets;
					//
					// fill in this surface struct...
					//
					pSurface->ident				=	0;	// ingame-use only, defaulted to 0 here
					pSurface->thisSurfaceIndex	=	iSurfaceIndex;
					pSurface->ofsHeader			= 	(byte *)pMDXMHeader - (byte *)pSurface;	// offset back to main header
					pSurface->numVerts			=	G2Exporter_Surface_GetNumVerts(iSurfaceIndex, iLODIndex);
					pSurface->numTriangles		=	G2Exporter_Surface_GetNumTris(iSurfaceIndex, iLODIndex);
//					pSurface->maxVertBoneWeights=	1;	// easy, eh? :-)
					
					//
					// write out triangles...
					//
					pSurface->ofsTriangles		=	at - (byte *)pSurface;
					for (int iTriangleIndex = 0; iTriangleIndex < pSurface->numTriangles; iTriangleIndex++)
					{
						mdxmTriangle_t *pTriangle = (mdxmTriangle_t *) at;
						at += sizeof(mdxmTriangle_t);

						for (int i=0; i<3; i++)
						{
							pTriangle->indexes[i] = G2Exporter_Surface_GetTriIndex(iSurfaceIndex,iTriangleIndex,i,iLODIndex);
						}
					}

					//
					// write out verts...(when exporting from MD3 these are all weighted to only 1 bone)
					//
					pSurface->ofsVerts			=	at - (byte *)pSurface;

					mdxmVertex_t			*pVerts			= (mdxmVertex_t *)			at;
					mdxmVertexTexCoord_t	*pVertTexCoords	= (mdxmVertexTexCoord_t	*)	&pVerts[pSurface->numVerts];
					at = (unsigned char *) &pVertTexCoords[pSurface->numVerts];	// skip over all this vert-writing...

					for (int iVertIndex = 0; iVertIndex < pSurface->numVerts; iVertIndex++)
					{
						mdxmVertex_t *pVert = &pVerts[iVertIndex];

						memcpy(pVert->normal,G2Exporter_Surface_GetVertNormal(iSurfaceIndex,iVertIndex,iLODIndex),sizeof(vec3_t));
						memcpy(pVert->vertCoords,G2Exporter_Surface_GetVertCoords(iSurfaceIndex,iVertIndex,iLODIndex),sizeof(vec3_t));
//						memcpy(pVert->texCoords,G2Exporter_Surface_GetTexCoords (iSurfaceIndex,iVertIndex,iLODIndex),sizeof(vec2_t));
						memcpy(pVertTexCoords[iVertIndex].texCoords,G2Exporter_Surface_GetTexCoords (iSurfaceIndex,iVertIndex,iLODIndex),sizeof(vec2_t));

						memset(pVert->BoneWeightings,0,sizeof(pVert->BoneWeightings));

						int iNumWeights = 1;
						pVert->uiNmWeightsAndBoneIndexes = (iNumWeights-1)<<30;
						int iWeight = 1023;	// highest weighting currently allowed (1.000f as 10 bit fixed point)
						pVert->BoneWeightings[0] = iWeight&0xFF;	// float->byte
						// the 2-bit pairs at 20,22,24,26 are the top 2 bits of each weighting (leaving bits 28&29 free)...
						//
						pVert->uiNmWeightsAndBoneIndexes |= (iWeight>>8)<<((0*2)+20);
						pVert->BoneWeightings[0] = 255;	// 1.0f weighting
  
//						int iWeightTest		= G2_GetVertWeights( pVert );
//						int iBoneIndex		= G2_GetVertBoneIndex( pVert, 0 );
//						float fBoneWeight	= G2_GetVertBoneWeight( pVert, 0 );
					}

					// remaining surface struct fields...
					//
					pSurface->numBoneReferences =	1;				
					pSurface->ofsBoneReferences =	at - (byte *) pSurface;
					int *piBonesUsed = (int *) at;				
					at += pSurface->numBoneReferences * sizeof(int);
					piBonesUsed[0] = 0;		// the one and only bone ref

					pSurface->ofsEnd			= at - (byte *) pSurface;
				}
			
				pLOD->ofsEnd = at - (byte *) pLOD;
			}

			pMDXMHeader->ofsEnd = at - (byte *) pMDXMHeader;
		}
/*
		// now create GLA file...
		//
		mdxaHeader_t *pMDXAHeader = (mdxaHeader_t *) at;
		at += sizeof(mdxaHeader_t);
		{// for brace-skipping...			

					pMDXAHeader->ident			=	MDXA_IDENT;
					pMDXAHeader->version		=	MDXA_VERSION;
			strncpy(pMDXAHeader->name,sLocalNameGLA,sizeof(pMDXAHeader->name));
					pMDXAHeader->name[sizeof(pMDXAHeader->name)-1]='\0';
					pMDXAHeader->fScale = 1.0f;
					pMDXAHeader->numFrames		=	1;	// inherently, when doing MD3 to G2 files
//					pMDXAHeader->ofsFrames		=	??????????????????
					pMDXAHeader->numBones		=	1;	// inherently, when doing MD3 to G2 files
//					pMDXAHeader->ofsCompBonePool=	??????????????????
//					pMDXAHeader->ofsSkel		=	??????????????????
//					pMDXAHeader->ofsEnd			=	??????????????????

			// write out bone hierarchy...
			//
			mdxaSkelOffsets_t * pSkelOffsets = (mdxaSkelOffsets_t *) at;
			at += (int)( &((mdxaSkelOffsets_t *)0)->offsets[ pMDXAHeader->numBones ] );
			
			pMDXAHeader->ofsSkel	= at - (byte *) pMDXAHeader; 
			for (int iSkelIndex = 0; iSkelIndex < pMDXAHeader->numBones; iSkelIndex++)
			{
				mdxaSkel_t *pSkel = (mdxaSkel_t *) at;

				pSkelOffsets->offsets[iSkelIndex] = (byte *) pSkel - (byte *) pSkelOffsets;

				// setup flags...
				//
						pSkel->flags = 0;	
				strcpy(	pSkel->name, "Generated by MD3View");	// doesn't matter what this is called I believe.
						pSkel->parent= -1;	// index of bone that is parent to this one, -1 = NULL/root

				Matrix4 BasePose;
						BasePose.Identity();
						BasePose.To3x4(pSkel->BasePoseMat.matrix);				

				Matrix4 BasePoseInverse;
						BasePoseInverse.Inverse(BasePose);
						BasePoseInverse.To3x4(pSkel->BasePoseMatInv.matrix);

						pSkel->numChildren	=	0;	// inherently, when doing MD3 to G2 files

				int iThisSkelSize = (int)( &((mdxaSkel_t *)0)->children[ pSkel->numChildren ] );

				at += iThisSkelSize;
			}


			// write out frames...
			//			
			pMDXAHeader->ofsFrames = at - (byte *) pMDXAHeader;			
			int iFrameSize = (int)( &((mdxaFrame_t *)0)->boneIndexes[ pMDXAHeader->numBones ] );
			for (int i=0; i<pMDXAHeader->numFrames; i++)
			{
				mdxaFrame_t *pFrame = (mdxaFrame_t *) at;
				at += iFrameSize;	// variable sized struct

				pFrame->boneIndexes[0] = 0;	// inherently, when doing MD3 to G2 files
			}

			// now write out compressed bone pool...
			//
			pMDXAHeader->ofsCompBonePool = at - (byte *) pMDXAHeader;
			for (int iCompBoneIndex = 0; iCompBoneIndex < 1; iCompBoneIndex++)
			{
				mdxaCompBone_t *pCompBone = (mdxaCompBone_t *) at;
				at += sizeof(mdxaCompBone_t);

				float matThis[3][4];

				Matrix4 Bone;
						Bone.Identity();
						Bone.To3x4(matThis);
				
				byte Comp[sizeof(mdxaCompBone_t)];
				MC_Compress(matThis,Comp);

				memcpy(pCompBone->Comp,Comp,sizeof(Comp));
			}
			
//			int iPoolBytesUsed = (CompressedBones.size()*MC_COMP_BYTES) + (iStats_NumBoneRefs*sizeof(int));
//			printf("( Compressed bone bytes: %d, using pool = %d. Saving = %d bytes )\n",iOldBoneSize,iPoolBytesUsed, iOldBoneSize - iPoolBytesUsed);

			// done...
			//
			pMDXAHeader->ofsEnd = at - (byte *) pMDXAHeader;		
		}
*/

		// write 'em out to disk...
		//
//		int iGLMSize = fwrite(pMDXMHeader,1,(byte *)pMDXAHeader - (byte *)pMDXMHeader, fhGLM);
		int iGLMSize = fwrite(pMDXMHeader,1,pMDXMHeader->ofsEnd, fhGLM);
		assert(iGLMSize == pMDXMHeader->ofsEnd);

//		int iGLASize = fwrite(pMDXAHeader,1,at - (byte *)pMDXAHeader, fhGLA);
//		assert(iGLASize == pMDXAHeader->ofsEnd);

		// and finally...
		//
		free(pbBuffer);
	}
	else
	{
		psErrMess = va("Error! Unable to allocate %d bytes for workspace!\n",iMallocSize);
	}

	return psErrMess;
}



// assumes 'psFullPathedFilename' is full-pathed filenam with ".glm" on the end,
//	ppsFullPathedNameGLA is optional feedback param if you want to know what the GLA name was.
//
// return = NULL for no error, else string of error for caller to display...
//
char *ExportGhoul2FromMD3(LPCSTR psFullPathedFilename, int iNumLODs, int iNumSurfaces, int iNumTags,
						  LPCSTR *ppsFullPathedNameGLA /* = NULL */
						  )
{
	char *psErrorMess = NULL;

	giNumLODs		= iNumLODs;
	giNumSurfaces	= iNumSurfaces;
	giNumTags		= iNumTags;

	FILE *fhGLM = fopen(psFullPathedFilename,"wb");
	if (  fhGLM)
	{
		static char sNameGLA[MAX_PATH];

//		strcpy(sNameGLA, Filename_WithoutExt(psFullPathedFilename) );
//		strcat(sNameGLA, ".gla");
		strcpy(sNameGLA,sDEFAULT_GLA_NAME);

//		FILE *fhGLA = fopen(sNameGLA,"wb");
//		if (fhGLA)
		{
			if ( ppsFullPathedNameGLA)
			{
				*ppsFullPathedNameGLA = &sNameGLA[0];
			}

			psErrorMess = ExportGhoul2FromMD3_Main(fhGLM, /*fhGLA,*/ psFullPathedFilename);

//			fclose(fhGLA);
		}
//		else
//		{
//			psErrorMess = va("Error: Unable to open file '%s'!\n");
//		}

		fclose(fhGLM);
	}
	else
	{
		psErrorMess = va("Error: Unable to open file '%s'!\n");
	}


	return psErrorMess;
}



/////////////////// eof ////////////////////

