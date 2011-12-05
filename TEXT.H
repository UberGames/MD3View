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

#ifndef TEXT_H
#define TEXT_H

#define TEXT_WIDTH 8
#define TEXT_DEPTH 8

//void Text_Create(void);	// called automatically internally
void Text_Destroy(void);	// should be called at program shutdown
void Text_Display(LPCSTR psString, Vec3 v3DPos, byte r, byte g, byte b);
int  Text_DisplayFlat(LPCSTR psString, int x, int y, byte r, byte g, byte b, bool bResizeStringIfNecessary = false);

extern int g_iScreenWidth;
extern int g_iScreenHeight;
extern bool gbTextInhibit;


#endif	// #ifndef TEXT_H

////////////////////////// eof ///////////////////////////



