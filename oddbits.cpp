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
#include "oddbits.h"


// at some stage I could do with adding the multi-call-protect code in this func to all the other char routines,
//	but I'll just do it as and when I hit problems...
//
char	*va(char *format, ...)
{	
	va_list		argptr;
	static char		string[8][1024];
	static int i=0;

	i=(i+1)&7;
	
	va_start (argptr, format);
	vsprintf (string[i], format,argptr);
	va_end (argptr);

	return string[i];	
}


// these MUST all be MB_TASKMODAL boxes now!!
//
void ErrorBox(const char *sString)
{
	MessageBox( NULL, sString, "Error",		MB_OK|MB_ICONERROR|MB_TASKMODAL );		
}
void InfoBox(const char *sString)
{
	MessageBox( NULL, sString, "Info",		MB_OK|MB_ICONINFORMATION|MB_TASKMODAL );		
}
void WarningBox(const char *sString)
{
	MessageBox( NULL, sString, "Warning",	MB_OK|MB_ICONWARNING|MB_TASKMODAL );
}

bool FileExists (LPCSTR psFilename)
{
	FILE *handle = fopen(psFilename, "r");
	if (!handle)
	{
		return false;
	}
	fclose (handle);
	return true;
}



// returns a path to somewhere writeable, without trailing backslash...
//
// (for extra speed now, only evaluates it on the first call, change this if you like)
//
char *scGetTempPath(void)
{	
	static char sBuffer[MAX_PATH];
	DWORD dwReturnedSize;
	static int i=0;

	if (!i++)
		{
		dwReturnedSize = GetTempPath(sizeof(sBuffer),sBuffer);

		if (dwReturnedSize>sizeof(sBuffer))
			{
			// temp path too long to return, so forget it...
			//
			strcpy(sBuffer,"c:");	// "c:\\");	// should be writeable
			}

		// strip any trailing backslash...
		//
		if (sBuffer[strlen(sBuffer)-1]=='\\')
			sBuffer[strlen(sBuffer)-1]='\0';
		}// if (!i++)

	return sBuffer;

}// char *scGetTempPath(void)


long filesize(FILE *handle)
{
   long curpos, length;

   curpos = ftell(handle);
   fseek(handle, 0L, SEEK_END);
   length = ftell(handle);
   fseek(handle, curpos, SEEK_SET);

   return length;
}

// returns -1 for error
int LoadFile (LPCSTR psPathedFilename, void **bufferptr)
{
	FILE	*f;
	int    length;
	void    *buffer;

	*bufferptr = 0;	// some code checks this as a return instead

	f = fopen(psPathedFilename,"rb");
	if (f)
	{
		length = filesize(f);	
		buffer = malloc (length+1 + 4096);	// keep this in sync with INPUT_BUF_SIZE, since the JPG loader is sloppy
		((char *)buffer)[length] = 0;
		int lread = fread (buffer,1,length, f);	
		fclose (f);

		if (lread==length)
		{	
			*bufferptr = buffer;
			return length;
		}
		free(buffer);
	}

	ErrorBox(va("Error reading file %s!",psPathedFilename));
	return -1;
}

/*
// takes (eg) "q:\quake\baseq3\textures\borg\name.tga"
//
//	and produces "textures/borg/name.tga"
//
void Filename_RemoveBASEQ(CString &string)
{
	string.Replace("\\","/");		

	int loc = string.Find("/baseq");	// 2,3, etc...
	if (loc >= 0)
	{
		string = string.Right(string.GetLength() - (loc+7+1));
	}	
}


// takes (eg) "textures/borg/name.tga"
//
// and produces "textures/borg"
//
void Filename_RemoveFilename(CString &string)
{
	string.Replace("\\","/");		

	int loc = string.ReverseFind('/');
	if (loc >= 0)
	{
		string = string.Left(loc);
	}
}


// takes (eg) "( longpath )/textures/borg/name.xxx"			// N.B.  I assume there's an extension on the input string
//
// and produces "name"
//
void Filename_BaseOnly(CString &string)
{
	string.Replace("\\","/");

	int loc = string.GetLength()-4;
	if (string[loc] == '.')
	{
		string = string.Left(loc);		// "( longpath )/textures/borg/name"
		loc = string.ReverseFind('/');
		if (loc >= 0)
		{
			string = string.Mid(loc+1);
		}
	}
}

void Filename_AccountForLOD(CString &string, int iLODLevel)
{
	if (iLODLevel)
	{
		int loc = string.ReverseFind('.');
		if (loc>0)
		{
			string.Insert( loc, va("_%d",iLODLevel));
		}		
	}	
}
*/

// returns actual filename only, no path
//
char *Filename_WithoutPath(LPCSTR psFilename)
{
	static char sString[MAX_PATH];
	LPCSTR p = strrchr(psFilename,'\\');

  	if (!p++)
		p=psFilename;

	strcpy(sString,p);

	return sString;

}// char *Filename_WithoutPath(char *psFilename)


// returns (eg) "\dir\name" for "\dir\name.bmp"
//
char *Filename_WithoutExt(LPCSTR psFilename)
{
	static char sString[MAX_PATH];

	strcpy(sString,psFilename);

	char *p = strrchr(sString,'.');		
	char *p2= strrchr(sString,'\\');

	// special check, make sure the first suffix we found from the end wasn't just a directory suffix (eg on a path'd filename with no extension anyway)
	//
	if (p && (p2==0 || (p2 && p>p2)))
		*p=0;	

	return sString;

}// char *Filename_WithoutExt(char *psFilename)


// loses anything after the path (if any), (eg) "\dir\name.bmp" becomes "\dir"
//
char *Filename_PathOnly(LPCSTR psFilename)
{
	static char sString[8][MAX_PATH];
	static int  iIndex =0;

	iIndex = (++iIndex)&7;

	strcpy(sString[iIndex],psFilename);	

	char *p= strrchr(sString[iIndex],'\\');
	if (p)
		*p=0;

	return sString[iIndex];

}// char *Filename_WithoutExt(char *psFilename)


// returns filename's extension only, else returns original string if no '.' in it...
//
char *Filename_ExtOnly(LPCSTR psFilename)
{
	static char sString[MAX_PATH];
	LPCSTR p = strrchr(psFilename,'.');

	if (!p)
		p=psFilename;

	strcpy(sString,p);

	return sString;

}// char *Filename_ExtOnly(char *psFilename);


// similar to strlwr, but (crucially) doesn't touch original...
//
char *String_ToLower(LPCSTR psString)
{
	static char sString[MAX_PATH];

	strcpy(sString,psString);
	return strlwr(sString);	

}// char *String_ToLower(char *psString)

// similar to strupr, but (crucially) doesn't touch original...
//
char *String_ToUpper(LPCSTR psString)
{
	static char sString[MAX_PATH];

	strcpy(sString,psString);
	return strupr(sString);	

}// char *String_ToUpper(char *psString)


// first letter uppercase, rest lower, purely cosmetic when printing certain strings
//
char *String_Neaten(LPCSTR psString)
{
	static char sString[MAX_PATH];

	strcpy(sString,psString);
	strlwr(sString);	

	int iCheckLen = strlen(sString);

	for (int i=0; i<iCheckLen; i++)
		{
		if (isalpha(sString[i]))			
			{
			sString[i] = toupper(sString[i]);
			break;
			}
		}

	return sString;

}// char *String_Neaten(char *psString)


// whole sentence lowercase, then upper-case first letter in every sequence (usually word starts)
//
char *String_NeatenEveryWord(LPCSTR psString)
{
	static char sString[MAX_PATH];

	strcpy(sString,psString);
	strlwr(sString);	

	int iCheckLen = strlen(sString);

	BOOL bWaitingAlpha = TRUE;

	for (int i=0; i<iCheckLen; i++)
		{
		if (bWaitingAlpha)
			{
			if (isalpha(sString[i]))			
				{
				sString[i] = toupper(sString[i]);
				bWaitingAlpha = !bWaitingAlpha;
				}
			}
		else
			{
			if (!isalpha(sString[i]))			
				bWaitingAlpha = !bWaitingAlpha;
			}
		}// for (int i=0; i<iCheckLen; i++)

	return sString;

}// char *String_NeatenEveryWord(char *psString)



// string replacer, useful to hacking path strings etc
//
// eg:-   printf("%s\n", String_Replace("I like computers","like", "fucking hate", TRUE));
//
// would produce "I fucking hate computers"
//
char *String_Replace(char *psString, char *psFind, char *psReplace, BOOL bCaseInSensitive)
{
	static char sString[MAX_PATH];	
	int iFoundOffset;
	char *psFound;

	if (bCaseInSensitive)
		{
		char sFind[MAX_PATH];

		strcpy(sString,psString);
		strcpy(sFind,  psFind);

		strlwr(sString);
		strlwr(sFind);

		psFound = strstr(sString,sFind);
		iFoundOffset = psFound - sString;
		}
	else
		{
		psFound = strstr(psString,psFind);
		iFoundOffset = psFound - psString;
		}

	if (psFound)
		{
		ZEROMEM(sString);
		strncpy(sString,psString,iFoundOffset);
		strcat (sString,psReplace);
		strcat (sString,&psString[iFoundOffset+strlen(psFind)]);
		}
	else
		{
		strcpy(sString,psString);
		}

	return sString;

}// char *String_Replace(char *psString, char *psFind, char *psReplace, BOOL bCaseSensitive)


// returns a string of up to but not including the first comma (if any) of the supplied string, else NULL if blank string
//
char *String_GetField(char *psString)
{
	static char sString[MAX_PATH];

	strcpy(sString,String_LoseLeadingWhitespace(psString));

	char *p = strchr(sString,',');
	if (p)
		*p=0;

	if (!strlen(sString))
		return NULL;

	return sString;

}// char *String_GetField(char *psString)

// loses the first comma-delineated field from the supplied string, else NULL if no more fields after current
//
char *String_LoseField(char *psString)
{
	static char sString[MAX_PATH];

	char *p = strchr(psString,',');
	if (p)
		strcpy(sString,p+1);
	else
		strcpy(sString,"");

	if (!strlen(String_LoseLeadingWhitespace(sString)))
		return NULL;

	return sString;

}// char *String_LoseField(char *psString)


char *String_LoseWhitespace(char *psString)
{
	static char sString[MAX_PATH];

	char *psSrc  = psString;
	char *psDest = sString;
	char c;		

	do
		{
		while (isspace(*psSrc)) psSrc++;

		*psDest++ = c = *psSrc++;
		}
	while (c);

	return sString;

}// char *String_LoseWhitespace(char *psString)


char *String_LoseLeadingWhitespace(char *psString)
{
	static char sString[MAX_PATH];

	char *psSrc  = psString;
	char *psDest = sString;

	while (isspace(*psSrc)) psSrc++;
	while ((*psDest++ = *psSrc++)!=0);

	return sString;

}// char *String_LoseLeadingWhitespace(char *psString)

char *String_LoseTrailingWhitespace(char *psString)
{
	static char sString[MAX_PATH];

	strcpy(sString,psString);

	if (strlen(sString))
		{
		char *p = &sString[strlen(sString)-1];
	
		while (isspace(*p))
			*p-- = 0;
		}

	return sString;

}// char *String_LoseTrailingWhitespace(char *psString)


// I know, i know, but it's helpful to keep things simple when using inline string filters...
//
char *String_EnsureTrailingChar(char *psString, char c)
{
	static char sString[MAX_PATH];

	strcpy(sString,psString);

	// no doubt this could be done really cleverly, but...
	//
	if (strlen(sString))
		{
		char *p = &sString[strlen(sString)-1];
		if (*p!=c)
			{
			p++;
			*p++=c;
			*p++=0;
			}
		}
	else
		{
		sString[0]=c;
		sString[1]=0;
		}

	return sString;

}// char *String_EnsureTrailingChar(char *psString, char c)



// I know, i know, but it's helpful to keep things simple when using inline string filters...
//
char *String_LoseTrailingChar(char *psString, char c)
{
	static char sString[MAX_PATH];

	strcpy(sString,psString);

	if (strlen(sString))
		{
		char *p = &sString[strlen(sString)-1];
		if (*p==c)
			*p=0;
		}

	return sString;

}// char *String_LoseTrailingChar(char *psString, char c)



char *String_LoseComment(char *psString)
{
	static char sString[MAX_PATH];

	strcpy(sString,psString);

	char *psComment = strchr(sString,';');
	if (psComment)
		*psComment=0;

	return sString;

}// char *String_LoseComment(char *psString)

char *String_LoseNewline(char *psString)
{
	static char sString[MAX_PATH];

	strcpy(sString,psString);

	char *psNewLine = strchr(sString,'\n');
	if (psNewLine)
		*psNewLine=0;		

	return sString;

}// char *String_LoseNewline(char *psString)


// used when sending output to a text file etc, keeps columns nice and neat...
//
char *String_EnsureMinLength(LPCSTR psString, int iMinLength)
{
	static char	sString[8][1024];
	static int i=0;

	i=(i+1)&7;

	strcpy(sString[i],psString);

	// a bit slow and lazy, but who cares?...
	//
	while (strlen(sString[i])<(UINT)iMinLength)
		strcat(sString[i]," ");

	return sString[i];

}// char *String_EnsureMinLength(char *psString, int iMinLength)


// returns either 0 for not NT, or 4 or 5 for versions (5=Windows 2000, which doesn't work properly)
//
int RunningNT(void)
{
	return 4;	// James wants the functions that check this to be available all the time, so...
/*
	static bool bAnswered = false;
	static int  iAnswer   = 0;

	if (!bAnswered)
	{
		OSVERSIONINFO osid;

		osid.dwOSVersionInfoSize = sizeof (osid);
		GetVersionEx (&osid);

		switch (osid.dwPlatformId)
		{
			// Window 3.x
			case VER_PLATFORM_WIN32s:
				break;

			// Windows 95
			case VER_PLATFORM_WIN32_WINDOWS:
 				break;

			// Windows NT
			case VER_PLATFORM_WIN32_NT:
			{
				if(osid.dwMajorVersion >= 4)
				{
					iAnswer = osid.dwMajorVersion;
				}

				break;
			}
		}
		bAnswered = true;
	}
	return iAnswer;
*/
}


void SystemErrorBox(DWORD dwError)
{
	LPVOID lpMsgBuf=0;

	FormatMessage( 
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		dwError,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR) &lpMsgBuf,
		0,
		NULL 
	);		
	ErrorBox( (LPCSTR) lpMsgBuf );
	LocalFree( lpMsgBuf ); 

}



///////////////////// eof ///////////////////

