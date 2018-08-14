#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <error.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

/*
#define GPIO_I2C_READ   	0x01
#define GPIO_I2C_WRITE  	0x03
#define RESET 						0x40
#define START_DOG 				0x41
#define FEED_DOG  				0x42
#define STOP_DOG  				0x43
*/
//---------------------------------------------

#ifdef hi3516a
// ¡¾HI3516A¡¿
#define RTC_SET_TIME 			0x44
#define RTC_GET_TIME 			0x45
#define DEV_RTC						"/dev/hi_rtc"
typedef unsigned int TIME_UNIT;
typedef struct RTC_T
{
      TIME_UNIT  year;
      TIME_UNIT  month;
      TIME_UNIT  mday;
      TIME_UNIT  hour;
      TIME_UNIT  min;
      TIME_UNIT  sec;
      TIME_UNIT  yday;
} rtc_time;

//---------------------------------------------
#elif defined hi3516
// ¡¾HI3516¡¿

#define RTC_SET_TIME 0x08
#define RTC_GET_TIME 0x09
#define DEV_RTC						"/dev/gpioi2c_32"
typedef unsigned char TIME_UNIT;
typedef struct RTC_T
{
      TIME_UNIT  year;
      TIME_UNIT  month;
      TIME_UNIT  mday;
      TIME_UNIT  yday;
      TIME_UNIT  hour;
      TIME_UNIT  min;
      TIME_UNIT  sec;
} rtc_time;

#elif defined hi3520
// ¡¾HI3520¡¿
#define DEV_RTC						"/dev/rtc0"
#define RTC_SET_TIME 			0x401c700a
#define RTC_GET_TIME 			0x801c7009
typedef unsigned int TIME_UNIT;
typedef struct RTC_T
{
      TIME_UNIT  year;
      TIME_UNIT  month;
      TIME_UNIT  mday;
      TIME_UNIT  hour;
      TIME_UNIT  min;
      TIME_UNIT  sec;
      TIME_UNIT  yday;
} rtc_time;

//---------------------------------------------

#else
#error RTC library only supports HI3516, HI3516A and HI3520 platform
#endif

/*
typedef struct Aptina_9M034_DATA{
	unsigned char   I2cDevAddr;
	unsigned short  I2cRegAddr;
	unsigned short  RWData    ;
} Aptina_9M034_DATA;
*/
static int write_time(rtc_time *rtc_data)
{
		int fd = -1;
		int ret;
    fd = open(DEV_RTC, 0);
    if(fd<0)
    {
    	printf("Open RTC device file "DEV_RTC" error - %s\n", strerror(errno));
			return -1;
    }

// for HI3516 board, RTC chip data should be converted to BCD code
#ifdef hi3516
		rtc_data->sec=((rtc_data->sec/10)<<4) | (rtc_data->sec%10);
		rtc_data->min=((rtc_data->min/10)<<4) | (rtc_data->min%10);
		rtc_data->hour=((rtc_data->hour/10)<<4) | (rtc_data->hour%10);
		rtc_data->mday=((rtc_data->mday/10)<<4) | (rtc_data->mday%10);
		rtc_data->month=((rtc_data->month/10)<<4) | (rtc_data->month%10);
		rtc_data->year=((rtc_data->year/10)<<4) | (rtc_data->year%10);
#endif
    ret = ioctl(fd, RTC_SET_TIME, rtc_data);
		close(fd);
		return ret;
}

static int read_time(rtc_time *rtc_data)
{
  int fd = -1;
	int ret;
  
  fd = open(DEV_RTC, 0);
  if(fd<0)
  {
  	printf("Open RTC device file "DEV_RTC" error - %s\n", strerror(errno));
		return -1;
  }

  ret = ioctl(fd, RTC_GET_TIME, rtc_data);
  //printf("ret=%d\n",ret);
	close(fd);
	
#ifdef hi3516
	// for Hi3516, RTC data is in BCD, so we need to convert it into equivalent binary value

	rtc_data->sec=((rtc_data->sec&0x70)>>4)*10+(rtc_data->sec&0x0f);
	rtc_data->min=((rtc_data->min&0x70)>>4)*10+(rtc_data->min&0x0f);
	rtc_data->hour=((rtc_data->hour&0x30)>>4)*10+(rtc_data->hour&0x0f);
	rtc_data->yday=rtc_data->yday&0x0f;
	rtc_data->mday=((rtc_data->mday&0x30)>>4)*10+(rtc_data->mday&0x0f);
	rtc_data->month=((rtc_data->month&0x10)>>4)*10+(rtc_data->month&0x0f);
	rtc_data->year=((rtc_data->year&0xf0)>>4)*10+(rtc_data->year&0x0f);
	//printf("rtc_data->year=%d,rtc_data->month=%d,rtc_data->mday=%d,rtc_data->yday=%d,rtc_data->hour=%d,rtc_data->min=%d,rtc_data->sec=%d\n",rtc_data->year,rtc_data->month,rtc_data->mday,rtc_data->yday,rtc_data->hour,rtc_data->min,rtc_data->sec);
#endif
	return ret;
}

static void systime2rtctime(const struct tm *tm, rtc_time *rtc)
{
#ifdef hi3516
	rtc->year = tm->tm_year - 100;
#else	
	rtc->year = tm->tm_year + 1900;
#endif	
	rtc->month = tm->tm_mon + 1;
	rtc->mday = tm->tm_mday;
	rtc->hour = tm->tm_hour;
	rtc->min = tm->tm_min;
	rtc->sec = tm->tm_sec;
	rtc->yday = 0;
}

static void rtctime2systime(const rtc_time *rtc, struct tm *tm)
{
#ifdef hi3516
	tm->tm_year = rtc->year;
	tm->tm_year += 100;
#else	
	tm->tm_year = rtc->year - 1900;
#endif	
	tm->tm_mon = rtc->month - 1;
	tm->tm_mday = rtc->mday;
	tm->tm_hour = rtc->hour;
	tm->tm_min = rtc->min;
	tm->tm_sec = rtc->sec;
	tm->tm_yday = 0;
}


int rtc_read(struct tm *tm)
{
	rtc_time rtc_data;
	if ( read_time(&rtc_data) != -1 )
	{
		rtctime2systime(&rtc_data, tm);
		return 0;
	}
	return -1;
}

int rtc_write(const struct tm *tm)
{
	rtc_time rtc_data;
	systime2rtctime(tm, &rtc_data);
	return write_time(&rtc_data);
}

int rtc_sync()
{
	struct tm tm;
	time_t tnow;
	rtc_time rtc_data;
	
	tnow = time(NULL); 
	localtime_r(&tnow, &tm);
	systime2rtctime(&tm, &rtc_data);
	return write_time(&rtc_data);
}

int rtc_load()
{
	struct tm tm_host;
	if ( rtc_read(&tm_host)==0 )
	{
		struct timeval tv;
		time_t tval;
		tval = mktime(&tm_host);
		tv.tv_sec = tval;
		tv.tv_usec = 0;
		return settimeofday( &tv, NULL );
	}
	return -1;	
}

#ifdef ENABLE_EXECUTABLE
#include <stdarg.h>
#include <time.h>

static void show_rtc_time()
{
	struct tm tm_host;
	if ( rtc_read(&tm_host)==0 )
		printf("RTC content: %04d/%02d/%02d %02d:%02d:%02d\n",
				tm_host.tm_year + 1900,
				tm_host.tm_mon + 1,
				tm_host.tm_mday,
				tm_host.tm_hour,
				tm_host.tm_min,
				tm_host.tm_sec );
	else
		printf("failed to read RTC content - %s\n", strerror(errno));
}

static void load_rtc_time()
{
	if ( rtc_load()!=0 )
		printf("failed to read RTC content - %s\n", strerror(errno));
	else
	{
		printf("load RTC time and set as system time:\n");
		show_rtc_time();
	}
}

static void sync_rtc_time()
{
	if ( rtc_sync()!=0 )
		printf("failed to write RTC contents - %s\n", strerror(errno));
	else
	{
		printf("write current system time to RTC done!\n");
		show_rtc_time();
	}
}

int main(int argc, char * const argv[])
{
	int opt;
    while ( (opt=getopt(argc, argv, "hvrsl" )) != -1 )
		{
			switch (opt)
			{
				case '?':
					printf("invalid option '%c', ignored\n", optopt);
					return -1;
				case 'h':
		      printf("usage: %s [options & arguments\n"
		      			 " options can be one of -h, -v, -r -s.\n"
		      			 " -h: show this help message\n"
		      			 " -v: show 'rtcsync' version\n"
		      			 " -r: read current RTC contents and display\n"
		      			 " -l: read current RTC contents and set system as RTC time\n"
		      			 " -s: read current system time and write to RTC chip\n"
		      			 "if no option & arguments at all, program show RTC contents (same as -r)\n"
		      			 "if arguments <YYYY> <MM> <DD> <hh> <mm> <ss> provided,\n"
		      			 "'%s' write given time to RTC and set as system time.\n"
		      			 "ex: %s 2016 08 15 14 36 58 will set system time and RTC contents as specified time\n"
		      			 , argv[0], argv[0], argv[0]);
					return 0;
				case 'v':
					printf("rtcsync V1.1 (build %s)...\r\n", __DATE__);
					return 0;
				case 'r':
					show_rtc_time();
					break;
				case 's':
					//sync_systime2rtc();
					sync_rtc_time();
					break;
				case 'l':
					load_rtc_time();
					break;
			}
		}
       
    if ( argc==1 )
				show_rtc_time();
				
		else if ( argc==7 && optind==1 )
		{
			// command line like: rtcsync 2016 8 15 14 47 32 
			struct tm tm_host;
			struct timeval tv;
			time_t tval;
			
     	tm_host.tm_year = atoi(argv[1])-1900;
    	tm_host.tm_mon = atoi(argv[2]) - 1;
    	tm_host.tm_mday = atoi(argv[3]);
    	tm_host.tm_hour = atoi(argv[4]);
    	tm_host.tm_min = atoi(argv[5]);
    	tm_host.tm_sec = atoi(argv[6]);
    	tm_host.tm_yday = 0;
    	if ( (tm_host.tm_year<0) || 
    			 (tm_host.tm_mon<0 || 11<tm_host.tm_mon) || 
    		   (tm_host.tm_mday<1 || 31<tm_host.tm_mday) ||
    		   (tm_host.tm_hour<0 || 23<tm_host.tm_hour) ||
    		   (tm_host.tm_min<0 || 59<tm_host.tm_min) ||
    		   (tm_host.tm_sec<0 || 59<tm_host.tm_sec) )
    	{
    		printf("invalid date/time value\n");
    		return -1;
    	}
    	tval = mktime( &tm_host );
    	if (tval != (time_t)(-1) )
    	{
				tv.tv_sec = tval;
				tv.tv_usec = 0;
				settimeofday( &tv, NULL );
	    	if ( rtc_write(&tm_host)!=0 )
	    		printf("failed to write RTC contents - %s\n", strerror(errno));
	    	else
	    		printf("set system time and write RTC done!\n");
	    }
	    else
	    	printf("invalid date/time arguments!\n");
    }
		return 0;
}

#endif
