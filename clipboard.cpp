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

#include <windows.h>
#include <stdio.h>
#include <assert.h>
#include "oddbits.h"
#include "clipboard.h"



extern HWND mainhWnd;



BOOL ClipBoard_SendDIB(LPVOID pvData, int iBytes)
{
	HGLOBAL hXferBuffer = GlobalAlloc((UINT)GMEM_MOVEABLE|GMEM_DDESHARE,(DWORD)iBytes);	
	if (hXferBuffer)
	{
   		char *psLockedDest = (char*) GlobalLock(hXferBuffer);	
		memcpy(psLockedDest,pvData,iBytes);
		GlobalUnlock(psLockedDest);	
	
		if (OpenClipboard(mainhWnd))
		{
			EmptyClipboard();												// empty it (all handles to NULL);
			if((SetClipboardData((UINT)CF_DIB,hXferBuffer))==NULL)
			{
				CloseClipboard();												
				ErrorBox("ClipBoard_SendDIB(): Dammit, some sort of problem writing to the clipboard...");
				return FALSE;	// hmmmm... Oh well.
			}
			CloseClipboard();
			return TRUE;
		}
	}

	ErrorBox(va("ClipBoard_SendDIB(): Dammit, I can't allocate %d bytes for some strange reason (reboot, then try again, else tell me - Ste)",iBytes));
	return FALSE;			
}


BOOL Clipboard_SendString(LPCSTR psString)
{
	HGLOBAL hXferBuffer = GlobalAlloc((UINT)GMEM_MOVEABLE|GMEM_DDESHARE,(DWORD)strlen(psString)+1);	
	if (hXferBuffer)
	{
   		char *psLockedDest = (char*) GlobalLock(hXferBuffer);	
		memcpy(psLockedDest,psString,strlen(psString)+1);
		GlobalUnlock(psLockedDest);	
	
		if (OpenClipboard(mainhWnd))
		{
			EmptyClipboard();												// empty it (all handles to NULL);
			if((SetClipboardData((UINT)CF_TEXT,hXferBuffer))==NULL)
			{
				CloseClipboard();												
				ErrorBox("Clipboard_SendString(): Dammit, some sort of problem writing to the clipboard...");
				return FALSE;	// hmmmm... Oh well.
			}
			CloseClipboard();
			return TRUE;
		}
	}

	ErrorBox(va("Clipboard_SendString(): Dammit, I can't allocate %d bytes for some strange reason (reboot, then try again, else tell me - Ste)",strlen(psString)+1));
	return FALSE;			
}



//////////////// eof ////////////////

