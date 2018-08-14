
#ifdef RW_TIME_HEAD
#define RW_TIME_HEAD

#define RW_YEAR
#define RW_MON
#define RW_DAY


#ifdef TIME_YEAR2016
#define RW_YEAR 2016
#elif defined TIME_YEAR2017
#define RW_YEAR 2017
#elif defined TIME_YEAR2018
#define RW_YEAR 2018
#elif defined TIME_YEAR2019
#define RW_YEAR 2019
#else
#define RW_YEAR 2017
#endif	

#if defined TIME_MON01
#define RW_MON 01
#elif defined TIME_MON02
#define RW_MON 02
#elif defined TIME_MON03
#define RW_MON 03
#elif defined TIME_MON04
#define RW_MON 04
#elif defined TIME_MON05
#define RW_MON 05
#elif defined TIME_MON06
#define RW_MON 06
#elif defined TIME_MON07
#define RW_MON 07
#elif defined TIME_MON08
#define RW_MON 08
#elif defined TIME_MON09
#define RW_MON 09
#elif defined TIME_MON10
#define RW_MON 10
#elif defined TIME_MON11
#define RW_MON 11
#else
#define RW_MON 12
#endif

#if defined DTIME_DAY01
#define RW_MON 01
#elif defined DTIME_DAY02
#define RW_MON 02
#elif defined DTIME_DAY03
#define RW_MON 03
#elif defined DTIME_DAY04
#define RW_MON 04
#elif defined DTIME_DAY05
#define RW_MON 05
#elif defined DTIME_DAY06
#define RW_MON 06
#elif defined DTIME_DAY07
#define RW_MON 07
#elif defined DTIME_DAY08
#define RW_MON 08
#elif defined DTIME_DAY09
#define RW_MON 09
#elif defined DTIME_DAY10
#define RW_MON 10
#elif defined DTIME_DAY11
#define RW_MON 11
#elif defined DTIME_DAY12
#define RW_MON 12
#elif defined DTIME_DAY13
#define RW_MON 13
#elif defined DTIME_DAY14
#define RW_MON 14
#elif defined DTIME_DAY15
#define RW_MON 15
#elif defined DTIME_DAY16
#define RW_MON 16
#elif defined DTIME_DAY17
#define RW_MON 17
#elif defined DTIME_DAY18
#define RW_MON 18
#elif defined DTIME_DAY19
#define RW_MON 19
#elif defined DTIME_DAY20
#define RW_MON 20
#elif defined DTIME_DAY21
#define RW_MON 21
#elif defined DTIME_DAY22
#define RW_MON 22
#elif defined DTIME_DAY23
#define RW_MON 23
#elif defined DTIME_DAY24
#define RW_MON 24
#elif defined DTIME_DAY25
#define RW_MON 25
#elif defined DTIME_DAY26
#define RW_MON 26
#elif defined DTIME_DAY27
#define RW_MON 27
#elif defined DTIME_DAY28
#define RW_MON 28
#elif defined DTIME_DAY29
#define RW_MON 29
#elif defined DTIME_DAY30
#define RW_MON 30
#else
#define RW_MON 31	
#endif


#endif