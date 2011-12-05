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

#include <windows.h>
#include <stdio.h>
#include <assert.h>
// #include "oddbits.h"
#include "bmp.h"


static bool BMP_FlipTrueColour(LPCSTR psFilename);


static int iBMP_PixelWriteOffset;
static PMEMORYBMP pBMP = NULL;
static int iBMP_MallocSize;
//
static FILE *fhBMP = 0;


bool BMP_GetMemDIB(void *&pvAddress, int &iBytes)
{
	if (pBMP)
	{
		pvAddress = pBMP;
		iBytes = iBMP_MallocSize;
		return true;
	}

	return false;
}



// open 24-bit RGB file either to disk or memory
//
// if psFileName == NULL, open memory file instead
//
bool BMP_Open(LPCSTR psFilename, int iWidth, int iHeight)
{
	BITMAPFILEHEADER BMPFileHeader;
	BITMAPINFOHEADER BMPInfoHeader;

	int iPadBytes	= (4-((iWidth * sizeof(RGBTRIPLE))%4))&3;
	int iWidthBytes =     (iWidth * sizeof(RGBTRIPLE))+iPadBytes;

	///////
	if (pBMP)
	{
		free(pBMP);
		pBMP = NULL;
	}
	fhBMP = NULL;
	///////

	if (psFilename)
	{
		fhBMP = fopen(psFilename,"wb");
		if (!(int)fhBMP)
			return false;
	}
	else
	{
		iBMP_MallocSize = sizeof(BITMAPINFOHEADER) + (iWidthBytes * iHeight);
		pBMP = (PMEMORYBMP) malloc ( iBMP_MallocSize );
		if (!pBMP)
			return false;
	}

	memset(&BMPFileHeader, 0, sizeof(BITMAPFILEHEADER));
	BMPFileHeader.bfType=(WORD)('B'+256*'M');
//	int iPad= ((sizeof(RGBTRIPLE)*iWidth)%3)*iHeight;	
//	BMPFileHeader.bfSize=sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER)+(sizeof(RGBTRIPLE)*iWidth*iHeight);//+iPad;
	BMPFileHeader.bfSize=sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER)+(iWidthBytes * iHeight);

	BMPFileHeader.bfOffBits=sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER);	// No palette
	
	if (fhBMP)
	{
		fwrite	(&BMPFileHeader,sizeof(BMPFileHeader),1,fhBMP);
	}
	else
	{
		// memory version doesn't use the BITMAPFILEHEADER structure
	}

	memset(&BMPInfoHeader, 0, sizeof(BITMAPINFOHEADER));
	BMPInfoHeader.biSize=sizeof(BITMAPINFOHEADER);
	BMPInfoHeader.biWidth=iWidth;
	BMPInfoHeader.biHeight=iHeight;
	BMPInfoHeader.biPlanes=1;
	BMPInfoHeader.biBitCount=24;
	BMPInfoHeader.biCompression=BI_RGB; 
	BMPInfoHeader.biSizeImage=0;// BMPFileHeader.bfSize - (sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER));	// allowed for BI_RGB bitmaps
	BMPInfoHeader.biXPelsPerMeter=0;	// don't know about these
	BMPInfoHeader.biYPelsPerMeter=0;
	BMPInfoHeader.biClrUsed=0;
	BMPInfoHeader.biClrImportant=0;

	if (fhBMP)
	{
		fwrite	(&BMPInfoHeader,sizeof(BMPInfoHeader),1,fhBMP);
	}
	else
	{
		pBMP->BMPInfoHeader = BMPInfoHeader;	// struct copy
		iBMP_PixelWriteOffset = 0;
	}

	return true;
}

bool BMP_WritePixel(byte Red, byte Green, byte Blue)
{
	RGBTRIPLE Trip = {0,0,0};

	Trip.rgbtRed	= Red;
	Trip.rgbtGreen	= Green;
	Trip.rgbtBlue	= Blue;

	if (fhBMP)
	{
		fwrite(&Trip, sizeof(RGBTRIPLE), 1, fhBMP);
	}
	else
	{
		RGBTRIPLE *pDest = (RGBTRIPLE *) ((byte *)pBMP->RGBData + iBMP_PixelWriteOffset);

		*pDest = Trip;

		iBMP_PixelWriteOffset += sizeof(RGBTRIPLE);
	}

	return true;
}

// BMP files need padding to 4-byte boundarys after writing each scan line... (which sucks, and messes up pixel indexing)
//
bool BMP_WriteLinePadding(int iPixelsPerLine)
{
	static char cPad[4]={0};

	int iPadBytes = (4-((iPixelsPerLine * sizeof(RGBTRIPLE))%4))&3;

	if (iPadBytes)
	{
		if (fhBMP)
		{
			fwrite( &cPad, iPadBytes, 1, fhBMP);
		}
		else
		{
			iBMP_PixelWriteOffset += iPadBytes;	// <g>, can't be bothered padding with zeroes
		}
	}
	return true;
}

// BMP files are stored upside down, but if we're writing this out as a result of doing an OpenGL pixel read, then
//	the src buffer will be upside down anyway, so I added this flip-bool -Ste
//
// (psFilename can be NULL for mem files)
//
bool BMP_Close(LPCSTR psFilename, bool bFlipFinal)
{
	if (fhBMP)
	{
		fclose (fhBMP);
	}
	else
	{
		#if 1

		int iPadBytes	= (4-((pBMP->BMPInfoHeader.biWidth * sizeof(RGBTRIPLE))%4))&3;
		int iWidthBytes =     (pBMP->BMPInfoHeader.biWidth * sizeof(RGBTRIPLE))+iPadBytes;

		assert(iBMP_PixelWriteOffset == iWidthBytes * pBMP->BMPInfoHeader.biHeight);
		assert((iBMP_PixelWriteOffset + (int)sizeof(BITMAPINFOHEADER)) == iBMP_MallocSize);

		#endif
	}

	if (bFlipFinal)
	{
		if (psFilename)
		{
			if (!BMP_FlipTrueColour(psFilename))
				return false;
		}
	}

	return true;
}


static bool BMP_FlipTrueColour(LPCSTR psFilename)
{
	BITMAPFILEHEADER BMPFileHeader;
	BITMAPINFOHEADER BMPInfoHeader;
	RGBTRIPLE *RGBTriples, *tTopLine, *tBottomLine;//, *AfterLastLine;
	BYTE  *byTopLine, *byBottomLine, *byAfterLastLine;
	RGBTRIPLE Trip;
	int x,y;
	int iPadBytes,iRealWidth;


// reopen it to flip it
	fhBMP=fopen(psFilename,"rb");	// checked fopen
	if (!(int)fhBMP)
		return false;

	fread	(&BMPFileHeader,sizeof(BMPFileHeader),1,fhBMP);
	fread	(&BMPInfoHeader,sizeof(BMPInfoHeader),1,fhBMP);
	iPadBytes = (4-((BMPInfoHeader.biWidth * sizeof(RGBTRIPLE))%4))&3;
	iRealWidth=(sizeof(RGBTRIPLE)*BMPInfoHeader.biWidth)+iPadBytes;

	RGBTriples=(RGBTRIPLE *)malloc(iRealWidth*BMPInfoHeader.biHeight);
	fread	(RGBTriples,iRealWidth*BMPInfoHeader.biHeight,1,fhBMP);

	fclose (fhBMP);

	byTopLine=(BYTE *)RGBTriples;
	byAfterLastLine=byTopLine+iRealWidth*BMPInfoHeader.biHeight;

// actually flip it
	for (y=0;	y<BMPInfoHeader.biHeight/2;	y++)
	{
		byBottomLine=byAfterLastLine-((y+1)*iRealWidth);

		tTopLine=(RGBTRIPLE *)byTopLine;
		tBottomLine=(RGBTRIPLE *)byBottomLine;

		for (x=0;	x<BMPInfoHeader.biWidth;	x++)
		{
			Trip=tTopLine[x];
			tTopLine[x]=tBottomLine[x];
			tBottomLine[x]=Trip;
		}

		byTopLine+=iRealWidth;
	}

	
// rewrite it flipped
	fhBMP=fopen(psFilename,"wb");	// checked fopen
	fwrite	(&BMPFileHeader,sizeof(BMPFileHeader),1,fhBMP);
	fwrite	(&BMPInfoHeader,sizeof(BMPInfoHeader),1,fhBMP);
	fwrite	(RGBTriples,(iRealWidth)*BMPInfoHeader.biHeight,1,fhBMP);
	fclose (fhBMP);
	free(RGBTriples);

	return true;
}




////////////////// eof //////////////////

