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

/*
error handler
*/

void Error (char *error, ...)
{
        va_list argptr;
        char    text[1024];       

        va_start (argptr,error);
        vsprintf (text, error,argptr);
        va_end (argptr);

        MessageBox( NULL, text, "Error", MB_OK );

        exit (1);
}


/* 
debugging output
*/
void Debug (char *error, ...)
{
        va_list argptr;
        char    text[4096];       

        va_start (argptr,error);
        vsprintf (text, error,argptr);
        va_end (argptr);
		
        MessageBox( NULL, text, "Debug", MB_OK );        
}
