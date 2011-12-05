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
#include "ndictionary.h"
#include "md3gl.h"
#include "md3view.h"

void setVertex( Vec3 v, float f1, float f2, float f3 )
{
	v[0] = f1;
	v[1] = f2;
	v[2] = f3;
}

void widget_AxisFaces( Vec3 *v )
{
	glBegin( GL_TRIANGLES );

    glVertex3fv( v[0] ); glVertex3fv( v[2] ); glVertex3fv( v[3] ); 
    glVertex3fv( v[3] ); glVertex3fv( v[1] ); glVertex3fv( v[0] );  
    glVertex3fv( v[4] ); glVertex3fv( v[5] ); glVertex3fv( v[7] );  
    glVertex3fv( v[7] ); glVertex3fv( v[6] ); glVertex3fv( v[4] );  
    glVertex3fv( v[0] ); glVertex3fv( v[1] ); glVertex3fv( v[5] );  
    glVertex3fv( v[5] ); glVertex3fv( v[4] ); glVertex3fv( v[0] );  
    glVertex3fv( v[1] ); glVertex3fv( v[3] ); glVertex3fv( v[7] );  
    glVertex3fv( v[7] ); glVertex3fv( v[5] ); glVertex3fv( v[1] );  
    glVertex3fv( v[3] ); glVertex3fv( v[2] ); glVertex3fv( v[6] );  
    glVertex3fv( v[6] ); glVertex3fv( v[7] ); glVertex3fv( v[3] );  
    glVertex3fv( v[2] ); glVertex3fv( v[0] ); glVertex3fv( v[4] );  
    glVertex3fv( v[4] ); glVertex3fv( v[6] ); glVertex3fv( v[2] );  
	
    glEnd();
}


void widget_Axis()
{
	Vec3 v[8];
	
	setVertex( v[0],	-1.0000,	1.0000,	0.0000   );
	setVertex( v[1],	1.0000,	1.0000,	0.0000   );
	setVertex( v[2],	-1.0000,	1.0000,	10.0000 );
	setVertex( v[3],	1.0000,	1.0000,	10.0000 );
	setVertex( v[4],	-1.0000,	-1.0000,	0.0000   );
	setVertex( v[5],	1.0000,	-1.0000,	0.0000   );
	setVertex( v[6],	-1.0000,	-1.0000,	10.0000 );
	setVertex( v[7],	1.0000,	-1.0000,	10.0000 );

	glColor3f( 0, 0, 1 );
	widget_AxisFaces( v );

	setVertex( v[0],	-1.0000,	0.0000,   	1.0000 );
	setVertex( v[1],	1.0000,	0.0000,   	1.0000 );
	setVertex( v[2],	-1.0000,	10.0000, 	1.0000 );
	setVertex( v[3],	1.0000,	10.0000, 	1.0000 );
	setVertex( v[4],	-1.0000,	0.0000,   	-1.0000 );
	setVertex( v[5],	1.0000,	0.0000,   	-1.0000 );
	setVertex( v[6],	-1.0000,	10.0000, 	-1.0000 );
	setVertex( v[7],	1.0000,	10.0000, 	-1.0000 );

	glColor3f( 0, 1, 0 );
	widget_AxisFaces( v );

	setVertex( v[0], 0.0000,   	-1.0000, 	1.0000 );
	setVertex( v[1], 0.0000,   	1.0000, 	1.0000 );
	setVertex( v[2], 10.0000, 	-1.0000, 	1.0000 );
	setVertex( v[3], 10.0000, 	1.0000, 	1.0000 );
	setVertex( v[4], 0.0000,   	-1.0000, 	-1.0000 );
	setVertex( v[5], 0.0000,   	1.0000, 	-1.0000 );
	setVertex( v[6], 10.0000, 	-1.0000, 	-1.0000 );
	setVertex( v[7], 10.0000, 	1.0000, 	-1.0000 );

	glColor3f( 1, 0, 0 );
	widget_AxisFaces( v );
}
