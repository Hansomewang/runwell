/*
 *	dbg_printf.c
 * 
 *  Author: Thomas Chang
 *  Date: 2014-02-26
 *
 *  This file contains two functions:
 *  dbg_setlevel(int nOutLevel) - set the output level, 0 would be the lowest level which is the default.
 *  dbg_getlevel() - get current level
 *  dbg_printf( level, fmt...) - print out the message to stdout. when nOutLevel >= level, output will go.
 *
 *  in your application, all tracing, debugging or message output can be category in different level
 *  using dbg_printf instead of printf. Therefore, user can turn on message to any detail level by
 *  assign it through the argument of application.
 */

//#define EN_RMTDBP  (put it in Makefile)

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#ifdef EN_RMTDBP
#include <pthread.h>
#include "utils_ptrlist.h"
#ifdef ENABLE_IPV6
#include "utils_net6.h"
#else
#include "utils_net.h"
#endif
#include "longtime.h"

//extern void lprintf(const char *fmt,...);

#define EN_PRSNLVL		// enable each remote client has different output level
	
static pthread_mutex_t	thread_lock = PTHREAD_MUTEX_INITIALIZER;	
static pthread_cond_t		thread_cond = PTHREAD_COND_INITIALIZER;

#define Thread_Lock()					pthread_mutex_lock(&thread_lock)
#define Thread_Unlock()				pthread_mutex_unlock(&thread_lock)


static PtrList Monitor_list = PTRLIST_INITIALIZER;
static PtrList Output_list = PTRLIST_INITIALIZER;

typedef struct tagMonitor {
	char peer_ip[INET6_ADDRSTRLEN];
	int  peer_port;
	int	fd;				// socket
	int level;		// output level, default is 3
} Monitor;

static pthread_t  dbg_threadId = 0;
static int port_listen;
static int fd_svc = 0;			// work thread listen socket
static int dbgthread_run = 0;
	
#endif	

static pid_t	dbg_pid = 0;
static int nConsoleLevel = 0;		// default output level (console output)
static int nRemoteLevel = 0;		// default monitor level for client
static int nMaxRemoteLevel=0;		// max remote monitor level
static int nMaxLevel = 0;				// max level (console and remote)
static int wait_flushed = 0;
static time_t  tLastTouch = 0;
static time_t	*pMarker = &tLastTouch;

#define max(a,b)	( (a) > (b) ? (a) : (b) )

#ifdef EN_RMTDBP		
static void get_maxlevel()
{
#ifdef EN_PRSNLVL	
	int maxlvl;
	POSITION pos;
	maxlvl = 0;
	for(pos=Monitor_list.head; pos!=NULL; pos=pos->next)
	{
		Monitor *monitor = (Monitor *)pos->ptr;
		maxlvl = max(maxlvl, monitor->level);
	}
	nMaxRemoteLevel = maxlvl;
#else
	nMaxRemoteLevel = (Monitor_list.count > 0) ? nRemoteLevel : 0;
#endif	
	nMaxLevel = max(nConsoleLevel,nMaxRemoteLevel);
	//printf("ConsoleLevel=%d, RemoteLevel=%d, MaxRemoteLevel=%d, MaxLevel=%d\n", 
	//		nConsoleLevel, nRemoteLevel, nMaxRemoteLevel,nMaxLevel);
}

static int prepare_fdset( fd_set *set )
{
	int nfdmax = fd_svc;
	POSITION pos;
	
	FD_ZERO(set);
	// TCP listen socket port 8000
	if ( fd_svc > 0 )
		FD_SET(fd_svc, set);
	// remote monitor's socket
	for(pos=Monitor_list.head; pos!=NULL; pos=pos->next)
	{
		Monitor *monitor = (Monitor *)pos->ptr;
		FD_SET( monitor->fd, set );
		nfdmax = max( nfdmax, monitor->fd );
	}
	return nfdmax;
}

static Monitor *accept_monitor()
{
	Monitor *monitor=NULL;
	int fd = sock_accept( fd_svc );
	if ( fd < 0 )
	{
		//printf("[log svc] abnormal - cannot accept client connection.\n");
		close( fd_svc );
		fd_svc = sock_listen( port_listen, NULL, 3 );
		if ( fd_svc== -1 )
		{
			//printf("--> cannot open log service listen port again.\n");
			fd_svc = 0;
		}
	}
	else
	{
		monitor = malloc( sizeof(Monitor) );
		memset( monitor, 0, sizeof(Monitor) );
		monitor->fd = fd;
		monitor->level = nRemoteLevel;
		strcpy( monitor->peer_ip, sock_getpeername( fd, &monitor->peer_port) );	
	}
	return monitor;
}

static void thread_timewait( int usec )
{
	struct timespec ts;
	clock_gettime( CLOCK_REALTIME, &ts ); 
	ts.tv_nsec += usec * 1000;
	if ( ts.tv_nsec > 1000000000LL )
	{
		ts.tv_nsec -= 1000000000LL;
		ts.tv_sec++;
	}

	pthread_mutex_lock(&thread_lock);
	pthread_cond_timedwait( &thread_cond, &thread_lock, &ts );
	pthread_mutex_unlock(&thread_lock);
	
}

static void *dbg_threadfxc( void *param )
{
	fd_set rd_set;
	struct timeval tv, *ptv;
	int nsel, nfds;
	char *ptr;
	
	dbgthread_run = 1;
	dbg_threadId = pthread_self();
	//printf("[log svc] - work thread (%d) up and run (pid=%d).\n", (int)dbg_threadId, dbg_pid);
	for( ;dbgthread_run; )
	{
		nfds = prepare_fdset( &rd_set );
		if ( nfds == 0 ) break;
    tv.tv_sec  = 0;
    tv.tv_usec = 0;	
    ptv = Monitor_list.count > 0 ? &tv : NULL;
    nsel = select(nfds+1, &rd_set, NULL, NULL, ptv);
		if ( nsel > 0 )
		{
			POSITION pos;
			Monitor *monitor;

			if ( FD_ISSET( fd_svc,&rd_set ) && (monitor=accept_monitor()) != NULL )
			{
				PtrList_append( &Monitor_list, monitor );
				get_maxlevel();
				//lprintf("[log svc] - accept a new monitor (ip=%s, port=%d), total=%d\n", 
				//		monitor->peer_ip, monitor->peer_port, Monitor_list.count );
			}
			// check for any input from monitor
			for( pos=Monitor_list.head; pos!=NULL; )
			{
				Monitor *monitor = (Monitor *)pos->ptr;
				POSITION pos_next = pos->next;
				
				if ( FD_ISSET( monitor->fd, &rd_set) )
				{
					char buf[8];
					if ( sock_read_line(monitor->fd, buf, sizeof(buf))<=0 )
					{ 
						PtrList_remove( &Monitor_list, pos );
						close( monitor->fd );		
						//lprintf("[log svc] - monitor %s:%d gone, remove it (%d left).\n", 
						//		monitor->peer_ip, monitor->peer_port, Monitor_list.count );	
						free( monitor );
						get_maxlevel();
					}
#ifdef EN_PRSNLVL					
					else
					{
						int nlevel = atoi(buf);
						if ( nlevel >= 0 )
							monitor->level = nlevel;
						get_maxlevel();
						//printf("[log svc] - client %s:%d switch to level %d. global max level=%d\n", 
						//	monitor->peer_ip, monitor->peer_port, monitor->level, nMaxLevel );	
					}
#endif					
				}
				pos = pos_next;
			}
		}
		// output all data to every monitor
		while( (ptr=PtrList_remove_head( &Output_list)) != NULL )
		{
			int len = strlen(ptr+1);
			POSITION pos;
			for(pos=Monitor_list.head; pos!=NULL; pos=pos->next )
			{
				Monitor *monitor = (Monitor *)pos->ptr;
#ifdef EN_PRSNLVL				
				if ( monitor->level >= (int)ptr[0] )
#endif					
					sock_write( monitor->fd, ptr+1, len );
			}
			free(ptr);
		}
		// wait for new data or time out (10000 usec)
		thread_timewait(10000);
	}
	while( (ptr=PtrList_remove_head( &Output_list)) != NULL )
		free( ptr );
	//printf("[log svc] - service thread (%d) exit.\n", (int)dbg_threadId);
	dbg_threadId = 0;
	dbg_pid = 0;
	return NULL;	
}
#else
static void get_maxlevel()
{
	nMaxLevel = nConsoleLevel;
}
#endif

int dbg_setremotelevel( int nNewLevel )
{
	int nOldLevel = nRemoteLevel;
	if ( nNewLevel >= 0 )
		nRemoteLevel = nNewLevel;
	get_maxlevel(); 
	return nOldLevel;
}

int dbg_setconsolelevel(int nNewLevel )
{
	int oldLevel = nConsoleLevel;
	if ( nNewLevel >= 0 )
		nConsoleLevel= nNewLevel;
	get_maxlevel(); 
	return oldLevel;
}

int dbg_getlevel()
{
	return nConsoleLevel;
}

int dbg_getpid()
{
	return (int)dbg_pid;
}

int dbg_gettid()
{
#ifdef EN_RMTDBP
	return (int)dbg_threadId;
#else
	return 0;
#endif	
}

int dbg_printf(int level, const char* format, ...)
{
	va_list va;
	int n;
	char line[2048];
#ifdef EN_RMTDBP		
	char *ptr;
#endif
	*pMarker = time(NULL);
	if ( nMaxLevel < level ) return 0;
	va_start( va, format );
	n = vsprintf( line, format, va );
	va_end( va );
	if ( n < 0 ) 
	{
		//printf("vsprintf error - %s\n", strerror(errno) );
		return 0;
	}
	if ( nConsoleLevel >= level )
	{
		fputs(line, stdout);
		fflush(stdout);
	}
#ifdef EN_RMTDBP	
	if ( Monitor_list.count > 0 && nMaxRemoteLevel >= level )
	{
		// append new output line to list
		if ( line[n-1] == '\n' && line[n-2] != '\r' )
		{
			line[n-1] = '\r';
			line[n++] = '\n';
			line[n] = '\0';;
		}
	 	ptr = malloc( n+2 );
	 	if ( ptr )
	 	{
	 		ptr[0] = level;
	 		memcpy(ptr+1, line, n+1 );
	 		PtrList_append( &Output_list, ptr );
	 	}
	 	// wake-up working thread
//		pthread_mutex_lock(&thread_lock);
		pthread_cond_signal(&thread_cond);
//		pthread_mutex_unlock(&thread_lock);
		if ( wait_flushed )
			while(Output_list.count > 0) usleep(100);
	}
#endif	
	return n;
}

int dbg_initialize(int port)
{
	*pMarker = time(NULL);
#ifdef EN_RMTDBP		
	port_listen = port;
	fd_svc = sock_listen( port, NULL, 3 );
	if ( fd_svc == -1 )
	{
		printf("faied to listen TCP port %d - dbg_service not avilable.\n",port );
		return -1;
	}
	else
		printf("[log svc] dbg_service initialized on TCP port %d\n", port);
	pthread_create( &dbg_threadId, 0, dbg_threadfxc, NULL );
#endif	
	dbg_pid = getpid();

	return 0;
}

void dbg_terminate()
{
#ifdef EN_RMTDBP		
	dbgthread_run = 0;
	if ( fd_svc != 0 )
		close( fd_svc );
	fd_svc = 0;
#endif	
}	
	 
void dbg_setmarkaddr( int *marker )
{
	pMarker = (time_t *)marker;
}

void dbg_flush(int twait_ms)
{
#ifdef EN_RMTDBP
	_longtime lt_waituntil = timeGetLongTime()+twait_ms;
	if ( dbgthread_run )
	{	
		printf("[%s] dbg_flush - pending output log line=%d\n", TIMESTAMP0(), Output_list.count);
		while(Output_list.count > 0 && timeGetLongTime() < lt_waituntil) 
			usleep(100);
		printf("[%s] dbg_flush - waitfor done, pending log=%d\n", TIMESTAMP0(), Output_list.count);
	}
#endif	
}

void dbg_setwaitflushed( int wait )
{
	wait_flushed = wait;
}

int dbg_getnumlogger()
{
#ifdef EN_RMTDBP		
	return Monitor_list.count;
#else
	return 0;
#endif		
}
const char* dbg_getloggeraddr(int idx)
{
#ifdef EN_RMTDBP		
	POSITION pos;
	int i;
	for(i=0, pos=Monitor_list.head; i<idx && pos!=NULL; pos=pos->next );
	if ( pos != NULL )
	{
		Monitor *monitor = (Monitor *)pos->ptr;
		return monitor->peer_ip;
	}
#endif	
	return 0;
}

void dbg_touch()
{
	*pMarker = time(NULL);
}

int dbg_getidlesec()
{
	return time(NULL) - (*pMarker);
}
