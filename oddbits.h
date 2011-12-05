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

#ifndef ODDBITS_H
#define ODDBITS_H

#include "assert.h"

#define SAFEFREE(arg) if (arg){free(arg);arg=0;}
#define ZEROMEM(arg) memset(arg, 0, sizeof(arg))
#define ASSERT assert
#ifdef _DEBUG
#define VERIFY(arg) ASSERT(arg)
#else
#define VERIFY(arg) arg
#endif 

char *va(char *format, ...);
bool FileExists (LPCSTR psFilename);

void ErrorBox(const char *sString);
void InfoBox(const char *sString);
void WarningBox(const char *sString);
#define GetYesNo(psQuery)	(!!(MessageBox(mdview.hwnd,psQuery,"Query",MB_YESNO|MB_ICONWARNING|MB_TASKMODAL)==IDYES))


char *scGetTempPath(void);
char *InputLoadFileName(char *psInitialLoadName, char *psCaption, const char *psInitialDir, char *psFilter);
long filesize(FILE *handle);
int  LoadFile (LPCSTR psPathedFilename, void **bufferptr);
//void Filename_RemoveBASEQ(CString &string);
//void Filename_RemoveFilename(CString &string);
//void Filename_BaseOnly(CString &string);
//void Filename_AccountForLOD(CString &string, int iLODLevel);

char *Filename_WithoutPath(LPCSTR psFilename);
char *Filename_WithoutExt(LPCSTR psFilename);
char *Filename_PathOnly(LPCSTR psFilename);
char *Filename_ExtOnly(LPCSTR psFilename);
char *String_ToLower(LPCSTR psString);
char *String_ToUpper(LPCSTR psString);
char *String_Neaten(LPCSTR psString);
char *String_NeatenEveryWord(LPCSTR psString);
char *String_Replace(char *psString, char *psFind, char *psReplace, BOOL bCaseInSensitive);
char *String_GetField(char *psString);
char *String_LoseField(char *psString);
char *String_LoseWhitespace(char *psString);
char *String_LoseLeadingWhitespace(char *psString);
char *String_LoseTrailingWhitespace(char *psString);
char *String_EnsureTrailingChar(char *psString, char c);
char *String_LoseTrailingChar(char *psString, char c);
char *String_LoseComment(char *psString);
char *String_LoseNewline(char *psString);
char *String_EnsureMinLength(LPCSTR psString, int iMinLength);
int RunningNT(void);

#define StartWait() HCURSOR hcurSave = SetCursor(::LoadCursor(NULL, IDC_WAIT))
#define RestoreWait() SetCursor(::LoadCursor(NULL, IDC_WAIT))
#define EndWait()   SetCursor(hcurSave)

#define STL_ITERATE_DECL(iter, dataname) for (dataname##_t::iterator iter = dataname.begin(); iter!=dataname.end(); ++iter)
#define STL_ITERATE(iter, dataname)		 for (iter = dataname.begin(); iter!=dataname.end(); ++iter)

void SystemErrorBox(DWORD dwError = GetLastError());

#endif	// #ifndef ODDBITS_H


/////////////////// eof ////////////////////


