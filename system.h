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

#ifndef _SYSTEM_H_
#define _SYSTEM_H_

/*
system dependent function exported to the rest of the application
redefine these for each platform being ported
*/

#ifdef WIN32
	#define _CRT_SECURE_NO_WARNINGS
	#define _CRT_NONSTDC_NO_WARNINGS
	#include <windows.h>
	#include <windowsx.h>
#endif

#include <GL\gl.h>
#include <GL\glu.h>
#include <iostream>
#include <memory.h>
#include <stdio.h>
#include <time.h>
#include <math.h>

double getDoubleTime (void);

void	repaint_main();
void	set_cursor( int x, int y );

bool	file_exists( LPCSTR fname );
char	*getCmdLine();
void	FreeCmdLine();
void	swap_buffers();

typedef int GLMODEL_DBLPTR;
void	tagMenu_append( char *name, GLMODEL_DBLPTR model );
void    tagMenu_seperatorAppend( char *name );
void    tagMenu_remove( GLMODEL_DBLPTR tagid );

#include "Error.h"
#include "Base.h"
#include "BaseMesh.h"



//
// I did it this way to avoid other modules having to know about HMENUs...
//
void Menu_UpperAnims_Clear(void);
void Menu_LowerAnims_Clear(void);
void Menu_UpperAnims_AddItem(LPCSTR psItem);
void Menu_LowerAnims_AddItem(LPCSTR psItem);

#ifndef g_iScreenWidth
#include "text.h"
#endif
bool ScreenShot(LPCSTR psFilename = NULL, LPCSTR psCopyrightMessage = NULL, int iWidth = g_iScreenWidth, int iHeight = g_iScreenHeight);


#endif