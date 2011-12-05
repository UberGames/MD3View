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


#ifndef _DISKIO_H_
#define _DISKIO_H_

void  putLittle16    ( INT16 num , FILE *f );
INT16 getLittle16    ( FILE *f );
void  putLittle32    ( INT32 num , FILE *f );
INT32 getLittle32    ( FILE *f );
void  putLittleFloat ( float num , FILE *f ); //32bit floating point number
float getLittleFloat ( FILE *f ); //32bit floating point number
void  putBig16       ( INT16 num , FILE *f );
INT16 getBig16       ( FILE *f );
void  putBig32       ( INT32 num , FILE *f );
INT32 getBig32       ( FILE *f );
void  putBigFloat    ( float num , FILE *f ); //32bit floating point number
float getBigFloat    ( FILE *f ); //32bit floating point number

#define get16		getLittle16
#define get32		getLittle32
#define getFloat	getLittleFloat
#define put16		putLittle16
#define put32		putLittle32
#define putFloat	putLittleFloat

#endif