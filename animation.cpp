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

#include "oddbits.h"
#include <vector>
#include "animation.h"
using namespace std;
int Model_EnsureCurrentFrameLegal(gl_model* model, int iNewFrame);

double getDoubleTime (void);

/*
sets all model frames to 0
*/
void rewindAnim()
{
	NodePosition pos;	
	gl_model *model;

	for (pos=mdview.modelList->first() ; pos!=NULL ; pos=mdview.modelList->after(pos)) 
	{
		model = (gl_model *)pos->element();
		model->currentFrame = 0;
		model->currentFrame = Model_EnsureCurrentFrameLegal(model, model->currentFrame);
	}
//	for (unsigned int i=0; i<mdview.model->Header.Mesh_num ; i++) {
//		mdview.frames[i]=0;		
//	}
}


/*
exectues every frame
*/
void animation_loop() 
{ 
	NodePosition pos;	
	gl_model *model;

	if (mdview.modelList->isEmpty()) 
		return;

	bool bModelUpdated = false;
	//
	// sanity check, legalise any locked frames in case they were missed elsewhere (dunno why/how that occurs, but...)
	//
	for (pos=mdview.modelList->first() ; pos!=NULL ; pos=mdview.modelList->after(pos)) 
	{
		model = (gl_model *)pos->element();

		int iFrame = Model_EnsureCurrentFrameLegal(model, model->currentFrame);

		if (model->currentFrame != iFrame)
		{
			model->currentFrame  = iFrame;
			bModelUpdated = true;
		}
	}
	if (!mdview.animate) 
	{
		if (mdview.interpolate)		
			mdview.frameFrac = 0.5f;		
		else
			mdview.frameFrac = 0;	

		//if (bModelUpdated)
		{
			render_mdview();
			swap_buffers();
		}
		return;
	}

	double timeStamp2 = getDoubleTime();
	mdview.frameFrac = (float)((timeStamp2 - mdview.timeStamp1) / mdview.animSpeed);
	
	if (mdview.frameFrac > 1.f) 
	{
		mdview.frameFrac = 0;
		mdview.timeStamp1 = timeStamp2;		

			for (pos=mdview.modelList->first() ; pos!=NULL ; pos=mdview.modelList->after(pos)) 
			{
				model = (gl_model *)pos->element();
				
				model->currentFrame = GetNextFrame_MultiLocked(model, model->currentFrame, 1);

				// note legalise before generic wrap, otherwise last sequence won't loop (if relevant)
				//
				model->currentFrame = Model_EnsureCurrentFrameLegal(model, model->currentFrame);

				if (model->currentFrame == model->iNumFrames) // itu?
					model->currentFrame = 0;				
			}			
			render_mdview();
		    swap_buffers();
		
	}
	else 
	if (mdview.interpolate || bModelUpdated)
	{
		render_mdview();
		swap_buffers();
	}	
}



// returns index else -1 for not frame-not-found-in-sequences...
//
int GetMultiLockedSequenceIndexFromFrame(int iFrame, bool bIsUpper )
{
	MultiSequenceLock_t* pMultiLock = (bIsUpper)?&MultiSequenceLock_Upper:&MultiSequenceLock_Lower;
	MultiSequenceLock_t::iterator it;

	int iIndex=0;
	for (it = pMultiLock->begin(); it != pMultiLock->end(); ++it, iIndex++)
	{
		int iSeqIndex = *it;

		Sequence_t* pSeq = (bIsUpper)?Animation_GetUpperSequence(iSeqIndex):Animation_GetLowerSequence(iSeqIndex);
		
		assert(pSeq);
		if (pSeq)
		{
			int iSeqFrameFirst = pSeq->iTargetFrame;
			int iSeqFrameLast  = (iSeqFrameFirst + pSeq->iFrameCount)-1;

			if (iFrame >= iSeqFrameFirst && iFrame <= iSeqFrameLast)
				return iIndex;
		}
	}

	return -1;
}

Sequence_t* GetMultiLockedSequenceFromFrame(int iFrame, bool bIsUpper )
{
	int iMultiLockSequenceIndex = GetMultiLockedSequenceIndexFromFrame( iFrame, bIsUpper );
	if (iMultiLockSequenceIndex == -1)
		return NULL;

	MultiSequenceLock_t* pMultiLock = (bIsUpper)?&MultiSequenceLock_Upper:&MultiSequenceLock_Lower;
	int iIndex = pMultiLock->at(iMultiLockSequenceIndex);

	return bIsUpper?Animation_GetUpperSequence(iIndex):Animation_GetLowerSequence(iIndex);
}



Sequence_t *GetLockedSequence_Upper(gl_model* model)
{
	if (model && (model == pModel_Upper || model->pModel_LOD0 == pModel_Upper) && iAnimLockNumber_Upper)
	{
		return Animation_GetUpperSequence( iAnimLockNumber_Upper-1 );			
	}

	return NULL;
}
	
Sequence_t *GetLockedSequence_Lower(gl_model* model)
{
	if (model && (model == pModel_Lower || model->pModel_LOD0 == pModel_Lower) && iAnimLockNumber_Lower)
	{
		return Animation_GetLowerSequence( iAnimLockNumber_Lower-1 );		
	}	

	return NULL;
}



// Another uberhack, sigh....
//
// gets the next frame in a multilocked sequence, where iStepVal can be +ve or -ve, (though usually just -1/0/1)
//
// (this isn't staggeringly fast at certain points, but 99% of the time it executes quickly)
//
int GetNextFrame_MultiLocked(gl_model* pModel, int iFrame, int iStepVal)
{	
	Sequence_t* pSeq = NULL;

	bool bIsUpper = false;

	if ((pSeq = GetLockedSequence_Upper(pModel)) != NULL)
		bIsUpper = true;
	else
	if ((pSeq = GetLockedSequence_Lower(pModel)) != NULL)
		bIsUpper = false;
	
	if (pSeq && pSeq->bMultiSeq)
	{
		int iNewFrame = iFrame;
		int iStep = (iStepVal<0)?-1:1;

		for (int iStepValRemaining = iStepVal; iStepValRemaining!=0; iStepValRemaining += -iStep)
		{
			int iThisFrame = iNewFrame;
			int iNextFrame = iNewFrame + iStep;

			int iMultiLockSequenceIndex = GetMultiLockedSequenceIndexFromFrame( iThisFrame, bIsUpper );
			if (iMultiLockSequenceIndex != -1)
			{
				MultiSequenceLock_t* pMultiLock = (bIsUpper)?&MultiSequenceLock_Upper:&MultiSequenceLock_Lower;
				int iIndex = pMultiLock->at(iMultiLockSequenceIndex);

				pSeq = bIsUpper?Animation_GetUpperSequence(iIndex):Animation_GetLowerSequence(iIndex);

				assert(pSeq);	//
				if (pSeq)		// itu?
				{
					// this is the sequence we're currently in, does the proposed next frame also sit within this one?...
					//
					int iSeqFrameFirst = pSeq->iTargetFrame;
					int iSeqFrameLast  = (iSeqFrameFirst + pSeq->iFrameCount)-1;

					if (iNextFrame >= iSeqFrameFirst && iNextFrame <= iSeqFrameLast)
					{
						// yes, so adopt it...
						//
						iNewFrame = iNextFrame;
					}
					else
					{
						// new frame is outside of current group, so get the index of the sequence after (or before) this one,
						//	and adopt the first or last frame of it...
						//
						int iNumAvailableSequences = pMultiLock->size();

						iMultiLockSequenceIndex += iStep;

						if (iMultiLockSequenceIndex >= iNumAvailableSequences)
							iMultiLockSequenceIndex = 0;
						if (iMultiLockSequenceIndex < 0)
							iMultiLockSequenceIndex = iNumAvailableSequences-1;

						iIndex = pMultiLock->at(iMultiLockSequenceIndex);
						Sequence_t* pSeq = bIsUpper?Animation_GetUpperSequence(iIndex):Animation_GetLowerSequence(iIndex);

						assert(pSeq);	//
						if (pSeq)		// itu?
						{
							iSeqFrameFirst = pSeq->iTargetFrame;
							iSeqFrameLast  = (iSeqFrameFirst + pSeq->iFrameCount)-1;

							iNewFrame = (iStep>0)?iSeqFrameFirst:iSeqFrameLast;
						}
						else
						{
							return iFrame+iStepVal;	 // ... let the overall legaliser adjust this frame sometime after returning
						}
					}
				}
				else
				{
					// I don't think we'll ever get here, but I've kind of stopped caring...
					//
					return iFrame+iStepVal; // ... let the overall legaliser adjust this frame sometime after returning
				}
			}
			else
			{
				// err...
				return iFrame+iStepVal;	// ... let the overall legaliser adjust this frame sometime after returning
			}
		}

		return iNewFrame;	
	}	
	else
	{	
		return iFrame+iStepVal;	
	}
}

// this only returns the legalised frame, it does NOT set it...
//
int Model_EnsureCurrentFrameLegal(gl_model* model, int iFrame)
{	
	Sequence_t *pSeq = NULL;

	bool bIsUpper = false;

	if ((pSeq = GetLockedSequence_Upper(model)) != NULL)
		bIsUpper = true;
	else
	if ((pSeq = GetLockedSequence_Lower(model)) != NULL)
		bIsUpper = false;
	
	if (pSeq)
	{
		if (pSeq->bMultiSeq)
		{
			// multi-seq locks are a special case, the wrap logic can be pretty freaky, so...
			//
			if (GetMultiLockedSequenceFromFrame(iFrame, bIsUpper ))
				return iFrame;	// frame is ok

			// if we got here then we're outside all current multi-seqs, unfortunately because the specified sequences 
			//	can be at any position within the master list order I can't just work out which 2 sequences I'm between
			//	and jump to the start of the higher one, so working on the principle that 99% of the time this code is
			//	used when lerping between frame & frame+1 I'll see if I can find iFrame-1 anywhere...
			//
			iFrame--;

			MultiSequenceLock_t* pMultiLock = (bIsUpper)?&MultiSequenceLock_Upper:&MultiSequenceLock_Lower;
			MultiSequenceLock_t::iterator it;

			for (it = pMultiLock->begin(); it != pMultiLock->end(); ++it)
			{
				int iSeqIndex = *it;

				Sequence_t* pSeq = (bIsUpper)?Animation_GetUpperSequence(iSeqIndex):Animation_GetLowerSequence(iSeqIndex);
				
				assert(pSeq);
				if (pSeq)
				{
					int iSeqFrameFirst = pSeq->iTargetFrame;
					int iSeqFrameLast  = (iSeqFrameFirst + pSeq->iFrameCount)-1;

					if (iFrame >= iSeqFrameFirst && iFrame <= iSeqFrameLast)
					{
						// got it, so is there another sequence after this?
						//
						if (++it != pMultiLock->end())
						{
							iSeqIndex = *it;
							pSeq = (bIsUpper)?Animation_GetUpperSequence(iSeqIndex):Animation_GetLowerSequence(iSeqIndex);
							
							assert(pSeq);
							if (pSeq)							
								return pSeq->iTargetFrame;	// return first frame of next seq in list
						}
						else
						{
							// reached end of list...
							//							
							break;
						}
					}
				}
			}

			// sod it, whatever happened, let's start at the beginning of the list again...
			//
			it = pMultiLock->begin();
			int iSeqIndex = *it;
			pSeq = (bIsUpper)?Animation_GetUpperSequence(iSeqIndex):Animation_GetLowerSequence(iSeqIndex);
			assert(pSeq);
			if (pSeq)
				iFrame = pSeq->iTargetFrame;
		}
		else
		{
			if (iFrame > (pSeq->iTargetFrame+pSeq->iFrameCount)-1)
			{
				// OOR above, loop or wrap?...
				//
				if (pSeq->iLoopFrame != -1)
				{
					// account for loop frame...
					//
					iFrame = pSeq->iTargetFrame + pSeq->iLoopFrame;
				}
				else
				{
					// no loop, straight wrap...
					//
	//				iFrame = pSeq->iTargetFrame;	// wrap
					iFrame =(pSeq->iTargetFrame+pSeq->iFrameCount)-1;	// stop at end
				}
			}
			else
			if (iFrame < pSeq->iTargetFrame)
			{
				// OOR below, wrap only...
				//
				iFrame = pSeq->iTargetFrame;
			}		
		}
	}
	//sanity
	if (iFrame > (int)model->iNumFrames)
	{
		iFrame = model->iNumFrames;
	}
	return iFrame;
}

void SetLODLevel(int iLOD)
{
	mdview.iLODLevel = iLOD;
}

void FrameAdvanceAnim(int iStepVal)	// basically I only use this as +/-1, but other vals should be ok
{
	NodePosition pos;	
	gl_model *model;

	for (pos=mdview.modelList->first() ; pos!=NULL ; pos=mdview.modelList->after(pos))
	{
		model = (gl_model *)pos->element();

		model->currentFrame = GetNextFrame_MultiLocked(model, model->currentFrame, iStepVal);

		model->currentFrame = Model_EnsureCurrentFrameLegal(model, model->currentFrame);	// handle range locking before generic wrap check

		int iFrame = model->currentFrame;	// saves messing with unsigned/signed range conflicts		

		// technically this isn't 100% correct if you were stepping by other values than +/- 1 because I'm
		//	just wrapping to the other end of the range, and not taking into account the amount that you 
		//	overflowed it by. If you want to alter this then help yourself.
		//
		if (iFrame >= (int)(model->iNumFrames))
		{
			iFrame = 0;
		}
		if (iFrame <0)	// fix for unsigned
		{
			iFrame = model->iNumFrames-1;
		}

		model->currentFrame = iFrame;
	}

	mdview.animate = false;
}


vector < Sequence_t > Sequences_LowerAnims;
vector < Sequence_t > Sequences_UpperAnims;
MultiSequenceLock_t MultiSequenceLock_Upper;
MultiSequenceLock_t MultiSequenceLock_Lower;
Sequence_t Sequence_Upper_Fake;
Sequence_t Sequence_Lower_Fake;
int iAnimLockNumber_Upper = 0;
int iAnimLockNumber_Lower = 0;
int iAnimDisplayNumber_Upper = 0;
int iAnimDisplayNumber_Lower = 0;
int iAnimLockLongestString= 0;	// for aesthetics
gl_model* pModel_Lower = NULL;
gl_model* pModel_Upper = NULL;

int Animation_GetNumLowerSequences(void)
{
	return Sequences_LowerAnims.size();
}

int Animation_GetNumUpperSequences(void)
{
	return Sequences_UpperAnims.size();
}

// iIndex either = 0-based index if +ve, or -2 (because of blah-1 before call)
//
Sequence_t* Animation_GetLowerSequence(int iIndex)
{
	if (iIndex<0)	// bleh...
		return &Sequence_Lower_Fake;

	return &Sequences_LowerAnims[iIndex];
}

// iIndex either = 0-based index if +ve, or -2 (because of blah-1 before call)
//
Sequence_t* Animation_GetUpperSequence(int iIndex)
{
	if (iIndex<0)	// bleh...
		return &Sequence_Upper_Fake;

	return &Sequences_UpperAnims[iIndex];
}

// to do a request by Shubes, this works out which sequence you would be in on this frame if you were in a lock...
//
Sequence_t* Animation_FromUpperFrame( int iFrame )
{
	int iLoopCount = Animation_GetNumUpperSequences();

	for (int i=0; i<iLoopCount; i++)
	{
		Sequence_t *pSeq = Animation_GetUpperSequence(i);

		if (iFrame >= pSeq->iTargetFrame &&
			iFrame <  pSeq->iTargetFrame + pSeq->iFrameCount
			)
		{
			return pSeq;
		}
	}

	return NULL;
}

Sequence_t* Animation_FromLowerFrame( int iFrame )
{
	int iLoopCount = Animation_GetNumLowerSequences();

	for (int i=0; i<iLoopCount; i++)
	{
		Sequence_t *pSeq = Animation_GetLowerSequence(i);

		if (iFrame >= pSeq->iTargetFrame &&
			iFrame <  pSeq->iTargetFrame + pSeq->iFrameCount
			)
		{
			return pSeq;
		}
	}

	return NULL;
}



void ClearAnimationCFG()
{
	Sequences_LowerAnims.clear();
	Sequences_UpperAnims.clear();

	Menu_UpperAnims_Clear();
	Menu_LowerAnims_Clear();

	Sequence_Upper_Fake.bMultiSeq = true;
	Sequence_Lower_Fake.bMultiSeq = true;

	iAnimLockNumber_Upper = 0;
	iAnimLockNumber_Lower = 0;
	iAnimDisplayNumber_Upper = 0;
	iAnimDisplayNumber_Lower = 0;
	iAnimLockLongestString= 0;

	pModel_Lower = NULL;
	pModel_Upper = NULL;

	mdview.xPos = 0.0f;
	mdview.yPos = 0.0f;

	mdview.rotAngleX = 
	mdview.rotAngleY =   0.0f;
	mdview.rotAngleZ = -90.0f;

	mdview.bAnimCFGLoaded = false;
	mdview.bAnimIsMultiPlayerFormat = false;
}


void ReportFrameMismatches(Sequence_t* pSeq, gl_model* pModel)
{
	if (pSeq)
	{
		int iModelAnims = pModel->iNumFrames;
		int iCFGAnims	= pSeq->iTargetFrame + pSeq->iFrameCount;	// no need to -1 because of 0-based indexing

		if (iModelAnims > iCFGAnims)
		{
			WarningBox(va("Model : \"%s\"\n\n... has %d frames, but animations for it only use %d frames\n\n( ... so %d frame(s) are currently just wasting space )",pModel->sMDXFullPathname,iModelAnims,iCFGAnims,  iModelAnims-iCFGAnims ));
		}
		else
		if (iModelAnims < iCFGAnims)
		{
			ErrorBox(va("Model : \"%s\"\n\n... has %d frames, but animations for it assume %d frames\n\n( ... so you'll get errors accessing the last %d frame(s)! )",pModel->sMDXFullPathname,iModelAnims,iCFGAnims, iCFGAnims-iModelAnims));
		}
	}
}


// this has now been re-written to only add to pulldown menus when all menus have been scanned, this way
//	I can strcat frame info to the seq names while keeping a smooth tabbing line...
//
// Note that this function can automatically read either ID format or Raven format files transparently...
//
void LoadAnimationCFG(LPCSTR psFullPath, HDC hDC)	// hDC for text metrics
{
	int  iLongestTextMetric = 0;
	SIZE Size;
	bool bOk = false;	

	FILE *handle = fopen(psFullPath,"rt");

	if (handle)
	{
		mdview.bAnimCFGLoaded = true;

		static char sLineBuffer[2048];

		int iFirstFrameAfterBoth = -1;	// stuff I need to do for ID's non-folded frame numbers
		int iFirstFrameAfterTorso= -1;

		while (1)
		{
			ZEROMEM(sLineBuffer);
			if (!fgets(sLineBuffer,sizeof(sLineBuffer),handle))
			{
				if (ferror(handle))
				{
					ErrorBox(va("Error while reading \"%s\"!",psFullPath));
					ClearAnimationCFG();
				}
				break;	// whether error or EOF
			}

			char sComment[2048] = {0};	// keep comments now because of the way ID cfg files work

			// zap any comments...
			//
			char *p = strstr(sLineBuffer,"//");
			if (p)
			{
				strcpy(sComment,p+2);
				*p=0;
			}

			// update, to read ID cfg files, we need to skip over some stuff that Raven ones don't have...
			//
			// our cfg files don't have "sex" (how depressingly apt...)
			//
			if (strnicmp(sLineBuffer,"sex",3)==0)
				continue;
			//
			// or this other crap either...
			//
			if (strnicmp(sLineBuffer,"footsteps",9)==0)
				continue;
			if (strnicmp(sLineBuffer,"headoffset",10)==0)
				continue;
			if (strnicmp(sLineBuffer,"soundpath",9)==0)
				continue;

			Sequence_t seq;
			memset(&seq,0,sizeof(seq));

			char sLine[2048];
			int iElementsDone = sscanf( sLineBuffer, "%s %d %d %d %d", &sLine, &seq.iTargetFrame, &seq.iFrameCount, &seq.iLoopFrame, &seq.iFrameSpeed );
			if (iElementsDone == EOF)
				continue;	// probably skipping over a comment line

			bool bElementsScannedOk = false;

			if (iElementsDone == 5)
			{
				// then it must be a Raven line...
				//
				bElementsScannedOk = true;
				mdview.bAnimIsMultiPlayerFormat = false;
			}
			else
			{
				// try scanning it as an ID line...
				//
				iElementsDone = sscanf( sLineBuffer, "%d %d %d %d", &seq.iTargetFrame, &seq.iFrameCount, &seq.iLoopFrame, &seq.iFrameSpeed );
				if (iElementsDone == 4)
				{
					mdview.bAnimIsMultiPlayerFormat = true;
					// scanned an ID line in ok, now convert it to Raven format...
					//
					iElementsDone = sscanf( sComment, "%s", &sLine );	// grab anim name from original saved comment
					if (iElementsDone == 1)
					{
						// ... and convert their loop format to ours...
						//
						if (seq.iLoopFrame == 0)
						{
							seq.iLoopFrame = -1;
						}
						else
						{
							seq.iLoopFrame = seq.iFrameCount - seq.iLoopFrame;
						}


						// now do the folding number stuff since ID don't do it in their files...
						//
						if ( !strnicmp(sLine,"TORSO_",6) && iFirstFrameAfterBoth == -1)
						{
							iFirstFrameAfterBoth = seq.iTargetFrame;
						}
						if ( !strnicmp(sLine,"LEGS_",5))
						{
							if (iFirstFrameAfterTorso == -1)
							{
								iFirstFrameAfterTorso = seq.iTargetFrame;
							}

							// now correct the leg framenumber...
							//
							if (iFirstFrameAfterBoth != -1)	// if it did, then there'd be no torso frames, so no adj nec.
							{
								seq.iTargetFrame -= (iFirstFrameAfterTorso - iFirstFrameAfterBoth);
							}
						}

						bElementsScannedOk = true;
					}
				}
			}

			if (bElementsScannedOk)
			{	
				seq.sName = sLine;
				//
				// this line seems to be ok...
				//
//				OutputDebugString(va("%s  %d %d %d %d\n",seq.sName.c_str(), seq.iTargetFrame, seq.iFrameCount, seq.iLoopFrame, seq.iFrameSpeed ));


				// "both" or "torso" get added to 'upper' menu...
				//
				if ( (!strnicmp(seq.sName.c_str(),"BOTH_",5)) || (!strnicmp(seq.sName.c_str(),"TORSO_",6)) )
				{
					Sequences_UpperAnims.push_back(seq);
					if (iAnimLockLongestString < (int)strlen(seq.sName.c_str()))
						iAnimLockLongestString = strlen(seq.sName.c_str());

					if (GetTextExtentPoint(	hDC,						// HDC hdc,           // handle to device context
											seq.sName.c_str(),			// LPCTSTR lpString,  // pointer to text string
											strlen(seq.sName.c_str()),	// int cbString,      // number of characters in string
											&Size						// LPSIZE lpSize      // pointer to structure for string size
											)
						)
					{
						if (iLongestTextMetric < Size.cx)
							iLongestTextMetric = Size.cx;
					}

//					Menu_UpperAnims_AddItem(seq.sName.c_str());					
				}

				// "both" or "legs" get added to 'lower' menu...
				//
				if ( (!strnicmp(seq.sName.c_str(),"BOTH_",5)) || (!strnicmp(seq.sName.c_str(),"LEGS_",5)) )
				{
					Sequences_LowerAnims.push_back(seq);
					if (iAnimLockLongestString < (int)strlen(seq.sName.c_str()))
						iAnimLockLongestString = strlen(seq.sName.c_str());

					if (GetTextExtentPoint(	hDC,						// HDC hdc,           // handle to device context
											seq.sName.c_str(),			// LPCTSTR lpString,  // pointer to text string
											strlen(seq.sName.c_str()),	// int cbString,      // number of characters in string
											&Size						// LPSIZE lpSize      // pointer to structure for string size
											)
						)
					{
						if (iLongestTextMetric < Size.cx)
							iLongestTextMetric = Size.cx;
					}
 
//					Menu_LowerAnims_AddItem(seq.sName.c_str());
				}
			}
			else
			{
				// so do we report this as an error or what?
				//
				ErrorBox(sLineBuffer);
			}
		}

		fclose(handle);

		// now add to menus... (this code is awful, it was simple at first then mutated with feature-add)
		//
		char sLine[2048];
		vector< Sequence_t >::iterator it;
		for (it=Sequences_UpperAnims.begin(); it!=Sequences_UpperAnims.end(); it++)
		{
			sprintf(sLine,(*it).sName.c_str());

			while (1)
			{
				GetTextExtentPoint(	hDC,			// HDC hdc,           // handle to device context
									sLine,			// LPCTSTR lpString,  // pointer to text string
									strlen(sLine),	// int cbString,      // number of characters in string
									&Size			// LPSIZE lpSize      // pointer to structure for string size
									);
				if (Size.cx >= iLongestTextMetric)
					break;

				strcat(sLine," ");
			}

			Menu_UpperAnims_AddItem(va("%s (%d...%d)%s",sLine,(*it).iTargetFrame,((*it).iTargetFrame+(*it).iFrameCount)-1,((*it).iLoopFrame==-1)?"":va("  Loop %d",(*it).iTargetFrame+(*it).iLoopFrame)));
		}

		for (it=Sequences_LowerAnims.begin(); it!=Sequences_LowerAnims.end(); it++)
		{
			sprintf(sLine,(*it).sName.c_str());

			while (1)
			{
				GetTextExtentPoint(	hDC,			// HDC hdc,           // handle to device context
									sLine,			// LPCTSTR lpString,  // pointer to text string
									strlen(sLine),	// int cbString,      // number of characters in string
									&Size			// LPSIZE lpSize      // pointer to structure for string size
									);
				if (Size.cx >= iLongestTextMetric)
					break;

				strcat(sLine," ");
			}

			Menu_LowerAnims_AddItem(va("%s (%d...%d)%s",sLine,(*it).iTargetFrame,((*it).iTargetFrame+(*it).iFrameCount)-1,((*it).iLoopFrame==-1)?"":va("  Loop %d",(*it).iTargetFrame+(*it).iLoopFrame)));
		}


		// a bit of sanity checking, to cope with something Bob tried to do...   :-)
		//
		Sequence_t* pSeq = NULL;
		gl_model* pModel;		
		
		if ((pModel = pModel_Lower)!=0)
		{
			pSeq = Animation_GetLowerSequence(Animation_GetNumLowerSequences()-1);
			ReportFrameMismatches(pSeq,pModel);
		}

		if ((pModel = pModel_Upper)!=0)
		{
			pSeq = Animation_GetUpperSequence(Animation_GetNumUpperSequences()-1);
			ReportFrameMismatches(pSeq,pModel);
		}
	}
}


