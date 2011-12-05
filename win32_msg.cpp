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

#ifdef WIN32

#include "system.h"
#include "ndictionary.h"
#include "md3gl.h"
#include "md3view.h"
#include "Resource.h"
#include "AFXRES.H"
#include "text.h"	
#include "oddbits.h"
#include "animation.h"
#include "bmp.h"
#include "clipboard.h"

extern HINSTANCE WinhInstance;
extern NodeSequenceInfo tagMenuList;
HDC  hDC_;

bool sys_rbuttondown = false;
bool sys_lbuttondown = false;
bool sys_mbuttondown = false;

OPENFILENAME *FileOpenDialog(HWND hwnd, WORD wCommand, WORD wNotify, HWND hwndCtrl, int type);
OPENFILENAME *FileSaveDialog(HWND hwnd, WORD wCommand, WORD wNotify, HWND hwndCtrl, int type);

/*
selects a 16 bit color file format, could be higher though it wouldn't work with a voodoo
*/
void SetPixelFormat( HDC hdc)
{
	PIXELFORMATDESCRIPTOR pfd, *ppfd;
    int pixelformat;
    ppfd = &pfd;    

	memset(ppfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
    ppfd->nSize = sizeof(PIXELFORMATDESCRIPTOR);
    ppfd->nVersion = 1;
    ppfd->dwFlags = PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    ppfd->dwLayerMask = PFD_MAIN_PLANE;
    ppfd->iPixelType = PFD_TYPE_RGBA;
    ppfd->cColorBits = 16;
    ppfd->cDepthBits = 16;
    ppfd->cAccumBits = 0;
    ppfd->cStencilBits = 0;

    pixelformat = ChoosePixelFormat( hdc, ppfd);

    if ( pixelformat == 0) {
        Error("ChoosePixelFormat failed");
    }

    if (ppfd->dwFlags & PFD_NEED_PALETTE) {
       Error ("ChoosePixelFormat needs palette" );
    }

    if (SetPixelFormat( hdc, pixelformat, ppfd) == FALSE) {
        Error("SetPixelFormat failed");
    }
}



/*
** ChoosePFD
**
** Helper function that replaces ChoosePixelFormat.
*/
#define MAX_PFDS 256

static int GLW_ChoosePFD( HDC hDC, PIXELFORMATDESCRIPTOR *pPFD )
{
	PIXELFORMATDESCRIPTOR pfds[MAX_PFDS+1];
	int maxPFD = 0;
	int i;
	int bestMatch = 0;

	OutputDebugString( va("...GLW_ChoosePFD( %d, %d, %d )\n", ( int ) pPFD->cColorBits, ( int ) pPFD->cDepthBits, ( int ) pPFD->cStencilBits) );

	// count number of PFDs
	//
	maxPFD = DescribePixelFormat( hDC, 1, sizeof( PIXELFORMATDESCRIPTOR ), &pfds[0] );

	if ( maxPFD > MAX_PFDS )
	{
		OutputDebugString( va( "...numPFDs > MAX_PFDS (%d > %d)\n", maxPFD, MAX_PFDS) );
		maxPFD = MAX_PFDS;
	}

	OutputDebugString( va("...%d PFDs found\n", maxPFD - 1) );

	FILE *handle = fopen("MD3View_GL_report.txt","wt");

	fprintf(handle,"Total PFDs: %d\n\n",maxPFD);

	// grab information
	for ( i = 1; i <= maxPFD; i++ )
	{
		DescribePixelFormat( hDC, i, sizeof( PIXELFORMATDESCRIPTOR ), &pfds[i] );

		fprintf(handle,"PFD %d/%d\n",i,maxPFD);
		fprintf(handle,"=========\n");		

#define FLAGDUMP(flag) if ( (pfds[i].dwFlags & flag ) != 0 ) fprintf(handle,"(flag: %s)\n",#flag);

		FLAGDUMP( PFD_DOUBLEBUFFER            );
		FLAGDUMP( PFD_STEREO                  );
		FLAGDUMP( PFD_DRAW_TO_WINDOW          );
		FLAGDUMP( PFD_DRAW_TO_BITMAP          );
		FLAGDUMP( PFD_SUPPORT_GDI             );
		FLAGDUMP( PFD_SUPPORT_OPENGL          );
		FLAGDUMP( PFD_GENERIC_FORMAT          );
		FLAGDUMP( PFD_NEED_PALETTE            );
		FLAGDUMP( PFD_NEED_SYSTEM_PALETTE     );
		FLAGDUMP( PFD_SWAP_EXCHANGE           );
		FLAGDUMP( PFD_SWAP_COPY               );
		FLAGDUMP( PFD_SWAP_LAYER_BUFFERS      );
		FLAGDUMP( PFD_GENERIC_ACCELERATED     );
		FLAGDUMP( PFD_SUPPORT_DIRECTDRAW      );

		if ( pfds[i].iPixelType == PFD_TYPE_RGBA )
		{
//			fprintf(handle,"RGBA mode\n");
		}
		else
		{
			fprintf(handle,"NOT RGBA mode!!!!!!!!!!!!\n");
		}

		fprintf(handle, "Colour bits: %d\n",pfds[i].cColorBits);
		fprintf(handle, "Depth  bits: %d\n",pfds[i].cDepthBits);

		fprintf(handle,"\n");
	}
	

	// look for a best match
	for ( i = 1; i <= maxPFD; i++ )
	{
		fprintf(handle,"(bestMatch: %d)\n",bestMatch );

		//
		// make sure this has hardware acceleration
		//
		if ( ( pfds[i].dwFlags & PFD_GENERIC_FORMAT ) != 0 ) 
		{
//			if ( !r_allowSoftwareGL->integer )
			{
//				if ( r_verbose->integer )
				{
					fprintf(handle,//OutputDebugString(
						 va ("...PFD %d rejected, software acceleration\n", i ));
				}
				continue;
			}
		}

		// verify pixel type
		if ( pfds[i].iPixelType != PFD_TYPE_RGBA )
		{
//			if ( r_verbose->integer )
			{
				fprintf(handle,//OutputDebugString(
					va("...PFD %d rejected, not RGBA\n", i) );
			}
			continue;
		}

		// verify proper flags
		if ( ( ( pfds[i].dwFlags & pPFD->dwFlags ) & pPFD->dwFlags ) != pPFD->dwFlags ) 
		{
//			if ( r_verbose->integer )
			{
				fprintf(handle,//OutputDebugString(
					va("...PFD %d rejected, improper flags (0x%x instead of 0x%x)\n", i, pfds[i].dwFlags, pPFD->dwFlags) );
			}
			continue;
		}

		// verify enough bits
		if ( pfds[i].cDepthBits < 15 )
		{
			fprintf(handle,va("...PFD %d rejected, depth bits only %d (<15)\n", i, pfds[i].cDepthBits) );
			continue;
		}
/*		if ( ( pfds[i].cStencilBits < 4 ) && ( pPFD->cStencilBits > 0 ) )
		{
			continue;
		}
*/
		//
		// selection criteria (in order of priority):
		// 
		//  PFD_STEREO
		//  colorBits
		//  depthBits
		//  stencilBits
		//
		if ( bestMatch )
		{
/*
			// check stereo
			if ( ( pfds[i].dwFlags & PFD_STEREO ) && ( !( pfds[bestMatch].dwFlags & PFD_STEREO ) ) && ( pPFD->dwFlags & PFD_STEREO ) )
			{
				bestMatch = i;
				continue;
			}
			
			if ( !( pfds[i].dwFlags & PFD_STEREO ) && ( pfds[bestMatch].dwFlags & PFD_STEREO ) && ( pPFD->dwFlags & PFD_STEREO ) )
			{
				bestMatch = i;
				continue;
			}
*/
			// check color
			if ( pfds[bestMatch].cColorBits != pPFD->cColorBits )
			{
				// prefer perfect match
				if ( pfds[i].cColorBits == pPFD->cColorBits )
				{
					bestMatch = i;
					continue;
				}
				// otherwise if this PFD has more bits than our best, use it
				else if ( pfds[i].cColorBits > pfds[bestMatch].cColorBits )
				{
					bestMatch = i;
					continue;
				}
			}

			// check depth
			if ( pfds[bestMatch].cDepthBits != pPFD->cDepthBits )
			{
				// prefer perfect match
				if ( pfds[i].cDepthBits == pPFD->cDepthBits )
				{
					bestMatch = i;
					continue;
				}
				// otherwise if this PFD has more bits than our best, use it
				else if ( pfds[i].cDepthBits > pfds[bestMatch].cDepthBits )
				{
					bestMatch = i;
					continue;
				}
			}
/*
			// check stencil
			if ( pfds[bestMatch].cStencilBits != pPFD->cStencilBits )
			{
				// prefer perfect match
				if ( pfds[i].cStencilBits == pPFD->cStencilBits )
				{
					bestMatch = i;
					continue;
				}
				// otherwise if this PFD has more bits than our best, use it
				else if ( ( pfds[i].cStencilBits > pfds[bestMatch].cStencilBits ) && 
					 ( pPFD->cStencilBits > 0 ) )
				{
					bestMatch = i;
					continue;
				}
			}
*/
		}
		else
		{
			bestMatch = i;
		}
	}

	fprintf(handle,"Bestmode: %d\n",bestMatch);
	
	if ( !bestMatch )
	{
		fprintf(handle,"No decent mode found!\n");
		fclose(handle);
		return 0;
	}

	if ( ( pfds[bestMatch].dwFlags & PFD_GENERIC_FORMAT ) != 0 )
	{
//		if ( !r_allowSoftwareGL->integer )
//		{
//			ri.Printf( PRINT_ALL, "...no hardware acceleration found\n" );
//			return 0;
//		}
//		else
		{
			fprintf(handle,//OutputDebugString(
				"...using software emulation\n" );
		}
	}
	else if ( pfds[bestMatch].dwFlags & PFD_GENERIC_ACCELERATED )
	{
		fprintf(handle,//OutputDebugString(
			"...MCD acceleration found\n" );
	}
	else
	{
		fprintf(handle,//OutputDebugString(
			"...hardware acceleration found\n" );
	}

	*pPFD = pfds[bestMatch];

	fclose(handle);

	return bestMatch;
}


/*
creates an OpenGL rendering context and makes it current
*/
void InitOpenGL( HWND hwnd )
{
	HDC hdc = GetDC( hwnd );
	mdview.hdc = hdc;

	static PIXELFORMATDESCRIPTOR pfd = 
	{
		sizeof(PIXELFORMATDESCRIPTOR),	// WORD  nSize;
		1,								// WORD  nVersion;				
		PFD_DRAW_TO_WINDOW |			// DWORD dwFlags;
		PFD_DOUBLEBUFFER   |			// 
		PFD_SUPPORT_OPENGL,				// 
		PFD_TYPE_RGBA,					// BYTE  iPixelType;
		24,								// BYTE  cColorBits;

								// not used to select mode...
								//
								0,								// BYTE  cRedBits;
								0,								// BYTE  cRedShift;
								0,								// BYTE  cGreenBits;
								0,								// BYTE  cGreenShift;
								0,								// BYTE  cBlueBits;
								0,								// BYTE  cBlueShift;
								0,								// BYTE  cAlphaBits;
								0,								// BYTE  cAlphaShift;
								0,								// BYTE  cAccumBits;
								0,								// BYTE  cAccumRedBits;
								0,								// BYTE  cAccumGreenBits;
								0,								// BYTE  cAccumBlueBits;
								0,								// BYTE  cAccumAlphaBits;

		32,//24,								// BYTE  cDepthBits;

								// not used to select mode...
								0,								// BYTE  cStencilBits;
								0,								// BYTE  cAuxBuffers;

		PFD_MAIN_PLANE,					// BYTE  iLayerType;

								// not used to select mode...
								0,								// BYTE  bReserved;
								0,								// DWORD dwLayerMask;
								0,								// DWORD dwVisibleMask;
								0								// DWORD dwDamageMask;
	};
/*
	// choose a pixel format that best matches the one we want...
	//
	int iPixelFormat = ChoosePixelFormat(hdc,&pfd);
	//
	// set the pixel format for this device context...
	//
*/
	int iPixelFormat = GLW_ChoosePFD( hdc, &pfd );

	VERIFY(SetPixelFormat(hdc, iPixelFormat, &pfd));
	   
	
//	SetPixelFormat( hdc );

	HGLRC glrc = wglCreateContext( hdc );
	mdview.glrc = glrc;
	
	if ( glrc == NULL) {	
		Error("Failed on wglCreateContext( HDC  hdc )");            
    }
                
    if (!wglMakeCurrent( hdc, glrc)) {        
        Error("Failed on wglMakeCurrent(..)" );
    }
}


bool gbMinimised = false;

/*
called every pass of the event loop
*/
void SysOnIdle()
{
//	OutputDebugString("Idle\n");
	if (!gbMinimised)
	{
		animation_loop();
	}
	else
	{
		Sleep(0);
	}
}

/*
renders the model with simple viewing parameters
*/
void SysOnPaint( HWND hwnd, bool bFlip /* = true */ ) 
{ 	
	if (!gbMinimised)
	{			
		PAINTSTRUCT ps;            
			
		BeginPaint(hwnd, &ps);

		wglMakeCurrent( mdview.hdc, mdview.glrc );
		render_mdview();

		if (bFlip)
		{
			HDC hDC = GetDC(hwnd);
			SwapBuffers( hDC );  
			ReleaseDC(hwnd,hDC);
		}

		EndPaint(hwnd, &ps);    
	}	
} 

/* 
notifies md3view of resize event 
*/
void SysOnSize(HWND hwnd, UINT state, int cx, int cy) 
{
	RECT rect;
	GetClientRect( hwnd, &rect );
	set_windowSize( rect.right, rect.bottom );

	g_iScreenWidth = rect.right - rect.left;
	g_iScreenHeight= rect.bottom- rect.top;

	gbMinimised = (state == SIZE_MINIMIZED);

	InvalidateRect( hwnd, NULL, FALSE );
}

/*
sets quit parameter to break out of the message loop
*/
int  SysOnDestroy(HWND hWnd)
{	
	Text_Destroy();	// while GL context still valid

	if (mdview.glrc)
	{
		wglDeleteContext( mdview.glrc );
		mdview.glrc = NULL;
	}

	if (mdview.hdc)
	{
		ReleaseDC(mdview.hwnd,mdview.hdc);
		mdview.hdc = NULL;
	}

	extern void FakeCvars_Shutdown(void);
	FakeCvars_Shutdown();

	mdview.done = true;	 	
	return 0; 	
}


/*
passes control to drag.cpp drag function
*/
POINT DragStartPoint;
void SysOnMouseMove(HWND hwnd, int x, int y, UINT keyFlags) 
{ 
	POINT point;
	GetCursorPos(&point);

	if (!(sys_rbuttondown || sys_lbuttondown || sys_mbuttondown)) 
	{
		//ScreenToClient(&point);
		return;
	}


	if (drag( (mkey_enum)keyFlags, point.x, point.y ))
	{
		SetCursorPos(DragStartPoint.x,DragStartPoint.y);
	}
}

/*
releases and shows the cursor again
*/
void SysOnRButtonUp(HWND hwnd, int x, int y, UINT flags) 
{ 
	if (!sys_rbuttondown) return;

	ReleaseCapture(); 
	ShowCursor( true ); 
	end_drag( (mkey_enum)flags, x, y );
	sys_rbuttondown = false;
}

/* 
same as above
*/
void SysOnLButtonUp(HWND hwnd, int x, int y, UINT flags) 
{ 
	if (!sys_lbuttondown) return;

	ReleaseCapture();
	ShowCursor( true ); 	
	end_drag( (mkey_enum)flags, x, y );
	sys_lbuttondown = false;
}

/*
on mouse down, hide the cursor and capture it to window
*/
void SysOnRButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags) 
{ 		
	GetCursorPos(&DragStartPoint);
	start_drag( (mkey_enum)keyFlags, DragStartPoint.x, DragStartPoint.y );
	SetCapture( hwnd );
	ShowCursor( false ); 
	sys_rbuttondown = true;
}

/*
*/
void SysOnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags) 
{ 		
	GetCursorPos(&DragStartPoint);

	start_drag( (mkey_enum)keyFlags, DragStartPoint.x, DragStartPoint.y );
	SetCapture( hwnd );
	ShowCursor( false ); 
	sys_lbuttondown = true;
}


/*
processes command issued from the tag menu
*/
// I've added an extra param to this call, it won't affect anyone else if you pass NULL as that param,
//	this involved a slight change below, but it's safe -slc
//
void SysOnTagCommand( HWND hwnd, int id, HWND hwndCtl, UINT codeNotify, char *psFullFilenameToUseInstead ) 
{
		int i = 0, tagID = id - ID_TAG_START;
		NodePosition pos;
		GLMODEL_DBLPTR dblptr;
		for (pos=tagMenuList.first() ; pos!=NULL ; pos=tagMenuList.after(pos)) {			
			if (i == tagID) break;
			i++;
		}

#ifdef DEBUG_PARANOID
		if (pos==NULL) Debug("tagMenuList entry not found");
#endif
		dblptr = (GLMODEL_DBLPTR)pos->element();
		
			char fullName[256]={0};
			OPENFILENAME *ofn=NULL;

			if (psFullFilenameToUseInstead)
			{
				strcpy(fullName,psFullFilenameToUseInstead);
			}
			else
			{
				ofn = FileOpenDialog(hwnd, id, codeNotify, hwndCtl, IDS_FILESTRING );
				if (ofn) 
				{
					strcpy( fullName, ofn->lpstrFile );
					strcat( fullName, ofn->lpstrFileTitle );
				}
			}

			if (strlen(fullName))
			{			
				if (dblptr != 0) freemdl_fromtag(dblptr);
				loadmdl_totag( fullName, dblptr );				
				
				InvalidateRect( hwnd, NULL, FALSE );
				
				//leave this here!! without the screen doesn't 
				//refresh properly sometimes!!
				InvalidateRect( mdview.hwnd, NULL, FALSE );
				if (ofn)
					free( ofn );			
			}
			else 
			{
				if (dblptr != 0) 
				{
					freemdl_fromtag( dblptr );
				}
			}

}



// we get here when the user selects one of the anim sequences on the new pulldown menus...
//
void SysOnAnimsCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify) 
{
//	OutputDebugString(va("Menu item code %d\n",id));	

	if (id < ID_MENUITEMS_LOWERANIMS)
	{
		// upper anim clicked on...
		//
		int iSelection = id-ID_MENUITEMS_UPPERANIMS;

		// 0 = choose seq
		// 1 = choose multi-seq
		// 2 = none
		// 3 = 1st seq
		// 4 = 2nd seq etc...

		switch (iSelection)
		{
			case 0:	// choose seq
			case 1: // choose multi-seq

				ErrorBox("Ignore this for now");
				iAnimLockNumber_Upper = 0; // xxxxxxxxxxxxxxxxxxxxx for now...
				break;

			default:
			
				iAnimLockNumber_Upper = iSelection-2;
				break;			
		}
	}
	else
	{
		// lower anim clicked on...
		//
		int iSelection = id-ID_MENUITEMS_LOWERANIMS;

		// 0 = choose seq
		// 1 = choose multi-seq
		// 2 = none
		// 3 = 1st seq
		// 4 = 2nd seq etc...

		switch (iSelection)
		{
			case 0:	// choose seq
			case 1: // choose multi-seq

				ErrorBox("Ignore this for now");
				iAnimLockNumber_Lower = 0; // xxxxxxxxxxxxxxxxxxxxx for now...
				break;

			default:

				iAnimLockNumber_Lower = iSelection-2;
				break;
		}
	}
}

#if 0
// takes (eg) "q:\quake\baseq3\textures\borg\name.tga"
//
//	and produces "q:\quake\baseq3\"		// note the trailing backslash, for this app only!!!!!!!!!!!!!!!!!!!!
//
// (copied from ShaderEd, but this app doesn't have MFC, so no CStrings...)
//
LPCSTR Filename_QUAKEBASEONLY(LPCSTR psFullPathedName /* CString &string */)
{
	static char sLine[1024];
	char *p;

	strcpy(sLine,psFullPathedName);

	while ((p=strchr(sLine,'/'))!=0) *p='\\';	// string.Replace("/","\\");
	strlwr(sLine);								// string.MakeLower();

/*	int loc = string.Find("\\quake");
	if (loc>=0)
	{
		loc = string.Find("\\",loc+1);	// pointing at "\\baseq3" (or "demoq3" etc)
		if (loc>=0)
		{
			loc = string.Find("\\",loc+1);	// pointing at first part of string past the quake dir stuff
			string = string.Left(loc);
		}
	}
*/

	p = strstr(sLine,"\\quake");
	if (p)
	{
		p = strstr(p+1,"\\");	// pointing at "\\baseq3" (or "demoq3" etc)
		if (p)
		{
			p = p = strstr(p+1,"\\");	// pointing at first part of string past the quake dir stuff
			*(p+1)=0;	// +1 ensures trailing slash is left on as well
		}
	}

	return sLine;
}
#endif



#define BASEDIRNAME "base" 
char		qdir[1024];
char		gamedir[1024];		// q:\quake\baseef

// totally hacky and awful code pasted from other bits of crud
LPCSTR Filename_QUAKEBASEONLY(LPCSTR psFullPathedName  )
{
	static char sLine[1024];
	char temp[1024];
	char *path = temp;
	const char	*c;
	const char *sep;
	int		len, count;

	sLine[0]=0;
	
	strcpy(path,psFullPathedName);
	_strlwr(path);
	
	// search for "base" in path from the RIGHT hand side (and must have a [back]slash just before it)
	
	len = strlen(BASEDIRNAME);
	for (c=path+strlen(path)-1 ; c != path ; c--)
	{
//		int i;
		
		if (!strnicmp (c, BASEDIRNAME, len)
			&& 
			(*(c-1) == '/' || *(c-1) == '\\')	// would be more efficient to do this first, but only checking after a strncasecmp ok ensures no invalid pointer-1 access
			)
		{			
			sep = c + len;
			count = 1;
			while (*sep && *sep != '/' && *sep != '\\')
			{
				sep++;
				count++;
			}
			strncpy (gamedir, path, c+len+count-path);			
			gamedir[c+len+count-path]=0;
			strncpy (qdir, path, c-path);
			qdir[c-path] = 0;
		}
	}

	strcpy(sLine,gamedir);
	while ((path=strchr(sLine,'/'))!=NULL) *path='\\';
	return sLine;
}





bool ExportThisModel( LPCSTR psFilename, gl_model* pModel, bool bExportAsMD3);
bool R_ExportModel( gl_model* pModel, bool bMD3)
{		
	bool bReturn = true;

	if (pModel)
	{
		OutputDebugString(va("Exporting model \"%s\"\n",pModel->sMDXFullPathname));

		// export this model...
		//
		ExportThisModel( va("%s%s",Filename_WithoutExt(pModel->sMDXFullPathname), bMD3?".md3":".glm"), pModel, bMD3 );
		
		// export its children...
		//
		for (UINT j=0; j<pModel->iNumTags ; j++) 
		{			
			gl_model *pChild = pModel->linkedModels[j];
			if (pChild)
			{
				if (!R_ExportModel(pChild, bMD3))
				{
					bReturn = false;
				}
			}
		}	
	}

	return bReturn;
}


void ExportMD3(void)
{	
	if (GetYesNo("Export as MD3, are you sure?"))
	{
		if (!R_ExportModel( mdview.baseModel, true ))
		{
			ErrorBox("Errors occured, unable to export as MD3!");
		}
		else
		{
			ExportThisModel( NULL, NULL, true);	// bool bExportAsMD3
		}
	}
}

bool g_bPerfect = false;
void ExportG2(bool bPerfect)
{
	g_bPerfect = bPerfect;

	if ( mdview.baseModel )
	{
		if (GetYesNo(va("Export as Ghoul2 .GLM/.GLA files%s, are you sure?",bPerfect?" ( *without* 90-degree skew )":"")))
		{
			if (!R_ExportModel( mdview.baseModel, false ))
			{
				ErrorBox("Errors occured, unable to export as Ghoul2!");
			}
			else
			{
				ExportThisModel( NULL, NULL, false);	// bool bExportAsMD3
			}
		}
	}
	else
	{
		ErrorBox( "No model loaded!");
	}
}




/*
processes events from the menu
*/
void SysOnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify) 
{ 
	// process tag menu requests
	if ((id >= ID_TAG_START) && (id < ID_TAG_START+tagMenuList.size())) {
		SysOnTagCommand( hwnd, id, hwndCtl, codeNotify, NULL );
		InvalidateRect( mdview.hwnd, NULL, FALSE );
	}

	if (id >= ID_MENUITEMS_UPPERANIMS && id<ID_MENUITEMS_NEXT)
	{
		SysOnAnimsCommand(hwnd, id, hwndCtl, codeNotify );
		FrameAdvanceAnim(0);	// cheat, force it to re-evaluate legaliser in case new locks are on
		InvalidateRect( hwnd, NULL, FALSE );
	}			

	switch(id)
	{
		case ID_FILE_OPEN:
		{
			char sFullName[256];
			strcpy(sFullName,"");

			if (getCmdLine())
			{
				strcpy( sFullName, getCmdLine() );
				FreeCmdLine();
			}
			else
			{
				OPENFILENAME *ofn;
				ofn = FileOpenDialog(hwnd, id, codeNotify, hwndCtl, IDS_FILESTRING );
				if (ofn) 
				{				
					strcpy( sFullName, ofn->lpstrFile );
					strcat( sFullName, ofn->lpstrFileTitle );
					free( ofn );
				}
			}

			if (strlen(sFullName))
			{
				strcpy( mdview.basepath, Filename_QUAKEBASEONLY(sFullName));
			
				free_mdviewdata();

				bool bOk = loadmdl( sFullName );

				// if model loaded ok, and it was called lower.md3, and had a tag called "tag_torso", then
				// automatically (try) load "upper.md3" from the same place
				bool bLowerMD3 = !stricmp(Filename_WithoutPath(sFullName),"lower.md3");
				bool bLowerMDR = !stricmp(Filename_WithoutPath(sFullName),"lower.mdr");

				if (bOk)
				{
					if ((bLowerMD3||bLowerMDR) && giTagMenuSubtractValue_Torso!=-1)
					{
						char sFullPathedName_Upper[MAX_PATH];

						strcpy(sFullPathedName_Upper,va("%s\\upper.%s",Filename_PathOnly(sFullName),bLowerMD3?"md3":"mdr"));

						if (file_exists(sFullPathedName_Upper))
						{
							pModel_Lower =	pLastLoadedModel;
											pLastLoadedModel = NULL;

							int newID = tagMenuList.size();				// get to end of tag list
								newID-= giTagMenuSubtractValue_Torso;	// back up to "tag_torso"
								newID+= ID_TAG_START;					// account for resource.h value					

								SysOnTagCommand( hwnd, newID, hwndCtl, codeNotify, sFullPathedName_Upper);

							pModel_Upper = pLastLoadedModel;	// NULL or ptr

							// switch on flat texturing and filtering (looks better for what we need)...
							//
							SysOnCommand(hwnd, ID_VIEW_TEXTURED, hwndCtl, codeNotify);
							SysOnCommand(hwnd, ID_VIEW_FILTEREDTEXTURE, hwndCtl, codeNotify);


							// now try auto-loading the head...
							//
							if (giTagMenuSubtractValue_Head!=-1)
							{
								char sFullPathedName_Head[MAX_PATH];

								strcpy(sFullPathedName_Head,va("%s\\head.md3",Filename_PathOnly(sFullName)));

								if (file_exists(sFullPathedName_Head))
								{
									pLastLoadedModel = NULL;

									int newID = tagMenuList.size();				// get to end of tag list
										newID-= giTagMenuSubtractValue_Head;	// back up to "tag_head"
										newID+= ID_TAG_START;					// account for resource.h value					

									SysOnTagCommand( hwnd, newID, hwndCtl, codeNotify, sFullPathedName_Head);
								}
							}
						}

						// now attempt the animation stuff...  (this is now done whether or not an UPPER model exists, to
						//										cope with weird stuff like the vermin model)
						//
						HDC hDC = GetDC(hwnd);
						LoadAnimationCFG(va("%s\\animation.cfg",Filename_PathOnly(sFullName)),hDC);
						ReleaseDC(hwnd,hDC);

						InvalidateRect( hwnd, NULL, FALSE );
					}
					else
					{
						// if you load something with "weapon" in it's path somewhere, and that ends in "_hand.md3", then
						//	try and auto-load the main components of a weapon onto the appropriate tags...
						//
						if (strstr(String_ToLower(sFullName),"weapon") && strstr(String_ToLower(sFullName),"_hand.md3"))
						{
							char sWeaponBasename[MAX_PATH];

							strcpy(sWeaponBasename,Filename_WithoutExt(Filename_WithoutPath(sFullName)));
							*strrchr(sWeaponBasename,'_')=0;	// '_' will be present at this point

							int iWeaponID = tagMenuList.size();				// get to end of tag list
								iWeaponID-= giTagMenuSubtractValue_Weapon;	// back up to correct tag
								iWeaponID+= ID_TAG_START;					// account for resource.h value					

							int iBarrelID = tagMenuList.size();				// get to end of tag list
								iBarrelID-= giTagMenuSubtractValue_Barrel;	// back up to correct tag
								iBarrelID+= ID_TAG_START;					// account for resource.h value					

							int iBarrel2ID = tagMenuList.size();				// get to end of tag list
								iBarrel2ID-= giTagMenuSubtractValue_Barrel2;	// back up to correct tag
								iBarrel2ID+= ID_TAG_START;					// account for resource.h value					

							int iBarrel3ID = tagMenuList.size();				// get to end of tag list
								iBarrel3ID-= giTagMenuSubtractValue_Barrel3;	// back up to correct tag
								iBarrel3ID+= ID_TAG_START;					// account for resource.h value					

							int iBarrel4ID = tagMenuList.size();				// get to end of tag list
								iBarrel4ID-= giTagMenuSubtractValue_Barrel4;	// back up to correct tag
								iBarrel4ID+= ID_TAG_START;					// account for resource.h value					

							int iTagMenuSubtractValue_Weapon	= giTagMenuSubtractValue_Weapon;
							int iTagMenuSubtractValue_Barrel	= giTagMenuSubtractValue_Barrel;
							int iTagMenuSubtractValue_Barrel2	= giTagMenuSubtractValue_Barrel2;
							int iTagMenuSubtractValue_Barrel3	= giTagMenuSubtractValue_Barrel3;
							int iTagMenuSubtractValue_Barrel4	= giTagMenuSubtractValue_Barrel4;

							if (iTagMenuSubtractValue_Weapon	!= -1)
								SysOnTagCommand( hwnd, iWeaponID, hwndCtl, codeNotify, va("%s\\%s.md3",		   Filename_PathOnly(sFullName),sWeaponBasename));
							if (iTagMenuSubtractValue_Barrel	!= -1)
								SysOnTagCommand( hwnd, iBarrelID, hwndCtl, codeNotify, va("%s\\%s_barrel.md3", Filename_PathOnly(sFullName),sWeaponBasename));
							if (iTagMenuSubtractValue_Barrel2	!= -1)
								SysOnTagCommand( hwnd, iBarrel2ID,hwndCtl, codeNotify, va("%s\\%s_barrel2.md3",Filename_PathOnly(sFullName),sWeaponBasename));
							if (iTagMenuSubtractValue_Barrel3	!= -1)
								SysOnTagCommand( hwnd, iBarrel3ID,hwndCtl, codeNotify, va("%s\\%s_barrel3.md3",Filename_PathOnly(sFullName),sWeaponBasename));
							if (iTagMenuSubtractValue_Barrel4	!= -1)
								SysOnTagCommand( hwnd, iBarrel4ID,hwndCtl, codeNotify, va("%s\\%s_barrel4.md3",Filename_PathOnly(sFullName),sWeaponBasename));
						}
					}
				}
				
				InvalidateRect( hwnd, NULL, FALSE );
				
				//leave this here!! without the screen doesn't 
				//refresh properly sometimes!!
				InvalidateRect( mdview.hwnd, NULL, FALSE );				
			}
			break;
		}
		case ID_FILE_EXPORTTORAW:
		{
			OPENFILENAME *ofn;
			ofn = FileSaveDialog(hwnd, id, codeNotify, hwndCtl, IDS_RAWFILEFILTER );
			if (ofn) 
			{
				char fullName[256];
				strcpy( fullName, ofn->lpstrFileTitle );
				
			
				write_baseModelToRaw( fullName );
					
				free( ofn );
			}
			break;
		}
		case ID_FILE_SAVE_MD3:
		{
			ExportMD3();
			break;
		}		
		case ID_FILE_SAVE_G2:
		{
			ExportG2(false);
			break;
		}
		case ID_FILE_SAVE_G2_PERFECT:
		{
			ExportG2(true);
			break;
		}
		case ID_FILE_IMPORTSKIN:
		{
			OPENFILENAME *ofn;
			ofn = FileOpenDialog(hwnd, id, codeNotify, hwndCtl, IDS_SKINFILEFILTER );
			if (ofn) 
			{
				char fullName[256];
				strcpy( fullName, ofn->lpstrFile );
				strcat( fullName, ofn->lpstrFileTitle );
			
				importNewSkin( fullName );
				
				InvalidateRect( hwnd, NULL, FALSE );
				
				//leave this here!! without the screen doesn't 
				//refresh properly sometimes!!
				InvalidateRect( mdview.hwnd, NULL, FALSE );
				free( ofn );
			}
			break;
		}
		case ID_FILE_IMPORTMULTISEQ:
		{
			OPENFILENAME *ofn;
			ofn = FileOpenDialog(hwnd, id, codeNotify, hwndCtl, IDS_SEQFILTER );
			if (ofn)
			{
				char fullName[256];
				strcpy( fullName, ofn->lpstrFile );
				strcat( fullName, ofn->lpstrFileTitle );

				ParseSequenceLockFile( fullName );
				
				InvalidateRect( hwnd, NULL, FALSE );
				
				//leave this here!! without the screen doesn't 
				//refresh properly sometimes!!
				InvalidateRect( mdview.hwnd, NULL, FALSE );
				free( ofn );
			}
			break;
		}
		case ID_FILE_EXIT:
		{
			mdview.done = true;
			break;
		}
		case ID_ABOUT:
		{
			MessageBox(hwnd, ABOUT_TEXT, "About",MB_OK | MB_ICONINFORMATION);
			break;
		}

		case ID_SCREENSHOT_CLIPBOARD:
		{
			StartWait();	
			
				gbTextInhibit = true;
				SysOnPaint( hwnd, false );	// false = no buffer flip
				ScreenShot(NULL,"(C) Raven Software 2000");
				gbTextInhibit = false;

				void *pvDIB;
				int iBytes;
				if (BMP_GetMemDIB(pvDIB, iBytes))
				{
					ClipBoard_SendDIB(pvDIB, iBytes);
				}

			EndWait();

			InvalidateRect( hwnd, NULL, FALSE );
			break;
		}
		case ID_SCREENSHOT_FILE:
		{	
			gbTextInhibit = true;
			SysOnPaint( hwnd, false );	// false = no buffer flip			

			gl_model *model = NULL;

			// get the last model loaded to use as snapshot name base...
			//
			if (!mdview.modelList->isEmpty()) 
			{
				for (NodePosition pos=mdview.modelList->first(); pos!=NULL ; pos=mdview.modelList->after(pos))
				{
					model = (gl_model *)pos->element();
				}

				assert(model);
				if (model)
				{
					char sBaseName[MAX_PATH];

					sprintf(sBaseName, Filename_WithoutPath(Filename_PathOnly(model->sMDXFullPathname)));

					int iName;
					for (iName=0; iName<1000; iName++)
					{
						char sFilename[MAX_PATH];

						sprintf(sFilename, "c:\\%s_%03d.bmp",sBaseName,iName);

						if (!FileExists(sFilename))
						{
							StartWait();

							ScreenShot(sFilename,"(C) Raven Software 2000");

							EndWait();
							break;
						}
					}
					if (iName==1000)
					{
						ErrorBox("Couldn't find a free save slot!");
					}
				}
			}
			else
			{
				ErrorBox("No model load to work out path from!\n\n( So why take a snapshot, dopey? :-)");
			}

			gbTextInhibit = false;
			InvalidateRect( hwnd, NULL, FALSE );
			break;
		}
		case ID_VIEW_COLOURPICKER:
		{
			CHOOSECOLOR cc;
			static COLORREF  crefs[16];

			memset(&cc,0,sizeof(cc));

			cc.lStructSize	= sizeof(cc);
			cc.hwndOwner	= mdview.hwnd;
//			cc.hInstance	= NULL;
			cc.lpCustColors	= crefs;
			cc.rgbResult	= mdview._B<<16 | mdview._G<<8 | mdview._R;	//  COLORREF     rgbResult; 
			cc.Flags		= CC_RGBINIT | CC_ANYCOLOR | CC_SOLIDCOLOR | /*CC_FULLOPEN | */ 0;

			if (ChooseColor(&cc))
			{
				DWORD d = cc.rgbResult;				
				mdview._B = (cc.rgbResult>>16) & 0xFF;
				mdview._G = (cc.rgbResult>>8 ) & 0xFF;
				mdview._R = (cc.rgbResult>>0 ) & 0xFF;
			} 
		}
		break;

		case ID_VIEW_REFRESHTEXTURE:
			refreshTextureRes();
			InvalidateRect( hwnd, NULL, FALSE );
			break;

		case ID_VIEW_WIREFRAME: 
			oglStateWireframe(); 
			InvalidateRect( hwnd, NULL, FALSE );
			break;

		case ID_VIEW_FLATSHADED: 
			oglStateShadedFlat(); 
			InvalidateRect( hwnd, NULL, FALSE );
			break;

		case ID_VIEW_TEXTURED: 
			oglStateFlatTextured(); 
			InvalidateRect( hwnd, NULL, FALSE );
			break;

		case ID_VIEWPOS_RESET:
			extern void reset_viewpos(void);
			reset_viewpos();
			InvalidateRect( hwnd, NULL, FALSE );
			break;

		case ID_VIEW_TEXTUREDWIREFRAME:
			oglStateShadedTexturedAndWireFrame();
			InvalidateRect( hwnd, NULL, FALSE );
			break;
			
		case ID_VIEW_TEXTUREDSHADED: 
			oglStateShadedTextured();
			InvalidateRect( hwnd, NULL, FALSE );
			break;

		case ID_VIEW_NEAREST:
			mdview.texMode = TEX_FAST;
			setTextureFilter();
			InvalidateRect( hwnd, NULL, FALSE );
			break;

		case ID_VIEW_UNFILTEREDTEXTURE:
			mdview.texMode = TEX_UNFILTERED;
			setTextureFilter();
			InvalidateRect( hwnd, NULL, FALSE );
			break;

		case ID_VIEW_FILTEREDTEXTURE:
			mdview.texMode = TEX_FILTERED;
			setTextureFilter();
			InvalidateRect( hwnd, NULL, FALSE );
			break;

		case ID_VIEW_LOADEDSTUFF:
			char *GetLoadedModelInfo(void);
			InfoBox(GetLoadedModelInfo());
			InvalidateRect( hwnd, NULL, FALSE );
			break;

		case ID_ANIMATION_SLOWER:
			mdview.animSpeed *= ANIM_SLOWER;
			break;

		case ID_ANIMATION_FASTER:
			mdview.animSpeed *= ANIM_FASTER;
			break;

		case ID_ANIMATION_STOP:
			mdview.animate = false;
			break;

		case ID_ANIMATION_START:
			mdview.animate = true;
			break;

		case ID_ANIMATION_REWIND:
			rewindAnim();
			InvalidateRect( hwnd, NULL, FALSE );
			break;

		case ID_FRAME_DW:
			FrameAdvanceAnim(-1);
			InvalidateRect( hwnd, NULL, FALSE );
			break;

		case ID_FRAME_UP:
			FrameAdvanceAnim(1)	;
			InvalidateRect( hwnd, NULL, FALSE );
			break;

		case ID_LOD0:
			SetLODLevel(0);
			InvalidateRect( hwnd, NULL, FALSE );
			break;

		case ID_LOD1:
			SetLODLevel(1);
			InvalidateRect( hwnd, NULL, FALSE );
			break;

		case ID_LOD2:
			SetLODLevel(2);
			InvalidateRect( hwnd, NULL, FALSE );
			break;

		extern void TextureList_ReMip(int iMIPLevel);

		case ID_PICMIP0:
			
			TextureList_ReMip(0);
			InvalidateRect( hwnd, NULL, FALSE );
			break;

		case ID_PICMIP1:
			
			TextureList_ReMip(1);
			InvalidateRect( hwnd, NULL, FALSE );
			break;

		case ID_PICMIP2:
			
			TextureList_ReMip(2);
			InvalidateRect( hwnd, NULL, FALSE );
			break;

		case ID_PICMIP3:
			
			TextureList_ReMip(3);
			InvalidateRect( hwnd, NULL, FALSE );
			break;

		case ID_PICMIP4:
			
			TextureList_ReMip(4);
			InvalidateRect( hwnd, NULL, FALSE );
			break;

		case ID_PICMIP5:
			
			TextureList_ReMip(5);
			InvalidateRect( hwnd, NULL, FALSE );
			break;

		case ID_PICMIP6:
			
			TextureList_ReMip(6);
			InvalidateRect( hwnd, NULL, FALSE );
			break;

		case ID_PICMIP7:
			
			TextureList_ReMip(7);
			InvalidateRect( hwnd, NULL, FALSE );
			break;

		case ID_FACE_INCANIM:
			SetFaceSkin(1);
			InvalidateRect( hwnd, NULL, FALSE );
			break;
		case ID_FACE_DECANIM:
			SetFaceSkin(-1);
			InvalidateRect( hwnd, NULL, FALSE );
			break;
		case ID_FACE_RESANIM:
			SetFaceSkin(0);
			InvalidateRect( hwnd, NULL, FALSE );
			break;

		case ID_VIEWAXIS:
			mdview.bAxisView = !mdview.bAxisView;
			InvalidateRect( hwnd, NULL, FALSE );
			break;

		case ID_VIEWALPHA:
			mdview.bUseAlpha = !mdview.bUseAlpha;
			InvalidateRect( hwnd, NULL, FALSE );
			break;

		case ID_VIEW_FOVTOGGLE:
			mdview.dFOV = (mdview.dFOV == 10.0f)?80.0f:(mdview.dFOV == 80.0f)?90:10.0f;

			RECT rect;
			GetClientRect( hwnd, &rect );
			set_windowSize( rect.right, rect.bottom );

			InvalidateRect( hwnd, NULL, FALSE);
			break;

		case ID_VIEW_BOUNDSTOGGLE:
			mdview.bBBox = !mdview.bBBox;
			InvalidateRect( hwnd, NULL, FALSE );
			break;

		case ID_VIEWLOWERANIM_INC:
			if (RunningNT() == 4)
			{
				if (++iAnimDisplayNumber_Lower > Animation_GetNumLowerSequences())	// num can be size+1 @ max (0=no seq lock)
					iAnimDisplayNumber_Lower = 0;
				InvalidateRect( hwnd, NULL, FALSE );
			}
			break;

		case ID_VIEWLOWERANIM_DEC:
			if (RunningNT() == 4)
			{
				if (--iAnimDisplayNumber_Lower < 0)
					  iAnimDisplayNumber_Lower = Animation_GetNumLowerSequences();
				InvalidateRect( hwnd, NULL, FALSE );
			}
			break;

		case ID_VIEWLOWERANIM_LOCK:
			if (RunningNT() == 4)
			{
				iAnimLockNumber_Lower = iAnimDisplayNumber_Lower;
				FrameAdvanceAnim(0);	// cheat, force it to re-evaluate legaliser in case new locks are on
				InvalidateRect( hwnd, NULL, FALSE );
			}
			break;

		case ID_VIEWUPPERANIM_INC:
			if (RunningNT() == 4)
			{
				if (++iAnimDisplayNumber_Upper > Animation_GetNumUpperSequences())	// num can be size+1 @ max (0=no seq lock)
					iAnimDisplayNumber_Upper = 0;
				InvalidateRect( hwnd, NULL, FALSE );
			}
			break;

		case ID_VIEWUPPERANIM_DEC:
			if (RunningNT() == 4)
			{
				if (--iAnimDisplayNumber_Upper < 0)
					iAnimDisplayNumber_Upper = Animation_GetNumUpperSequences();
				InvalidateRect( hwnd, NULL, FALSE );
			}
			break;

		case ID_VIEWUPPERANIM_LOCK:
			if (RunningNT() == 4)
			{
				iAnimLockNumber_Upper = iAnimDisplayNumber_Upper;
				FrameAdvanceAnim(0);	// cheat, force it to re-evaluate legaliser in case new locks are on
				InvalidateRect( hwnd, NULL, FALSE );
			}
			break;

		case ID_ANIMATION_INTERPOLATE:
			if (mdview.interpolate) mdview.interpolate = false;
			else mdview.interpolate = true;
			break;
	}

	// I see no reason not to do this, it also helps with things like toggling interp OFF when doing a slow anim, which
	//	wouldn't otherwise instantly cause a screen redraw to remove the work "(interpolated)" from the screen...
	//
	InvalidateRect( hwnd, NULL, FALSE );
}


void SysOnKeyDown(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags) { 

}
void SysOnKeyUp(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags) { }



bool SysOnCreate(HWND hwnd, CREATESTRUCT FAR* lpCreateStruct) 
{ 
	mdview.hwnd = hwnd;
	InitOpenGL( hwnd );
    initialize_glstate();

	extern void OnceOnly_GLVarsInit(void);
	OnceOnly_GLVarsInit();
	extern void FakeCvars_OnceOnlyInit(void);
	FakeCvars_OnceOnlyInit();

	RECT rect;
	GetClientRect( hwnd, &rect );
	set_windowSize( rect.right, rect.bottom );	
	
    return true;
}




/* ---------------------------------------------- dialog code -------------------------------------------- */

/*
saves the directory name after every dialog use
*/

char	szDirName[256]={0};
void SetDirectory(LPOPENFILENAME lpofn)
{
    lpofn->lpstrFile[lpofn->nFileOffset] = '\0';
    lstrcpy(szDirName, lpofn->lpstrFile);
	//lstrcpy(mdview.basepath, lpofn->lpstrFile);
}


/*
handles the file open dialog
*/
OPENFILENAME *FileOpenDialog(HWND hwnd, WORD wCommand, WORD wNotify, HWND hwndCtrl, int type)
{
    OPENFILENAME *ofn = (OPENFILENAME *)malloc(sizeof( OPENFILENAME ));
	memset( ofn, 0, sizeof( OPENFILENAME ) );
    static char szFile[256];       // filename string
    static char szFileTitle[256];  // file-title string
    static char szFilter[256];     // filter string

    char chReplace;         // strparator for szFilter
    int i, cbString;        // integer count variables

    // Retrieve the current directory name and store it in szDirName.

    if (szDirName[0] == '\0')
    {
		GetCurrentDirectory(256,szDirName);
    }

    // Place the terminating null character in the szFile.

    szFile[0] = '\0';

    // Load the filter string from the resource file.

    cbString = LoadString(WinhInstance, type, szFilter, sizeof(szFilter));

    // Add a terminating null character to the filter string.

    chReplace = szFilter[cbString - 1];
    for (i = 0; szFilter[i] != '\0'; i++)
    {
        if (szFilter[i] == chReplace)
            szFilter[i] = '\0';
    }

    // Set the members of the OPENFILENAME structure.

    ofn->lStructSize = sizeof(OPENFILENAME);
    ofn->hwndOwner = hwnd;
    ofn->lpstrFilter = szFilter;
    ofn->nFilterIndex = 1;
    ofn->lpstrFile = szFile;
    ofn->nMaxFile = sizeof(szFile);
    ofn->lpstrFileTitle = szFileTitle;
    ofn->nMaxFileTitle = sizeof(szFileTitle);
    ofn->lpstrInitialDir = szDirName;
    ofn->Flags = OFN_SHOWHELP | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    // Display the Open dialog box.

    if (GetOpenFileName(ofn))
    {        
        SetDirectory(ofn);
		return ofn;
    }
	return NULL;
}

/*
handles the file save dialog
*/
OPENFILENAME *FileSaveDialog(HWND hwnd, WORD wCommand, WORD wNotify, HWND hwndCtrl, int type)
{
    OPENFILENAME *ofn = (OPENFILENAME *)malloc(sizeof( OPENFILENAME ));
	memset( ofn, 0, sizeof( OPENFILENAME ) );
    static char szFile[256];       // filename string
    static char szFileTitle[256];  // file-title string
    static char szFilter[256];     // filter string
    char chReplace;         // strparator for szFilter
    int i, cbString;        // integer count variables

    // Retrieve the current directory name and store it in szDirName.

    if (szDirName[0] == '\0')
    {
        GetCurrentDirectory(256,szDirName);
    }

    // Place the terminating null character in the szFile.

    szFile[0] = '\0';

    // Load the filter string from the resource file.

    cbString = LoadString(WinhInstance, type, szFilter, sizeof(szFilter));

    // Add a terminating null character to the filter string.

    chReplace = szFilter[cbString - 1];
    for (i = 0; szFilter[i] != '\0'; i++)
    {
        if (szFilter[i] == chReplace)
            szFilter[i] = '\0';
    }

    // Set the members of the OPENFILENAME structure.

    ofn->lStructSize = sizeof(OPENFILENAME);
    ofn->hwndOwner = hwnd;
    ofn->lpstrFilter = szFilter;
    ofn->nFilterIndex = 1;
    ofn->lpstrFile = szFile;
    ofn->nMaxFile = sizeof(szFile);
    ofn->lpstrFileTitle = szFileTitle;
    ofn->nMaxFileTitle = sizeof(szFileTitle);
    ofn->lpstrInitialDir = szDirName;
    ofn->Flags = OFN_SHOWHELP | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    // Display the Open dialog box.

    if (GetSaveFileName(ofn))
    {        
        //SetDirectory(ofn);
		return ofn;
    }
	return NULL;
}

#endif