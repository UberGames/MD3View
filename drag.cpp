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

int m_x, m_y;

/*! commands to handle mouse dragging, uses key_flags defines above */
void start_drag( mkey_enum keyFlags, int x, int y )
{
	m_x = x;
	m_y = y;
}

bool drag(  mkey_enum keyFlags, int x, int y )
{
	bool bRepaintAndSetCursor = false;

	if ( keyFlags != 0 )
	{
		if ( keyFlags & KEY_LBUTTON )
		{				
			if ((x != m_x) || (y != m_y))
			{
				short s = GetAsyncKeyState(VK_MENU);
				if (s & 0x8000)
				{
					mdview.xPos += ((float)(x - m_x)/10.f) * MOUSE_XPOS_SCALE;
					mdview.yPos -= ((float)(y - m_y)/10.f) * MOUSE_YPOS_SCALE;
				}
				else
				{
					s = GetAsyncKeyState(0x5A);	// Z key
					if ( s&0x8000)
					{
						mdview.rotAngleZ += (float)(x - m_x) * MOUSE_ROT_SCALE;
//						mdview.rotAngleZ += (float)(y - m_y) * MOUSE_ROT_SCALE;
						if (mdview.rotAngleZ> 360.0f) mdview.rotAngleZ=mdview.rotAngleZ-360.0f;
						if (mdview.rotAngleZ<-360.0f) mdview.rotAngleZ=mdview.rotAngleZ+360.0f;
					}
					else
					{
						mdview.rotAngleY += (float)(x - m_x) * MOUSE_ROT_SCALE;
						mdview.rotAngleX += (float)(y - m_y) * MOUSE_ROT_SCALE;
						if (mdview.rotAngleY> 360.0f) mdview.rotAngleY=mdview.rotAngleY-360.0f;
						if (mdview.rotAngleY<-360.0f) mdview.rotAngleY=mdview.rotAngleY+360.0f;
						if (mdview.rotAngleX> 360.0f) mdview.rotAngleX=mdview.rotAngleX-360.0f;
						if (mdview.rotAngleX<-360.0f) mdview.rotAngleX=mdview.rotAngleX+360.0f;
					}
				}
				repaint_main();
				set_cursor( m_x, m_y );  
				bRepaintAndSetCursor = true;
			}
		} else
		if ( keyFlags & KEY_RBUTTON ) 
		{
			if ( y != m_y )
			{
				mdview.zPos += ((float)(y - m_y)/10.f) * MOUSE_ZPOS_SCALE;
				if (mdview.zPos<-1000.f) mdview.zPos=-1000.f;
				if (mdview.zPos> 1000.f) mdview.zPos= 1000.f;
				InvalidateRect( mdview.hwnd, NULL, FALSE );
				set_cursor( m_x , m_y );  
				bRepaintAndSetCursor = true;
			}
		}
	}
	return bRepaintAndSetCursor;
}

void end_drag(  mkey_enum keyFlags, int x, int y )
{
}


/*
=======
rotates an object by placing a trackball sphere around it
=======
*/
/*
int ModelControl::TrackballRotate(SNodePrimitive *shape, int sx, int sy, int ex, int ey)
{
  if (!m_shape) return 0;
  
  // simple quick method to avoid NAN results
  if ((abs(sx - ex) < 2) && (abs(sy - ey) < 2)) {
    return 0;
  }

  IAVector scale( TRACKBALL_SCALE, TRACKBALL_SCALE, TRACKBALL_SCALE);  
  IAMatrix tbMatrix = scale_mat(scale) * shape->m_inv_CTM;
  IAPoint isecPnt2, isecPnt1, center, origin;
  IAVector vec1, vec2, crossVec;
  Idouble dotA, angle;

  if (!m_tracer->ISectRayWithSphere( tbMatrix, &isecPnt1, sx, sy, 
				     m_scene->width(), m_scene->height() )) { 
    return 0;
  }

  if (!m_tracer->ISectRayWithSphere( tbMatrix, &isecPnt2, ex, ey, 
				     m_scene->width(), m_scene->height() )) {    
    return 0;
  }
  
  center = shape->m_CTM * center;
  vec1 = isecPnt1 - center;
  vec2 = isecPnt2 - center; 
  vec1.normalize();
  vec2.normalize();

  crossVec = cross( vec1, vec2 );
  dotA = dot( vec1, vec2 );
  angle = -atan2( 1, dotA );
  
  shape->m_CTM_rotate = rot_mat(origin, crossVec, angle) * shape->m_CTM_rotate;
  shape->updateCTM();
  
  return 1;
}

SNodePrimitive *ISectTracer::ShootRay( int x, int y, int width, int height,
				       Intersection *isec, Ray *ray )
{
  NodePosition pos;
  Ray filmRay, worldRay, objectRay;
  IAMatrix filmToWorld = m_scene->camera()->filmToWorld();    
  Intersection iStack[MAX_INTERSECT_PER_RAY], *iStackP, *iStackP2, *bestISec;
  int numIntersect, isectTotal;
  NodeSequenceInfo *gData = m_scene->m_geometryList;
  Idouble tbest;
  SNodePrimitive *primData;
  Shape *rayShape;
  

      filmRay.d[X] = ((double)(2*x)/(double)width) - 1;
      filmRay.d[Y] = 1-((double)(2*y)/(double)height);     
      
      filmRay.d[Z] = -1; filmRay.d[W] = 1;
      filmRay.P = IAPoint();
      
      worldRay.d = filmToWorld * filmRay.d;
      worldRay.P = filmToWorld * filmRay.P;
      normalize( worldRay.d );

      iStackP = iStack;   
      isectTotal = 0;
      bestISec = NULL;
      
      // calculate intersections for all shapes
      for (pos = gData->first(); pos != NULL; pos = gData->after(pos) ) {
	primData = (SNodePrimitive *)pos->element();                           // get entry from geometry list
	rayShape = primData->m_shape;

	objectRay.d = primData->m_inv_CTM * worldRay.d;                      // transform ray to object space
        objectRay.P = primData->m_inv_CTM * worldRay.P;	

	if (!rayShape->IntersectTest( &objectRay )) {
	  if (filmRay.d[X] != 0) continue;
	}

      	numIntersect = rayShape->Intersect( &objectRay, iStackP, primData ); // calculate intersection       

	iStackP += numIntersect;
	isectTotal += numIntersect;
      }


      // find best t for the pixel
      if (iStackP > iStack) {	 
	tbest = MAX_FLOAT;
	for (int i = 0; i < isectTotal; i++) {
	  if (iStack[i].m_t < tbest) {
	    tbest = iStack[i].m_t;
	    bestISec = &iStack[i];
	  }
	}
      }
       
      if (bestISec) { 	
	// copy the relevant data
	if (ray) {
	  ray->P = worldRay.P;
	  ray->d = worldRay.d;
	}
	if (isec) bestISec->copy( isec );
      	return bestISec->m_shape;
      }
      else {        
        return NULL;
      }
}

int ISectTracer::ISectRayWithSphere( IAMatrix &object, IAPoint *isec,
				     int x, int y, int width, int height)
{
  Ray filmRay, worldRay, objectRay;
  IAMatrix filmToWorld = m_scene->camera()->filmToWorld();    
  
  filmRay.d[X] = ((double)(2*x)/(double)width) - 1;
  filmRay.d[Y] = 1-((double)(2*y)/(double)height); 
  filmRay.d[Z] = -1; filmRay.d[W] = 1;
  filmRay.P = IAPoint();  

  worldRay.d = filmToWorld * filmRay.d;
  worldRay.P = filmToWorld * filmRay.P;
  normalize( worldRay.d );

  objectRay.d = object * worldRay.d;
  objectRay.P = object * worldRay.P;

  IAVector d = objectRay.d;
  IAPoint P =  objectRay.P;

  double a = d[Z]*d[Z] + d[X]*d[X] + d[Y]*d[Y]; 
  double b = 2*(P[X]*d[X] + P[Z]*d[Z] + P[Y]*d[Y]);
  double c = P[X]*P[X] + P[Y]*P[Y] + P[Z]*P[Z] - 0.25;
  double t, det = b*b-4*a*c;

  if (det > 0) { 
    t = (-b+det)/a;
    *isec = worldRay.P + worldRay.d * t;
    return 1;
  }
  else return 0;
}

int CubeShape::Intersect( Ray *ray, Intersection *iStack, SNodePrimitive *data )
{
  double t, det, u, v;
  IAVector d = ray->d;
  IAPoint P = ray->P;
  Intersection *iStackP = iStack;

  // test intersection with z up face
  if (d[Z] != 0) { 
    t = -((P[Z]+0.5) / d[Z]);
    u = P[X] + d[X]*t;
    v = P[Y] + d[Y]*t;

    if ((u > -0.5) && (u < 0.5) && (v > -0.5) && (v < 0.5)) {
       iStackP->m_t = t;
       iStackP->m_shape = data;
       iStackP->m_faceID = FACE_ZPOS;
       iStackP++;
    }
  }

  // test intersection with z down face
  if (d[Z] != 0) { 
    t = -((-P[Z]+0.5) / -d[Z]);
    u = P[X] + d[X]*t;
    v = P[Y] + d[Y]*t;

    if ((u > -0.5) && (u < 0.5) && (v > -0.5) && (v < 0.5)) {
       iStackP->m_t = t;
       iStackP->m_shape = data;
       iStackP->m_faceID = FACE_ZNEG;
       iStackP++;
    }
  }

    // test intersection with y up face
  if (d[Y] != 0) { 
    t = -((P[Y]+0.5) / d[Y]);
    u = P[X] + d[X]*t;
    v = P[Z] + d[Z]*t;

    if ((u > -0.5) && (u < 0.5) && (v > -0.5) && (v < 0.5)) {
       iStackP->m_t = t;
       iStackP->m_shape = data;
       iStackP->m_faceID = FACE_YPOS;
       iStackP++;
    }
  }

  // test intersection with y down face
  if (d[Y] != 0) { 
    t = -((-P[Y]+0.5) / -d[Y]);
    u = P[X] + d[X]*t;
    v = P[Z] + d[Z]*t;

    if ((u > -0.5) && (u < 0.5) && (v > -0.5) && (v < 0.5)) {
       iStackP->m_t = t;
       iStackP->m_faceID = FACE_YNEG;
       iStackP->m_shape = data;
       iStackP++;
    }
  }

    // test intersection with x up face
  if (d[X] != 0) { 
    t = -((P[X]+0.5) / d[X]);
    u = P[Z] + d[Z]*t;
    v = P[Y] + d[Y]*t;

    if ((u > -0.5) && (u < 0.5) && (v > -0.5) && (v < 0.5)) {
       iStackP->m_t = t;
       iStackP->m_faceID = FACE_XPOS;
       iStackP->m_shape = data;
       iStackP++;
    }
  }

  // test intersection with x down face
  if (d[X] != 0) { 
    t = -((-P[X]+0.5) / -d[X]);
    u = P[Z] + d[Z]*t;
    v = P[Y] + d[Y]*t;

    if ((u > -0.5) && (u < 0.5) && (v > -0.5) && (v < 0.5)) {
       iStackP->m_t = t;
       iStackP->m_shape = data;
       iStackP->m_faceID = FACE_XNEG;
       iStackP++;
    }
  }
    
  return (int)(iStackP-iStack);  
}
*/