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

#ifndef TR_JPEG_INTERFACE_H
#define TR_JPEG_INTERFACE_H


#ifdef __cplusplus
extern "C"
{
#endif


#ifndef LPCSTR 
typedef const char * LPCSTR;
#endif

void LoadJPG( const char *filename, unsigned char **pic, int *width, int *height );

void JPG_ErrorThrow(LPCSTR message);
void JPG_MessageOut(LPCSTR message);
#define ERROR_STRING_NO_RETURN(message) JPG_ErrorThrow(message)
#define MESSAGE_STRING(message)			JPG_MessageOut(message)


void *JPG_Malloc( int iSize );
void JPG_Free( void *pvObject);


#ifdef __cplusplus
};
#endif



#endif	// #ifndef TR_JPEG_INTERFACE_H


////////////////// eof //////////////////

