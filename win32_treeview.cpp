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
#include <windows.h>
#endif

class Core
{
public:
	Core( int argc, char **argv );

	void exec();
private:
};

class Event;

class Window
{
public:
	Window( Window *parent=NULL, int style=0, const char *label=NULL );
	virtual ~Window();

	void setGeometry( int x, int y, int w, int  h );
	
	virtual void repaint();
	virtual void event( Event *evt );

private:

#ifdef WIN32
	HWND g_hwnd;
	HDC  g_hdc;
#endif

};

class WindowTreeView
{
public:
private:
};