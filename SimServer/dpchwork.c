/*
 * =====================================================================================
 *
 *       Filename:  dpchwork.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2018-05-06 09:12:47 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:   (), 
 *        Company:  
 *
 * =====================================================================================
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <ctype.h>
#include "./utils/longtime.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdarg.h>
#include <sys/mman.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/types.h>          /*  See NOTES */
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <linux/if.h>
#include <string.h>
#include <dirent.h>


extern void trace_log(const char *fmt,...);
static char interface[128] = {"ppp0"};
static pthread_t thread;

static int bQuit = 0;
static int begin_work = 0;
static pthread_mutex_t mutex =PTHREAD_MUTEX_INITIALIZER ;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

static int MainWait( int tout ) 
{
    struct timespec ts;  
    clock_gettime( CLOCK_REALTIME, &ts ); 
    ts.tv_nsec += ( tout % 1000 )*1000*1000;
    ts.tv_sec += tout;
    pthread_mutex_lock( &mutex );
    pthread_cond_timedwait( &cond, &mutex , &ts );
    pthread_mutex_unlock( &mutex );
    return 0; 
}

static void MainSignal()
{
    pthread_mutex_lock( &mutex );
    pthread_cond_signal( &cond );
    pthread_mutex_unlock( &mutex );
}

static int is_interface_up(const char *eth_inf, char *ip , int maxzize)
{  
    int sd;  
    struct sockaddr_in sin;  
    struct ifreq ifr;  
    sd = socket(AF_INET, SOCK_DGRAM, 0);  
    if (-1 == sd)  
        return -1;        
    strcpy( ifr.ifr_name, eth_inf == NULL ? "eth0" : eth_inf);  
    if ( ioctl(sd, SIOCGIFADDR, &ifr) < 0 )  
	{
        close(sd);  
        return -1;  
    }  
    close(sd);  
	strcpy( ip, inet_ntoa( sin.sin_addr ));
	return 0;
}

static int read_file(const char *path, char *buffer, int size )
{
	int fd = open( path, O_RDONLY );
	if( fd < 0 )
		return -1;
	read( fd, buffer, size );
	close( fd );
	return 0;
}
static int find_process( const char *pidname, int pidlist[], int max_count )
{
#define DEFAUTL_PROCESS_PATH  "/proc"
    DIR *dir;
    struct dirent *ptr;
    char cmd_line[120] = {0};
    char content[256];
    int pid, count = 0;
    if ( access( DEFAUTL_PROCESS_PATH, F_OK ) != 0 || (dir=opendir(DEFAUTL_PROCESS_PATH)) == NULL)
    {   
        return 0;
    }   
    while( (ptr = readdir( dir )) != NULL )
    {   
        // if it is . or ../
        if( strcmp( ptr->d_name, "." ) == 0 || strcmp( ptr->d_name, ".." ) == 0 ) 
            continue;
        strcpy( content, ptr->d_name );
        if( isdigit( content[0] ) != 0 || atoi( content ) < 500 )
            continue;
        pid = atoi( content );
        if( pid < 500 )
        continue;
        sprintf( cmd_line, "/proc/%d/cmdline", pid );
        memset( content, 0, sizeof( content )); 
       if( 0 >=  read_file( cmd_line, content, sizeof( content )) )
		   continue;
        if( count < max_count && strstr( content, pidname ) ) 
            pidlist[count++] = pid;
    }   
    closedir( dir );
    return count;
}

static void *work_thread( void *arg )
{
	char ip[0];
	int pidlist[10];
	int count;
	while( !bQuit )
	{
		if( !begin_work )
		{
			usleep( 500000 );
			continue;
		}
		// first check if has the interface 
		if( is_interface_up( interface , ip, sizeof( ip )) != 0 )
		{
			trace_log("net interface %s not create yet..\r\n", interface );
			sleep( 2 );
			continue;
		}
		trace_log("get the interface [%s] ip is [%s]\r\n", interface , ip );
		count = find_process( "udhcpc" , pidlist, sizeof( pidlist )/sizeof( int ));
		if( strstr( ip, "0.0.0.0" ) )
		{
			trace_log("get the ip addr is error,\r\n");
			if( count == 0 )
			{
				trace_log("no dhcp client run, run it..\r\n");
				system( "udhcpc -i ppp0 " );
			}
			sleep( 10 );
		}else{
			while( count > 0 ) {
				trace_log("find the dhcp client process[%d].. close it ..\r\n", pidlist[count]);
				kill( pidlist[count--], SIGKILL );
			}
			trace_log("we have get the ip from ppp0, now we need to sleep for a day ..\r\n");
			MainWait( 10000 );
		}
	}
	return NULL;
}

#define ENABLE_DHCP

int dhcp_wakeup( int enable )
{
#ifndef ENABLE_DHCP
	return 0;
#endif
	trace_log("wake up dhcp thread..");
	begin_work = enable;
	MainSignal();
	return 0;
}

int dhcp_init( const char *inter )
{
#ifndef ENABLE_DHCP
	return 0;
#endif
	if( inter )
		strcpy( interface, inter );
	pthread_create( &thread, NULL, work_thread, NULL );
	return 0;
}

int dpch_exit()
{
#ifndef ENABLE_DHCP
	return 0;
#endif
	bQuit = 1;
	pthread_cancel( thread );
	pthread_join( thread , NULL );
	return 0;
}
