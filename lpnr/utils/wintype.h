#ifndef _WINTYPE_INCLUDED
#define _WINTYPE_INCLUDED

typedef int							DWORD;
typedef unsigned short	WORD;
typedef unsigned char 	BYTE;
typedef signed long long	INT64;
typedef int							BOOL;
typedef void *					HANDLE;
typedef int							SOCKET;
typedef void *					PVOID;
typedef const char*	  	LPCTSTR;

#define TRUE	1
#define FALSE	0

#ifdef linux
typedef int	bool;
#define true 1
#define false 0
#endif

#endif
