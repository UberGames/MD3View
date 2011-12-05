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

/*
win32 specific driver file:
this include the WinMain function and all the code to initialize the win32 gui
look for comments in md3view.h to see how the viewer system independent code needs to be
initialized and used in the framwork
*/

#ifdef WIN32

#include "system.h"
#include "ndictionary.h"
#include "md3gl.h"
#include "md3view.h"
#include "resource.h"
#include "AFXRES.H"
#include "oddbits.h"
#include "bmp.h"

HINSTANCE	WinhInstance;
HWND		mainhWnd;
char*	CmdLine = NULL;

#define WINDOW_STYLE WS_OVERLAPPEDWINDOW

// system specific event handler functions
bool SysOnCreate(HWND hwnd, CREATESTRUCT FAR* lpCreateStruct);
int  SysOnDestroy(HWND hWnd);		
void SysOnMouseMove(HWND hwnd, int x, int y, UINT keyFlags);
void SysOnRButtonUp(HWND hwnd, int x, int y, UINT flags);
void SysOnLButtonUp(HWND hwnd, int x, int y, UINT flags);
void SysOnRButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags);
void SysOnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags);
void SysOnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
void SysOnKeyDown(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags);
void SysOnKeyUp(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags);
void SysOnPaint( HWND hwnd, bool bFlip = true );
void SysOnSize(HWND hwnd, UINT state, int cx, int cy);
void SysOnIdle();

NodeSequenceInfo tagMenuList;

/*
add a seperator
*/
void tagMenu_seperatorAppend( char *name )
{
	char newname[512];
	strcpy( newname, "...");
	strcat( newname, name );
	strcat( newname, "...");
	char *n = new char[strlen(newname)+1];
	strcpy( n, newname );

	HMENU subMenu = GetSubMenu( GetMenu(mdview.hwnd), TAG_MENU_ID );
	AppendMenu( subMenu, MF_DISABLED|MF_STRING, ID_TAG_START+tagMenuList.size(), n );
	tagMenuList.insertLast( (Object)NULL );
}

/*
remove a tag entry from the menu
*/
void tagMenu_remove( GLMODEL_DBLPTR tagid )
{
	NodePosition pos;
	GLMODEL_DBLPTR dblptr;
	int i=1;
	bool remove=false;

	// get the tag menu position
	for (pos=tagMenuList.first() ; pos!=NULL ; pos=tagMenuList.after(pos)) {
		dblptr = (GLMODEL_DBLPTR)pos->element();
		if (dblptr == tagid) break;
		i++;
	}

	// return if not found
	if (!pos) return;
	
	// remove the menu
	HMENU subMenu = GetSubMenu( GetMenu(mdview.hwnd), TAG_MENU_ID );
	DeleteMenu( subMenu, i, MF_BYPOSITION );
		
	// see if this is the last tag entry in a block
	// if so remove the seperator
	NodePosition b = tagMenuList.before(pos);
	NodePosition a = tagMenuList.after(pos);
	

	if (b==0) {
		if (a==0) remove = true;
		else if (a->element() == 0) remove = true;
	}
	else if  (b->element() == 0) {	
		if (a==0) remove = true;
		else if (a->element() == 0) remove = true;
	}

	if (remove) {
			DeleteMenu( subMenu, i-1, MF_BYPOSITION );
			tagMenuList.remove( b );
	}
		
	
	// remove the pos from list
	tagMenuList.remove( pos );	
}


/*
add a tag to the menu
*/
void tagMenu_append( char *name, GLMODEL_DBLPTR model )
{
	char *n = new char[strlen(name)+1];
	strcpy( n, name );

	HMENU subMenu = GetSubMenu( GetMenu(mdview.hwnd), TAG_MENU_ID );
	AppendMenu( subMenu, MF_ENABLED|MF_STRING, ID_TAG_START+tagMenuList.size(), n );
	
	tagMenuList.insertLast( (Object)model );
}


void swap_buffers()
{
	SwapBuffers( mdview.hdc );
}

char *getCmdLine()
{		
	return CmdLine;
}
void FreeCmdLine()
{
	if (CmdLine)
	{		
		delete [] CmdLine;	
		CmdLine = NULL;
	}
}

bool file_exists( LPCSTR fname )
{
	if (!fname) return false;
	FILE *f = fopen( fname, "r" );
	if (f) {
		fclose(f);
		return true;
	}
	else {
		return false;
	}
}

void repaint_main()
{
	InvalidateRect( mdview.hwnd, NULL, FALSE );
}


void set_cursor( int x, int y )
{
	SetCursorPos( x, y );  
}

/*
time measuring stuff
*/
double getDoubleTime (void)
{
	return (double)clock() / (double)CLOCKS_PER_SEC;
}

/*
main event handler
*/


// event handler itself
LONG WINAPI WinProcInstance(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{

	  switch (message)
	  {
		HANDLE_MSG( hwnd, WM_DESTROY,     SysOnDestroy );
		HANDLE_MSG( hwnd, WM_CLOSE,	      SysOnDestroy );
		HANDLE_MSG( hwnd, WM_CREATE,	  SysOnCreate );     
		HANDLE_MSG( hwnd, WM_PAINT,	      SysOnPaint );
		HANDLE_MSG( hwnd, WM_SIZE,		  SysOnSize );     
		HANDLE_MSG( hwnd, WM_COMMAND,     SysOnCommand );  	 
		HANDLE_MSG( hwnd, WM_LBUTTONDOWN, SysOnLButtonDown );
		HANDLE_MSG( hwnd, WM_RBUTTONDOWN, SysOnRButtonDown );
		HANDLE_MSG( hwnd, WM_LBUTTONUP,   SysOnLButtonUp );
		HANDLE_MSG( hwnd, WM_RBUTTONUP,   SysOnRButtonUp );
		HANDLE_MSG( hwnd, WM_KEYDOWN,     SysOnKeyDown );
		HANDLE_MSG( hwnd, WM_KEYUP,       SysOnKeyUp );     
		case WM_MOUSEMOVE:
		{
			SysOnMouseMove(hwnd, (int)(short)LOWORD(lParam),(int)(short)HIWORD(lParam),(UINT)(wParam));
			return 0L;
		}
		default:       
			return(DefWindowProc(hwnd, message, wParam, lParam));
	  }
}





HMENU hMainMenu = NULL;
HMENU hMenuUpperAnims = NULL;
HMENU hMenuLowerAnims = NULL;

// clears menu, then adds items "none" and seperator...
//
void Menu_Clear(HMENU hMenu, int iBaseID)
{
	if ( hMenu )
	{
		int iCount = GetMenuItemCount( hMenu );

		for (int i=iCount-1; i>=0; i--)
		{
			VERIFY(DeleteMenu( hMenu, i, MF_BYPOSITION));
		}
		DrawMenuBar(mainhWnd);

		// add default "choose seq" and "choose multi-seq", then a seperator...
		//																	   
		AppendMenu(	hMenu,						// HMENU hMenu,         // handle to menu to be changed
					MF_STRING,					// UINT uFlags,         // menu-item flags
					iBaseID + 0,				// UINT_PTR uIDNewItem, // menu-item identifier or handle to drop-down menu or submenu
					"** Choose Seq **"			// LPCTSTR lpNewItem    // menu-item content
					);

		AppendMenu(	hMenu,						// HMENU hMenu,         // handle to menu to be changed
					MF_STRING,					// UINT uFlags,         // menu-item flags
					iBaseID + 1,				// UINT_PTR uIDNewItem, // menu-item identifier or handle to drop-down menu or submenu
					"** Choose Multi-Seq **"	// LPCTSTR lpNewItem    // menu-item content
					);

		AppendMenu(	hMenu,						// HMENU hMenu,         // handle to menu to be changed
					MF_SEPARATOR,				// UINT uFlags,         // menu-item flags
					NULL,						// UINT_PTR uIDNewItem, // menu-item identifier or handle to drop-down menu or submenu
					NULL						// LPCTSTR lpNewItem    // menu-item content
					);

		// now add a default "none" and a seperator...
		//
		AppendMenu(	hMenu,						// HMENU hMenu,         // handle to menu to be changed
					MF_STRING,					// UINT uFlags,         // menu-item flags
					iBaseID + 2,				// UINT_PTR uIDNewItem, // menu-item identifier or handle to drop-down menu or submenu
					"(none)"					// LPCTSTR lpNewItem    // menu-item content
					);

		AppendMenu(	hMenu,						// HMENU hMenu,         // handle to menu to be changed
					MF_SEPARATOR,				// UINT uFlags,         // menu-item flags
					NULL,						// UINT_PTR uIDNewItem, // menu-item identifier or handle to drop-down menu or submenu
					NULL						// LPCTSTR lpNewItem    // menu-item content
					);

		DrawMenuBar(mainhWnd);

		iCount = GetMenuItemCount( hMenu );
	}
}

void Menu_AddItem(HMENU hMenu, int iBaseID, LPCSTR psItem)
{
	if ( hMenu )
	{
		int iID = GetMenuItemCount( hMenu );
			iID-= 2;		// compensate for 2 seperators, now this is ID of next we're about to add

		AppendMenu(	hMenu,						// HMENU hMenu,         // handle to menu to be changed
					MF_STRING,			// UINT uFlags,         // menu-item flags
					iBaseID + iID,				// UINT_PTR uIDNewItem, // menu-item identifier or handle to drop-down menu or submenu
					psItem						// LPCTSTR lpNewItem    // menu-item content
					);

		DrawMenuBar(mainhWnd);
	}
}

void Menu_UpperAnims_Clear(void)
{
	Menu_Clear( hMenuUpperAnims, ID_MENUITEMS_UPPERANIMS );
}

void Menu_LowerAnims_Clear(void)
{
	Menu_Clear( hMenuLowerAnims, ID_MENUITEMS_LOWERANIMS );
}

void Menu_UpperAnims_AddItem(LPCSTR psItem)
{
	Menu_AddItem( hMenuUpperAnims, ID_MENUITEMS_UPPERANIMS, psItem );
}

void Menu_LowerAnims_AddItem(LPCSTR psItem)
{
	Menu_AddItem( hMenuLowerAnims, ID_MENUITEMS_LOWERANIMS, psItem );
}




/*
creates window
*/

void WindowSystemInit( HINSTANCE hInstance )
{
	WNDCLASS wc;

    memset (&wc, 0, sizeof(wc));
    
    wc.style         = 0;
    wc.lpfnWndProc   = (WNDPROC)WinProcInstance;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInstance;
    wc.hIcon         = LoadIcon   (NULL, IDI_APPLICATION);
    wc.hCursor       = LoadCursor (NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(BLACK_BRUSH);
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = "mainWindow";

    RegisterClass(&wc);

	mainhWnd = CreateWindow (
				"mainWindow" , 
				FILENAME,
                WINDOW_STYLE,
                0, 0, 
				1000, // unfortunately I can't just #ifdef these 2 lines because CreateWindow is also a macro and C can't nest 'em... <sigh>
				600,	//
                0,
                LoadMenu(hInstance,MAKEINTRESOURCE(IDR_MENU1)),
                hInstance,
                NULL);

    ShowWindow ( mainhWnd, SW_SHOW );
    UpdateWindow ( mainhWnd );	

	hMainMenu = GetMenu(mainhWnd);
	hMenuUpperAnims = CreateMenu();
	hMenuLowerAnims = CreateMenu();

	AppendMenu(	hMainMenu,					// HMENU hMenu,         // handle to menu to be changed
				MF_POPUP|MF_STRING,			// UINT uFlags,         // menu-item flags
				(UINT_PTR) hMenuUpperAnims,	// UINT_PTR uIDNewItem, // menu-item identifier or handle to drop-down menu or submenu
				"(Upper Anim Sequences)"	// LPCTSTR lpNewItem    // menu-item content
				);

	AppendMenu(	hMainMenu,					// HMENU hMenu,         // handle to menu to be changed
				MF_POPUP|MF_STRING,			// UINT uFlags,         // menu-item flags
				(UINT_PTR) hMenuLowerAnims,	// UINT_PTR uIDNewItem, // menu-item identifier or handle to drop-down menu or submenu
				"(Lower Anim Sequences)"	// LPCTSTR lpNewItem    // menu-item content
				);

	Menu_UpperAnims_Clear();
	Menu_LowerAnims_Clear();

	DrawMenuBar(mainhWnd);

	if (getCmdLine() != NULL )
	{
		SysOnCommand(mainhWnd, ID_FILE_OPEN, 0, 0);
//		if (!loadmdl( getCmdLine() )) {
//			Debug( "could not load %s", getCmdLine() );		
	}
}

/*
initializes app
calls
*/
void Init( HINSTANCE hInstance )
{		
	WindowSystemInit( hInstance );    
}


/*
main program entry point
*/
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
     MSG        msg;    

	 if ( ( lpCmdLine    != NULL ) && 
		  ( lpCmdLine[0] != '\0' ) )
	 {
		 CmdLine = new char[strlen(lpCmdLine)+1];
		 strcpy(CmdLine,lpCmdLine);

		 // sometimes the OS puts quotes around the whole command line (duh!!!!), so get rid of them...
		 //
		 if (CmdLine[0]=='"' && CmdLine[strlen(CmdLine)-1]=='"')
		 {
			 strcpy(CmdLine,lpCmdLine+1);
			 CmdLine[strlen(CmdLine)-1]=0;
		 }
		 while (strchr(CmdLine,'/')) *strchr(CmdLine,'/')='\\'; 
	 }

	 WinhInstance = hInstance;

	 /* initilizes mdview data */
	 init_mdview(lpCmdLine);

	 Init( hInstance );
	 mdview.done = false;
	 
     // main message loop     
     while (!mdview.done)
     {		 
			 SysOnIdle();
			 while (PeekMessage (&msg, NULL, 0, 0, PM_REMOVE))
			 {						
	      		TranslateMessage (&msg);
				TranslateAccelerator( mainhWnd, LoadAccelerators( hInstance, MAKEINTRESOURCE(IDR_ACCELERATOR1)), &msg ); 
      			DispatchMessage (&msg);						
			 }
     }   	 
	
	shutdown_mdviewdata();
	return 1;


}		



// if psFilename == NULL, takes a memory screenshot in DIB format (for copying to clipboard)
//
bool ScreenShot(LPCSTR psFilename,			// else NULL = take memory snapshot (for clipboard)
				LPCSTR psCopyrightMessage,	// /* = NULL */
				int iWidth,					// /* = <screenwidth>  */
				int iHeight					// /* = <screenheight> */
				)
{
	bool bReturn = false;

	int iOldPack;	
	glGetIntegerv(GL_PACK_ALIGNMENT,&iOldPack);
	glPixelStorei(GL_PACK_ALIGNMENT,1);
	
	void *pvGLPixels = malloc (iWidth * iHeight * 3);	// 3 = R,G,B

	if (pvGLPixels)
	{
		if (psCopyrightMessage)
		{
			bool bOldInhibit = gbTextInhibit;
			gbTextInhibit = false;
			Text_DisplayFlat(psCopyrightMessage, 0, (iHeight-TEXT_DEPTH)-1,255,255,255);	// y-1 = aesthetic only
			gbTextInhibit = bOldInhibit;
		}

		glReadPixels(	0,					// x
						0,					// y (from bottom left)
						iWidth,				// width
						iHeight,			// height
						GL_RGB,				// format
						GL_UNSIGNED_BYTE,	// type
						pvGLPixels			// buffer ptr 			
						);

		// save area is valid size...
		//
		if (BMP_Open(psFilename, iWidth, iHeight))
		{
			for (int y=0; y<iHeight; y++)				
			{
				LPGLRGBBYTES 
				lpGLRGBBytes = (LPGLRGBBYTES) pvGLPixels;
				lpGLRGBBytes+= y * iWidth;

				for (int x=0; x<iWidth; x++, lpGLRGBBytes++)
				{
					BMP_WritePixel(lpGLRGBBytes->r,lpGLRGBBytes->g,lpGLRGBBytes->b);
				}

				BMP_WriteLinePadding(iWidth);	// arg is # pixels per row	
			}

			BMP_Close(psFilename,false);	// false = bFlipFinal			

			bReturn = true;
		}

		free(pvGLPixels);
		pvGLPixels = NULL;	// yeah...yeah
	}	

	glPixelStorei(GL_PACK_ALIGNMENT,iOldPack);

	return bReturn;
}


#endif
