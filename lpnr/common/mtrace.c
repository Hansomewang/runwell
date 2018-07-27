/*
 * mtrace.c
 * for multiple log files application using this instead of trace.c
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
//#include <direct.h>
#include <fcntl.h>
#if defined _WIN32 || defined _WIN64
//#define _CRT_SECURE_NO_WARNINGS 		// disable the annoying warning 4996
#include <windows.h>
#include <io.h>
#include <direct.h>
typedef HANDLE HMUTEX;
#else		// [Linux]
#include <pthread.h>
#include <error.h>
#include <errno.h>
#define DEF_LOG_DIR			"/var/log"
typedef int		BOOL;
typedef pthread_mutex_t		HMUTEX;
// mimic Windows SDK call
#define GetLastError()		(errno)
#define CreateDirectory(path, attr)		(mkdir(path, attr==NULL ? 777 : 777)!=-1)
#define MAX_PATH 256

#ifndef _TRUEFALSE_DEFINED_
#define _TRUEFALSE_DEFINED_
#define TRUE	1
#define FALSE	0
#define true	1
#define false	0
#endif

#include <fcntl.h> 
#include <unistd.h>
#include <sys/time.h> 
#endif
//#define ENABLE_MUTEX
#define DEF_LOG_DIR			"/rwlog"
#define DEF_LOG_BASEFILE	"RunWellAP"
#define MAX_LOGSIZE			4194304		// 4 MB
#define MAX_ROLLCOUNT		30			// <BaseName>01.log ~ <BaseName>30.log
#define MAGIC_NO				0xcb34ae78
#define ISVALID_HANDLE(h) ( (h)!=NULL && ((HTRACE) h)->nMagicNo == MAGIC_NO )

static char __def_log_dir[MAX_PATH] = DEF_LOG_DIR;
typedef struct {
	int		nMagicNo;
	char m_strLogFile[MAX_PATH];
	char m_strFormat[MAX_PATH];
	char m_strBaseName[64];
	size_t  sz_limit;
	int	   m_rollcnt;
	FILE	*m_fp;
	BOOL m_bEnable;
	BOOL m_bKeepOpen;
	HMUTEX	m_hMutex;
} TraceStruct, *HTRACE;

#ifdef ENABLE_MUTEX
#ifdef WIN32
#define Mutex_Lock(hMutex)			WaitForSingleObject( hMutex, INFINITE )
#define Mutex_Unlock(hMutex)		ReleaseMutex( hMutex )
#else
#define Mutex_Lock(hMutex)			pthread_mutex_lock( &hMutex )
#define Mutex_Unlock(hMutex)		pthread_mutex_unlock( &hMutex )
#endif

#else
#define Mutex_Lock(hMutex)
#define Mutex_Unlock(hMutex)
#endif

static int set_log_dir( HTRACE hTrace, const char *logdir )
{
	int len;

	if ( logdir==NULL )
		strcpy( hTrace->m_strLogFile, __def_log_dir );
	else
		strcpy( hTrace->m_strLogFile, logdir );
	len = (int)strlen( hTrace->m_strLogFile );
	if ( hTrace->m_strLogFile[len-1] == '/' || hTrace->m_strLogFile[len-1] == '\\' )
		hTrace->m_strLogFile[len-1] = '\0';
	if ( !CreateDirectory( hTrace->m_strLogFile, NULL ) )
		len = GetLastError();			// for debugger to see the error code

	strcat( hTrace->m_strLogFile, "/" );
	strcat( hTrace->m_strLogFile, hTrace->m_strBaseName );
	strcpy( hTrace->m_strFormat, hTrace->m_strLogFile );
	strcat( hTrace->m_strLogFile, ".log" );
	strcat( hTrace->m_strFormat, "-%02d" );
	strcat( hTrace->m_strFormat, ".log" );
	return 0;
}

void *mlog_init(const char *LogDir, const char *BaseName )
{
	HTRACE hTrace = (HTRACE)malloc(sizeof(TraceStruct) );
	if ( hTrace==NULL )
		return hTrace;

	memset( hTrace, 0, sizeof(TraceStruct) );
	hTrace->nMagicNo = MAGIC_NO;
	hTrace->m_bKeepOpen = TRUE;
	hTrace->m_bEnable = TRUE;
	hTrace->sz_limit = MAX_LOGSIZE;
	hTrace->m_rollcnt = MAX_ROLLCOUNT;
#ifdef ENABLE_MUTEX
#if defined _WIN32 || defined _WIN64
	hTrace->m_hMutex  = CreateMutex(
						NULL,				// default security attributes
						FALSE,				// initially not owned
						NULL);				// mutex name (NULL for unname mutex)
#else
	pthread_mutex_init(&hTrace->m_hMutex, NULL);
#endif
#endif

	strcpy(hTrace->m_strBaseName, BaseName==NULL ? DEF_LOG_BASEFILE : BaseName);
	set_log_dir(hTrace, LogDir);
	hTrace->m_fp  = fopen( hTrace->m_strLogFile, "a" );

	return hTrace;
};

void mlog_close(void *h)
{
	if ( ISVALID_HANDLE(h) )
	{
		HTRACE hTrace = (HTRACE)h;
		if ( hTrace->m_fp != NULL )
			fclose( hTrace->m_fp );
#ifdef ENABLE_MUTEX
#if defined _WIN32 || defined _WIN64
		if ( hTrace->m_hMutex )
			CloseHandle( hTrace->m_hMutex );
#else
		pthread_mutex_destroy(&hTrace->m_hMutex);
#endif
#endif
		free( hTrace );
	}
}

void mlog_setpath( void *h, const char *path)
{
	HTRACE hTrace = (HTRACE)h;

	if ( h==NULL )
		strcpy(__def_log_dir, path);
	else
	{
		if ( !ISVALID_HANDLE(h) )  return;
		if ( hTrace->m_fp != NULL )
			fclose(hTrace->m_fp);
		hTrace->m_fp = NULL;
		set_log_dir(hTrace, path);
	}
	// 不需要open log file、下次调用 mtrace_log时会open
}

static void log_rollforward(HTRACE hTrace)
{
	char	cLog1[ MAX_PATH ], cLog2[ MAX_PATH ];
	int		i;

	sprintf( cLog2, hTrace->m_strFormat, hTrace->m_rollcnt );
	unlink( cLog2 );					// unlink last one
	for( i=hTrace->m_rollcnt-1; i>0; i-- )
	{
		sprintf( cLog1, hTrace->m_strFormat, i );
		rename( cLog1, cLog2 );	
		strcpy( cLog2, cLog1 );
	}
	rename( hTrace->m_strLogFile, cLog1 );
}

static const char* time_stamp()
{
	static char timestr[64];
#ifdef linux
	char *p = timestr;
	struct timeval tv;
	gettimeofday( &tv, NULL );

	strftime( p, sizeof(timestr), "%y/%m/%d %H:%M:%S", localtime( &tv.tv_sec ) );
	sprintf( p + strlen(p), ".%03lu", tv.tv_usec/1000 );

#else
		SYSTEMTIME	tnow;
		GetLocalTime( &tnow );
		sprintf( timestr, "%04d/%02d/%02d %02d:%02d:%02d.%03d", 
					tnow.wYear, tnow.wMonth, tnow.wDay, 
					tnow.wHour, tnow.wMinute, tnow.wSecond, tnow.wMilliseconds );
#endif
	return timestr;
}

int mtrace_log (void *h, const char *fmt, ...)
{
	va_list	va;
	char		str[ 8192 ] = "";
	int			rc = 0;
	struct	stat	st;
	HTRACE hTrace = (HTRACE)h;

	if ( !ISVALID_HANDLE(h) || !hTrace->m_bEnable ) 
		return 0;
	Mutex_Lock( hTrace->m_hMutex );
	va_start(va, fmt);
	vsprintf(str, fmt, va);
	va_end(va);

	//printf(str);
	if ( hTrace->m_fp != NULL )
	{
	#if defined _WIN32 || defined _WIN64
		fstat( _fileno(hTrace->m_fp), &st );
		if ( st.st_size > (_off_t)hTrace->sz_limit )
	#else
		fstat( fileno(hTrace->m_fp), &st );
		if ( st.st_size > (off_t)hTrace->sz_limit )
	#endif 	
		{
			fclose( hTrace->m_fp );
			hTrace->m_fp = NULL;
			log_rollforward(hTrace);
		}
	}
	if ( hTrace->m_fp == NULL )
		hTrace->m_fp = fopen( hTrace->m_strLogFile, "a" );

	if ( hTrace->m_fp != NULL )
	{
		if ( fmt[0] != '\t' )
		{
			rc = fprintf(hTrace->m_fp, "[%s] %s", time_stamp(),  str);
		}
		else
		{
			rc = fprintf(hTrace->m_fp, "%26s%s", " ", str+1 );
		}
		if ( hTrace->m_bKeepOpen )
			fflush( hTrace->m_fp );
		else
		{
			fclose( hTrace->m_fp );
			hTrace->m_fp = NULL;
		}
	}
	Mutex_Unlock( hTrace->m_hMutex );
	return rc;
}

void mlog_enable( void *h, BOOL bEnable )
{
	if ( ISVALID_HANDLE(h) )
	{
		HTRACE hTrace = (HTRACE)h;
		hTrace->m_bEnable  = bEnable;
		if ( !bEnable && hTrace->m_fp )
		{
			fclose(hTrace->m_fp);
			hTrace->m_fp = NULL;
		}
	}
}

void mlog_keepopen( void *h, BOOL bKeepOpen )
{
	if ( ISVALID_HANDLE(h) )
	{
		HTRACE hTrace = (HTRACE)h;
		hTrace->m_bKeepOpen  = bKeepOpen;
		if ( !bKeepOpen && hTrace->m_fp!=NULL )
		{
			fclose( hTrace->m_fp );
			hTrace->m_fp = NULL;
		}
	}
}

void mlog_setlimitcnt( void *h, int limit, int cnt )
{
	if ( ISVALID_HANDLE(h) )
	{
		HTRACE hTrace = (HTRACE)h;
		hTrace->sz_limit  = limit * 1024;
		hTrace->m_rollcnt = cnt;
	}
}
