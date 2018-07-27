#ifndef __MTRACE_INCLUDED__
#define __MTRACE_INCLUDED__

/* NOTE: 
	程序在 inlcude "mtrace.h" 之前就要定义AP是否要使能日志功能。最好定义在Makefile里面
	一般在Release的版本会关闭log。重要的log（流程，事件等）不是debug程序本身需要的，
	应该另外单独记录，别和DEBUG用途的输出日志混在一起。

	原来使用trace,c/Trace.h的程序，要改为mtrace.c/mtrace.h，简单的方式是：
	1. include "Trace.h" 改为 include "mtrace.h"
	2. 在程序模块里面定义一个file scope的变量
		static void *hModuleLog=NULL;

	3. 将原来TRACE_xxxx 的函数定义为 MTRACE_xxxx
		#define LOG_INIT(dir,base)		((hModuleLog=MLOG_INIT(dir,base)) != NULL)
		#define TRACE_LOG(fmt,...)		MTRACE_LOG(hModuleLog,fmt)
		#define LOG_CLOSE()				MLOG_CLOSE(hModuleLog)
*/
#if defined __DEBUG || defined ENABLE_LOG

#define MLOG_INIT					mlog_init
#define MTRACE_LOG				mtrace_log
#define MLOG_CLOSE				mlog_close
#define MLOG_SETPATH			mlog_setpath
#define MLOG_ENABLE				mlog_enable
#define MLOG_KEEPOPEN		mlog_keepopen
#define MLOG_SETLIMITCNT		mlog_setlimitcnt

#else		// if not defined __DEBUG

#if defined _WIN32 || defined _WIN64
#define MLOG_INIT					(1 ? NULL : mlog_init)
#define MTRACE_LOG				(1 ? (void)0 : mtrace_log)
#define MLOG_CLOSE				(1 ? (void)0 : mlog_close)
#define MLOG_SETPATH			(1 ? (void)0 : mlog_setpath)
#define MLOG_ENABLE				(1 ? (void)0 : mlog_enable)
#define MLOG_KEEPOPEN		(1 ? (void)0 : mlog_keepopen)
#define MLOG_SETLIMITCNT		(1 ? (void)0 : mlog_setlimitcnt)
#else		// linux
#define MLOG_INIT(dir,name)
#define MTRACE_LOG(h, fmt...)
#define MLOG_CLOSE(h)
#define MLOG_ENABLE(h,b)
#define MLOG_KEEPOPEN(h,b)
#define MLOG_SETLIMITCNT(h,cnt)
//#define 
#endif

#endif

#ifdef __cplusplus
extern "C" {
#endif

extern void *mlog_init( const char *LogDir, const char *BaseName );
extern void mtrace_log( void *h, const char *fmt, ...);
extern void mlog_close( void *h );
extern void mlog_setpath( void *h, const char *path);
// 临时允许/禁止log。Log被禁止的时候，调用 mtrace_log不起作用。默认是enable
extern void mlog_enable( void  *h, BOOL bEnable );
// 是否保持日志文件打开的状态，避免频繁打开，关闭文件。默认是KEEP-OPEN
extern void mlog_keepopen( void  *h, BOOL bKeep );
// 设置每个log文件大小为 'limit' KB, 一共有'cnt'个滚动备份旧文件
extern void mlog_setlimitcnt( void *h, int liimit, int cnt );

#ifdef __cplusplus
}
#endif

#endif

