/*
 * pl_main.c
 *
 * Transpeed Co. Laser Vehicle Profiler (rw) main functions
 *
 * Date: 2016-06-21
 * Author: Thomas Chang
 *
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <getopt.h>
#include <pthread.h>
#include <signal.h>
#include <sys/shm.h>
#include <time.h>
#include <math.h>
#include <sys/time.h>
#include <error.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/sysinfo.h>
#include <sys/reboot.h>
#include <linux/reboot.h>
#include "rw_time.h"
#include "./utils/dbg_printf.h"
#include "./utils/longtime.h"
#include "./utils/utils_net.h"
#include "./utils/utils_file.h"
#include "./utils/utils_netcfg.h"
#include "./utils/utils_shm.h"

#define SYSLOG_FILE		"./rwsys.log"
#define SYSLOG_BKUP		"./rwsys.bak"
#define TRANSLOG_FILE		"./rwtrans.log"
#define TRANSLOG_BKUP		"./rwtrans.bak"
#define SYS_LIMIT			512000			// 512K
#define TRANS_LIMIT		512000			// 512K
/*
 * Parking Lot Controller application configuration header
 */
// [STRING SECTION]
// 每个设备里的程序必须保证该值不一样
#define SHM_PROCID			16

#define DIR_TMPFILE			"/var/tmp/"
#define DIR_SHMKEY			DIR_TMPFILE

#define DEF_FONTNAME		"楷体"
#define DEF_ASCIIFONT		"Times New Roman"
#define STR_NETCONF			"./netconf.dat"

#define PROGRAM_SHORTNAME	"SimServer"				// run well auto update services
#define PROGRAM_NAME		"Run Well 4GSimServer"
#define PROGRAM_VERSION		"0.0.0.1"
#define PROGRAM_BINVER		0x00001
#ifdef RW_YEAR
#define PROGRAM_BUILT			RW_YEAR"-"RW_MON"-"RW_DAY
#define STR_VERSION				"Ver"RW_YEAR"."RW_MON"."RW_DAY
#else
#define PROGRAM_BUILT			"2017-04-29"
#define STR_VERSION				"Ver2017.04.29"
#endif

#define PROGRAM_OWNER			"Runwell Control Technology Ltd. (No. 52, Weixi Rd., Suzhou, Jiangsu, P.R.C)"
#define PROGRAM_AUTHOR		"Mike Dai (Dai, HaiMing) (mike_dai@126.com)" 
typedef int BOOL;
#define FALSE	0
#define TRUE	1
#define FAILURE	-1 
#define SUCCESS 0

extern void RW_Main();
extern void RW_Exit();

void show_copyright( void )
{
	printf(
		"+--------------------------------------------------------------------------------------------+\n"
		"| "PROGRAM_NAME", version "PROGRAM_VERSION" (built on "PROGRAM_BUILT")\n"
		"| Copyright (C): "PROGRAM_OWNER"\n"
		"| Author:        "PROGRAM_AUTHOR"\n"
		"+--------------------------------------------------------------------------------------------+\n"
	);
	printf("Hardware Model: TLC520\n");
}

/**********************************
 *  rw shared memory pointer
 **********************************/
static pid_t	first_parent_pid = 0;
static int		first_parent_quit = 0;

// program command line arguments paramters
BOOL g_bHasRTC=FALSE;
BOOL g_bHasSDCard=FALSE;
int	gnEnabler=0;
int gnOutLevel=0;

// global configurations (following three item are member of NODE_INFO_S
NET_CONFIG_S	g_stNETConf;

// file scope global variables
static BOOL gbDaemonMode = FALSE;

#define PAGE_SIZE		1024

// rw server shared memory
typedef struct {
	int			rw_power_on;
	int			total_respawn;			// total number of respawn 
	// start, stop control (this part cannot reset)
	int			rw_server_run;		// lpnr running control 1 is run, 0 is stop.
	int			rw_engine_run;		// lpnr engine process is running 
	int			master_pid;					// when running as daemon, this is pid of master process
	int			engine_pid;					// when running as daemon, this is pid of lpnr processing process (the engine)
	int 		dbg_marktime;				// last time process invoke dbg_printf to show any process output (it's time_t actually).
} RW_SHM_S;

RW_SHM_S *g_shmptr=NULL;

/*
 * macro functions
 */
#define shm_is_attached()		(g_shmptr!=NULL)

#define shm_reset_engine() \
	do { \
		g_shmptr->engine_pid = 0; \
		g_shmptr->rw_engine_run = 0; \
	} while(0)

int rw_create_shm()
{
	PSHMContext shmc;
	shmc = SHMC_Create( DIR_SHMKEY, SHM_PROCID, PAGE_SIZE);
	if ( shmc == NULL )
		return -1;
	g_shmptr = shmc->shm_ptr;
	return 0;
}

int rw_attach_shm()
{
	PSHMContext shmc;
	shmc = SHMC_Attach( DIR_SHMKEY, SHM_PROCID, PAGE_SIZE);
	if ( shmc == NULL )
		return -1;
	g_shmptr = shmc->shm_ptr;
	free( shmc );
	return 0;
}

void rw_detach_shm()
{
		if ( g_shmptr ) 
	 		shmdt( g_shmptr );
	 	g_shmptr = NULL;
}

void rw_init_shm()
{
	if ( g_shmptr )
	{
		memset( g_shmptr, 0, PAGE_SIZE );
		g_shmptr->total_respawn = -1;		// so that after first respawn will be 0 after ++ operation.
	}
}

void rw_show_shm()
{
	// TODO
}

/////////////////////////////////////////////////////////////////////////////
void param_default()
{
	get_network_config(&g_stNETConf);
}

int param_write()
{
	struct stat st;
	int fd = open(STR_NETCONF, O_WRONLY|O_CREAT, 0644);
	if ( fd != -1 )
	{
		write(fd, &g_stNETConf, sizeof(g_stNETConf));
		close(fd);
		return 0;		
	}
	return -1;
}

void param_load()
{
	int fd = open( STR_NETCONF, O_RDONLY);
	if ( fd==-1 )
	{
		param_default();
		param_write();
	}
	else
	{
		read(fd, &g_stNETConf, sizeof(g_stNETConf));
		close(fd);	
		set_network_config(&g_stNETConf,NULL);
	}
}

void param_show()
{
	char strIP[16], strMask[16], strGW[16];
	INET_NTOA2(g_stNETConf.ip,strIP);
	INET_NTOA2(g_stNETConf.netmask,strMask);
	INET_NTOA2(g_stNETConf.gw,strGW);
	printf(
			 "NetWork Config:\n"
			 "\t- IP: %s\n"
			 "\t- Mask: %s\n"
			 "\t- Gateway: %s\n"
			 , strIP
			 , strMask
			 , strGW
			 );
}

int param_update( NET_CONFIG_S *netcfg  )
{
	int rc=0;
	if ( netcfg != NULL )
	{
		set_network_config(netcfg, &g_stNETConf);
		memcpy(&g_stNETConf, netcfg, sizeof(NET_CONFIG_S));
	}
	param_write();
	return rc;
}

/////////////////////////////////////////////////////////////////////////////
void factory_default_script()
{
	// 写入控制器对应的开机脚本文件
	static const char *default_rcS = 
	"";
	int fd;
	int len = strlen(default_rcS);
	if( len > 0 ) unlink("/etc/init.d/rcS");
	if ( len > 0 && (fd = open("/etc/init.d/rcS", O_WRONLY|O_CREAT|O_TRUNC, 755 )) != -1 )
	{
		unlink("/etc/init.d/rcS");
		if ( write( fd, default_rcS, len)==len )
			lprintf("rw server engine replace system startup script /etc/init.d/rcS with default content %d bytes!\n", len);
		close(fd);
	}
	else
		lprintf("server failed to open and replace default /etc/init,d/rcS\n");
}

void factory_reset()
{
	lprintf("【FACTORY RESET】恢复出厂设置 ...\n");
	// unlink configuration file
	unlink(STR_NETCONF);
	// reset network config file to default
	factory_default_netconf();
	// reset startup script
	factory_default_script();
	// reboot
	lprintf("!!!系统重启!!!\n");
	reboot(LINUX_REBOOT_CMD_RESTART);
}

/*
 * sleep and usleep will be interrupted by signal and cannot guarenty
 * the time to wait. waitfor do the real wait for specified time period.
 */
void init_itimer(int which)
{
   struct itimerval tmr;

	tmr.it_value.tv_sec = 86400;	// one full day. Shall be enough
	tmr.it_value.tv_usec = 0;
	tmr.it_interval.tv_sec = 86400;
	tmr.it_interval.tv_usec = 0;

	setitimer ( which, &tmr, NULL );
}

long get_itimer( int which )
{
    	struct  itimerval tmr;
    	long	delta_s, delta_us;

    	getitimer( which, &tmr );
    	delta_s = tmr.it_interval.tv_sec - tmr.it_value.tv_sec - 1;
    	delta_us = 1000000 - tmr.it_value.tv_usec;
    	if ( delta_us == 1000000 ) {
    		delta_s++;
    		delta_us = 0;
    	}
    	// when elapsed time is very short, some time you get
    	// negative delta_s. Zero it when it happened.
    	if ( delta_s < 0 ) {
    		delta_s = 0;
    		delta_us = 0;
    	}
    	return delta_s * 1000 + delta_us/1000;
} 
void waitfor( int msec )
{
	int msec_left = msec;
	init_itimer( ITIMER_REAL );
	do {
		usleep( msec_left*1000 );
		msec_left = msec - get_itimer(ITIMER_REAL);
	} while ( msec_left > 0 );
}
/******************************************************************************
* function : check and init global var. 
******************************************************************************/

void lprintf(const char *fmt,...)
{
	// log message in a peticular log file. 
	FILE *fp = fopen(SYSLOG_FILE, "a");
	long	fsize;
	char buf[1024];
	
	va_list va;
	va_start( va, fmt );
	vsprintf(buf, fmt, va );
	va_end( va );
	
	if ( buf[0]=='\t' )
	{
		fprintf(fp, "%24s%s", " ", buf+1 );
		fprintf(stderr, "%24s%s", " ", buf+1 );
	}
	else
	{
		fprintf(fp, "[%s] %s", TIMESTAMP(), buf );
		fprintf(stderr, "[%s] %s", TIMESTAMP(), buf );
	}
	fsize = ftell(fp);
	fclose(fp);
	if ( fsize > SYS_LIMIT )
	{
		unlink( SYSLOG_BKUP );
		rename( SYSLOG_FILE, SYSLOG_BKUP );
	}
}

void ltrans(const char *fmt,...)
{
	// log message in a peticular log file. 
	FILE *fp = fopen(TRANSLOG_FILE, "a");
	long	fsize;
	char buf[1024];
	
	va_list va;
	va_start( va, fmt );
	vsprintf(buf, fmt, va );
	va_end( va );
	
	PRINTF1(buf);
	if ( buf[0]=='\t' )
		fprintf(fp, "%24s%s", " ", buf+1 );
	else
		fprintf(fp, "[%s] %s", TIMESTAMP(), buf );
	fsize = ftell(fp);
	fclose(fp);
	if ( fsize > TRANS_LIMIT )
	{
		unlink( TRANSLOG_BKUP );
		rename( TRANSLOG_FILE, TRANSLOG_BKUP );
	}
}

static void RW_HandleSig(int signo)
{
    if (SIGINT == signo || SIGTSTP == signo)
    {
    	lprintf("program exit on interrupt, pid=%d\n", getpid()); 
    }
    else
    {
    	lprintf("【Engine dead】signal %s received. process pid %d terminated.\n", 
    		strsignal(signo), getpid() );
    }
	shm_reset_engine();
	waitfor(100);
	if ( !gbDaemonMode ) 
	{
		g_shmptr->rw_server_run = 0;
	}
    exit(0);
}

static void signal_handle()
{
	sigset(SIGSEGV, RW_HandleSig);
	sigset(SIGTERM, RW_HandleSig);
	sigset(SIGINT, RW_HandleSig);
	sigset(SIGBUS, RW_HandleSig);
	sigset(SIGPIPE, RW_HandleSig);
}

static void first_parent_on_signal_quit( int sig )
{
	first_parent_quit = 1;	// set flag so that first parent will quit
}

static int rw_daemon_init( void )
{
	pid_t	pid;

	if( (pid = fork()) < 0 ) 
	{
		// fork error
		return -1;
	} 
	else if ( pid != 0 ) 
	{
		// parent process, wait for quit signal from child
		signal( SIGTERM, first_parent_on_signal_quit );
		while( !first_parent_quit )
			sleep(1);
		lprintf("first parent (pid=%d) goes byebye, child (pid=%d) become a daemon now...\n", 
				getpid(), pid );
		exit(0);
	}

	// child process, go on
	first_parent_pid = getppid();

//	chdir("/");		// change current dir to /
	setsid();		// child goes on and become session leader
	umask(0);

	return 0;
}

void sa_childdead( int signo )
{
		int status;
		pid_t pid;

		// phoenix child process exit. should be due to unexpected program error. try to start it again
		while ( (pid=waitpid(0, &status, WNOHANG)) > 0 )
		{
			char *childwho[] = { "other child", "rw engine" };
			int n = pid==g_shmptr->engine_pid ? 1 : 0;
			if( WIFSIGNALED(status) ) 
			{
				lprintf("[Monitor sa_childdead] %s (pid=%d) terminated by signal '%s'.\n",
						childwho[n], pid, strsignal( WTERMSIG( status ) ) );
			} 
			else if ( WIFEXITED(status) )
			{
				int exit_code = WEXITSTATUS( status );
				lprintf( "[Monitor sa_childdead] %s (pid=%d) exited with code '%d'.\n", 
						childwho[n], pid, exit_code );
			}
			if ( n )
				g_shmptr->rw_engine_run = 0;
		}
 		sigset( SIGCHLD, sa_childdead );
}

/*
 * read system time, if RTC not loaded, system time shall be very old. then we have to
 * set a recent time. our time-0 is 2014-08-30 00:00:00
 */
static void setDefSystemTime()
{
		struct timeval tv;
		struct tm tm_host;
		time_t t_host;
		
		time(&t_host);
		localtime_r(&t_host,&tm_host);
		if ( tm_host.tm_year < 114 )
		{
			tm_host.tm_year = 114;
			tm_host.tm_mon = 7;
			tm_host.tm_mday = 30;
			tm_host.tm_hour = 0;
			tm_host.tm_min = 0;
			tm_host.tm_sec = 0;
			t_host = mktime( &tm_host );
			tv.tv_sec = t_host;
			tv.tv_usec = 0;
			settimeofday( &tv, NULL );
			lprintf("No RTC, set Default System Time as 2014/08/30 00:00:00\n");
		}
		else
		{
			lprintf("Has RTC. system time is %04d/%02d/%02d %02d:%02d:%02d\n",
					tm_host.tm_year+1900,  tm_host.tm_mon+1, tm_host.tm_mday,
					tm_host.tm_hour, tm_host.tm_min,  tm_host.tm_sec );
			g_bHasRTC = TRUE;			// RTC exist in this camera, boot-up procedure has been read the RTC and set system time accordingly.
		}
}
	
static void show_help( const char *pname )
{
	printf("Usage: %s [-o{1~3}] [-s] [-x|-r] <-h>\n", pname );
	printf("where:\n"
			   " --out|-o [1~3] set the debug output message level. must be 1~3, default is 0.\n"
			   "    the greater of the level the more detail message will be output.\n"
			   " --daemon|-s work as daemon service. respawn whenever child working thread is dead.\n"
			   "    must go with -a option.\n"
			   " --stop|-x shutdown current running program.\n"
			   " --restart|-r restart program. (stop then start)\n"
			   " --help|-h show this message and exit\n"
			   );
}
	
/* get available memory in KB */
int get_freemem(void)
{
/*	
 	NOTE: total availabe memory is freeram + bufferram. don't know why, the freeram entry
 	      in sysinfo structure is always 0. So we have to grab information from top.
	struct sysinfo info;
	if ( sysinfo(&info)==0 )
		return info.freeram + info.bufferram;
	return -1;
*/	
	char buf[132];
	FILE *fp;
	int	 /*mem_used,*/ mem_free, mem_buff, mem_cached;
	char *ptr_free, /**ptr_used,*/ *ptr_buff, *ptr_cached;
	
	system("top -n 1 > /var/top.txt");
	fp = fopen( "/var/top.txt", "r" );
	if ( fp != NULL )
	{
		fgets( buf, sizeof(buf), fp );
		fclose(fp);
		//ptr_used = strstr(buf, "Mem:") + 4;
		ptr_free = strstr(buf, "used,") + 5;
		ptr_buff = strstr(buf, "shrd,") + 5;
		ptr_cached = strstr(buf, "buff,") + 5;
		//mem_used = atoi(ptr_used);
		mem_free = atoi(ptr_free);
		mem_buff = atoi(ptr_buff);
		mem_cached = atoi(ptr_cached);
		return mem_free+mem_buff+mem_cached;
	}
	return -1;
}


static int get_last_reboot(void)
{
	char line[256];
	FILE *fp = fopen(SYSLOG_FILE, "r");
	struct tm _tm;
	time_t  t_daemon_last = 0;
	int  delta_sec;
	
	if ( fp == NULL ) return 0;
	while ( fgets( line, sizeof(line), fp) != NULL )
	{
		if ( strstr(line, "D A E M O N   M O D E   S T A R T") != NULL )
		{
			if ( sscanf(line, "[%2d/%2d/%2d %2d:%2d:%2d", 
						&_tm.tm_year, &_tm.tm_mon, &_tm.tm_mday, 
						&_tm.tm_hour, &_tm.tm_min, &_tm.tm_sec )==6 )
			{
				_tm.tm_mon--;				// convert to 0 based.
				_tm.tm_year += 100;		//  year since 1900
				t_daemon_last = mktime( &_tm );
			}
		}
	}
	fclose(fp);
	delta_sec = time(NULL) - t_daemon_last;
	lprintf("==> last daemon begin time to now is %d seconds...\n", delta_sec );
	return delta_sec;
}

static void rw_stop_server();
/******************************************************************************
* function    : main() 
* Description : 
*    Startup the system either in interactive mode, automatic mode or darmon mode
*	depends on the run time arguments.
*    
******************************************************************************/

int main(int argc, char * const argv[])
{
	int opt, longidx;
	struct stat st;
	struct option rw_opt[] = {
		{ "help", no_argument, NULL, 'h' },
		{ "daemon", no_argument, NULL, 's' },
		{ "out", required_argument, NULL, 'o' },
		{ "stop", no_argument, NULL, 'x'},
		{ "restart", no_argument, NULL, 'r' },
		{ NULL, 0, NULL, 0 }
	};
	BOOL	bStop = FALSE;
	BOOL bStart = FALSE;
	BOOL	bRestart = FALSE;		// power on, ignore the existing copy probing.
	time_t	t_child_respawn;

	show_copyright();
	//show_special_version();
	// parse run time argumnets
	while ( (opt=getopt_long(argc, argv, ":ho:rsx", rw_opt, &longidx )) != -1 )
	{
		switch (opt)
		{
			case ':':
				printf("missing required parameter for command option %c.\n", opt);
				return FAILURE;
			case '?':
				printf("unknown option %c, ignored\n", optopt );
				break;
			case 's':
				gbDaemonMode = TRUE;
				break;
			case 'o':
				gnOutLevel = atoi(optarg);
				dbg_setconsolelevel(gnOutLevel);
				dbg_setremotelevel(gnOutLevel);
				break;
			case 'x':		// --stop
				bStop = TRUE;
				break;
			case 'r':
				bRestart = TRUE;
				gbDaemonMode = TRUE;
				break;
			case 'h':
				show_help(argv[0]);
				return SUCCESS;
		}							 
	}

	// read system time, if RTC not present, set default time
	setDefSystemTime();
	// test does SD card present and collect any dead image 
	g_bHasSDCard = stat("/mnt/mmc/nosd", &st) != 0;
	PRINTF1(PROGRAM_SHORTNAME" - SD card %s.\n", g_bHasSDCard ? "PRESENT" : "NOT LOADED" );
	
	// attach shared memory if not exist create it.
	if ( rw_attach_shm() != 0 && bStop )
	{
		PRINTF(PROGRAM_SHORTNAME" server is not running - do nothing.\n");
		return FAILURE;
	}			
	if (!shm_is_attached() )
	{
		if ( rw_create_shm()==-1 )
		{
			PRINTF("failed to create shared memory. terminate program.\n" );
			return FAILURE;
		}
		rw_init_shm();
	}

	if ( bStop )
	{
		rw_stop_server();
		if ( !bStart )  
		{
		rw_detach_shm();
			return SUCCESS;
		}
	}

	lprintf("====== START "PROGRAM_SHORTNAME" SERVER PROCESS .... ======\n");
	if ( 1 )
	{
		pid_t	pid;
		// check for redundant launch of engine.
		PRINTF("probe another copy of server (pid=%d) and engine (pid=%d)...\n",
					g_shmptr->master_pid,g_shmptr->engine_pid );
		if ( !bRestart && 
				 ( (g_shmptr->master_pid!=0 && kill(g_shmptr->master_pid,0)==0) || 
				 (g_shmptr->engine_pid!=0 && kill(g_shmptr->engine_pid,0)==0) ) )
		{
			PRINTF("Program alreay running. please stop previous copy first by command './hitool -x'.\n");
			rw_detach_shm();
			return FAILURE;
		}

		// running for RW Surveillance application.
		if ( !gbDaemonMode )
		{
				g_shmptr->master_pid = 0;
				g_shmptr->engine_pid = getpid();
				g_shmptr->rw_server_run = 1;
				dbg_setmarkaddr( (int *)&g_shmptr->dbg_marktime );
				signal_handle();
				RW_Main();
				RW_Exit();
		}
		else	//////////////////// [DAEMON MODE] ////////////////
		{
			rw_daemon_init();
			// set signal to catch SIGCHLD
			lprintf("<<<<<<<<<<<<<<<   D A E M O N   M O D E   S T A R T   >>>>>>>>>>>>> [Version:"STR_VERSION"]\n" );
			sigset( SIGCHLD, sa_childdead );
				g_shmptr->master_pid = getpid();
				g_shmptr->rw_server_run = 1;
				dbg_setmarkaddr( (int *)&g_shmptr->dbg_marktime );
	phoenix_reborn:
				t_child_respawn = time(NULL);
				dbg_touch();
				if( (pid = fork()) < 0 ) 
				{
					// fork fail
					lprintf("phoenix child fork failed.\n");
					exit(0);
				} 
				else if ( pid != 0 ) 
				{
					int  status;
					// phoenix parent process - wait for phoenix child process exit
					// usually, it should never exit. Just in case. If it exit must be program bug.
					// we log the event and restart the child.
					lprintf("[Monitor] start a phoenix child as engine process which pid=%d\n", pid );
					g_shmptr->engine_pid = pid;
					g_shmptr->rw_engine_run = 1;
					
					for(;g_shmptr->rw_engine_run && g_shmptr->rw_server_run;)		// while engine work thread LPNR_Main is running
					{
						int msec_left = 1000;
						int rc_pid;
						/*
						 * wait for 1 second. if child dead, I will be waken and return from usleep call.
						 * but we have to wait for child to finish its signal handler and reset 'lpnr_engine_run'
						 * otherwise, we would respawn a new one before old one gone.
						 */
						init_itimer( ITIMER_REAL );
						do {
							usleep( msec_left*1000 );
							msec_left = 1000 - get_itimer(ITIMER_REAL);
						} while ( msec_left > 0 && g_shmptr->rw_engine_run );
						
						// if engine not running (caught by sa_childdead), break the loop and respawn.
						if ( !g_shmptr->rw_engine_run ) break;		
							
						/*
						 * check for child killed by under-line system. Kill signal cannot be caught. therefore
						 * g_shmptr->lpnr_engine_run still set even engine process has been killed.
						 */
						if ( (rc_pid=waitpid(pid, &status, WNOHANG)) != 0 ) 
						{
							lprintf("[Monitor] "PROGRAM_SHORTNAME" engine process is killed. respawn it.\n");
							goto phoenix_reborn;	
						}
					}
					// when get to this point, either system shutdown or engine is dead
					if ( !g_shmptr->rw_server_run )
					{
						lprintf("[Server] - Stop, kill ParkLot engine process and quit.\n");
						kill( pid, SIGKILL );
					}
					if ( g_shmptr->rw_server_run )
					{
						int i, rc_wpid=-1;
						// make sure engine is dead by wait it. this can prevent it from staying in zombie
						for ( i=0; i<10 && g_shmptr->rw_engine_run; i++ )
						{
							waitfor(100);			// wait 100 msec
							// check for engine process state. waitpid return >0 mean dead and in zombie now is leaved 
							// 0 means child is still running, -1 means that it's gone before waitpid
							if ( (rc_wpid=waitpid(pid, &status, WNOHANG)) !=0 ) break;
							lprintf("[Monitor] "PROGRAM_SHORTNAME" engine process (pid=%d) still exist, send SIGKILL...\n", pid );
							kill( pid, SIGKILL );
						}
						if ( rc_wpid==0 )
						{
							lprintf("[Monitor] GRAVE ERROR phoenix child process blocked and not able to kill. reboot system...\n");
							lprintf("======================   R E B O O T I N G   ===========================\n");
							reboot(LINUX_REBOOT_CMD_RESTART);
							return FAILURE;
						}
						/*
						if ( time(NULL) - t_child_respawn < 3 )
						{
							lprintf("[Monitor] phoenix child engine process exit within 3 seconds - stop!\n");
							//lprintf("======================   R E B O O T I N G   ===========================\n");
							//reboot(LINUX_REBOOT_CMD_RESTART);
							return FAILURE;
						}
						else */
							lprintf("[Monitor] phoenix child engine process has gone, respawn it.\n");
						goto phoenix_reborn;
					}
				}
				else // [PHOENIX CHILD PROCESS]
				{
					// phoenix child process - execute LPNR_Mian function (the engine)
					// inform first parent process to quit, we are a respawnable daemon now.
					if( first_parent_pid != 0 )
					{
						kill( first_parent_pid, SIGTERM );
						first_parent_pid = 0;
					}
					// attach shared memory
				if ( rw_attach_shm() == 0 )
						dbg_setmarkaddr( (int *)&g_shmptr->dbg_marktime );
					else
					{
						lprintf("["PROGRAM_SHORTNAME" phoenix (pid=%d)] Error - failed to attach shared memory.\n", getpid());
						exit(-1);
					}
					dbg_touch();
					g_shmptr->total_respawn++;
					lprintf("["PROGRAM_SHORTNAME" phoenix (pid=%d)] child process up and run. respawn#=%d.\n", getpid(), g_shmptr->total_respawn);
					// phoenix child process, who does the real work
					signal_handle();
					RW_Main();
					RW_Exit();
					shm_reset_engine();
				}	 // end of PHOENIX CHILD Process
		}  // end of if [DAEMON MODE]
	}
		PRINTF("program exit normally!\n");
	rw_detach_shm();
    return SUCCESS;
}

static void rw_stop_server()
{
		int i;
		BOOL bServerQuit = FALSE;
		
		if (g_shmptr->master_pid==0 && g_shmptr->engine_pid==0 )
		{
			printf("rw process is not running, ignored!\n");
			return;
		}
		lprintf("[Shutdown Server] server pid=%d, engine pid=%d......\n", g_shmptr->master_pid, g_shmptr->engine_pid);
		g_shmptr->rw_server_run = 0;
		for(i=0; i<10; i++ )
		{
			// test if server and engine process all gone
			if ( (g_shmptr->master_pid==0 || kill(g_shmptr->master_pid,0)==-1) && 
					  kill(g_shmptr->engine_pid,0)==-1 )
			{
				bServerQuit = TRUE;
				break;
			}
			waitfor( 10 );	// wait 10 msec
		}
		if ( !bServerQuit )
		{
			PRINTF(PROGRAM_SHORTNAME" server and/or engine refused to quit. kill them.\n");
			if ( g_shmptr->master_pid && kill(g_shmptr->master_pid,SIGTERM)!=0 )
				PRINTF("kill master pid error (%s)\n", strerror(errno) );
			if ( kill(g_shmptr->engine_pid,SIGTERM) != 0 )
				PRINTF("kill engine pid error (%s)\n", strerror(errno) );
			waitfor( 5 );
		}
		else
		{
			PRINTF(PROGRAM_SHORTNAME" server & engine terminated.\n");
		}
		g_shmptr->master_pid = 0;
		shm_reset_engine();
}
