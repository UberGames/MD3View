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

#ifndef ANIMATION_H
#define ANIMATION_H


#include <vector>
#include <set>
#include <list>
using namespace std;


typedef vector<int>	MultiSequenceLock_t;
extern MultiSequenceLock_t MultiSequenceLock_Upper;
extern MultiSequenceLock_t MultiSequenceLock_Lower;


typedef struct
{
	string	sName;
	int		iTargetFrame;
	int		iFrameCount;
	int		iLoopFrame;
	int		iFrameSpeed;
	bool	bMultiSeq;
} Sequence_t;

int GetNextFrame_MultiLocked(gl_model* pModel, int iFrame, int iStepVal);
Sequence_t* GetMultiLockedSequenceFromFrame(int iFrame, bool bIsUpper );
int Model_EnsureCurrentFrameLegal(gl_model* model, int iFrame);
Sequence_t* Animation_GetLowerSequence(int iIndex);
Sequence_t* Animation_GetUpperSequence(int iIndex);
int Animation_GetNumLowerSequences(void);
int Animation_GetNumUpperSequences(void);
Sequence_t* Animation_FromUpperFrame(int iFrame);
Sequence_t* Animation_FromLowerFrame(int iFrame);
void ClearAnimationCFG();
void LoadAnimationCFG(LPCSTR psFullPath, HDC hDC);
extern int iAnimLockNumber_Upper;
extern int iAnimLockNumber_Lower;
extern int iAnimDisplayNumber_Upper;
extern int iAnimDisplayNumber_Lower;
extern int iAnimLockLongestString;
extern gl_model* pModel_Lower;
extern gl_model* pModel_Upper;


#endif	// #ifndef ANIMATION_H

//////////////////// eof //////////////////////


