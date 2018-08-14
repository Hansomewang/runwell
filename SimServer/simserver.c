#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include "./utils/longtime.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdarg.h>
#include <sys/mman.h>
#include <dirent.h>

// #define CHIP_3531
#define CHIP_3520

typedef enum SIMSTAT
{
	SIMFSM_NONE = 0, // 表示无状态，SIM卡无节点，无pppd程序后台启动
	SIMFSM_HAS_SIM,	 // 节点出来了，这个时候需要启动pppd服务了
	SIMFSM_STARTED,  // 存在pppd和节点,可以ping服务器了
}SIMFSM_E;

typedef struct {
	char key[64];
	char value[128];
}SimpleConfig;
static char remote_ip[16];
SimpleConfig configlist[20];
static int find_process( const char *info );

extern ssize_t getline(char **lineptr, size_t *n, FILE *stream);
extern int iponline( const char *IP );

#define MAX_LOGSIZE 1000000
#define LOG_FILE "sim7100c_run.log"
#define BKUP_FILE "sim7100c_runbak.log"

static const char* time_stamp()
{
	static char	mybuf[ 40 ];
	char num[5] = {0};
	struct timeval tv; 
	struct timezone tz;
	gettimeofday( &tv, &tz );
	time_t now = tv.tv_sec;
	struct tm *tmp = localtime( &now );
	strftime(mybuf, sizeof( mybuf ), "[%F %H:%M:%S.", tmp);
	sprintf( num, "%d]", ( int )tv.tv_usec % 1000 );
	strcat( mybuf, num );
	return mybuf;
}

void trace_log(const char *fmt,...)
{
	int rc = 0;
	static FILE *fp;
	if(fp==NULL)
	 fp = fopen(LOG_FILE, "a");
	long	fsize;
	char buf[1024];
	va_list va;
	va_start( va, fmt );
	vsprintf(buf, fmt, va );
	va_end( va );
	if  ( fp != NULL )
	{
		fsize = ftell(fp);
		if ( fsize > MAX_LOGSIZE )
		{
			fclose( fp );
			fp = NULL;
			unlink( BKUP_FILE );
			rename( LOG_FILE, BKUP_FILE );			
		}		
		else if ( fsize <= 0 )
		{
			fclose( fp );
			fp = NULL;
			fp = fopen(LOG_FILE, "a");
		}
	}
	if ( fp == NULL )
		fp = fopen(LOG_FILE, "a");
	if ( fp != NULL )
	{
		if ( buf[0]=='\t' )
			rc = fprintf(fp, "%24s%s", " ", buf+1 );
		else
			rc = fprintf(fp, "%s %s", time_stamp(), buf );
		printf("%s %s", time_stamp(), buf );
		if ( rc < 0 )
		{
			fclose( fp );
			fp = NULL;
		}
		else
			fflush( fp );
	}
}

static int sparse_line( const char *srcbuf , char *buf, char *key, char *value)
{
	char *midptr;
	int len = strlen( srcbuf );
	if( len > 128 ) 
		return -1;
	const char *ptr = srcbuf;
	for( ; *ptr == ' '; ptr++ );
	if( *ptr == '#' )
	{
		printf("解释行，不做处理！\r\n");
		return -1;
	}

	if( (midptr = strstr( srcbuf, "=" ) ) == NULL ) 
		return -1;
	memset( key, 0, 128 );
	memset( value, 0, 128 );
	strncpy( key, srcbuf, midptr - srcbuf );
	key[ midptr - srcbuf]  = '\0';
	strncpy( value , midptr + 1, len - ( midptr - srcbuf ));
	char *tail = NULL;
	if( ( tail = strstr( value, "#" ) ) != NULL )
	{
		tail --;
		len = tail - value;
		for(; (*tail == '\0' ||  *tail == ' ' ||  *tail == '\t' || *tail == '\n' ) && len > 0  ; tail--, len -- );
		*(tail + 1) = '\0';
		return 0;
	}
	tail = value + strlen( value );
	len =strlen( value );
	for(; (*tail == '\0' ||  *tail == ' ' ||  *tail == '\t' || *tail == '\n' ) && len > 0  ; tail--, len -- );
	*(tail + 1) = '\0';
	return 0;
}

static int conf_init()
{
	memset( configlist, 0, sizeof( configlist ));
	return 0;
}

static int write_config()
{
	char buf[1024] = {"ServerIP=192.168.254.5"};
	int fd = open( "SimSer.conf", O_RDWR | O_CREAT | O_TRUNC );
	if( fd < 0 )
		return -1;
	write( fd, buf, strlen( buf ) + 1 );
	close(fd);
	return 0;
}

int load_config(const char *path)
{
	if( access( path, F_OK ) != 0 )
	{
		trace_log("配置文件[%s]不存在，创建默认的配置文件..\r\n", path);
		if( write_config() == -1 )
		{
			trace_log("config file %s not exist\r\n", path);
			return -1;
		}
	}
	conf_init();
	struct stat sta;
	stat( path, &sta);
	if( sta.st_size <= 0 )
		return -1;
	FILE * fp;
	char * line = NULL;
	size_t len = 0;
	ssize_t read;
	int index = 0;
	fp = fopen( path, "r");
	if (fp == NULL)
		return -1;
	char bufline[128], bufkey[128], bufvalue[128];
	while ((read = getline(&line, &len, fp)) != -1) {
		if( sparse_line( line , bufline, bufkey, bufvalue ) == 0 )
		{
			strcpy( configlist[index].key, bufkey );
			strcpy( configlist[index].value, bufvalue );
			index++;
			printf("Register key:%s value:%s\r\n", bufkey, bufvalue );
		}
	}
	fclose( fp );
	if (line)
       free(line);
	return 0;
}

// 获取对应key值对应的value
const char *get_value( const char *key )
{
	int i = 0;
	for( i = 0; i < sizeof( configlist ) / sizeof( configlist[0] ); i ++ )
	{
		if( configlist[i].key[0] == '\0' )
			break;
		if( strcmp( key, configlist[i].key ) == 0 )
			return configlist[i].value;
	}
	return NULL;
}

static void himm(unsigned int phy_addr, unsigned int ulNew )
{
#define PAGE_SIZE 0x1000
#define PAGE_SIZE_MASK 0xfffff000
	void *addr=NULL;
	void *pMem;
	unsigned int phy_addr_in_page;
	unsigned int page_diff;
	unsigned int size_in_page;
	unsigned int size = 128;
	unsigned int ulOld;
	int fd;
	if ((fd = open ("/dev/mem", O_RDWR | O_SYNC) ) < 0) 
  	{
		trace_log("Unable to open /dev/mem: %s\n", strerror(errno));
		return;
  	}
	/* addr align in page_size(4K) */
	phy_addr_in_page = phy_addr & PAGE_SIZE_MASK;
	page_diff = phy_addr - phy_addr_in_page;
	size_in_page =((size + page_diff - 1) & PAGE_SIZE_MASK) + PAGE_SIZE;
	addr = mmap ((void *)0, size_in_page, PROT_READ|PROT_WRITE, MAP_SHARED, fd,  phy_addr_in_page);
	if (addr == MAP_FAILED)
	{
		trace_log("himm run failed!\n" );
		close( fd );
		return;
	}
	pMem = (void *)(addr+page_diff);
	ulOld = *(unsigned int *)pMem;
	*(unsigned int *)pMem = ulNew;
	printf("pyAddr:%08X :0x%08X --> 0x%08X \n", phy_addr, ulOld, ulNew);
	close( fd );
	munmap(addr, size_in_page);
}

static int sim_node_exist()
{
	char *ppp_dev = NULL;
#ifdef CHIP_3520
	ppp_dev = "/dev/ttyUSB3";
#elif defined CHIP_3531
	ppp_dev = "/dev/ttyUSB3";
#endif	
	return access( ppp_dev, F_OK ) == 0 ? 1 : 0;
}

// 系统初始化，映射GPIO节点，开启复用关系
static int SystemInit( int force )
{
	trace_log("GPIO初始化..\r\n");
#ifdef CHIP_3520
// 
	himm( 0x200F00B0, 0 );
	himm( 0x20150400, 0x40 );
	himm( 0x20150100, 0x00 );
#elif defined CHIP_3531
// 18_3 
	himm( 0x200F0254, 0x00 );
	himm( 0x20270400, 0x08 );
	himm( 0x20270020, 0x00 );
#endif
	return 0;
}

static int SIMRest()
{
#ifdef CHIP_3520
	// 拉高
	himm( 0x20150100, 0xff );
	usleep( 600000 );
	himm( 0x20150100, 0x00 );
#elif defined CHIP_3531
	himm( 0x20270020, 0xff );
	usleep( 600000 );
	himm( 0x20270020, 0x00 );
#endif
	return 0;
}

static void SIM7100C_Start()
{
	// 如果都不存在ppd节点，则初始化管脚复用
	if( 0 == sim_node_exist() )
		SystemInit(1); 
    load_config( "SimSer.conf" );
    const char *strip = get_value( "ServerIP" );    
    if( strip == NULL )
        strip = "192.168.254.5";
    strcpy( remote_ip, strip );
}

static void pppd_call()
{
    const char *cmd = "pppd call wcdma &";
    if( 0 == system( cmd ) ) 
        trace_log("run pppd services!\r\n");
    else
        trace_log("call services failed! :%s \r\n", strerror( errno )); 
}

static SIMFSM_E NowStatus()
{
	if( 0 < find_process( "pppd" ) )
		return SIMFSM_STARTED;
	if( sim_node_exist() )
		return SIMFSM_HAS_SIM;
	else
		return SIMFSM_NONE;
}

static void read_conf(const char *path, char *buffer, int size  )
{
    int fd = open( path, O_CREAT | O_WRONLY );
	if( fd < 0 ) 
	    return -1;
	read( fd, buffer, size );
    close( fd );
	return 0;
}

static int check_route()
{
	char buf[4096];
	int fd = open( "/proc/net/route", O_RDONLY );
	if( fd < 0 )
	{
		trace_log("open route failed!:%s \r\n", strerror( errno ) );
		return -1;
	}
	read( fd, buf, sizeof( buf ));
	close( fd );
	if( strstr( buf, "ppp0    00000000        00000000" ) != NULL )
		return 1;
	else 
		return 0;
}



static void route_reset()
{
	static int init = 0;
	if( init )
		return;
	trace_log("reset the route table, to ensure the network is reachable!\r\n");
	const char *routedel = "route del default";
	const char *routeadd = "route add -net default netmask 0.0.0.0 dev ppp0";
	system( routedel );
	usleep( 10000 );
	system( routeadd );
	init = 1;
}

static void SignalHandle( int code )
{
	
}

static int write_pid(int pid)
{
#define PID_TMP_FILE "/home/simserv/Pid.dat"
	int fd = -1;
	fd = open( PID_TMP_FILE, O_RDWR | O_CREAT|O_TRUNC );
	if( fd < 0 )
		return -1;
	if( sizeof( pid ) != write( fd, &pid, sizeof( pid )) )
	{
		close( fd );
		return -1;
	}
	close( fd );
	return 0;
}


// 获取进程名对应的ID号，（ 仅支持非Deamon的单进程 )
static int find_process( const char *info )
{
#define DEFAUTL_PROCESS_PATH  "/proc"
	DIR *dir;
	struct dirent *ptr;
	char num;
	char full_path[120] = {0};
	dir = opendir( DEFAUTL_PROCESS_PATH );
	if( dir == NULL )
	{
		printf("open dir error:" DEFAUTL_PROCESS_PATH "\r\n");
		return 0;
	}	
	while( (ptr = readdir( dir )) != NULL )
	{
		// if it is . or ../
		if( ptr->d_name[0] == '.' )
			continue;
		num = ptr->d_name[0];
		if( '0' <= num && num <= '9' )
		{
			if( atoi( ptr->d_name ) < 500 )
				continue;
			snprintf( full_path, sizeof( full_path ), DEFAUTL_PROCESS_PATH "/%s/cmdline", ptr->d_name );
			if( access( full_path, F_OK ) == 0 )
			{
				FILE *file = fopen( full_path, "r" );
				char buf[30] = {0};
				if( file )
				{
					fgets( buf, sizeof( buf ), file );
					fclose( file );
					if( strstr( buf, info ) )
					{
						closedir( dir );
						return atoi( ptr->d_name );
					}
				}
			}
		}
	}
	closedir( dir );
	return -1;
}

int dpch_exit();
int dhcp_init( const char *inter );
int dhcp_wakeup( int enable );


static void set_sim_node()
{
	char *path = "/etc/ppp/peers/wcdma";
	char *str_old = "/dev/ttyUSB2";
	char *qcqmi = "/dev/qcqmi*";
	char *ppp_dev6 = "/dev/ttyUSB6";

	char *str_new = NULL;

	if( access( qcqmi, F_OK ) == 0 )
	 {
		 printf("find qcqmi* node  repleace /dev/ttyUSB1 \n ");
		 str_new = "/dev/ttyUSB1";
		 file_replace_str(path, str_old, str_new, 0);
	 }
	 else if( access( ppp_dev6 , F_OK ) == 0 )
	 {
		 printf("find ttyUSB6 node  repleace /dev/ttyUSB4 \n ");
		 str_new = "/dev/ttyUSB4";
		 file_replace_str(path, str_old, str_new, 0);
	 }

}

void RW_Main()
{
	int HasResetRoute = 0;
	int count=0;
	dhcp_init(NULL );
	SIM7100C_Start();
	_longtime interval_tick = timeGetLongTime();
	// sigset( SIGUSR1, SignalHandle );
	SIMFSM_E NowFsm,LasFsm = NowStatus();
	write_pid(getpid());
	int has_ppp0;
	while( 1 )
	{
		NowFsm = NowStatus();
		if( interval_tick  > timeGetLongTime() )
		{
			// 如果状态未发生改变则一直延时，如果状态发生改变，则重置计时器，直接进入下一下状态，并进行应该做的事情
			if( LasFsm != NowFsm )
			{
				printf("Last:%d now:%d \r\n", LasFsm, NowFsm );
				interval_tick  =  timeGetLongTime();
				 LasFsm  =  NowFsm;
			}
			else
				usleep(200000);
			continue;
		}
		if( NowFsm == SIMFSM_NONE )
		{
			// 每次重置sim需要等待节点产生，这个需要一个时间!,我们设置20秒超时，差不多够了
			trace_log("SIM发送重置信号，等待30等待节点生成!\r\n");
			dhcp_wakeup( 0 );
			SIMRest();
			interval_tick = timeGetLongTime() + 20*1000;
		} else if(  NowFsm == SIMFSM_HAS_SIM )
		{
			// 当存在pppd节点后开始启动pppd服务，给5秒启动时间
			trace_log("Call PPPD!");
			
		#ifdef CHIP_3520
			set_sim_node();
		#endif

			pppd_call();
			// 给10秒启动时间
			interval_tick = timeGetLongTime() + 5*1000;
		} else if(  NowFsm == SIMFSM_STARTED )
		{
			if( HasResetRoute == 0)
			{
				HasResetRoute = 1;
				if( check_route() == 0 )
				{
					trace_log("检查路由表没有默认指向ppp0的路由，添加路由！\r\n");
					route_reset();
				}else{
					trace_log("已经存在关于pppd的路由表项，不再添加路由！\r\n");
				}
			}
			if( iponline( remote_ip ) == 0 )
			{
				dhcp_wakeup( 1 );
				count++;
				interval_tick = timeGetLongTime() + 1*1000;
				//如果15秒一直ping不通，则reset sim模块
				if( count > 15 )
				{
					trace_log("连续15秒无法ping通服务器，重启SIM模块\r\n");
					NowFsm = SIMFSM_NONE;
					count = 0;
				}
				continue;
			}else
			{
				interval_tick = timeGetLongTime() + 5*1000;
				printf("ping通服务器，睡5秒!\r\n");
			}
			count = 0;
		}else
			;	// do nothing
	}
}

void RW_Exit()
{
	dpch_exit();
	// do nothing.
}
