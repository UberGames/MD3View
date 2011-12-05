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
#include "animation.h"
#include "text.h"	
#include "oddbits.h"

MDViewState mdview;

void draw_stats();	

/*
renders the view which so far is just a model frame
*/
void render_mdview()
{
	//draw_view();	
	draw_viewSkeleton();
	draw_stats();	
}

void reset_viewpos(void)
{
	mdview.xPos = mdview.yPos = 0.0f;
	mdview.zPos = -2.f;
    mdview.rotAngleX = mdview.rotAngleY = 0;
	mdview.rotAngleZ = -90.0f;
}

/*
initializes the viewer global state data
*/
void init_mdview(const char* lpcommandline)
{
	mdview.dFOV = 90.0f;
	mdview.iLODLevel = 0;
	mdview.iSkinNumber = 0;
	mdview.bAxisView = false;
	reset_viewpos();
	mdview.animSpeed = 0.1;	// so 1/this = 10 = 10FPS
	mdview.timeStamp1 = getDoubleTime();
	mdview.texMode = TEX_FILTERED;
	mdview.faceSide = GL_CCW;
	mdview.animate = false;
	mdview.interpolate = true;
	mdview.bUseAlpha = false;	// default this to OFF since artists keep making 32 bit textures with no alpha, then complain about pieces missing!!!!
	mdview.bBBox = false;
	mdview._R = mdview._G = mdview._B = 256/5;	// dark grey
	mdview.bAnimCFGLoaded = false;
	mdview.bAnimIsMultiPlayerFormat = false;

	// allocate texture resource manager
	mdview.textureRes = new NodeDictionaryInfo( new StrKeyComparatorInfo() );
	mdview.topTextureBind = 0;

	// allocate model list
	mdview.modelList = new NodeSequenceInfo();

//	strcpy( mdview.basepath, "/");

	if ( lpcommandline != NULL && lpcommandline[0] != '\0' ) { //got an initial dir 
		char	szDirName[256]={0};
		strcpy (szDirName, lpcommandline );
		char *p = strrchr(szDirName,'\\');	//back up off the filename.
		if (p) {
			*p=0;
		}
		SetCurrentDirectory( szDirName);	//this will make future open dialogs in the same dir as the command line file
		strcpy( mdview.basepath, szDirName);
	} else {
#if 1
		strcpy( mdview.basepath, "w:\\game\\base\\models\\");	// this is just to default the 1st use of OpenFileDialog, and doesn't affect anything else once the app gets going
		SetCurrentDirectory( mdview.basepath );
#else
		char	szDirName[256]={0};
		GetModuleFileName(NULL, szDirName, 256);
		char *p = strrchr(szDirName,'\\');	//back up off the filename.
		if (p) {
			*p=0;
		}
		SetCurrentDirectory( szDirName );
		strcpy( mdview.basepath, szDirName);
#endif
	}
}



void	delete_gl_model( gl_model *model );
bool	loadmdx(gl_model& newModel, char* filename);

/* ------------------------------------------------ loading code -------------------------------------------------- */

/*
loads up md3 data
*/
int giTagMenuSubtractValue_Torso;
int giTagMenuSubtractValue_Head;
int giTagMenuSubtractValue_Weapon;	// this shit gets worse by the minute...
int giTagMenuSubtractValue_Barrel;
int giTagMenuSubtractValue_Barrel2;
int giTagMenuSubtractValue_Barrel3;
int giTagMenuSubtractValue_Barrel4;
gl_model* pLastLoadedModel;

// if psLOD0Filename != NULL, then we're loading an MD3 LOD model, and psLOD0Filename is a pointer to original name
//
bool loadmdl_original( char *filename, LPCSTR psLOD0Filename, gl_model* pLOD1, gl_model* pLOD2 )
{	
	gl_model* model = new gl_model; memset( model, 0, sizeof(gl_model) );

	if (pLOD1)
	{
		model->pModel_LOD1 = pLOD1;
		pLOD1->pModel_LOD0 = model;
	}

	if (pLOD2)
	{
		model->pModel_LOD2 = pLOD2;
		pLOD2->pModel_LOD0 = model;
	}

	// if there's a default skin for this model, don't try and load the internal shaders, this is just so
	//	we don't get bugged by loads of file-missing errors if the model is only designed to use skin files...
	//
	bool bDefaultSkinExists = file_exists(SkinName_FromPathedModelName(filename));

	if (bDefaultSkinExists)
	{
		gbIgnoreTextureLoad = true;
	}

	if (!loadmdx( *model, filename )) 
	{
		gbIgnoreTextureLoad = false;
		delete_gl_model( model );model = NULL;
		return false;
	}
	gbIgnoreTextureLoad = false;
	pLastLoadedModel = model;

	strcpy( model->sHeadSkinName,"default");
	strcpy( model->sMDXFullPathname, filename);
	strcpy(	model->sMD3BaseName,Filename_WithoutPath(
//													Filename_WithoutExt(
																		filename
//																		)
													)
			);

	if (psLOD0Filename)	
		return true;	// don't bother with the rest of this stuff now...

	giTagMenuSubtractValue_Torso  =
	giTagMenuSubtractValue_Head	  =
	giTagMenuSubtractValue_Weapon =
	giTagMenuSubtractValue_Barrel =
	giTagMenuSubtractValue_Barrel2=
	giTagMenuSubtractValue_Barrel3=
	giTagMenuSubtractValue_Barrel4= -1;
	
	Model_ApplySkin(model, SkinName_FromPathedModelName(model->sMDXFullPathname));	

	model->modelListPosition = mdview.modelList->insertLast( model );
	mdview.baseModel = (gl_model *)mdview.modelList->first()->element();

	tagMenu_seperatorAppend( filename );
	for (unsigned int i=0 ; i<model->iNumTags ; i++) 
	{
		tagMenu_append( model->tags[0][i].Name, (GLMODEL_DBLPTR)&model->linkedModels[i] );
		if (!stricmp(model->tags[0][i].Name,"tag_torso"))
		{
			giTagMenuSubtractValue_Torso = i;
		}
		else
		if (!stricmp(model->tags[0][i].Name,"tag_head"))
		{
			giTagMenuSubtractValue_Head	= i;
		}
		else
		if (!stricmp(model->tags[0][i].Name,"tag_weapon"))
		{
			giTagMenuSubtractValue_Weapon = i;
		}
		else
		if (!stricmp(model->tags[0][i].Name,"tag_barrel"))
		{
			giTagMenuSubtractValue_Barrel = i;
		}
		else
		if (!stricmp(model->tags[0][i].Name,"tag_barrel2"))
		{
			giTagMenuSubtractValue_Barrel2 = i;
		}
		else
		if (!stricmp(model->tags[0][i].Name,"tag_barrel3"))
		{
			giTagMenuSubtractValue_Barrel3 = i;
		}
		else
		if (!stricmp(model->tags[0][i].Name,"tag_barrel4"))
		{
			giTagMenuSubtractValue_Barrel4 = i;
		}
	}

	// arrrghhh!!!!...
	//
	giTagMenuSubtractValue_Torso	= model->iNumTags - giTagMenuSubtractValue_Torso;
	giTagMenuSubtractValue_Head		= model->iNumTags - giTagMenuSubtractValue_Head;
	giTagMenuSubtractValue_Weapon	= (giTagMenuSubtractValue_Weapon ==-1)?-1:model->iNumTags - giTagMenuSubtractValue_Weapon;
	giTagMenuSubtractValue_Barrel	= (giTagMenuSubtractValue_Barrel ==-1)?-1:model->iNumTags - giTagMenuSubtractValue_Barrel;
	giTagMenuSubtractValue_Barrel2	= (giTagMenuSubtractValue_Barrel2==-1)?-1:model->iNumTags - giTagMenuSubtractValue_Barrel2;
	giTagMenuSubtractValue_Barrel3	= (giTagMenuSubtractValue_Barrel3==-1)?-1:model->iNumTags - giTagMenuSubtractValue_Barrel3;
	giTagMenuSubtractValue_Barrel4	= (giTagMenuSubtractValue_Barrel4==-1)?-1:model->iNumTags - giTagMenuSubtractValue_Barrel4;
	//
	// this value is now the value (if NZ) to sub from the menu count (then add ID_TAG_START) to reach "tag_torso"ID of the menu item
	//
	SetFaceSkin(0);	// safety to always do this

	return true;
}


// this now auto-loads LOD models for MD3s...
//
bool loadmdl( char *filename )
{
	gl_model* pModel_LOD1 = NULL;
	gl_model* pModel_LOD2 = NULL;

	// new stuff, if it's an MD3, try loading in LOD versions of it as well, but do them first, so that
	//	all the goofy shit to do with tagmenu stuff is still left in valid state...
	//
	LPCSTR psMD3EXT = strstr(String_ToLower(filename),".md3");
	if (psMD3EXT)
	{
		char sNewFilename[MAX_PATH];

		strcpy(sNewFilename,Filename_WithoutExt(filename));
		strcat(sNewFilename,"_1.md3");
		if (loadmdl_original(sNewFilename, filename, NULL, NULL))
			pModel_LOD1 = pLastLoadedModel;

		strcpy(sNewFilename,Filename_WithoutExt(filename));
		strcat(sNewFilename,"_2.md3");
		if (loadmdl_original(sNewFilename, filename, NULL, NULL))
			pModel_LOD2 = pLastLoadedModel;
	}

	// finally, load the model we were originally asking for...
	//
	return loadmdl_original(filename, NULL, pModel_LOD1, pModel_LOD2);
}



/*
loads a model into the slot pointed to by dblptr
*/
void loadmdl_totag( char *fullName, GLMODEL_DBLPTR dblptr )
{
	gl_model **m_dblptr = (gl_model **)dblptr;
	gl_model *newmodel;
	
	NodePosition pos;
	if (loadmdl( fullName )) {
		pos = mdview.modelList->last();
		newmodel = (gl_model *)pos->element();
		*m_dblptr = newmodel;		
	}
}


/*
loads a new skin
*/
void importNewSkin( char *fullName )
{
/*
	GLuint newTexBind = loadTexture( fullName );
	NodePosition pos;
	gl_model *model;
	gl_mesh *mesh;
	int iNumMeshes,i;
	unsigned int j;

	for (pos=mdview.modelList->first() ; pos!=NULL ; pos=mdview.modelList->after(pos)) {
		model = (gl_model *)pos->element();
		iNumMeshes = model->iNumMeshes;
		for (i=0 ; i<iNumMeshes ; i++) {
			mesh = &model->meshes[i];
			for (j=0 ; j<mesh->skinNum ; j++) {
				mesh->bindings[j] = newTexBind;
			}
		}
	}
*/
	ApplyNewSkin(fullName);
}


bool ParseSequenceLockFile(LPCSTR psFilename)
{
	#define UL_BAD   0
	#define UL_UPPER 1
	#define UL_LOWER 2
	int iUpperOrLower = UL_BAD;

	FILE *fHandle = fopen(psFilename,"rt");

	if (fHandle)
	{
		// file is text file, 
		// first line is "upper" or "lower" (ignore case), then sequence lock names till empty line or EOF		

		static char sLineBuffer[2048];	

		int iMembers = 0;

		MultiSequenceLock_t MultiSequenceLock;
		MultiSequenceLock.clear();

		while (1)
		{
			ZEROMEM(sLineBuffer);
			if (!fgets(sLineBuffer,sizeof(sLineBuffer),fHandle))
			{
				if (ferror(fHandle))
				{
					ErrorBox(va("Error while reading \"%s\"!",psFilename));
					iUpperOrLower = UL_BAD;
				}
				break;	// whether error or EOF
			}

			// zap any comments...
			//
			char *p = strstr(sLineBuffer,"//");
			if (p)
				*p=0;

			strcpy(sLineBuffer,String_ToUpper(String_LoseWhitespace(sLineBuffer)));

			if (strlen(sLineBuffer))
			{
				// analyse this line...
				//
				if (!iMembers)
				{
					if (strnicmp(sLineBuffer,"upper",5)==0)
					{
						iUpperOrLower = UL_UPPER;
					}
					else
					if (strnicmp(sLineBuffer,"lower",5)==0)
					{
						iUpperOrLower = UL_LOWER;
					}
					else
					{
						ErrorBox("First line of file must be \"upper\" or \"lower\"!");
						iUpperOrLower = UL_BAD;
						break;
					}
				}
				else
				{
					// is this string a valid sequence for upper/lower?...
					//
					int iCount = (iUpperOrLower == UL_UPPER)?Animation_GetNumUpperSequences():Animation_GetNumLowerSequences();

					int iScanIndex;
					for (iScanIndex=0; iScanIndex<iCount; iScanIndex++)
					{
						Sequence_t* pSeq = (iUpperOrLower == UL_UPPER)?Animation_GetUpperSequence(iScanIndex):Animation_GetLowerSequence(iScanIndex);

						if (strcmp(pSeq->sName.c_str(),sLineBuffer)==0)
						{
							// found this line, it's valid, so keep it...
							//
							MultiSequenceLock.push_back(iScanIndex);
							break;
						}
					}
					if (iScanIndex==iCount)
					{
						ErrorBox(va("Couldn't find specified sequence \"%s\" for model \"%s\"",sLineBuffer,(iUpperOrLower == UL_UPPER)?"UPPER":"LOWER"));
						iUpperOrLower = UL_BAD;
						break;
					}
				}
				iMembers++;
			}
		}

		if (iUpperOrLower != UL_BAD // avoid giving an error because of an existing error
			&& iMembers == 1)	// ie seq file only contains "upper" or "lower"
		{
			ErrorBox("No enum entries found in file");
			iUpperOrLower = UL_BAD;
		}

		switch (iUpperOrLower)
		{
			case UL_BAD:	

				// do nothing
				//
				break;

			case UL_UPPER:	
				
				MultiSequenceLock_Upper.clear(); 
				MultiSequenceLock_Upper = MultiSequenceLock;									
				iAnimLockNumber_Upper = -1;	
				break;

			case UL_LOWER:	
				
				MultiSequenceLock_Lower.clear();
				MultiSequenceLock_Lower = MultiSequenceLock;
				iAnimLockNumber_Lower = -1;
				break;

			default:		// I must have forgotten some new type
				assert(0);					
				break;	
		}			

		fclose(fHandle);
	}
	else
	{
		ErrorBox(va("Error opening file \"%s\"",psFilename));
	}	

	return (iUpperOrLower != UL_BAD);
}


/* ------------------------------------------ delete code ------------------------------------------------------- */


/*
frees all mdviewdata, and effectively resets the data state back to init
*/
void free_mdviewdata()
{
	NodePosition pos;
	gl_model *model;

	/*
	for(pos = mdview.modelList->first() ; pos!=NULL ; pos=next) {		
		model = (gl_model *)pos->element();
		// takes care of removing model from the list and everything, so assume pos is invalid		
		delete_gl_model( model );

		// get next after cuz the list might have been changed by delete_gl_model
		next = mdview.modelList->after(pos);
	}
	*/


	if (!mdview.modelList->isEmpty()) {
		do {
			pos = mdview.modelList->first();	

			model = (gl_model *)pos->element();
			delete_gl_model( model ); // this removes its own position from the list

		} while (mdview.modelList->first()!=NULL);
	}

	// if there is anything else left in texture res remove, 
	// though deleting all gl_models in list should make it empty
	freeTextureRes();	

	ClearAnimationCFG();
}


/*
frees a model that has been loaded before by loadmdl_totag into the modelptr slot
*/
void freemdl_fromtag( GLMODEL_DBLPTR modelptr )
{
	gl_model **m_dblptr = (gl_model **)modelptr;
	gl_model *model = *m_dblptr;
	
	if (!model) return;
	
	delete_gl_model( model );
	
	*m_dblptr = NULL;		
}

/*
frees data as for shutdown, call this only as the last thing before exiting or bad thigns will happen
*/
void shutdown_mdviewdata()
{
	free_mdviewdata();
	delete mdview.modelList; mdview.modelList = NULL;
	delete mdview.textureRes; mdview.textureRes = NULL;
}







int iTextX;
int iTextY;

int giTotVerts;
int giTotTris;
int giModelsStatted;

void stat_gl_model( gl_model *__model, char *psAttachedVia )
{
#define ARB_NAME_PADDING 26
#define ARB_LOD_PADDING 9
#define ARB_ATTACHNAME_PADDING 24
#define ARB_VERTINFO_PADDING 15

	gl_model* model = __model;

	if (model)
	{
		giModelsStatted++;

		if (model->modelType == MODTYPE_MD3)
		{
			switch (mdview.iLODLevel)
			{
				case 0:	break;
				case 1: model = model->pModel_LOD1;	break;
				case 2: model = model->pModel_LOD2; break;
			}
	
			if (!model)
			{
				Text_DisplayFlat(	va("%s", String_EnsureMinLength("( LOD model missing )",ARB_NAME_PADDING)),
									iTextX, iTextY, 
									0, 255,0,		// RGB
									false
								);

				iTextY += TEXT_DEPTH;
				return;
			}
		}
	}

	int iNextX = 0;
	if (model)
	{
		iNextX = 
		Text_DisplayFlat(	va("%s Frame %4d/%4d", 
								String_EnsureMinLength(va("\"%s\"",model->sMD3BaseName),ARB_NAME_PADDING), 							
								model->currentFrame, model->iNumFrames),	
							iTextX, iTextY, 
							0, 255,0,		// RGB
							false
						);
	}
	else
	{
		iTextY += TEXT_DEPTH;	// seperate totals a bit

		iNextX = 
						//  va("%s Frame 1234/1234"
		Text_DisplayFlat(	va("%s                ", String_EnsureMinLength("( totals )",ARB_NAME_PADDING)),
							iTextX, iTextY, 
							0, 255,0,		// RGB
							false
						);
	}


	if (model)
	{
		// LODs info...
		//
		if (model->modelType == MODTYPE_MDR)
		{
			md4LOD_t *pLOD = (md4LOD_t *) ((byte *)model->Q3ModelStuff.md4 + model->Q3ModelStuff.md4->ofsLODs);
			int iWhichLod = mdview.iLODLevel;

			if (iWhichLod >= model->Q3ModelStuff.md4->numLODs)
				iWhichLod  = model->Q3ModelStuff.md4->numLODs-1;				

			iNextX = Text_DisplayFlat(	String_EnsureMinLength(va("(LOD %1d/%1d)",iWhichLod+1,model->Q3ModelStuff.md4->numLODs),ARB_LOD_PADDING),
										iNextX+(2*TEXT_WIDTH), iTextY, 
										255/2,255,255/2,		// RGB
										false
									);
		}
		else
		{
			iNextX = Text_DisplayFlat(	String_EnsureMinLength(va("(LOD %1d)",mdview.iLODLevel+1),ARB_LOD_PADDING),
										iNextX+(2*TEXT_WIDTH), iTextY, 
										255/2,255,255/2,		// RGB
										false
									);
		}
	}
	else
	{
		iNextX = Text_DisplayFlat(	String_EnsureMinLength("",ARB_LOD_PADDING),
									iNextX+(2*TEXT_WIDTH), iTextY, 
									255/2,255,255/2,		// RGB
									false
								);
	}

	if (model)
	{
		// Verts/tris info...
		//
		iNextX =
		Text_DisplayFlat(	String_EnsureMinLength(
													va("(V:%4d T:%4d)",model->iRenderedVerts,model->iRenderedTris),
													ARB_VERTINFO_PADDING
													),
							iNextX+(2*TEXT_WIDTH), iTextY, 
							255, 255/2, 255/2,		// RGB (pink)
							false
						);
		giTotVerts+= model->iRenderedVerts;
		giTotTris += model->iRenderedTris;
	}
	else
	{
		// Verts/tris info...
		//
		iNextX =
		Text_DisplayFlat(	String_EnsureMinLength(
													va("(V:%4d T:%4d)",giTotVerts,giTotTris),
													ARB_VERTINFO_PADDING
													),
							iNextX+(2*TEXT_WIDTH), iTextY, 
							255, 255/2, 255/2,		// RGB (pink)
							false
						);
	}


	if (model)
	{
		// attached-via info...
		//
		iNextX =
		Text_DisplayFlat(	String_EnsureMinLength(
													va("%s", psAttachedVia?va("(attached: \"%s\")",psAttachedVia):""),
													ARB_ATTACHNAME_PADDING
													),	
							iNextX+(2*TEXT_WIDTH), iTextY, 
							0, 255/2,0,		// RGB
							false
						);

		// print either the locked local frame number in red (if anim locking is on), or just the anim sequence name
		//	if there's one corresponding to this...
		//
		Sequence_t* pSeq = NULL;
		bool bIsUpper = false;
		if ((model == pModel_Upper || model->pModel_LOD0 == pModel_Upper ) && iAnimLockNumber_Upper)
		{
			pSeq = Animation_GetUpperSequence( iAnimLockNumber_Upper-1 );			
			bIsUpper = true;
		}
		if ((model == pModel_Lower || model->pModel_LOD0 == pModel_Lower ) && iAnimLockNumber_Lower)
		{
			pSeq = Animation_GetLowerSequence( iAnimLockNumber_Lower-1 );
			bIsUpper = false;
		}
		if (pSeq)
		{
			bool bWasMulti = false;
			if (pSeq->bMultiSeq)			
			{
				pSeq = GetMultiLockedSequenceFromFrame(model->currentFrame, bIsUpper );
				bWasMulti = true;
			}
			
			if (pSeq)
			{
				iNextX = Text_DisplayFlat(	va("%d/%d",model->currentFrame-pSeq->iTargetFrame,pSeq->iFrameCount),
									iNextX+(2*TEXT_WIDTH), iTextY, 
									255,				// R
									0,					// G
									bWasMulti?255:0,	// B	// multi displays as purple
									false
								);
			}
		}
		//else
		if (!pSeq)
		{
			if ((model == pModel_Upper || model->pModel_LOD0 == pModel_Upper ) && !iAnimLockNumber_Upper)
			{
				pSeq = Animation_FromUpperFrame( model->currentFrame );
			}
			if ((model == pModel_Lower || model->pModel_LOD0 == pModel_Lower ) && !iAnimLockNumber_Lower)
			{
				pSeq = Animation_FromLowerFrame( model->currentFrame );
			}
			if (pSeq)
			{				
				iNextX = Text_DisplayFlat(  va("%s",pSeq->sName.c_str()), iNextX + (2*TEXT_WIDTH), iTextY, 0,200,200, true);	// dim cyan
			}
		}
	}

	iTextY += TEXT_DEPTH;
}


void stat_skeleton( gl_model *model, char *psAttachedVia )
{
	if (psAttachedVia == NULL)
	{
		// first time, so zap totals...
		//		
		giTotVerts = 0;
		giTotTris  = 0;
		giModelsStatted = 0;
	}

	stat_gl_model( model, psAttachedVia );

	unsigned int sf=model->currentFrame, ef=model->currentFrame+1;


	// draw all the attached child models
	for (UINT j=0; j<model->iNumTags ; j++) 
	{			
		gl_model *child = model->linkedModels[j];
		if (child)
		{
			Tag *tag = &model->tags[sf][j];				

			stat_skeleton( child, tag->Name );			
		}
	}

	if (psAttachedVia == NULL)
	{
		// done, print totals...
		//
		if (giModelsStatted>1)		// no point if only 1 printed
			stat_gl_model( NULL, NULL );
	}
}


void draw_stats()
{
	if (mdview.baseModel)
	{
		iTextX = 2*TEXT_WIDTH;	// arb start pos 2 in from both edges
		iTextY = 4*TEXT_DEPTH;	// ... or 4... :-)

//		// Displays text at a 2d screen coord. 0,0 is top left corner, add TEXT_DEPTH per Y to go down a line
//
//		Text_DisplayFlat("testing testing HELLO!!", 100,100, 0, 255,0, true);
//		
//		Vec3 v={0,0,0};
//		Text_Display("testing testing HELLO!!", v, 255,0,0);
//
//
		gl_model *model = mdview.baseModel;

		stat_skeleton( model, NULL );

		// anim locks on the whole model?, if so display 'em...
		//
		char sString[1024];
		for (int i=0; i<2; i++)
		{
			// display anim locks at bottom of screen...
			//
			Sequence_t* pSeq=NULL;

			bool bIsUpper = false;

			if (i==0 && iAnimLockNumber_Upper )
			{
				pSeq = Animation_GetUpperSequence( iAnimLockNumber_Upper-1 );			
				bIsUpper = true;
			}

			if (i==1 && iAnimLockNumber_Lower )
			{
				pSeq = Animation_GetLowerSequence( iAnimLockNumber_Lower-1 );
				bIsUpper = false;
			}

			if (pSeq)
			{				
				int iYpos = g_iScreenHeight-((!i?4:3)*TEXT_DEPTH);

				// cheat, do this next bit here just to get the X coord, then overwrite later
				sprintf(sString,"%s anim lock: %s  Frames: %4d...%4d%s",!i?"Upper":"Lower",String_EnsureMinLength(pSeq->sName.c_str(),iAnimLockLongestString),pSeq->iTargetFrame,(pSeq->iTargetFrame+pSeq->iFrameCount)-1,String_EnsureMinLength((pSeq->iLoopFrame==-1)?"":va("  loop: %3d(%3d)",pSeq->iLoopFrame,pSeq->iTargetFrame+pSeq->iLoopFrame),25));				
				int iXpos = (g_iScreenWidth/2)-( (strlen(sString)/2)*TEXT_WIDTH);

				if (pSeq->bMultiSeq)
				{
					Sequence_t *pCurrentMultiSeq = GetMultiLockedSequenceFromFrame(bIsUpper?pModel_Upper->currentFrame:pModel_Lower->currentFrame, bIsUpper );
					int iStrlenAtCurrentSeqPoint = 0;

					sprintf(sString,"%s anim lock: ",bIsUpper?"Upper":"Lower");

					MultiSequenceLock_t* pMultiLock = (bIsUpper)?&MultiSequenceLock_Upper:&MultiSequenceLock_Lower;
					MultiSequenceLock_t::iterator it;
					for (it = pMultiLock->begin(); it != pMultiLock->end(); ++it)
					{
						int iSeqIndex = *it;

						pSeq = (bIsUpper)?Animation_GetUpperSequence(iSeqIndex):Animation_GetLowerSequence(iSeqIndex);
						
						assert(pSeq);
						if (pSeq)
						{
							if (pSeq == pCurrentMultiSeq)
								iStrlenAtCurrentSeqPoint = strlen(sString);

							strcat(sString,va("%s  ",pSeq->sName.c_str()));
						}
					}

//					int iXpos = (g_iScreenWidth/2)-( (strlen(sString)/2)*TEXT_WIDTH);					
				
					Text_DisplayFlat(sString, iXpos, iYpos, 255,0,0, true);

					// now overlay the highlighted one for current...
					//
					if (pCurrentMultiSeq)
					{
						iXpos += iStrlenAtCurrentSeqPoint*TEXT_WIDTH;
		
						Text_DisplayFlat(pCurrentMultiSeq->sName.c_str(), iXpos, iYpos, 255,0,255, true);					
					}
				}
				else
				{
					sprintf(sString,"%s anim lock: %s  Frames: %4d...%4d%s",!i?"Upper":"Lower",String_EnsureMinLength(pSeq->sName.c_str(),iAnimLockLongestString),pSeq->iTargetFrame,(pSeq->iTargetFrame+pSeq->iFrameCount)-1,String_EnsureMinLength((pSeq->iLoopFrame==-1)?"":va("  loop: %3d(%3d)",pSeq->iLoopFrame,pSeq->iTargetFrame+pSeq->iLoopFrame),25));
					
//					int iXpos = (g_iScreenWidth/2)-( (strlen(sString)/2)*TEXT_WIDTH);					
				
					Text_DisplayFlat(sString, iXpos, iYpos, 255,0,0, true);
				}
			}

			if (RunningNT() == 4)	// only needed on NT4, NT 2000 and W95/98 are ok
			{
				pSeq = NULL;

				if (i==0 && iAnimDisplayNumber_Upper)
					pSeq = Animation_GetUpperSequence( iAnimDisplayNumber_Upper-1);
				if (i==1 && iAnimDisplayNumber_Lower)
					pSeq = Animation_GetLowerSequence( iAnimDisplayNumber_Lower-1);

				if (pSeq)
				{	
					if (pSeq->bMultiSeq)
					{
						assert(0);	// should never be able to get here in the animlock display-cycler				
					}
					else
					{
						sprintf(sString,"( %s anim     : %s  Frames: %4d...%4d%s )",!i?"Upper":"Lower",String_EnsureMinLength(pSeq->sName.c_str(),iAnimLockLongestString),pSeq->iTargetFrame,(pSeq->iTargetFrame+pSeq->iFrameCount)-1,String_EnsureMinLength((pSeq->iLoopFrame==-1)?"":va("  loop: %3d(%3d)",pSeq->iLoopFrame,pSeq->iTargetFrame+pSeq->iLoopFrame),25));					
						
						int iXpos = (g_iScreenWidth/2)-( ((strlen(sString)/2)/*+2*/)*TEXT_WIDTH);	// 2 chars back from LOCk string, because of bracket+space at start
						int iYpos = g_iScreenHeight-((!i?7:6)*TEXT_DEPTH);

						Text_DisplayFlat(sString, iXpos, iYpos, 0,200,200, true);	// dim yellow
					}
				}
			}
		}

		// display current FPS and interp state...
		//
		sprintf(sString,"FPS: %2.2f %s",1/(mdview.animSpeed),mdview.animate?"(Playing)":"(Stopped)");

		iTextX = 
		Text_DisplayFlat(sString,	(g_iScreenWidth/2)-( (strlen(sString)/2)*TEXT_WIDTH),
									1*TEXT_DEPTH,
									255,255,255,
									false
						);

		if (mdview.interpolate)
		{
			iTextX = Text_DisplayFlat("(Interpolated)", iTextX+(2*TEXT_WIDTH),1*TEXT_DEPTH, 255/2,255/2,255/2,false);
		}

		iTextX = Text_DisplayFlat(va("(LOD: %d)",mdview.iLODLevel+1), iTextX+(2*TEXT_WIDTH), 1*TEXT_DEPTH, 255/2,255,255/2,false);
/*		Text_DisplayFlat(sString,	g_iScreenWidth-((strlen(sString)+2)*TEXT_WIDTH),
																	 2 *TEXT_DEPTH,
									255,255,255,
									false
						);*/

		iTextX = Text_DisplayFlat(va("( FOV: %g )",mdview.dFOV), iTextX+(2*TEXT_WIDTH),1*TEXT_DEPTH, 255, 255, 255, false);


		if (mdview.bUseAlpha)
		{
			Text_DisplayFlat("( Alpha )", iTextX+(2*TEXT_WIDTH),1*TEXT_DEPTH, 128, 128, 128, false);
		}

		// display head skin numbers in top left...
		//
		gl_model* pModel = R_FindModel( mdview.baseModel, "head");
		if (pModel)
		{
			sprintf(sString,"Head skin: %s", va("\"%s%s\"",model->sHeadSkinName,!mdview.iSkinNumber?"":va("-%1d",mdview.iSkinNumber)));

			Text_DisplayFlat(sString,	2*TEXT_WIDTH,//(g_iScreenWidth/2)-( (strlen(sString)/2)*TEXT_WIDTH),
										1*TEXT_DEPTH,
										200,200,200,
										false
							);
		}

		// display which format anim table this is using (if any)...
		//
		if (mdview.bAnimCFGLoaded)
		{
			if (mdview.bAnimIsMultiPlayerFormat)
				sprintf(sString,"( MultiPlayer ) ");
			else
				sprintf(sString,"( SinglePlayer ) ");

			int iYpos = g_iScreenHeight-(2*TEXT_DEPTH);
			int iXpos = g_iScreenWidth -(strlen(sString)*TEXT_WIDTH);

			Text_DisplayFlat(sString, iXpos, iYpos, 128,128,128, true);	// grey
		}


		// display current picmip state...
		//
		{
			extern int TextureList_GetMip(void);

			int iYpos = g_iScreenHeight-(2*TEXT_DEPTH);
			int iXpos = 1*TEXT_WIDTH;

			sprintf(sString,"( PICMIP: %d )",TextureList_GetMip());
			Text_DisplayFlat(sString,	
								iXpos, iYpos,
								100,100,100,
								false
							);
		}
	}
}


