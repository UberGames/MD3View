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
#include "DiskIO.h"
#include "Targa.h"
#include "jpeg_interface.h"

typedef unsigned char byte;

struct TargaHeader {
        unsigned char   id_length, colormap_type, image_type;
        unsigned short  colormap_index, colormap_length;
        unsigned char   colormap_size;
        unsigned short  x_origin, y_origin, width, height;
        unsigned char   pixel_size, attributes;
};

void Error (char *error, ...);
void Debug (char *error, ...);

struct wordByte
{
	unsigned char byte1;
	unsigned char byte2;
};

union wordByteUnion
{
	short word;
	wordByte b;
};

/*
=======
accepts RGB format data in pixels and writes it uncompressed into file
=======
*/

void writeTGA( char *name, byte *pixels, int width, int height)
{
    TargaHeader             targa_header;
    FILE *fin;

    fin = fopen( name, "wb" );
    if (!fin) {
       Debug( "Coulnd't open file for writing" );
       return;
    }
    
    memset( &targa_header, 0, sizeof( TargaHeader ) );       

    fputc( 0, fin ); //targa_header.id_length = 0;
	fputc( 0, fin ); //targa_header.colormap_type = 0;
	fputc( 2, fin ); //targa_header.image_type = 2;
    put16( 0, fin ); //targa_header.colormap_index = 0;
	put16( 0, fin ); //targa_header.colormap_length = 0;
    fputc( 0, fin );			   //targa_header.colormap_size = 0;
    put16( 0,fin ); //targa_header.x_origin = 0; 
	put16( 0, fin ); //targa_header.y_origin = 0;
	put16( width, fin ); 
	put16( height,fin );
    fputc( 24, fin ); //targa_header.pixel_size = 24 ;
	fputc( 0, fin );  //targa_header.attributes = 0;
        
	for (int i = 0; i < width*height*3; i+= 3)
	{
		fputc( (int)pixels[i+2], fin );
		fputc( (int)pixels[i+1], fin );
		fputc( (int)pixels[i], fin );
	}

    fclose(fin);
}

        
// now attempts to load a JPG if TGA load fails...
//    
// (return value sent to "*format" param isn't actually used anywhere)
//
bool loadTGA (char *name, byte **pixels, unsigned int *width, unsigned int *height, unsigned int *format)
{
        int                             columns, rows, numPixels;
        byte                    *pixbuf;
        int                             row, column;
        FILE                    *fin;
        byte                    *targa_rgba;
        TargaHeader             targa_header;

        fin = fopen (name, "rb");
        if (!fin) //Debug( "Couldn't read textures" );
		{
			char sLine[256];

			strcpy(sLine,name);
			strlwr(sLine);
			char *p = strstr(sLine,".tga");						
			if (p)
			{
				strcpy(p,".jpg");

				LoadJPG( sLine, pixels, (int*)width, (int*)height );

				if (*pixels)
					return true;
			}
			return false;
		}

        targa_header.id_length = fgetc(fin);
        targa_header.colormap_type = fgetc(fin);
        targa_header.image_type = fgetc(fin);
        
        targa_header.colormap_index = get16(fin);
        targa_header.colormap_length = get16(fin);
        targa_header.colormap_size = fgetc(fin);
        targa_header.x_origin = get16(fin);
        targa_header.y_origin = get16(fin);
        targa_header.width = get16(fin);
        targa_header.height = get16(fin);
        targa_header.pixel_size = fgetc(fin);
        targa_header.attributes = fgetc(fin);

        if (targa_header.image_type!=2 
                && targa_header.image_type!=10)
                Error ("LoadTGA: Only type 2 and 10 targa RGB images supported\n");

        if (targa_header.colormap_type !=0 
                || (targa_header.pixel_size!=32 && targa_header.pixel_size!=24))
                Error ("Texture_LoadTGA: Only 32 or 24 bit images supported (no colormaps)\n");

		*format = targa_header.pixel_size;

        columns = targa_header.width;
        rows = targa_header.height;
        numPixels = columns * rows;

        if (width)
                *width = columns;
        if (height)
                *height = rows;
        targa_rgba = new unsigned char[numPixels*4];
        *pixels = targa_rgba;

        if (targa_header.id_length != 0)
                fseek(fin, targa_header.id_length, SEEK_CUR);  // skip TARGA image comment
        
        if (targa_header.image_type==2) {  // Uncompressed, RGB images
                for(row=rows-1; row>=0; row--) {
                        pixbuf = targa_rgba + row*columns*4;
                        for(column=0; column<columns; column++) {
                                unsigned char red,green,blue,alphabyte;
                                switch (targa_header.pixel_size) {
                                        case 24:
                                                        
                                                        blue = getc(fin);
                                                        green = getc(fin);
                                                        red = getc(fin);
                                                        *pixbuf++ = red;
                                                        *pixbuf++ = green;
                                                        *pixbuf++ = blue;
                                                        *pixbuf++ = 255;
                                                        break;
                                        case 32:
                                                        blue = getc(fin);
                                                        green = getc(fin);
                                                        red = getc(fin);
                                                        alphabyte = getc(fin);
                                                        *pixbuf++ = red;
                                                        *pixbuf++ = green;
                                                        *pixbuf++ = blue;
                                                        *pixbuf++ = alphabyte;
                                                        break;
                                }
                        }
                }
        }
        else if (targa_header.image_type==10) {   // Runlength encoded RGB images
                unsigned char red,green,blue,alphabyte,packetHeader,packetSize,j;
                for(row=rows-1; row>=0; row--) {
                        pixbuf = targa_rgba + row*columns*4;
                        for(column=0; column<columns; ) {
                                packetHeader=getc(fin);
                                packetSize = 1 + (packetHeader & 0x7f);
                                if (packetHeader & 0x80) {        // run-length packet
                                        switch (targa_header.pixel_size) {
                                                case 24:
                                                                blue = getc(fin);
                                                                green = getc(fin);
                                                                red = getc(fin);
                                                                alphabyte = 255;
                                                                break;
                                                case 32:
                                                                blue = getc(fin);
                                                                green = getc(fin);
                                                                red = getc(fin);
                                                                alphabyte = getc(fin);
                                                                break;
                                        }
        
                                        for(j=0;j<packetSize;j++) {
                                                *pixbuf++=red;
                                                *pixbuf++=green;
                                                *pixbuf++=blue;
                                                *pixbuf++=alphabyte;
                                                column++;
                                                if (column==columns) { // run spans across rows
                                                        column=0;
                                                        if (row>0)
                                                                row--;
                                                        else
                                                                goto breakOut;
                                                        pixbuf = targa_rgba + row*columns*4;
                                                }
                                        }
                                }
                                else {                            // non run-length packet
                                        for(j=0;j<packetSize;j++) {
                                                switch (targa_header.pixel_size) {
                                                        case 24:                                                        
                                                                        blue = getc(fin);
                                                                        green = getc(fin);
                                                                        red = getc(fin);
                                                                        *pixbuf++ = red;
                                                                        *pixbuf++ = green;
                                                                        *pixbuf++ = blue;
                                                                        *pixbuf++ = 255;
                                                                        break;
                                                        case 32:
                                                                        blue = getc(fin);
                                                                        green = getc(fin);
                                                                        red = getc(fin);
                                                                        alphabyte = getc(fin);
                                                                        *pixbuf++ = red;
                                                                        *pixbuf++ = green;
                                                                        *pixbuf++ = blue;
                                                                        *pixbuf++ = alphabyte;
                                                                        break;
                                                }
                                                column++;
                                                if (column==columns) { // pixel packet run spans across rows
                                                        column=0;
                                                        if (row>0)
                                                                row--;
                                                        else
                                                                goto breakOut;
                                                        pixbuf = targa_rgba + row*columns*4;
                                                }                                               
                                        }
                                }
                        }
                        breakOut:;
                }
        }
        
        fclose(fin);
		return true;
}
