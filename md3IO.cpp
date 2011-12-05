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
#include "DiskIO.h"
#include "oddbits.h"
#include "matcomp.h"
#include <stddef.h>	// for offsetof macro
#include "mdx_format.h"
#include "g2export.h"
#include "matrix4.h"

#pragma warning (disable : 4786)	//ident trunc
#include <list>
using namespace std;


// if enabled, this'll make the g2 model have same orientation as MD3,
// if disabled, the model will have all that weird 90-degree bollocks applied to it.
//
//#define	PERFECT_CONVERSION	// this is now runtime, controlled by "bool g_bPerfect"
extern bool g_bPerfect;

#ifndef M_PI
#define M_PI		3.14159265358979323846	// matches value in gcc v2 math.h
#endif

#define DEG2RAD( a ) ( ( (a) * M_PI ) / 180.0F )
#define RAD2DEG( a ) ( ( (a) * 180.0f ) / M_PI )


/*
** NormalToLatLong
**
** We use two byte encoded normals in some space critical applications.
** Lat = 0 at (1,0,0) to 360 (-1,0,0), encoded in 8-bit sine table format
** Lng = 0 at (0,0,1) to 180 (0,0,-1), encoded in 8-bit sine table format
**
*/
void NormalToLatLong( const vec3_t normal, byte bytes[2] ) 
{
	// check for singularities
	if ( normal[0] == 0 && normal[1] == 0 ) {
		if ( normal[2] > 0 ) {
			bytes[0] = 0;
			bytes[1] = 0;		// lat = 0, long = 0
		} else {
			bytes[0] = 128;
			bytes[1] = 0;		// lat = 0, long = 128
		}
	} else {
		int	a, b;

		a = RAD2DEG( atan2( normal[1], normal[0] ) ) * (255.0f / 360.0f );
		a &= 0xff;

		b = RAD2DEG( acos( normal[2] ) ) * ( 255.0f / 360.0f );
		b &= 0xff;

		bytes[0] = b;	// longitude
		bytes[1] = a;	// lattitude
	}
}

double VectorLength( const vec3_t v ) 
{
	double	length;
	
	length = sqrt (v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
	return length;
}


void ClearBounds (vec3_t mins, vec3_t maxs)
{
	mins[0] = mins[1] = mins[2] = 99999;
	maxs[0] = maxs[1] = maxs[2] = -99999;
}

void AddPointToBounds( const vec3_t v, vec3_t mins, vec3_t maxs ) 
{
	int		i;
	vec_t	val;

	for (i=0 ; i<3 ; i++)
	{
		val = v[i];
		if (val < mins[i])
			mins[i] = val;
		if (val > maxs[i])
			maxs[i] = val;
	}
}



#define OUTPOS() //OutputDebugString(va("0x%X\n",filesize(fHandle)))

list <string> FilesWritten;
bool bAutoOverWrite = false;
bool bAskThem       = true;
gl_model* gpModel=NULL;
bool ExportThisModel( LPCSTR psFilename, gl_model* pModel, bool bExportAsMD3)
{
	bool bG2MultiLODExportHasErrors = false;
	bool bG2ForceSingleLODExport = false;

	// this compiler is being really gay and insisting I move these up here to avoid their initialising being
	//	skipped by the first 'goto' commands, even though they're not used later. Duh!!!
	//	
	int iOutLODCount = 1;
	int iOutLOD;


	gpModel = pModel;	// so ghoul2 exporter code can get at the model data

//	OutputDebugString(va("#### Exporting: %s\n",psFilename));

	if (!pModel)	// call with NULL pModel to display info on stuff written
	{
		string str;

		str = "Files written:\n\n";

		list <string>::iterator it;
		STL_ITERATE(it,FilesWritten)
		{
			str += (*it).c_str();
			str += "\n";
		}
		str += "\n\nNo errors";
		InfoBox(str.c_str());
		return true;
	}

	StartWait();
	FILE *fHandle = NULL;

	bool bOneFrameMD3ExportHack = false;

	if (!bExportAsMD3)	// and hence Ghoul2...
	{
		if (pModel->modelType != MODTYPE_MD3)
		{
			ErrorBox("Error! Currently, this program will only export loaded MD3's as Ghoul2 models");
			goto error;
		}
		// and if it's an MD3, then it must only be one frame since there's no skeletal information...
		//		
		if (pModel->iNumFrames != 1)
		{
			if (!GetYesNo( va("Only MD3 models with 1 frame can be exported as Ghoul2, this model has %d frames!\n\nExport just the first frame? ( 'NO' will abort export )",pModel->iNumFrames)))
			{
				goto error;
			}
			bOneFrameMD3ExportHack = true;
		}

		// because of multiple-LOD-from-multiple-model issues I now need to pre-scan some stuff to check for
		//	mismatching surfaces etc...
		//
		if (pModel->pModel_LOD1 || pModel->pModel_LOD2)
		{
			// multiple models loaded, need to run checks...
			//
			char sExportSingleLODPrompt[4096]="Unknown";//{0};
			//
			bool bMisMatchingSurfacenameQuestionAlreadyAsked = false;
			int iLOD;
			for (iLOD=0; iLOD<3; iLOD++)
			{
				gl_model* pMod = NULL;
				
				switch (iLOD)
				{
					//case 0: pMod = pModel; break;
					case 1: pMod = pModel->pModel_LOD1;	break;
					case 2: pMod = pModel->pModel_LOD2; break;
				}

				if (pMod)
				{
					if (pModel->modelType != MODTYPE_MD3)
					{
						sprintf(sExportSingleLODPrompt,"LOD %d is not an MD3 model",iLOD);
						bG2MultiLODExportHasErrors = true;
						break;
					}
					if (pMod->iNumMeshes > pModel->iNumMeshes)
					{
						sprintf(sExportSingleLODPrompt,"LOD %d has more surfaces than LOD 0",iLOD);
						bG2MultiLODExportHasErrors = true;
						break;
					}
					if (pMod->iNumMeshes < pModel->iNumMeshes)
					{
						// this is ok because I'll generate fake surfaces, but let's just see if the surfaces
						//	so far match, and if not, issue a warning...
						//
						if (!bMisMatchingSurfacenameQuestionAlreadyAsked)
						{
							for (int iMesh=0; iMesh<(int)pMod->iNumMeshes; iMesh++)
							{
								LPCSTR pLOD0MeshName = pModel->pMeshes[iMesh].sName;
								LPCSTR pLODXMeshName = pModel->pMeshes[iMesh].sName;

								if (stricmp(pLOD0MeshName,pLODXMeshName))
								{
									bG2ForceSingleLODExport = !GetYesNo(va("Multi-LOD-export issue:\n\nLOD %d has less surfaces than LOD 0 (which isn't a problem, I can insert extra dummy surfaces), but the common surface names also mismatch, surface %d is called \"%s\" in this LOD, and \"%s\" in the original\n\nDo you still want to go ahead and export as multi-LOD?\n\n('NO' will export single-LOD only)",
																		iLOD, iMesh, pLODXMeshName, pLOD0MeshName));
									bMisMatchingSurfacenameQuestionAlreadyAsked = true;
									break;
								}
							}
						}
					}
				}
			}

			// Ok, do we need to offer them a prompt to export single-LOD only?
			//
			if (iLOD != 3)
			{
				if (bG2MultiLODExportHasErrors)
				{
					bG2ForceSingleLODExport = GetYesNo(va("Multi-LOD-export failure:\n\n%s\n\nDo you want to go ahead and export as a single-LOD model?",sExportSingleLODPrompt));

					if (!bG2ForceSingleLODExport)	// since we CANNOT export multi-lods because of the errtype, exit if they say 'no'.
						goto error;
				}
			}
		}
	}

	if (pModel == mdview.baseModel)
	{
//		OutputDebugString("!!!! First model\n");
		// the only model, or the first in a sequence if exporting several...
		//
		FilesWritten.clear();
		if (bAskThem)
		{
			//bAskThem       = true;
			bAutoOverWrite = false;
		}
	}

	switch (pModel->modelType)	// type of current model, not export format
	{
		case MODTYPE_MD3:	iOutLODCount = 1;									break;
		case MODTYPE_MDR:	iOutLODCount = pModel->Q3ModelStuff.md4->numLODs;	break;
		default:			ErrorBox("casedefault: Unhandled model type");		goto error;
	}

	char sFilename[MAX_PATH];
	
	for (iOutLOD=0; iOutLOD<iOutLODCount; iOutLOD++)
	{
		strcpy(sFilename,psFilename);

		char *p;
		while ((p=strchr(sFilename,'\\'))!=NULL)
			*p='/';

		if (iOutLOD)
		{
			char *p=strrchr(sFilename,'.');
			assert(p);
			if (p)
			{
				strcpy(p,va("_%d.md3",iOutLOD));
			}
		}

		bool bGoAhead = true;
		if (FileExists(sFilename))
		{
			if (bAskThem)
			{
				bGoAhead = GetYesNo(va("Output file \"%s\" already exists, overwrite?",sFilename));

				if (bAskThem)
				{
					bAskThem =!GetYesNo("Apply this decision to all future queries?");					
				}
				if (!bAskThem)
				{
					bAutoOverWrite = bGoAhead;
				}
			}
			else
			{
				bGoAhead = bAutoOverWrite;
			}
			RestoreWait();
		}
		
		if (bGoAhead)
		{
			if (bExportAsMD3)
			{
				fHandle = fopen(sFilename,"wb");

				if (fHandle)
				{
					FilesWritten.push_back(sFilename);	
					//
					// fuck this, it's tedious not having a structure to work with (and for rewinding to field offsets during
					//	writing), so...
					//
					typedef struct
					{
						char	ID[4];
						int		iVersion;
						char	sName[68];
						int		iNumFrames;
						int		iNumTags;
						int		iNumMeshes;
						int		iMaxSkinNum;
						int		iHeadSize;
						int		iOffsetToTags;		// from file-0
						int		iOffsetToMeshes;	//
						int		iFileSize;
					} MD3Head, *lpMD3Head;

					MD3Head Header;

					strncpy(Header.ID,"IDP3",4);
					Header.iVersion			= 15;
					strcpy(Header.sName,Filename_WithoutPath(Filename_WithoutExt(sFilename)));
					Header.iNumFrames		= bOneFrameMD3ExportHack ? 1 : pModel->iNumFrames;
					Header.iNumTags			= pModel->iNumTags;
					Header.iMaxSkinNum		= 0;	// ? (always seems to be 0 in models I've tried)
					Header.iHeadSize		= sizeof(Header);
					Header.iOffsetToTags	= sizeof(Header) + (Header.iNumFrames * sizeof md3BoundFrame_t);
					Header.iOffsetToMeshes	= Header.iOffsetToTags + (Header.iNumFrames * Header.iNumTags * sizeof(md3Tag_t)/*BoneFrame_t*/);
					Header.iFileSize		= NULL;

					md4LOD_t *pLOD = NULL;			
					switch (pModel->modelType)
					{			
						case MODTYPE_MD3:	
							
							Header.iNumMeshes = pModel->iNumMeshes;
							break;

						case MODTYPE_MDR:	
						{
							pLOD = (md4LOD_t *) ((byte *)pModel->Q3ModelStuff.md4 + pModel->Q3ModelStuff.md4->ofsLODs);
							for ( int i=0; i<iOutLOD; i++)
							{
								pLOD = (md4LOD_t*)( (byte *)pLOD + pLOD->ofsEnd );
							}
							Header.iNumMeshes = pLOD->numSurfaces;	
							break;
						}

						default: 
							ErrorBox("casedefault: Unhandled model type");						
							goto error;
					}		

					fwrite(&Header,sizeof(Header),1,fHandle);

			OUTPOS();

					// write the boundary frames...
					//
					md3BoundFrame_t *pBoundFrames = NULL;	// MDRs will have to come back and fill this in later during freeze
					long lBoundFrameWriteSeekPos = ftell(fHandle);

					int iFrame = 0;
					for (iFrame = 0; iFrame<Header.iNumFrames; iFrame++)
					{
						md3BoundFrame_t BoundFrame;

						switch (pModel->modelType)
						{			
							case MODTYPE_MD3:

								// just write out the original one from when the model was loaded in...
								//
								BoundFrame = pModel->pMD3BoundFrames[iFrame];
								break;

							case MODTYPE_MDR:

								if (iFrame==0)						
									pBoundFrames = new md3BoundFrame_t[Header.iNumFrames];

								// and setup this entry ready for freeze-code boundary-determiner later...
								//
								ClearBounds(pBoundFrames[iFrame].bounds[0], pBoundFrames[iFrame].bounds[1]);

								// for the moment, just write out zeroes...
								//
								memset(&BoundFrame,0,sizeof(BoundFrame));
								break;

							default:
								ErrorBox("casedefault: Unhandled model type");
								goto error;
						}

						fwrite(&BoundFrame, sizeof(BoundFrame),1,fHandle);
					}
			OUTPOS();

					// write tags... (MDR already uses MD3 tags in this app)
					//
					for (iFrame = 0; iFrame<Header.iNumFrames; iFrame++)
					{
						for (int iTag = 0; iTag<Header.iNumTags; iTag++)
						{
							md3Tag_t* pTag = &pModel->tags[iFrame][iTag];

							fwrite(pTag, sizeof(*pTag),1,fHandle);
						}
					}

			OUTPOS();
					// write meshes...
					//
					for (int iMesh = 0; iMesh<Header.iNumMeshes; iMesh++)
					{
						gl_mesh		 *pMesh		= NULL;	// for writing from MD3 source
						md4Surface_t *pSurface	= NULL;	//  "     "      "  MDR   "

						typedef struct
						{
							char	ID[4];
							char	sName[68];
							int		iNumMeshFrames;
							int		iNumSkins;
							int		iNumVertices;
							int		iNumTriangles;
							int		iOffsetToTriangles;
							int		iHeadSize;
							int		iOffsetToTexVecs;
							int		iOffsetToVertices;
							int		iMeshSize;
						} MD3MeshHead, *lpMD3MeshHead;

						MD3MeshHead MeshHeader;

						strncpy(MeshHeader.ID,"IDP3",4);

						switch (pModel->modelType)
						{			
							case MODTYPE_MD3:

								pMesh = &pModel->pMeshes[ iMesh ];

								strcpy(MeshHeader.sName, pMesh->sName);
								MeshHeader.iNumMeshFrames	= bOneFrameMD3ExportHack ? 1 : pMesh->iNumMeshFrames;	// same as global count, but convenient
								MeshHeader.iNumSkins		= pMesh->iNumSkins;
								MeshHeader.iNumVertices		= pMesh->iNumVertices;
								MeshHeader.iNumTriangles	= pMesh->iNumTriangles;
								break;

							case MODTYPE_MDR:										
							{
								pSurface = (md4Surface_t *)( (byte *)pLOD + pLOD->ofsSurfaces );

								for (int iSurf=0; iSurf<iMesh; iSurf++)
								{
									pSurface = (md4Surface_t *)( (byte *)pSurface + pSurface->ofsEnd );
								}

								strcpy(MeshHeader.sName,	  pSurface->name);
								MeshHeader.iNumMeshFrames	= pModel->iNumFrames;	// same as global count, but convenient
								MeshHeader.iNumSkins		= 1;
								MeshHeader.iNumVertices		= pSurface->numVerts;
								MeshHeader.iNumTriangles	= pSurface->numTriangles;
								break;					
							}
							default:
								ErrorBox("casedefault: Unhandled model type");
								goto error;
						}

						typedef struct
						{
							short x,y,z,normal;
						}MD3MeshVert;

						MeshHeader.iOffsetToTriangles	= sizeof(MeshHeader) + MeshHeader.iNumSkins * sizeof(MeshHeader.sName);
						MeshHeader.iHeadSize			= sizeof(MeshHeader);
						MeshHeader.iOffsetToTexVecs		= MeshHeader.iOffsetToTriangles + (MeshHeader.iNumTriangles * sizeof(TriVec));
						MeshHeader.iOffsetToVertices	= MeshHeader.iOffsetToTexVecs   + (MeshHeader.iNumVertices  * sizeof(TexVec));
						MeshHeader.iMeshSize			= MeshHeader.iOffsetToVertices  + (MeshHeader.iNumVertices  * MeshHeader.iNumMeshFrames * sizeof(MD3MeshVert));

						fwrite(&MeshHeader,sizeof(MeshHeader),1,fHandle);

			OUTPOS();

						// this is one of these leftover crap things, AFAIK it's always 1, that's all that's stored anyway, so...
						//
						for (int iSkin=0; iSkin<MeshHeader.iNumSkins; iSkin++)
						{
							char sName[68];

							switch (pModel->modelType)
							{			
								case MODTYPE_MD3:
									
									strcpy(sName, pMesh->sTextureName);
									break;

								case MODTYPE_MDR:
									strcpy(sName, pSurface->shader);
									break;

								default:
									ErrorBox("casedefault: Unhandled model type");
									goto error;
							}

							fwrite(sName,sizeof(sName),1,fHandle);
						}
			OUTPOS();

						shaderCommands_t* pTess = NULL;

						// write triangles...
						//
						for (int iTri=0; iTri<MeshHeader.iNumTriangles; iTri++)
						{
							TriVec Tri;

							switch (pModel->modelType)
							{			
								case MODTYPE_MD3:
									
									memcpy(Tri,pMesh->triangles[iTri],sizeof(Tri));	// stupid compiler won't do an assign
									break;

								case MODTYPE_MDR:
									
									pTess = Freeze_Surface( pSurface, 0 );	// frame 0 is fine for working out tri indexes and (next) tex coords							

									memcpy(Tri,&pTess->indexes[iTri*3],sizeof(Tri));
									break;

								default:
									ErrorBox("casedefault: Unhandled model type");
									goto error;
							}
							
							put32(Tri[0],fHandle);
							put32(Tri[1],fHandle);
							put32(Tri[2],fHandle);
						}

			OUTPOS();

						// write tex coords...
						//
						for (int iVert=0; iVert<MeshHeader.iNumVertices; iVert++)
						{
							float u,v;

							switch (pModel->modelType)
							{			
								case MODTYPE_MD3:
									
									u = pMesh->textureCoord[iVert][0];
									v = pMesh->textureCoord[iVert][1];						
									break;

								case MODTYPE_MDR:

									assert(pTess);
									u = pTess->texCoords[iVert][0][0];
									v = pTess->texCoords[iVert][0][1];							
									break;

								default:
									ErrorBox("casedefault: Unhandled model type");
									goto error;
							}

							putFloat(u,fHandle);
							putFloat(v,fHandle);
						}

			OUTPOS();

						// actual mesh data...
						//
						for (int iFrame=0; iFrame<MeshHeader.iNumMeshFrames; iFrame++)
						{
							for (int iVert=0; iVert<MeshHeader.iNumVertices; iVert++)
							{
								short x,y,z,normal;

								switch (pModel->modelType)
								{			
									case MODTYPE_MD3:
										
										x = (short)((pMesh->meshFrames[iFrame].pVerts[iVert][0])*64);
										y = (short)((pMesh->meshFrames[iFrame].pVerts[iVert][1])*64);
										z = (short)((pMesh->meshFrames[iFrame].pVerts[iVert][2])*64);
										normal =	 pMesh->meshFrames[iFrame].pNormalIndexes[iVert];
										break;

									case MODTYPE_MDR:

										if (iVert==0)
											pTess = Freeze_Surface( pSurface, iFrame );

										x = (short)((pTess->xyz[iVert][0])*64);
										y = (short)((pTess->xyz[iVert][1])*64);
										z = (short)((pTess->xyz[iVert][2])*64);

										byte thing[2];
										NormalToLatLong( pTess->normal[iVert], thing );
										normal = *(short*)thing;	/////xxxxxxxxxxxxx this needs testing in-game
										
										// extra stuff needed for MDR source, we need to fill in the stuff about boundary frames
										//	here, which MD3 didn't need to do because it just writes out whatever it found when
										//	the model was loaded in...
										//
										assert(pBoundFrames);
										if (pBoundFrames)
										{
											AddPointToBounds (pTess->xyz[iVert], pBoundFrames[iFrame].bounds[0], pBoundFrames[iFrame].bounds[1]);
										}

										break;

									default:
										ErrorBox("casedefault: Unhandled model type");
										goto error;
								}

								put16(x,fHandle);
								put16(y,fHandle);
								put16(z,fHandle);
								put16(normal,fHandle);
							}
						}
			OUTPOS();
					}

					// if we were writing from an MDR source, we need to go back and write out the newly-generated boundary frames
					//	(writing from MD3s will have used the frames found during original load)
					//
					if (pBoundFrames)
					{
						// do some final processing...
						//
						for (int iFrame=0; iFrame<Header.iNumFrames; iFrame++)
						{
							pBoundFrames[iFrame].localOrigin[0] = 
							pBoundFrames[iFrame].localOrigin[1] = 
							pBoundFrames[iFrame].localOrigin[2] = 0;

							strcpy(pBoundFrames[iFrame].name,"MD3View_Raven");	// creator	( name[16] !!! )

							// work out largest radius from mins/maxs... (for remaining field in boundframe struct)
							//
							vec3_t tmpVec;
							float maxRadius = 0;

							for (int j=0; j<8; j++)
							{
								tmpVec[0] = pBoundFrames[iFrame].bounds[(j&1)!=0][0];
								tmpVec[1] = pBoundFrames[iFrame].bounds[(j&2)!=0][1];
								tmpVec[2] = pBoundFrames[iFrame].bounds[(j&4)!=0][2];

								if (VectorLength( tmpVec ) > maxRadius )
									maxRadius = VectorLength( tmpVec );
							}

							pBoundFrames[iFrame].radius = maxRadius;
						}

						// now write all boundframes out...
						//
						fseek(fHandle,lBoundFrameWriteSeekPos,SEEK_SET);
						fwrite(pBoundFrames,sizeof(md3BoundFrame_t),Header.iNumFrames,fHandle);
						delete [] pBoundFrames;
						pBoundFrames = NULL;				
					}
					
					// now go back and write in the file size...
					//		
					fseek(fHandle,offsetof(MD3Head,iFileSize), SEEK_SET);
					put32(filesize(fHandle),fHandle);
					fclose(fHandle);
				}
				else
				{
					ErrorBox(va("Error: Unable to write to '%s'!\n",sFilename));
				}
			}
			else
			{
				// exporting an MD3 model as a ghoul2 model, so we first need to ask how many LODs there are from the special LOD-models loaded,
				//	even though they aren't part of the same MD3 model...
				//
				int iActualNumLODs = iOutLODCount;
				if (pModel->pModel_LOD1) iActualNumLODs++;
				if (pModel->pModel_LOD2) iActualNumLODs++;
				LPCSTR psNameGLA = NULL;				

				iActualNumLODs = bG2ForceSingleLODExport?1:iActualNumLODs;

				char *psErrMess = ExportGhoul2FromMD3(sFilename, iActualNumLODs, pModel->iNumMeshes, pModel->iNumTags,
														&psNameGLA
														);
				if (!psErrMess)
				{
					FilesWritten.push_back(sFilename);
					FilesWritten.push_back(psNameGLA);
				}
				else
				{
					ErrorBox(psErrMess);
				}
			}
		}
	}
	
		EndWait();
		return true;

error:	
	EndWait();
	if (fHandle)
	{
		fclose(fHandle);
		fHandle = NULL;

		list <string>::iterator it;
		STL_ITERATE(it,FilesWritten)
		{
			remove((*it).c_str());
		}
	}

	return false;
}


// ghoul2 exported interface functions...
//
LPCSTR G2Exporter_Surface_GetName(int iSurfaceIndex)
{	
	// always assume LOD0 here, so ignore other loaded MD3 models...
	//
	switch (gpModel->modelType)
	{
		case MODTYPE_MD3:
		{
			if (iSurfaceIndex < (int)gpModel->iNumMeshes)
			{
				// standard surface...
				//
				gl_mesh *pMesh =&gpModel->pMeshes[iSurfaceIndex];
				return pMesh->sName;
			}
			else
			{
				// tag surface...
				//
				static char sTemp[1024];
				Tag* pTag = &gpModel->tags[0][iSurfaceIndex - gpModel->iNumMeshes];
				//
				// prepend name with a '*', and remove "tag_" from name start if present...
				//
				strcpy(sTemp,"*");
				strcat(sTemp, (!strnicmp(pTag->Name,"tag_",4)) ? &pTag->Name[4] : pTag->Name);
				return sTemp;
			}
		}

		default:
		{				
			static bool bAlreadyReported = false;
			if (!bAlreadyReported)
			{
				bAlreadyReported = true;
				assert(0);
				ErrorBox(va("G2Exporter_Surface_GetName(): Error! modeltype %d has no case handler!\n",gpModel->modelType));
			}
		}
		break;
	}

	return "Error";
}

LPCSTR G2Exporter_Surface_GetShaderName(int iSurfaceIndex)
{
	// always assume LOD0 here, so ignore other loaded MD3 models...
	//
	switch (gpModel->modelType)
	{
		case MODTYPE_MD3:
		{
			if (iSurfaceIndex < (int)gpModel->iNumMeshes)
			{
				// standard surface...
				//
				gl_mesh *pMesh =&gpModel->pMeshes[iSurfaceIndex];
				return pMesh->sTextureName;
			}
			else
			{
				// tag surface...
				//
				return "[NoMaterial]";
			}
		}

		default:
		{				
			static bool bAlreadyReported = false;
			if (!bAlreadyReported)
			{
				bAlreadyReported = true;
				assert(0);
				ErrorBox(va("G2Exporter_Surface_GetShaderName(): Error! modeltype %d has no case handler!\n",gpModel->modelType));
			}
		}
		break;
	}

	return "Error";
}


static gl_model *Model_AccountForLOD(gl_model* pModel, int iLOD)
{
	gl_model *pMod = pModel;

	switch (iLOD)
	{
		case 0:
			break;

		case 1:
			pMod = pMod->pModel_LOD1?pMod->pModel_LOD1:pMod;
			break;

		case 2:
			pMod = pMod->pModel_LOD2?pMod->pModel_LOD2:pMod;
			break;

		default:
			assert(0);
	}

	return pMod;
}


int G2Exporter_Surface_GetNumVerts(int iSurfaceIndex, int iLOD)
{
	gl_model* pModel = Model_AccountForLOD(gpModel, iLOD);

	switch (pModel->modelType)
	{
		case MODTYPE_MD3:
		{
			if (iSurfaceIndex < (int)gpModel->iNumMeshes)	// gpModel, not pModel, to cope with models of differing # meshes
			{
				if (iSurfaceIndex < (int)pModel->iNumMeshes)	// now check surface is legal for THIS LOD model...
				{
					// standard surface...
					//
					gl_mesh *pMesh =&pModel->pMeshes[iSurfaceIndex];
					return pMesh->iNumVertices;
				}
				else
				{
					// surface was a legal index for LOD 0, but not for this one, so return fake surface info...
					//
					return 1;
				}
			}
			else
			{
				// tag surface...
				//
				return 3;	// for one triangle
			}
		}

		default:
		{				
			static bool bAlreadyReported = false;
			if (!bAlreadyReported)
			{
				bAlreadyReported = true;
				assert(0);
				ErrorBox(va("G2Exporter_Surface_GetNumVerts(): Error! modeltype %d has no case handler!\n",pModel->modelType));
			}
		}
		break;
	}

	return 0;
}

int G2Exporter_Surface_GetNumTris(int iSurfaceIndex, int iLOD)
{
	gl_model* pModel = Model_AccountForLOD(gpModel, iLOD);

	switch (pModel->modelType)
	{
		case MODTYPE_MD3:
		{
			if (iSurfaceIndex < (int)gpModel->iNumMeshes)	// gpModel, not pModel, to cope with models of differing # meshes
			{
				if (iSurfaceIndex < (int)pModel->iNumMeshes)	// now check surface is legal for THIS LOD model...
				{			
					// standard surface...
					//
					gl_mesh *pMesh =&pModel->pMeshes[iSurfaceIndex];
					return pMesh->iNumTriangles;
				}
				else
				{
					// surface was a legal index for LOD 0, but not for this one, so return fake surface info...
					//
					return 1;
				}
			}
			else
			{
				// tag surface...
				//
				return 1;	// for one triangle
			}
		}

		default:
		{				
			static bool bAlreadyReported = false;
			if (!bAlreadyReported)
			{
				bAlreadyReported = true;
				assert(0);
				ErrorBox(va("G2Exporter_Surface_GetNumTris(): Error! modeltype %d has no case handler!\n",pModel->modelType));
			}
		}
		break;
	}

	return 0;
}

int G2Exporter_Surface_GetTriIndex(int iSurfaceIndex, int iTriangleIndex, int iTriVert, int iLOD)
{
	gl_model* pModel = Model_AccountForLOD(gpModel, iLOD);

	switch (pModel->modelType)
	{
		case MODTYPE_MD3:
		{
			if (iSurfaceIndex < (int)gpModel->iNumMeshes)	// gpModel, not pModel, to cope with models of differing # meshes
			{
				if (iSurfaceIndex < (int)pModel->iNumMeshes)	// now check surface is legal for THIS LOD model...
				{
					// standard surface...
					//
					gl_mesh *pMesh		= &pModel->pMeshes[iSurfaceIndex];
					TriVec	*pTriIndexes= &pMesh->triangles[iTriangleIndex];
					return pTriIndexes[0][iTriVert];
				}
				else
				{
					// surface was a legal index for LOD 0, but not for this one, so return fake surface info...
					//
					return 0;	// only one vert in a fake surface, so index is always zero
				}
			}
			else
			{
				// tag surface...
				//
				return iTriVert;
			}
		}

		default:
		{
			static bool bAlreadyReported = false;
			if (!bAlreadyReported)
			{
				bAlreadyReported = true;
				assert(0);
				ErrorBox(va("G2Exporter_Surface_GetTriIndex(): Error! modeltype %d has no case handler!\n",pModel->modelType));
			}
		}
		break;
	}

	return 0;
}


//////////////////////////////////////////////
//
// Some crap to get at Quake's weird tables simply to determine vert normals for MD3s...
//
#define FUNCTABLE_SIZE		1024
#define FUNCTABLE_SIZE2		10
#define FUNCTABLE_MASK		(FUNCTABLE_SIZE-1)
float	sinTable[FUNCTABLE_SIZE];
//float	squareTable[FUNCTABLE_SIZE];
//float	triangleTable[FUNCTABLE_SIZE];
//float	sawToothTable[FUNCTABLE_SIZE];
//float	inverseSawToothTable[FUNCTABLE_SIZE];
static void EnsureQuakeTablesSetup(void)
{
	static bool bDone = false;

	if (!bDone)
	{
		bDone = true;

		for ( int i = 0; i < FUNCTABLE_SIZE; i++ )
		{
			sinTable[i]		= sin( DEG2RAD( i * 360.0f / ( ( float ) ( FUNCTABLE_SIZE - 1 ) ) ) );
//			squareTable[i]	= ( i < FUNCTABLE_SIZE/2 ) ? 1.0f : -1.0f;
//			sawToothTable[i] = (float)i / FUNCTABLE_SIZE;
//			inverseSawToothTable[i] = 1.0 - tr.sawToothTable[i];
//
//			if ( i < FUNCTABLE_SIZE / 2 )
//			{
//				if ( i < FUNCTABLE_SIZE / 4 )
//				{
//					tr.triangleTable[i] = ( float ) i / ( FUNCTABLE_SIZE / 4 );
//				}
//				else
//				{
//					tr.triangleTable[i] = 1.0f - tr.triangleTable[i-FUNCTABLE_SIZE / 4];
//				}
//			}
//			else
//			{
//				tr.triangleTable[i] = -tr.triangleTable[i-FUNCTABLE_SIZE/2];
//			}
		}
	}
}

//////////////////////////////////////////////



vec3_t *G2Exporter_Surface_GetVertNormal(int iSurfaceIndex, int iVertIndex, int iLOD)
{
	gl_model* pModel = Model_AccountForLOD(gpModel, iLOD);

	static vec3_t v3={0};

	switch (pModel->modelType)
	{
		case MODTYPE_MD3:
		{
			if (iSurfaceIndex < (int)gpModel->iNumMeshes)	// gpModel, not pModel, to cope with models of differing # meshes
			{
				if (iSurfaceIndex < (int)pModel->iNumMeshes)	// now check surface is legal for THIS LOD model...
				{			
					// standard surface...
					//
					gl_mesh		*pMesh			= &pModel->pMeshes[iSurfaceIndex];
					short		*pNormalIndexes	= pMesh->meshFrames[0].pNormalIndexes;
					
					EnsureQuakeTablesSetup();	// heh, well-tacky.

					unsigned int lat, lng;				

					lat = ( pNormalIndexes[iVertIndex] >> 8 ) & 0xff;
					lng = ( pNormalIndexes[iVertIndex] & 0xff );
					lat *= (FUNCTABLE_SIZE/256);
					lng *= (FUNCTABLE_SIZE/256);

					// decode X as cos( lat ) * sin( long )
					// decode Y as sin( lat ) * sin( long )
					// decode Z as cos( long )

					v3[0] = sinTable[(lat+(FUNCTABLE_SIZE/4))&FUNCTABLE_MASK] * sinTable[lng];
					v3[1] = sinTable[lat] * sinTable[lng];
					v3[2] = sinTable[(lng+(FUNCTABLE_SIZE/4))&FUNCTABLE_MASK];
					
					if ( g_bPerfect )
					{	
						return &v3;
					}
					else
					{
						// 90-degree skew thing...
						//
						Matrix4 Swap;
								Swap.Identity();
						
						if (1)
						{
							Swap.SetRow(0,Vect3(0.0f,-1.0f,0.0f));
							Swap.SetRow(1,Vect3(1.0f,0.0f,0.0f));
						}
						Swap.CalcFlags();

						Vect3 v3In((const float *)v3);
						static Vect3 v3Out;
						Swap.XFormVect(v3Out,v3In);
						return (vec3_t*) &v3Out;
					}
				}
				else
				{
					// surface was a legal index for LOD 0, but not for this one, so return fake surface info...
					//
					memset(v3,0,sizeof(v3));
					return &v3;
				}
			}
			else
			{
				// tag surface...
				//
				// I don't think tag-surfaces normals have any meaning for ghoul2, so...
				//
				//TagFrame *pTag = &pModel->tags[0][iSurfaceIndex - pModel->iNumMeshes];
				memset(v3,0,sizeof(v3));
				return &v3;
			}
		}

		default:
		{
			static bool bAlreadyReported = false;
			if (!bAlreadyReported)
			{
				bAlreadyReported = true;
				assert(0);
				ErrorBox(va("G2Exporter_Surface_GetVertNormal(): Error! modeltype %d has no case handler!\n",pModel->modelType));
			}
		}
		break;
	}

	return &v3;
}

vec3_t *G2Exporter_Surface_GetVertCoords(int iSurfaceIndex, int iVertIndex, int iLOD)
{
	gl_model* pModel = Model_AccountForLOD(gpModel, iLOD);

	static vec3_t v3={0};
	switch (pModel->modelType)
	{
		case MODTYPE_MD3:
		{
			if (iSurfaceIndex < (int)gpModel->iNumMeshes)	// gpModel, not pModel, to cope with models of differing # meshes
			{
				if (iSurfaceIndex < (int)pModel->iNumMeshes)	// now check surface is legal for THIS LOD model...
				{			
					// standard surface...
					//
					gl_mesh *pMesh		= &pModel->pMeshes[iSurfaceIndex];
					Vec3	*pVerts		= &pMesh->meshFrames[0].pVerts[iVertIndex];

					if ( g_bPerfect )
					{
						return  pVerts;
					}
					else
					{					
						// 90-degree skew thing...
						//
						Matrix4 Swap;
								Swap.Identity();
						
						if (1)
						{
							Swap.SetRow(0,Vect3(0.0f,-1.0f,0.0f));
							Swap.SetRow(1,Vect3(1.0f,0.0f,0.0f));
						}
						Swap.CalcFlags();

						Vect3 v3In((const float *)pVerts);
						static Vect3 v3Out;
						Swap.XFormVect(v3Out,v3In);
						return (vec3_t*) &v3Out;
					}
				}
				else
				{
					// surface was a legal index for LOD 0, but not for this one, so return fake surface info...
					//
					memset(v3,0,sizeof(v3));
					return &v3;
				}
			}
			else
			{					
				// tag surface...
				//
				assert(iVertIndex<3);

				Tag* pTag = &pModel->tags[0][iSurfaceIndex - pModel->iNumMeshes];				
				vec3_t v3New;

				if ( g_bPerfect )
				{				
					v3New[0] = pTag->axis[0][iVertIndex] ;
					v3New[1] = pTag->axis[1][iVertIndex] ;
					v3New[2] = pTag->axis[2][iVertIndex] ;

					// don't worry about how this crap works, it just does (arrived at by empirical methods... :-)
					//
					//  (mega-thanks to Gil as usual)
					//
					if (iVertIndex==0)
					{
						VectorCopy(pTag->origin,v3);
					}
					else
					if (iVertIndex==1)
					{
						v3New[0] = pTag->axis[0][iG2_TRISIDE_MIDDLE];
						v3New[1] = pTag->axis[1][iG2_TRISIDE_MIDDLE];
						v3New[2] = pTag->axis[2][iG2_TRISIDE_MIDDLE];
						
						VectorAdd(pTag->origin,v3New,v3);
					}
					else
					{					
						v3New[0] = pTag->axis[0][iG2_TRISIDE_LONGEST];
						v3New[1] = pTag->axis[1][iG2_TRISIDE_LONGEST];
						v3New[2] = pTag->axis[2][iG2_TRISIDE_LONGEST];
						
						VectorSubtract(pTag->origin,v3New,v3);
					}
					return &v3;
				}
				else
				{
					// 90-degree skew thing...
					//
					v3New[0] = pTag->axis[0][iVertIndex] ;
					v3New[1] = pTag->axis[1][iVertIndex] ;
					v3New[2] = pTag->axis[2][iVertIndex] ;

					// don't worry about how this crap works, it just does (arrived at by empirical methods... :-)
					//
					//  (mega-thanks to Gil as usual)
					//
					if (iVertIndex==2)
					{
						VectorCopy(pTag->origin,v3);
					}
					else
					if (iVertIndex==1)
					{
						v3New[0] =   2.0f * pTag->axis[1][iG2_TRISIDE_MIDDLE];
						v3New[1] = -(2.0f * pTag->axis[0][iG2_TRISIDE_MIDDLE]);
						v3New[2] =   2.0f * pTag->axis[2][iG2_TRISIDE_MIDDLE];					
						
						VectorSubtract/*Add*/(pTag->origin,v3New,v3);
					}
					else
					{					
						v3New[0] = pTag->axis[1][iG2_TRISIDE_LONGEST];
						v3New[1] = -pTag->axis[0][iG2_TRISIDE_LONGEST];
						v3New[2] = pTag->axis[2][iG2_TRISIDE_LONGEST];
						
						VectorSubtract(pTag->origin,v3New,v3);
					}

	//				return (vec3_t*) &v3;

					Matrix4 Swap;
							Swap.Identity();
					
					if (1)
					{
						Swap.SetRow(0,Vect3(0.0f,-1.0f,0.0f));
						Swap.SetRow(1,Vect3(1.0f,0.0f,0.0f));
					}
					Swap.CalcFlags();

					Vect3 v3In((const float *)v3);
					static Vect3 v3Out;
					Swap.XFormVect(v3Out,v3In);

					return (vec3_t*) &v3Out;
				}
			}
		}

		default:
		{
			static bool bAlreadyReported = false;
			if (!bAlreadyReported)
			{
				bAlreadyReported = true;
				assert(0);
				ErrorBox(va("G2Exporter_Surface_GetVertCoords(): Error! modeltype %d has no case handler!\n",pModel->modelType));
			}
		}
		break;
	}

	return &v3;
}

vec2_t *G2Exporter_Surface_GetTexCoords(int iSurfaceIndex, int iVertIndex, int iLOD)
{
	gl_model* pModel = Model_AccountForLOD(gpModel, iLOD);

	static vec2_t v2={0};

	switch (pModel->modelType)
	{
		case MODTYPE_MD3:
		{
			if (iSurfaceIndex < (int)gpModel->iNumMeshes)	// gpModel, not pModel, to cope with models of differing # meshes
			{
				if (iSurfaceIndex < (int)pModel->iNumMeshes)	// now check surface is legal for THIS LOD model...
				{			
					// standard surface...
					//
					gl_mesh *pMesh		= &pModel->pMeshes[iSurfaceIndex];
					return	&pMesh->textureCoord[iVertIndex];
				}
				else
				{
					// surface was a legal index for LOD 0, but not for this one, so return fake surface info...
					//
					memset(v2,0,sizeof(v2));
					return &v2;
				}
			}
			else
			{					
				// tag surface...
				//
				// Texture coords aren't relevant for tag-surfaces, so...
				//				
				memset(v2,0,sizeof(v2));
				return &v2;
			}
		}

		default:
		{
			static bool bAlreadyReported = false;
			if (!bAlreadyReported)
			{
				bAlreadyReported = true;
				assert(0);
				ErrorBox(va("G2Exporter_Surface_GetTexCoords(): Error! modeltype %d has no case handler!\n",pModel->modelType));
			}
		}
		break;
	}

	return &v2;
}


// returns NULL for ok, else error message of problem...
//
const char *loadmesh( gl_mesh& newMesh , FILE* F )
{
	LPCSTR psErrMess = NULL;
	char	ID[5];

	fread(ID, 4,1,F);ID[4]=0;
	if (strcmp(ID,"IDP3")==0)
	{
		char		name[69];
		UINT32		triangleStart;
		UINT32		headerSize;
		UINT32		texvecStart;
		UINT32		vertexStart;
		UINT32		meshSize;

		fread(name,68,1,F);name[65]=0; //65 chars, 32-bit aligned == 68 chars in file
		strcpy(newMesh.sName,name);

		newMesh.iNumMeshFrames	= get32(F);
		newMesh.iNumSkins		= get32(F);
		newMesh.iNumVertices	= get32(F);
		newMesh.iNumTriangles	= get32(F);
		triangleStart			= get32(F);
		headerSize				= get32(F);
		texvecStart				= get32(F);
		vertexStart				= get32(F);
		meshSize				= get32(F);

		if ( ( headerSize == 108           ) && //header should be 108 bytes long
			 ( meshSize   >  triangleStart ) &&
			 ( meshSize   >  texvecStart   ) &&
			 ( meshSize   >  vertexStart   ) )
		{
			unsigned int i,j;
			char	texName[68];

			newMesh.meshFrames	  = new FMeshFrame[newMesh.iNumMeshFrames];
			newMesh.triangles	  = new TriVec	  [newMesh.iNumTriangles ];
			newMesh.textureCoord  = new TexVec    [newMesh.iNumVertices  ];
			newMesh.iterMesh	  = new Vec3	  [newMesh.iNumVertices  ];
			newMesh.bindings	  = new GLuint    [newMesh.iNumSkins     ];

			for (i=0;i<newMesh.iNumSkins;i++)
			{
				fread(texName,68,1,F);texName[65]=0;
				//65 chars, 32-bit aligned == 68 chars in file

				newMesh.bindings[ i ] = loadTexture(texName);
				
				if (!i)
					strcpy(newMesh.sTextureName,texName);
			}

			for (i=0;i<newMesh.iNumTriangles;i++)
			{
				newMesh.triangles[ i ][ 0 ] = get32(F);
				newMesh.triangles[ i ][ 1 ] = get32(F);
				newMesh.triangles[ i ][ 2 ] = get32(F);
			}

			for (i=0;i<newMesh.iNumVertices;i++)
			{
				newMesh.textureCoord[ i ][ 0 ] = getFloat(F);
				newMesh.textureCoord[ i ][ 1 ] = getFloat(F);
			}

//			UINT16 unknown;

			for (i=0;i<newMesh.iNumMeshFrames;i++)
			{

//				newMesh.meshFrames[ i ] = new Vec3[newMesh.iNumVertices];
				newMesh.meshFrames[ i ].pVerts			= new Vec3[newMesh.iNumVertices];
				newMesh.meshFrames[ i ].pNormalIndexes	= new short[newMesh.iNumVertices];

				for (j=0;j<newMesh.iNumVertices;j++)
				{
					// big hack, change later!
					newMesh.meshFrames[ i ].pVerts[ j ][0]  = ((float)get16(F)) / 64;
					newMesh.meshFrames[ i ].pVerts[ j ][1]  = ((float)get16(F)) / 64;
					newMesh.meshFrames[ i ].pVerts[ j ][2]  = ((float)get16(F)) / 64;
					newMesh.meshFrames[ i ].pNormalIndexes[j]= get16(F);	// not actually used by this viewer, but stored for later writing if nec.
				}
			}
			
			return NULL;//true;
		} 
		else
		{			
			static char sError[1024];
			strcpy(sError,"Error(s) loading mesh:\n\n");

			if ( headerSize != 108 )
			{			
				sprintf(sError+strlen(sError),"headerSize(%d) != 108\n",headerSize);
			}

			if ( meshSize <= triangleStart )
			{
				sprintf(sError+strlen(sError),"meshSize(%d) <= triangleStart(%d)\n",meshSize,triangleStart);
			}

			if ( meshSize <= texvecStart )
			{
				sprintf(sError+strlen(sError),"meshSize(%d) <= texvecStart(%d)\n",meshSize,texvecStart);
			}

			if ( meshSize <= vertexStart )
			{
				sprintf(sError+strlen(sError),"meshSize(%d) <= vertexStart(%d)\n",meshSize,vertexStart);
			}

			psErrMess = sError;
			return psErrMess;	//false;
		}
	} else 
	{
		psErrMess = va("Error loading mesh, found \"%s\" instead of \"IDP3\"!",ID);	
		return psErrMess;	//false;
	}
}



static qboolean R_LoadMD4( model_t *mod, void *buffer, const char *mod_name ) 
{
	int					i, j, k, l;
	md4Header_t			*pinmodel, *md4;
    md4Frame_t			*frame;
	md4LOD_t			*lod;
	md4Surface_t		*surf;
	md4Triangle_t		*tri;
	md4Vertex_t			*v;
	int					version;
	int					size;
///////////////////////////////////	shader_t			*sh;
	int					frameSize;
	md4Tag_t			*tag;


	pinmodel = (md4Header_t *)buffer;

	version = LittleLong (pinmodel->version);
	if (version != MD4_VERSION) 
	{
		Error( "R_LoadMD4: %s has wrong version (%i should be %i)\n", mod_name, version, MD4_VERSION);
		return qfalse;
	}

	mod->modelType = MODTYPE_MDR;
	size = LittleLong(pinmodel->ofsEnd);
	mod->dataSize += size;
	md4 = mod->md4 = (md4Header_t*) malloc ( size );

	memcpy( md4, buffer, LittleLong(pinmodel->ofsEnd) );

    LL(md4->ident);
    LL(md4->version);
    LL(md4->numFrames);
    LL(md4->numBones);
    LL(md4->numLODs);
    LL(md4->ofsFrames);	// this will be -ve for compressed models
    LL(md4->ofsLODs);
    LL(md4->numTags);
    LL(md4->ofsTags);
    LL(md4->ofsEnd);

	mod->numLods = md4->numLODs -1 ;	//copy this up to the model for ease of use - it wil get inced after this.

	if ( md4->numFrames < 1 ) 
	{
		Error ( "R_LoadMD4: %s has no frames\n", mod_name );
		return qfalse;
	}

	if (md4->ofsFrames<0)
	{
		// compressed model...
		//
		// (actually, forget the swapping, this is only for MD3View which is only for Wintel anyway)
	}
	else
	{
		// uncompressed model...
		//
    
		// swap all the frames
		frameSize = (int)( &((md4Frame_t *)0)->bones[ md4->numBones ] );
		for ( i = 0 ; i < md4->numFrames ; i++, frame++) 
		{
			frame = (md4Frame_t *) ( (byte *)md4 + md4->ofsFrames + i * frameSize );
    		frame->radius = LittleFloat( frame->radius );
			for ( j = 0 ; j < 3 ; j++ ) {
				frame->bounds[0][j] = LittleFloat( frame->bounds[0][j] );
				frame->bounds[1][j] = LittleFloat( frame->bounds[1][j] );
	    		frame->localOrigin[j] = LittleFloat( frame->localOrigin[j] );
			}
			for ( j = 0 ; j < (int) (md4->numBones * sizeof( md4Bone_t ) / 4) ; j++ ) 
			{
				((float *)frame->bones)[j] = LittleFloat( ((float *)frame->bones)[j] );
			}
		}
	}

	// swap all the LOD's
	lod = (md4LOD_t *) ( (byte *)md4 + md4->ofsLODs );
	for ( l = 0 ; l < md4->numLODs ; l++) {

		// swap all the surfaces
		surf = (md4Surface_t *) ( (byte *)lod + lod->ofsSurfaces );
		for ( i = 0 ; i < lod->numSurfaces ; i++) {
			LL(surf->numTriangles);
			LL(surf->ofsTriangles);
			LL(surf->numVerts);
			LL(surf->ofsVerts);
			LL(surf->ofsEnd);
			LL(surf->ofsHeader);
			
			if ( surf->numVerts > SHADER_MAX_VERTEXES ) 
			{
				Error ("R_LoadMD4: %s has more than %i verts on a surface (%i)",
					mod_name, SHADER_MAX_VERTEXES, surf->numVerts );
				return qfalse;
			}
			if ( surf->numTriangles*3 > SHADER_MAX_INDEXES ) 
			{
				Error ("R_LoadMD4: %s has more than %i triangles on a surface (%i)",
						mod_name, SHADER_MAX_INDEXES / 3, surf->numTriangles );
				return qfalse;
			}
		
			// change to surface identifier
			surf->ident = SF_MD4;

			// lowercase the surface name so skin compares are faster
			strlwr( surf->name );

			// strip off a trailing _1 or _2
			// this is a crutch until i update carcass
			j = strlen( surf->name );
			if ( j > 2 && surf->name[j-2] == '_' ) {
				surf->name[j-2] = 0;
			}
/*
			// register the shaders
			sh = R_FindShader( surf->shader, LIGHTMAP_NONE, qtrue);
			if ( sh->defaultShader ) {
				surf->shaderIndex = 0;
			} else {
				surf->shaderIndex = sh->index;
			}
*/
			surf->shaderIndex = loadTexture(surf->shader);

			// swap all the triangles
			tri = (md4Triangle_t *) ( (byte *)surf + surf->ofsTriangles );
			for ( j = 0 ; j < surf->numTriangles ; j++, tri++ ) {
				LL(tri->indexes[0]);
				LL(tri->indexes[1]);
				LL(tri->indexes[2]);
			}

			// swap all the vertexes
			v = (md4Vertex_t *) ( (byte *)surf + surf->ofsVerts );
			for ( j = 0 ; j < surf->numVerts ; j++ ) {
				v->normal[0] = LittleFloat( v->normal[0] );
				v->normal[1] = LittleFloat( v->normal[1] );
				v->normal[2] = LittleFloat( v->normal[2] );

				v->texCoords[0] = LittleFloat( v->texCoords[0] );
				v->texCoords[1] = LittleFloat( v->texCoords[1] );

				v->numWeights = LittleLong( v->numWeights );

				for ( k = 0 ; k < v->numWeights ; k++ ) {
					v->weights[k].boneIndex = LittleLong( v->weights[k].boneIndex );
					v->weights[k].boneWeight = LittleFloat( v->weights[k].boneWeight );
				   v->weights[k].offset[0] = LittleFloat( v->weights[k].offset[0] );
				   v->weights[k].offset[1] = LittleFloat( v->weights[k].offset[1] );
				   v->weights[k].offset[2] = LittleFloat( v->weights[k].offset[2] );
				}
				v = (md4Vertex_t *)&v->weights[v->numWeights];
			}

			// find the next surface
			surf = (md4Surface_t *)( (byte *)surf + surf->ofsEnd );
		}

		// find the next LOD
		lod = (md4LOD_t *)( (byte *)lod + lod->ofsEnd );
	}
	tag = (md4Tag_t *) ( (byte *)md4 + md4->ofsTags );
	for ( i = 0 ; i < md4->numTags ; i++) {
		LL(tag->boneIndex);
		tag++;
	}

	return qtrue;
}

void AxisClear( vec3_t axis[3] ) {
	axis[0][0] = 1;
	axis[0][1] = 0;
	axis[0][2] = 0;
	axis[1][0] = 0;
	axis[1][1] = 1;
	axis[1][2] = 0;
	axis[2][0] = 0;
	axis[2][1] = 0;
	axis[2][2] = 1;
}

void R_GetAnimTag( md4Header_t *mod, int framenum, int iTagIndex/*const char *tagName*/ ,md3Tag_t * dest) 
{
	int				i;
	int				frameSize;
	md4CompFrame_t	*cframe;
	md4Frame_t		*frame;
	md4Tag_t		*tag;

	if ( framenum >= mod->numFrames ) 
	{
		// it is possible to have a bad frame while changing models, so don't error
		framenum = mod->numFrames - 1;
	}

	tag = (md4Tag_t *)((byte *)mod + mod->ofsTags);
	for ( i = 0 ; i < mod->numTags ; i++, tag++ )
	{
		if (iTagIndex == i)
//		if ( !strcmp( tag->name, tagName ) )
		{
			strcpy(dest->name,tag->name);

			if (mod->ofsFrames <0)
			{
				// compressed model...
				//
				frameSize = (int)( &((md4CompFrame_t *)0)->bones[ mod->numBones ] );
				cframe = (md4CompFrame_t *)((byte *)mod + (-mod->ofsFrames) + framenum * frameSize );

				md4Bone_t Bone;

				MC_UnCompress(Bone.matrix, cframe->bones[tag->boneIndex].Comp);

				VectorCopy(&Bone.matrix[0][0], dest->axis[0] );
				VectorCopy(&Bone.matrix[1][0], dest->axis[1] );
				VectorCopy(&Bone.matrix[2][0], dest->axis[2] );

				dest->origin[0]=Bone.matrix[0][3];
				dest->origin[1]=Bone.matrix[1][3];
				dest->origin[2]=Bone.matrix[2][3];
				
				return;
			}
			else
			{
				// uncompressed model...
				//
				frameSize = (int)( &((md4Frame_t *)0)->bones[ mod->numBones ] );
				frame = (md4Frame_t *)((byte *)mod + mod->ofsFrames + framenum * frameSize );
	#if 1
				VectorCopy(&frame->bones[tag->boneIndex].matrix[0][0], dest->axis[0] );
				VectorCopy(&frame->bones[tag->boneIndex].matrix[1][0], dest->axis[1] );
				VectorCopy(&frame->bones[tag->boneIndex].matrix[2][0], dest->axis[2] );
	#else //transpose rot part
				{
					int j,k;
					for (j=0;j<3;j++)
					{
						for (k=0;k<3;k++)
							dest->axis[j][k]=frame->bones[tag->boneIndex].matrix[k][j];
					}
				}
	#endif
				dest->origin[0]=frame->bones[tag->boneIndex].matrix[0][3];
				dest->origin[1]=frame->bones[tag->boneIndex].matrix[1][3];
				dest->origin[2]=frame->bones[tag->boneIndex].matrix[2][3];				
				return;
			}
		}
	}
	AxisClear( dest->axis );
	VectorClear( dest->origin );
	strcpy(dest->name,"");
}

bool	loadmodel( gl_model& newModel , FILE* F, char *filename )
{
	// some generic stuff...
	//
	newModel.modelType		= MODTYPE_BAD;
	newModel.modelListPosition = NULL;
	newModel.parent			= NULL;
	newModel.currentFrame	= 0;
	newModel.isBlended		= false;
	newModel.blendParam1	= GL_ONE;
	newModel.blendParam2	= GL_ONE;

	// now work out the model type...

	char	ID[5];

	ID[0] = fgetc(F);
	ID[1] = fgetc(F);
	ID[2] = fgetc(F);
	ID[3] = fgetc(F);
	ID[4] = 0;

	if (strcmp(ID,"IDP3")==0)
	{
		newModel.modelType = MODTYPE_MD3;

		char		name[69];
		UINT32		maxskinNum;
		UINT32		headerSize;
		UINT32		tagStart;
		UINT32		tagEnd;
		UINT32		fileSize;
		UINT32		version;

		version					= get32(F);

		fread(name,68,1,F);name[65]=0; //65 chars, 32-bit aligned == 68 chars in file

		newModel.iNumFrames		= get32(F);
		newModel.iNumTags		= get32(F);
		newModel.iNumMeshes		= get32(F);
		maxskinNum				= get32(F);
		headerSize				= get32(F);
		tagStart				= get32(F);
		tagEnd					= get32(F);
		fileSize				= get32(F);

		if ( ( headerSize == 108           ) && //header should be 108 bytes long
			 ( fileSize   >  tagStart      ) &&
			 ( fileSize   >=  tagEnd       ) &&
			 ( version    == 15            ) )
		{
			unsigned int i,j;

			//initialize matrix to identity matrix
			for (i=0;i<16;i++) 
				newModel.CTM_matrix[ i ] = 0;
			newModel.CTM_matrix[  1 ] = 1;
			newModel.CTM_matrix[  6 ] = 1;
			newModel.CTM_matrix[ 11 ] = 1;
			newModel.CTM_matrix[ 16 ] = 1;

			newModel.pMD3BoundFrames = new md3BoundFrame_t[newModel.iNumFrames];
			
			for (i=0;i<newModel.iNumFrames;i++)
			{
				newModel.pMD3BoundFrames[ i ].bounds[0][0] = getFloat(F);	// mins
				newModel.pMD3BoundFrames[ i ].bounds[0][1] = getFloat(F);
				newModel.pMD3BoundFrames[ i ].bounds[0][2] = getFloat(F);				
				newModel.pMD3BoundFrames[ i ].bounds[1][0] = getFloat(F);	// maxs
				newModel.pMD3BoundFrames[ i ].bounds[1][1] = getFloat(F);
				newModel.pMD3BoundFrames[ i ].bounds[1][2] = getFloat(F);
				newModel.pMD3BoundFrames[ i ].localOrigin[0] = getFloat(F);
				newModel.pMD3BoundFrames[ i ].localOrigin[1] = getFloat(F);
				newModel.pMD3BoundFrames[ i ].localOrigin[2] = getFloat(F);
				newModel.pMD3BoundFrames[ i ].radius 		= getFloat(F);
				
				fread(newModel.pMD3BoundFrames[ i ].name,16,1,F);
			}

			if (newModel.iNumTags!=0)
			{
				newModel.linkedModels = new gl_model*[newModel.iNumTags];
				for (i=0;i<newModel.iNumTags;i++)
					newModel.linkedModels[ i ] = NULL;

				newModel.tags = 
					new TagFrame[newModel.iNumFrames];

				for (i=0;i<newModel.iNumFrames;i++)
				{
					newModel.tags[ i ] = 
						new Tag[newModel.iNumTags];

					for (j=0;j<newModel.iNumTags;j++)
					{
//						fread(newModel.tags[ i ][ j ].Name   ,12,1,F);
//						fread(newModel.tags[ i ][ j ].unknown,52,1,F);
						fread(newModel.tags[ i ][ j ].Name,MAX_QPATH,1,F);

						newModel.tags[ i ][ j ].Position[ 0 ]		= getFloat(F);
						newModel.tags[ i ][ j ].Position[ 1 ]		= getFloat(F);
						newModel.tags[ i ][ j ].Position[ 2 ]		= getFloat(F);
						newModel.tags[ i ][ j ].Matrix[ 0 ][ 0 ]	= getFloat(F);
						newModel.tags[ i ][ j ].Matrix[ 1 ][ 0 ]	= getFloat(F);
						newModel.tags[ i ][ j ].Matrix[ 2 ][ 0 ]	= getFloat(F);
						newModel.tags[ i ][ j ].Matrix[ 0 ][ 1 ]	= getFloat(F);
						newModel.tags[ i ][ j ].Matrix[ 1 ][ 1 ]	= getFloat(F);
						newModel.tags[ i ][ j ].Matrix[ 2 ][ 1 ]	= getFloat(F);
						newModel.tags[ i ][ j ].Matrix[ 0 ][ 2 ]	= getFloat(F);
						newModel.tags[ i ][ j ].Matrix[ 1 ][ 2 ]	= getFloat(F);
						newModel.tags[ i ][ j ].Matrix[ 2 ][ 2 ]	= getFloat(F);
					}
				}
			} else
			{
				newModel.linkedModels	= NULL;
				newModel.tags			= NULL;
			}

			newModel.pMeshes =
				new gl_mesh[newModel.iNumMeshes];

			memset(newModel.pMeshes,0,sizeof(gl_mesh) * newModel.iNumMeshes);

			for (i=0;i<newModel.iNumMeshes;i++)
			{
				const char *psErrMess = loadmesh(newModel.pMeshes[ i ],F);
				if (psErrMess)
				{
					ErrorBox(va("Error in \"%s\", mesh \"%s\":\n\n%s\n",filename,newModel.pMeshes[ i ].sName,psErrMess));
					return false;
				}
			}

			return true;
		}
		else
		{
			// if we fail this early, we must get zero some counters or de-allocer will crash...
			//
			newModel.iNumFrames	= 0;
			newModel.iNumTags	= 0;
			newModel.iNumMeshes	= 0;
			ErrorBox(va("Bad header in MD3 file:\n\n\"%s\"\n",filename));
			return false;
		}
	}
	else 
	{		
		if (strcmp(ID,"RDM5")==0)	// Raven's .MDR format
		{
			newModel.modelType = MODTYPE_MDR;

			int iMallocSize = filesize(F);

			byte *pbLoadedFile = (byte *) malloc (iMallocSize);

			if (!pbLoadedFile)
			{
				Error ("Error mallocing %d bytes!\n",iMallocSize);
				return false;
			}

			rewind(F);
			fread(pbLoadedFile,1,iMallocSize,F);
			
			bool bReturn = !!R_LoadMD4( &newModel.Q3ModelStuff, pbLoadedFile, filename );

			if (bReturn)
			{
				newModel.iNumFrames = newModel.Q3ModelStuff.md4->numFrames;
				newModel.iNumTags	= newModel.Q3ModelStuff.md4->numTags;	

				if (newModel.iNumTags != 0)
				{
					newModel.linkedModels = new gl_model*[newModel.iNumTags];
					for (int i=0;i<(int)newModel.iNumTags;i++)
						newModel.linkedModels[ i ] = NULL;

					newModel.tags = new TagFrame[newModel.iNumFrames];

					for (int i=0;i<(int)newModel.iNumFrames;i++)
					{
						newModel.tags[ i ] = new Tag[newModel.iNumTags];

						md4Tag_t* tags = (md4Tag_t*)
											(
											((int) newModel.Q3ModelStuff.md4->ofsTags) +
											((int) newModel.Q3ModelStuff.md4)
											);
						
						for (int j=0;j<(int)newModel.iNumTags;j++)
						{
							R_GetAnimTag( newModel.Q3ModelStuff.md4, i, j , &newModel.tags[i][j]);
						}
					}
				} 
				else
				{
					newModel.linkedModels	= NULL;
					newModel.tags			= NULL;
				}
			}

			free(pbLoadedFile);

			return bReturn;			
		}
		else
		{
			ErrorBox(va("Unable to work out model format for file:\n\n\"%s\"\n",filename));
		}
	}
		return false;
}

/*
cleans up allocated model data
*/
void delete_gl_model( gl_model *model )
{
	unsigned int i, j, t;
	gl_mesh *mesh;	
	gl_model *linkModel, *parent;

	if (model == NULL) return;
		
	// delete tag and bone frame data

	if (model->tags)
	{
		for (i=0; i<model->iNumFrames;i++)
		{
			if (model->tags[i])
			{
				delete [] model->tags[i];
				model->tags[i] = NULL;
			}
		}						
	}

	if (model->tags) {
		delete [] model->tags; 
		model->tags = NULL;
	}

	if (model->pMD3BoundFrames) 
	{
		delete [] model->pMD3BoundFrames; 
		model->pMD3BoundFrames = NULL;
	}

	// remove all meshes
	for (i=0 ; i<model->iNumMeshes ; i++) {
		mesh = &model->pMeshes[i];

		if (mesh->triangles) {
			delete [] mesh->triangles; 
			mesh->triangles = NULL;
		}

		if (mesh->textureCoord) {
			delete [] mesh->textureCoord; mesh->textureCoord = NULL;
		}

		if (mesh->iterMesh) { 
			delete [] mesh->iterMesh; mesh->iterMesh = NULL;
		}

		// free textures from the mesh
		if (mesh->bindings) 
		{
			for (t=0 ; t<mesh->iNumSkins ; t++) 
				freeTexture( mesh->bindings[t] );

			delete [] mesh->bindings; mesh->bindings = NULL;
		}
		
		for (j=0 ; j<mesh->iNumMeshFrames ; j++) 
		{
			if (mesh->meshFrames[j].pVerts)
			{
				delete [] mesh->meshFrames[j].pVerts; 
				mesh->meshFrames[j].pVerts = NULL;
			}

			if (mesh->meshFrames[j].pNormalIndexes)
			{
				delete [] mesh->meshFrames[j].pNormalIndexes; 
				mesh->meshFrames[j].pNormalIndexes = NULL;
			}
		}

		if (mesh->meshFrames) {
			delete [] mesh->meshFrames;	mesh->meshFrames = NULL;
		}
	}

	if (model->pMeshes) {
		delete [] model->pMeshes; model->pMeshes = NULL;
	}
	

	// remove references from parent
	parent = model->parent;
	if (parent != NULL) {
		for (i=0 ; i<parent->iNumTags ; i++) {
			if (parent->linkedModels[i] == model) {
				parent->linkedModels[i] = NULL;
			}
		}
		model->parent = NULL;
	}


	if (model->pModel_LOD1)
	{
		delete_gl_model( model->pModel_LOD1 );
		model->pModel_LOD1 = NULL;
	}
	if (model->pModel_LOD2)
	{
		delete_gl_model( model->pModel_LOD2 );
		model->pModel_LOD2 = NULL;
	}
	
	
	if (model->linkedModels) {
		// remove references from children
		for (i=0 ; i<model->iNumTags ; i++) {
			linkModel = model->linkedModels[i];
			if (linkModel != NULL) {
			  	//linkModel->parent = NULL;
				delete_gl_model( linkModel );
			}
		}
	}

	if (!model->pModel_LOD0)	// only do this if we're not an MD3 LOD model
	{
		// remove the model from mdview.modelList
		if (model->modelListPosition) {
			mdview.modelList->remove( model->modelListPosition );
			if (model == mdview.baseModel) {
				mdview.baseModel = NULL;
			}
		}

		// remove all entries in the tag menu
		for (i=0 ; i<model->iNumTags ; i++) {
			tagMenu_remove( (GLMODEL_DBLPTR)&model->linkedModels[i] );
		}
	}

	// if it was an MDR file...
	//
	if (model->Q3ModelStuff.md4)
	{
		free(model->Q3ModelStuff.md4);
		model->Q3ModelStuff.md4 = NULL;  
	}

	delete model;
}

bool	loadmdx(gl_model& newModel, char* filename)
{
	FILE* F;
	if ( ( F = fopen( filename , "rb" ) ) != NULL ) 
	{
		bool bRet = loadmodel(newModel,F, filename);
		fclose(F);
		return bRet;
	} else
		return false;
}


/*
writes out the mesh to a raw file
*/
void write_baseModelToRaw( char *fname )
{

	gl_model *model = mdview.baseModel;
	gl_mesh		*mesh;
	int i, mesh_num;
	Vec3 *vecs;
	TriVec	*tris;	
	int tri_num, t, v, n;

	
	// open up file for writing
	FILE *rawfile = fopen( fname, "wt" );
	if (!rawfile) { 
		Debug("Could not write to raw file %s", fname);
		return;
	}

	// write out header
	fprintf(rawfile, "Object1\n");		
	mesh_num = model->iNumMeshes;		

	// for all the meshes in a model
	for (i=0 ; i<mesh_num; i++) 
	{		
		mesh	  = &model->pMeshes[i];				
	
		vecs	= mesh->meshFrames[model->currentFrame].pVerts;	
		tris	= mesh->triangles;		
		tri_num = mesh->iNumTriangles;				
				
		// write out all the triangles as 3 vertices
		for (t=0 ; t<tri_num ; t++) {
			for (v=0 ; v<3 ; v++) {
				for (n=0 ; n<3 ; n++) {
					fprintf(rawfile, "%f ", vecs[tris[t][v]][n] );
				}				
			}
			fprintf(rawfile, "\n");
		}
	}

	fclose(rawfile);
}


