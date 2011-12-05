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
#include "diskio.h"

void putLittle16 ( INT16 num , FILE *f )
{
	union
	{
		struct
		{
	        UINT8    b1, b2;
		};
		UINT16	 i1;
	} u;

	u.i1 = num;

	fputc( u.b1, f );
	fputc( u.b2, f );
}

INT16 getLittle16 (FILE *f)
{
	union
	{
		struct
		{
	        UINT8    b1, b2;
		};
		UINT16	 i1;
	} u;

	u.b1 = fgetc(f);
	u.b2 = fgetc(f);

	return u.i1;
}

void putLittle32 ( INT32 num , FILE *f )
{
	union 
	{
		struct
		{
	        UINT8    b1, b2, b3, b4;
		};
		UINT32	 i1;
	} u;

	u.i1 = num;

	fputc( u.b1, f );
	fputc( u.b2, f );
	fputc( u.b3, f );
	fputc( u.b4, f );
}

INT32 getLittle32 (FILE *f)
{
	union 
	{
		struct
		{
	        UINT8    b1, b2, b3, b4;
		};
		UINT32	 i1;
	} u;

	u.b1 = fgetc(f);
	u.b2 = fgetc(f);
	u.b3 = fgetc(f);
	u.b4 = fgetc(f);

	return u.i1;
}

void putLittleFloat( float num  , FILE *f )//32bit floating point number
{
	union 
	{
		struct
		{
	        UINT8    b1, b2, b3, b4;
		};
		float	 f1;
	} u;

	u.f1 = num;

	fputc( u.b1, f );
	fputc( u.b2, f );
	fputc( u.b3, f );
	fputc( u.b4, f );
}

float getLittleFloat (FILE *f) //32bit floating point number
{
	union 
	{
		struct
		{
			UINT8    b1, b2, b3, b4;
		};
		float	 f1;
	} u;

	u.b1 = fgetc(f);
	u.b2 = fgetc(f);
	u.b3 = fgetc(f);
	u.b4 = fgetc(f);

	return u.f1;
}

void putBig16 ( INT16 num , FILE *f )
{
	union 
	{
		struct
		{
	        UINT8    b1, b2, b3, b4;
		};
		INT16	 i1;
	} u;

	u.i1 = num;

	fputc( u.b2, f );
	fputc( u.b1, f );
}

INT16 getBig16 (FILE *f)
{
	union 
	{
		struct
		{
	        UINT8    b1, b2;
		};
		UINT16	 i1;
	} u;

	u.b2 = fgetc(f);
	u.b1 = fgetc(f);

	return u.i1;
}

void putBig32 ( INT32 num , FILE *f )
{
	union 
	{
		struct
		{
	        UINT8    b1, b2, b3, b4;
		};
		INT32	 i1;
	} u;

	u.i1 = num;

	fputc( u.b4, f );
	fputc( u.b3, f );
	fputc( u.b2, f );
	fputc( u.b1, f );
}

INT32 getBig32 (FILE *f)
{
	union 
	{
		struct
		{
	        UINT8    b1, b2, b3, b4;
		};
		UINT32	 i1;
	} u;

	u.b4 = fgetc(f);
	u.b3 = fgetc(f);
	u.b2 = fgetc(f);
	u.b1 = fgetc(f);

	return u.i1;
}

void putBigFloat ( float num , FILE *f ) //32bit floating point number
{
	union 
	{
		struct
		{
	        UINT8    b1, b2, b3, b4;
		};
		float	 f1;
	} u;

	u.f1 = num;

	fputc( u.b4, f );
	fputc( u.b3, f );
	fputc( u.b2, f );
	fputc( u.b1, f );
}

float getBigFloat (FILE *f) //32bit floating point number
{
	union 
	{
		struct
		{
			UINT8    b1, b2, b3, b4;
		};
		float	 f1;
	} u;

	u.b4 = fgetc(f);
	u.b3 = fgetc(f);
	u.b2 = fgetc(f);
	u.b1 = fgetc(f);

	return u.f1;
}
