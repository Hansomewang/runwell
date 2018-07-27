/*
 *  rwlpnrapi.c
 *	
 *	This source file implement the RunWell vehicle license plate number recognition (LPNR) camera
 *  API. 
 *
 *  This source file is dual plateform coded (linux and Windows). In Windows, using VC6 or VS 20xx can generate
 *  a DLL with each exported API in CALLTYPE (defined in rwlpnrapi.h, could be C or PASCAL) calling sequence.
 *  In Linux plateform, it's very easy to create a so file (shared object) for Lane software to bind into their application
 *  in runtime (of cause, to statically link into lane application is also applicable). In Linux, the API calling sequency
 *  is 'C'.
 *
 *  In Windows plateform, Event is forward to application via Windows PostMessage system call, while in LInux
 *  plateform, a callback function assigned by application is invoked to notice following event:
 *	  event number 1: LPNR equipment online
 *    event number 2: LPNR equipment offline
 *    event number 3: LPNR equipment start one image processing
 *    event number 4: LPNR processing done. data is available
 *
 *  Author: Thomas Chang
 *  Copy Right: RunWell Control Technology, Suzhou
 *  Date: 2014-10-29
 *  
 * NOTE 
 *  To build a .so file in linux, use command:
 *  $ gcc -o3 -Wall -shared -orwlpnrapi.so  rwlpnrapi.c
 *  To build a standalone executable tester for linux, include log, use command:
 *  $ gcc -o3 -Wall -DENABLE_LOG -DENABLE_TESTCODE -olpnrtest -lpthread -lrt rwlpnrapi.c 
 *  To enable the program log for so
 *  $ gcc -o3 -Wall -DENABLE_LOG -shared -rwlpnrapi.so -lpthread rwlpnrapi.c
 *
 *  To built a DLL for Windows plateform,  use Microsoft VC or Visual Studio, create a project for DLL and add rwlpnrapi.c, rwlpnrapi.h lpnrprotocol.h
 *  and rwlpnrapi.def (define the DLL export API entry name, otherwise, VC or VS will automatically add prefix/suffix for each exported entries).
 *  
 *
 *  ��������ʹ�ô˺�����˵����
 *
 *  1. ��ʼ����
 *	   	LPNR_Init ����ΪLPNR�豸��IP��ַ�ִ�������0��ʾ�ɹ���-1 ��ʾʧ�� (IP��ַ���󣬻��Ǹ�IPû����ӦUDPѯ��)
 *		  
 *		 
 *	2. ���ûص�����(linux)��������Ϣ���մ���(Windows)
 *		 LPNR_SetCallBack -- for linux and/or Windows
 *		 LPNR_SetWinMsg   -- for windows
 *			��ʼ���ɹ����������á�
 *
 *	3. ��ȡ���ƺ��룺
 *		 LPNR_GetPlateNumber - ����Ϊ���ú������泵�ƺ����ִ���ָ�룬���ȱ����㹻�����ƺ�����Ϊ
 *       <��ɫ><����><����>�����磺����A123456. ���û�б�ʶ�����ƣ������ִ�"���ƾ�ʶ"��
 *
 *	4. ��ȡ���Ʋ�ɫСͼ
 *		 LPNR_GetPlateColorImage - ����Ϊ����ͼƬ��bufferָ�룬ͼƬ����Ϊbmp��ʽ��ֱ�Ӵ�������buffer��һ��
 *       �ļ�����һ��bmpͼ������������ֵ������buffer�ĳ��ȡ�Ӧ�ó����豣֤buffer�����㹻����Ҫ�ĳ���Ϊ
 *			 ����Сͼ����x����+54����Ϊ����ͼƬ�ߴ�ÿ�δ���������ͬ�������ṩһ�������ܵĳߴ硣
 * 
 *	4. ��ȡ���ƶ�ֵ��Сͼ
 *		 LPNR_GetPlateBinaryImage - ����Ϊ����ͼƬ��bufferָ�룬ͼƬ����Ϊbmp��ʽ��ֱ�Ӵ�������buffer��һ��
 *       �ļ�����һ��bmpͼ������������ֵ������buffer�ĳ��ȡ�Ӧ�ó����豣֤buffer�����㹻����Ҫ�ĳ���Ϊ
 *			 ����Сͼ����x����+54����Ϊ����ͼƬ�ߴ�ÿ�δ���������ͬ�������ṩһ�������ܵĳߴ硣
 *
 *	5. ��ȡץ����ͼ �����г���ʶ������������ͼƬ��
 *		 LPNR_GetCapturedImage - ����Ϊ����ͼƬ��bufferָ�룬ͼƬ����Ϊjpg��ʽ��ֱ�Ӵ�������buffer��һ��
 *       �ļ�����һ��jpegͼ������������ֵ������buffer�ĳ��ȡ�Ӧ�ó����豣֤buffer�����㹻����Ҫ�ĳ��ȸ���Ϊ
 *       ���������� x factor. factor�������õ�JPEGѹ��������ԼΪ0.1~0.5��
 *
 *  6. ��ȡ��ǰʵʱͼ��֡ ����ʼ������ʵʱͼ�����ͣ��ֱ����ǰͼ�������ɲż������£�
 *		 LPNR_GetLiveFrame - ������LPNR_GetCapturedImage��ͬ��Ҳ��һ��jpeg֡�����ݡ�Ĭ����ʹ�ܷ���ʵʱͼ��
 *		 ���Ե���LPNR_EnableLiveFrame����ʵʱͼ���ͣ���С���縺�ɡ�
 *
 *  7. ѯ��״̬�ĺ���
 *		LPNR_IsOnline - �Ƿ����ߣ����ز���ֵ
 *		LPNR_IsIdle - ʶ����Ƿ����ʶ�����У����ز���ֵ
 *     LPNR_GetPlateColorImageSize - ���س��Ʋ�ɫСͼ��С
 *     LPNR_GetPlateBinaryImageSize - ���س��ƶ�ֵ��Сͼ��С
 *     LPNR_GetCapturedImageSize - ����ץ�Ĵ�ͼ��С��# of bytes)
 *     LPNR_GetHeadImageSize - ��ȡ��ͷͼ��С������ʹ�ܷ��ͳ�ͷͼ��
 *     LPNR_GetQuadImageSize - ��ȡ1/4ͼͼƬ��С������ʹ�ܷ���ץ��Сͼ��
 *	   LPNR_GetTiming - ��ȡʶ�����ɵ�ǰʶ�������ĵ�ʱ�� ���ܹ���ʱ����ܴ���ʱ�䣩
 *
 *  8. �ͷŴ���������ݺ�ͼƬ(�������)
 *		 LPNR_ReleaseData
 *
 *  9. ������ץ��ʶ��
 *		 LPNR_SoftTrigger - ץ�� + ʶ��
 *	     LPNR_TakeSnapFrame - ץ��һ��ͼ����������ʶ��
 *
 *	10. ����
 *		 LPNR_Terminate - ���ô˺�������ʹ��LPNR����̬���ڲ������߳̽������ر��������ӡ�
 *		
 *  11. ������������
 *		LPNR_SyncTime - ͬ��ʶ���ʱ��ͼ����ʱ�� ���·���ʱ���
 *		LPNR_EnableLiveFrame - ʹ�ܻ��ǽ���ʶ�������ʵʱͼ��֡
 *      LPNR_ReleaseData - �ͷŵ�ǰʶ������̬���������
 *		LPNR_Lock/LPNR_Unlock - �������������ݣ���ȡʵʱ֡�����κ�ʶ��������֮ǰ�ȼ�������ȡ�������������ȡ�����б������̸߳��ġ�
 *		LPNR_GetPlateAttribute - ��ȡ����/����������Ϣ
 *		LPNR_GetExtDI - ��ȡһ�������չDI״̬
 *		LPNR_GetMachineIP - ��ȡʶ���IP�ִ�
 *		LPNR_GetCameraLabel - ��ȡʶ�����ǩ�����֣�
 *
 *	�¼���ţ�
 *	�ص�����ֻ��һ�����������¼���š�����windows������������Ϣ֪ͨ�� LPARAM���棬WPARAMΪ�����¼��������
 *  ��� (��LPNR_Init����)��
 *    �¼����1��LPNR���ߡ�
 *    �¼����2��LPNR����
 *    �¼����3����ʼ����ʶ����
 *    �¼����4��ʶ�����������Ի�ȡ�����
 *	  �¼����5��ʵʱͼ��֡���£����Ի�ȡ�����»��档
 *    �¼����6����������������Ȧ���������λ��Ҫ���յ��¼����4��ſ���ȥ��ȡ������Ϣ��ͼƬ��
 *    �¼����7�������뿪������Ȧ�����
 *    �¼����8����չDI��״̬�б仯��200D, 200P, 500������չIO��
 *
 *  NOTE - ENABLERS
 *  1. ENABLE_LOG	- define �󣬳�����¼��־�ڹ̶����ļ���
 *  2. ENABLE_TESTCODE - define �󣬲��Գ���ʹ�ܣ����Ա������ִ�г�����ԣ�linux�汾���У�
 *
 *  �޸ģ�
 *  A. 2015-10-01��
 *     1. �������� LPNR_GetHeadImageSize, LPNR_GetHeadImage
 *         ���ܣ���ȡ��ͷͼ��ͼ���СΪCIF ��PALΪ720x576�� �Գ���λ��Ϊ���ĵ㣬 û�г�������ʶ����Ϊ���ĵġ�
 *     2. �������� LPNR_GetQuadImageSize, LPNR_GetQuadImage
 *         ���ܣ���ȡ1/4�����ͼ��
 *     3. �������� LPNR_GetHeadImageSize, LPNR_GetHeadImage��ȡ��ͷͼ��ͼƬ��СΪCIF
 *	   ��������ͼ��û���ַ�������Ϣ��Ҫ���ʶ����汾 1.3.1.21 ������֮����С�
 *
 * B 2016-01-15
 *		1. �����¼����6��7��8  �����ʶ�������汾1.3.2.3֮�󣨺����汾��
 *      2. �������� LPNR_GetExtDI�� LPNR_GetMachineIP��LPNR_GetPlateAttribute
 *	    3. ��ʼ��ʶ���ʱ���Ȳ���ָ��IP��ʶ�����û����ӦUDP��ѯ����û�еĻ���ʾ��IPû��ʶ�����û�����߻��ǳ���û�����С�
 *          һ�����ش������������ڳ�ʼ��ʱ������֪��ʶ����ǲ��Ǳ��ס����õȳ�ʱû�����ߡ�
 *		4. ��־����λ���޸� 
 *			- linux: /var/log/lpnr_<ip>.log ��/var/log������ڣ�
 *          - Windows: ./LOG/LPNR-<IP>.log (ע�⣬���򲻻ᴴ��LOG�ļ��У������ֹ�������
 *
 * C 2016-04-13
 *		1. �����ӿ� LPNR_GetCameraLabel
 *
 * D 2016-12-16
 *     1. �����ӿ� - LPNR_QueryPlate: �ڲ��Ƚ���TCP���ӵ�����£��򵥵�һ������ʵ��UDP��������ѯ���ƺ��룬�ر�UDP�����ؽ����
 *          �˺������ڶԳ�λ�������ѯ����λ������Ҫ�����������ӵ�����£�һһ��ѯÿ����λ�ĵ�ǰ���ƺ��롣�����ƺ���������
 *          �����ƾ�ʶ��, ��ʾû�г����������г�û�йҳ��ƣ�����������Ǹ���λ����ѯͣ������λ�����ǰ��λ�ϳ����ĳ��ƺ��롣
 *
 * E 2017-06-07
 *     1. �����ӿ� 
 *			- LPNR_LightCtrl - ����ʶ��������ƣ���200A���ͣ�����ʹ��������ON��OFFѶ�ſ������˻�̧��ˡ�
 *			- LPNR_SetExtDO - ������չDO����߱���չIO�Ļ��Ͳ���Ч��)
 * F 2017-07-11
 *     1. �����ӿ�
 *		  - DLLAPI BOOL CALLTYPE LPNR_SetOSDTimeStamp(HANDLE h, BOOL bEnable, int x, int y);
 *
 * G 2017-09-29
 *		1. ����ʶ�������Ѷ��������ƽӿ�
 *		  - DLLAPI BOOL CALLTYPE LPNR_PulseOut(HANDLE h, int pin, int period, int count);
 *		2. ��������͸������
 *		  - DLLAPI BOOL CALLTYPE LPNR_COM_init(HANDLE h, BOOL bEnable, int Speed);
 *		  - DLLAPI BOOL CALLTYPE LPNR_COM_aync(HANDLE h, BOOL bEnable);
 *		  - DLLAPI BOOL CALLTYPE LPNR_COM_send(HANDLE h, BYTE *data, int len);
 *		  - DLLAPI BOOL CALLTYPE LPNR_COM_iqueue(HANDLE h);
 *		  - DLLAPI BOOL CALLTYPE LPNR_COM_peek(HANDLE h, BYTE *RxData, int size);
 *		  - DLLAPI BOOL CALLTYPE LPNR_COM_read(HANDLE h, BYTE *RxData, int size);
 *		  - DLLAPI int CALLTYPE LPNR_COM_remove(HANDLE h, int size);
 *		  - DLLAPI int CALLTYPE LPNR_COM_clear(HANDLE h);
 * 
 * H 2018-03-07
 *    1. LPNR_GetPlateAttribute �����У�����attr������������
 *		  attr[5]��������ɫ���� (PLATE_COLOR_E)
 *        attr[6]:  ��������(PLATE_TYPE_E)
 *    2. ����ɫ���Ƶĳ�����ɫ���ָ�Ϊ"��", ���磺����E12345. �Ա����ɫ���䳵������
 *    ע�� �¼�EVT_PLATENUM����ǰ���͵ĳ��ƺ��룬ʶ����汾1.3.10.3��ʼ������ɫ�Żᷢ�͡���������Ƿ��͡��̡���
 *            �յ�EVT_DONE��ȥ��ȡ�ĳ��ƺ��룬��ֻҪ��֧������Դ����ʶ����汾������ɫ�ƶ��������֡���Ϊ����
 *            ��̬����ݳ�����ɫ���������ɫ���֡�
 *
 *  I 2018-03-26
 *    1. �����Ȧ����˫���ܣ���ͷ����β����ͷ+��β����ʶ�����ǰ�ϱ��ĳ��ƺ�������ǳ�βʶ�𣬻��ڳ��ƺ��������Ϻ�׺
 *        (��β). ��̬���������׺ɾ���������������� attr[1]����Ϊ��β�����յ�EVT_PLATENUM�¼��󣬿��Ի�ȡ���ƺ��루LPNR_GetPlateNumber��
 *       �Լ���ͷ��β��Ϣ��LPNR_GetPlateAttribute�������Ǵ�ʱ��ȡ��attributeֻ��attr[1]��������ġ������ĳ�Ա��Ҫ���յ�EVT_DONE�¼����ٵ���
 *       һ��LPNR_GetPlateAttribute��ȡ��
 *    2. ����һ���¼� EVT_ACK. ʶ����յ����п���֡(DO, PULSE)�󣬻ᷢ��Ӧ��֡��DataType��DTYP_ACK, DataId�����п���֡�Ŀ����� ctrlAttr.code
 *
 * J  2018-03-31
 *    1. LPNR_GetExtDI�ӿڣ���ȡDIֵ����Ϊ LPNR_GetExtDIO (��ȡ��ǰDI��DO��ֵ)
 */
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#ifdef linux
#include <error.h>
#endif
#include <errno.h>
#include "lpnrprotocol.h"



// enabler
// #define ENABLE_LOG
// #define ENABLE_BUILTIN_SOCKET_CODE
// #define ENABLE_LPNR_TESTCODE
#ifndef ENABLE_BUILTIN_SOCKET_CODE
#include <utils_net.h>
#endif

#define IsValidHeader( hdr)  ( ((hdr).DataType & DTYP_MASK) == DTYP_MAGIC )

#define FREE_BUFFER(buf)	\
	if ( buf ) \
	{ \
		free( buf ); \
		buf = NULL; \
	}

#ifdef linux 
// -------------------------- [LINUX] -------------------------
#include <unistd.h>
#include <signal.h>
#include <assert.h>
#include <pthread.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// nutalize or mimic windows specific functions
#define winsock_startup()
#define winsock_cleanup()
#define closesocket(fd)	close(fd)
#define WSAGetLastError()	( errno )
// map other windows functions to linux equivalent
#define Sleep(n)		usleep((n)*1000)
#define ASSERT			assert
// nutral macro functions with common format for both windows and linux
#define Mutex_Lock(h)		pthread_mutex_lock(&(h)->hMutex )
#define Mutex_Unlock(h)	pthread_mutex_unlock(&(h)->hMutex )
#define Ring_Lock(h)		pthread_mutex_lock(&(h)->hMutexRing )
#define Ring_Unlock(h)	pthread_mutex_unlock(&(h)->hMutexRing )
#define DeleteObject(h)		pthread_mutex_destroy(&h)
//#define Mutex_Lock(h)	
//#define Mutex_Unlock(h)	
#define min(a,b)	( (a)>(b) ? (b) : (a) )
// log function. 
#ifdef ENABLE_LOG
#define TRACE_LOG(h, fmt...)		trace_log(h, fmt)
#else
#define TRACE_LOG(h, fmt...)		
#endif
#define LOG_FILE	"/var/log/lpnr-%s.log"		// /var/log is the log file director in linux convention
#define BKUP_FILE	"/var/log/lpnr-%s.bak"		// for embedded linux system, /var/log usually is a RAM file system
static void *lpnr_workthread_fxc( void *arg );

unsigned long GetTickCount()
{
	static signed long long begin_time = 0;
	static signed long long now_time;
	struct timespec tp;
	unsigned long tmsec = 0;
	if ( clock_gettime(CLOCK_MONOTONIC, &tp) != -1 )
	{
		now_time = tp.tv_sec * 1000 + tp.tv_nsec / 1000000;
	}
	if ( begin_time = 0 )
		begin_time = now_time;
	tmsec = (unsigned long)(now_time - begin_time);
	return tmsec;
}

#else	
// -------------------------- [WINDOWS] -------------------------
#include <winsock2.h>
#include <Ws2tcpip.h>

// common enabler - better put them in VS project setting
//#define _CRT_SECURE_NO_WARNINGS
//#define _CRT_SECURE_NO_DEPRECATE

// nutral macro functions with common format for both windows and linux
#define Mutex_Lock(h)			WaitForSingleObject((h)->hMutex, INFINITE )
#define Mutex_Unlock(h)		ReleaseMutex(  (h)->hMutex )
#define Ring_Lock(h)			WaitForSingleObject((h)->hMutexRing, INFINITE )
#define Ring_Unlock(h)		ReleaseMutex(  (h)->hMutexRing )
static BOOL m_nWinSockRef = 0;

/* 'strip' is IP string, 'sadd_in' is struct sockaddr_in */
#define inet_aton( strip, saddr_in )	( ( (saddr_in) ->s_addr = inet_addr( (strip) ) ) != INADDR_NONE )  
#ifdef ENABLE_LOG
#define TRACE_LOG			trace_log
#else
#define TRACE_LOG     1 ? (void)0 : trace_log
#endif
#define LOG_FILE			"/rwlog/LPNR-%s.log"
#define BKUP_FILE		"/rwlog/LPNR-%s.bak"
DWORD WINAPI lpnr_workthread_fxc(PVOID lpParameter);
#endif

#include "rwlpnrapi.h"


// -------------------------- [END] -------------------------

#define LPNR_PORT			6008
#define MAX_LOGSIZE		4194304		// 4 MB
#define MAGIC_NUM			0xaabbccdd

#define STAT_RUN				1		// work thread running
#define STAT_END			2		// work thread end loop
#define STAT_EXIT			3		// work thread exit

#define MAX_RING_SIZE		1024

typedef enum {
	UNKNOWN=0,
	NORMAL,
	DISCONNECT,
	RECONNECT,
}  LINK_STAT_E;

typedef enum {
	OP_IDLE=0,
	OP_PROCESS,
	OP_RREPORT,
} OP_STAT_E;

typedef struct tagLPNRObject
{
	DWORD	dwMagicNum;
	SOCKET sock;
	char  strIP[16];
	int		status;
	char label[SZ_LABEL];
	LINK_STAT_E	enLink;
	OP_STAT_E		enOper;
	DWORD tickLastHeared;
	int		acked_ctrl_id;			// �յ������п��������ACK֡DataId, ������ݾ������п���֡�Ŀ��������� header.ctrlAttr.code
#ifdef linux
	pthread_t		hThread;
	pthread_mutex_t		hMutex;
#else	
	HANDLE	hThread;
	HANDLE	hMutex;
	HWND		hWnd;
	int				nMsgNo;
#endif	
	LPNR_callback  cbfxc;
	PVOID		pBigImage;			// input image in jpeg
	int				szBigImage;			// input image size
	PVOID		pSmallImage;			// plate image in BMP (note - Xinluwei is in JPEG
	int				szSmallImage;
	PVOID		pBinImage;
	int				szBinImage;
	PVOID		pHeadImage;
	int				szHeadImage;
	PVOID		pT3DImage;
	int				szT3DImage;
	PVOID		pQuadImage;
	int				szQuadImage;
	PVOID		pLiveImage;
	int				nLiveSize;
	int				nLiveAlloc;
	char			strPlate[12];
	PlateInfo	plateInfo;
	int				process_time;		// in msec
	int				elapsed_time;		// in msec
	// ����ʶ��ԭ��
	TRIG_SRC_E enTriggerSource;		// TRIG_SRC_E ö�����͵� intֵ 
	//	for model with extended GPIO 
	int				diParam;				// ��16λ��last DI value����16λ��current DI value
	short			dio_val[2];			// [0]��DI��[1]��DO. ����ץ�Ļ��ڿͻ������ӻ���������CTRL_READEXTDIO�ϱ���
														// dio_val[0]Ӧ����Զ��diParam�ĵ�16λ��ͬ��
	// �汾ѶϢ
	VerAttr		verAttr;
	// ���������ѶϢ
	ParamConf	paramConf;
	ExtParamConf extParamConf;
	DataAttr		dataCfg;
	H264Conf	h264Conf;
	OSDConf	osdConf;
	OSDPayload osdPayload;
	// ͸���������ݽ���ring buffer (for model with external serial port)
#ifdef linux
	pthread_mutex_t		hMutexRing;
#else	
	HANDLE	hMutexRing;
#endif
	int		ring_head, ring_tail;
	BYTE	ring_buf[MAX_RING_SIZE];
} HVOBJ,		*PHVOBJ;

#define IS_VALID_OBJ(pobj)	( (pobj)->dwMagicNum==MAGIC_NUM )

#define NextTailPosit(h)		( (h)->ring_tail==MAX_RING_SIZE-1 ? 0 : (h)->ring_tail+1 )
#define NextHeadPosit(h)	( (h)->ring_head==MAX_RING_SIZE-1 ? 0 : (h)->ring_head+1 )
#define IsRingEmpty(h)		( (h)->ring_head==(h)->ring_tail )
#define IsRingFull(h)			( NextTailPosit(h) == (h)->ring_head )
#define RingElements(h)		(((h)->ring_tail >= (h)->ring_head) ? ((h)->ring_tail - (h)->ring_head) : (MAX_RING_SIZE - (h)->ring_head + (h)->ring_tail) )
#define	PrevPosit(h,n)		( (n)==0 ? MAX_RING_SIZE-1 : (n)-1 )
#define	NextPosit(h,n)		( (n)==MAX_RING_SIZE-1 ? 0 : (n)+1 )
#define RingBufSize(h)		( MAX_RING_SIZE-1 )
// pos ����ring_buf���λ�ã������ǵڼ������ݣ���head��ʼ����0��
#define PositIndex(h,pos)		( (pos)==(h)->ring_tail ? -1 : ((pos)>=(h)->ring_head ? (pos)-(h)->ring_head : ((MAX_RING_SIZE-(h)->ring_head-1) + (pos))) )
// PositIndex�ķ�������idx����head��ʼ�ĵڼ������ݣ�head��0��������ֵ����ring_buffer���λ�á�
#define IndexPoist(h,idx)	(  (idx)+(h)->ring_head<MAX_RING_SIZE ? (idx)+(h)->ring_head : (idx+(h)->ring_head+1-MAX_RING_SIZE) )
#define GetRingData(h,pos)		((h)->ring_buf[pos])
#define SetRingData(h,pos, b)		(h)->ring_buf[pos] = (BYTE)(b);

// function prototypes for forward reference
///////////////////////////////////////////////////////////
// TCP functions
#ifdef _WIN32
#define sock_read( s, buf, size )	recv( s, buf, size, 0 )
#define sock_write(s, buf, size )	send( s, buf, size, 0 )
#define sock_close( s )				closesocket( s )
#define WINSOCK_START()		winsock_startup()
#define WINSOCK_STOP()		winsock_cleanup()
#else
#define WINSOCK_START()
#define WINSOCK_STOP()
#endif

#ifdef ENABLE_BUILTIN_SOCKET_CODE
#ifndef linux		// windows plateform
static int winsock_startup()
{
	int rc = 0;
	if ( !m_nWinSockRef )
	{
		WORD wVersionRequested = MAKEWORD( 2, 2 );
		WSADATA wsaData;
		rc = WSAStartup( wVersionRequested, &wsaData );
	}
	if ( rc==0 ) m_nWinSockRef++;
	return rc;
}
static int winsock_cleanup()
{
	if ( m_nWinSockRef>0 && --m_nWinSockRef==0 )
		return WSACleanup();
	else
		return 0;
}
#else		// linux
typedef struct sockaddr_in SOCKADDR_IN;

#endif
static int sock_dataready( SOCKET fd, int tout )
{
	fd_set	rfd_set;
	struct	timeval tv, *ptv;
	int	nsel;

	FD_ZERO( &rfd_set );
	FD_SET( fd, &rfd_set );
	if ( tout == -1 )
	{
		ptv = NULL;
	}
	else
	{
		tv.tv_sec = 0;
		tv.tv_usec = tout * 1000;
		ptv = &tv;
	}
	nsel = select( (int)(fd+1), &rfd_set, NULL, NULL, ptv );
	if ( nsel > 0 && FD_ISSET( fd, &rfd_set ) )
		return 1;
	return 0;
}

static SOCKET sock_connect( const char *strIP, int port )
{
	SOCKET 			fd;
	struct sockaddr_in	destaddr;

	memset( & destaddr, 0, sizeof(destaddr) );
	destaddr.sin_family = AF_INET;
	destaddr.sin_port = htons( (short)port );
  if ((inet_aton(strIP, & destaddr.sin_addr)) == 0)
		return INVALID_SOCKET;
	fd = socket(PF_INET, SOCK_STREAM, 0);
	if (fd < 0)   return INVALID_SOCKET;

	if ( connect(fd, (struct sockaddr *)&destaddr, sizeof(destaddr)) < 0 )
	{
		sock_close(fd);
		return INVALID_SOCKET;
	}
	return fd;
}

static int sock_read_n_bytes(SOCKET fd, void* buffer, int n)
{
	char *ptr = (char *)buffer;
	int len;

	while( n > 0 ) 
	{
		//len = recv(fd, ptr, n, MSG_WAITALL);
		if ( sock_dataready(fd, 3000)==0 ) break;		// data time out. in 1000 msec no following TCP package
		len = recv(fd, ptr, n, 0);
		if( len <= 0)			// socket broken. sender close it or directly connected device (computer) power off
			return -1;
		ptr += len;
		n -= len;
	}
	return (int)(ptr-(char*)buffer);
}

static int sock_skip_n_bytes(SOCKET fd, int n)
{
	char buffer[ 1024 ];
	int len;
	int left = n;

	while( left > 0 ) 
	{
		//len = recv(fd, ptr, n, MSG_WAITALL);
		if ( sock_dataready(fd, 1000)==0 ) break;		// data time out. in 1000 msec no following TCP package
		len = recv(fd, buffer, min( n, sizeof( buffer ) ), 0);
		if( len == SOCKET_ERROR )
			return -1;
		if( len <= 0 ) break;
		left -= len;
	}
	return (n-left);
}

// drain all data currently in socket input buffer
static int sock_drain( SOCKET fd )
{
	int	n = 0;
	char	buf[ 1024 ];

	while ( sock_dataready( fd, 0 ) )
		n += recv(fd, buf, sizeof(buf), 0);
	return n;
}

static int sock_drain_until( SOCKET fd, unsigned char *soh, int ns )
{
	char buffer[1024];
	char *ptr;
	int i, len, nskip=0;
	while ( sock_dataready(fd,300) )
	{
		/* peek the data, but not remove the data from the queue */
		if ( (len = recv(fd, buffer, sizeof(buffer), MSG_PEEK)) == SOCKET_ERROR || len<=0 )
			return -1;

		/* try to locate soh sequence in buffer */
		for(i=0, ptr=buffer; i<=len-ns; i++, ptr++) 
			if ( 0==memcmp( ptr, soh, ns) )
				break;
		nskip += (int)(ptr - buffer);
		if ( i > len-ns )
			recv( fd, buffer, len, 0 );
		else
			recv( fd, buffer, (int)(ptr-buffer), 0 );
		if ( i <= len-ns )
			break;
	}
	return nskip;
}

static SOCKET sock_udp_open()
{
	return socket(AF_INET,SOCK_DGRAM,0);		
}
static int sock_udp_timeout( SOCKET sock, int nTimeOut )
{
	return setsockopt( sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&nTimeOut, sizeof ( nTimeOut ));
}
static int sock_udp_send0( SOCKET udp_fd, DWORD dwIP, int port, const char * msg, int len )
{
	SOCKADDR_IN	udp_addr;
	SOCKET sockfd;
	int	 tmp = 1, ret;
	BOOL bBroadcast=TRUE;
	
	if ( udp_fd == INVALID_SOCKET )
	{
		sockfd = socket( AF_INET, SOCK_DGRAM, 0 );
		if( sockfd == INVALID_SOCKET ) return SOCKET_ERROR;
		if ( dwIP == INADDR_BROADCAST )
			setsockopt( sockfd, SOL_SOCKET, SO_BROADCAST, (char *)&bBroadcast, sizeof(BOOL) );
		else
    	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&tmp, sizeof(tmp));
	}
	else
		sockfd = udp_fd;

	memset( & udp_addr, 0, sizeof(udp_addr) );
	udp_addr.sin_family = AF_INET;
	udp_addr.sin_addr.s_addr = dwIP;		// NOTE - dwIP is already network format. DON'T USE htonl(dwIP)
	udp_addr.sin_port = htons( (short)port );
	if ( len == 0 )
		len = (int)strlen( msg );

	ret = sendto( sockfd, msg, len, 0, (const struct sockaddr *) & udp_addr, sizeof(udp_addr) );

	if ( udp_fd == INVALID_SOCKET )
		sock_close( sockfd );

	return ret;
}

static int sock_udp_recv( SOCKET sock, char *buf, int size, DWORD *IPSender )
{
	SOCKADDR_IN  sender;
	int		addrlen = sizeof( sender );
	int		ret;

	if ( (ret = recvfrom( sock, buf, size, 0, (struct sockaddr *)&sender, &addrlen )) > 0 )
	{
		if ( IPSender != NULL )
			*IPSender = sender.sin_addr.s_addr;
	}
	return ret;
}
#else

#endif

//////////////////////////////////////////////////////////////
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
static void trace_log(PHVOBJ hObj, const char *fmt,...)
{
	va_list		va;
	char		str[ 1024 ] = "";
	char		file[128];
	FILE *fp;
	
	va_start(va, fmt);
	vsprintf(str, fmt, va);
	va_end(va);
	sprintf(file, LOG_FILE, hObj ? hObj->strIP : "null" );
	fp = fopen( file, "a" );
	if ( fp!=NULL && ftell(fp) > MAX_LOGSIZE )
	{
		char bkfile[128];
		fclose(fp);
		sprintf(bkfile, BKUP_FILE, hObj ? hObj->strIP : "null" );
		remove( bkfile );
		rename( file, bkfile );
		fp = fopen( file, "a" );
	}
	if ( fp != NULL )
	{
		if ( fmt[0] != '\t' )
			fprintf(fp, "[%s] %s", time_stamp(), str );
		else
			fprintf(fp, "%26s%s", " ", str+1 );
		fclose(fp);
	}
}

///////////////////////////////////////////////////////////////
// LPNR API
static const char *GetEventText( int evnt )
{
	switch (evnt)
	{
	case EVT_ONLINE	:	return "����ʶ�������";
	case EVT_OFFLINE:	return "����ʶ�������";
	case EVT_FIRED:		return "��ʼʶ����";
	case EVT_DONE:		return "ʶ������������ѽ������";
	case EVT_LIVE:		return "ʵʱ֡����";
	case EVT_VLDIN:		return "��������������Ȧ�����";
	case EVT_VLDOUT:	return "�����뿪������Ȧ�����";
	case EVT_EXTDI:		return "��չDI��״̬�仯";
	case EVT_SNAP:		return "���յ�ץ��֡";
	case EVT_ASYNRX:	return "���յ�͸��������������";
	case EVT_PLATENUM: return "���յ���ǰ���͵ĳ��ƺ�";
	case EVT_VERINFO:	return "�յ�ʶ����������汾ѶϢ";
	case EVT_ACK:			return "�յ����п���֡��ACK�ر�";
	}
	return "δ֪�¼����";
}

static const char *GetTriggerSourceText(TRIG_SRC_E enTrig)
{
	switch(enTrig)
	{
	case IMG_UPLOAD:
		return "��λ������ͼƬʶ��";
	case IMG_HOSTTRIGGER:
		return "��λ��������";
	case IMG_LOOPTRIGGER:
		return "��ʱ�Զ�����";
	case IMG_AUTOTRIG:
		return "��ʱ�Զ�����";
	case IMG_VLOOPTRG:
		return "������Ȧ����";
	case IMG_OVRSPEED:
		return "���ٴ���";
	}
	return "Unknown Trigger Source";
}

static void NoticeHostEvent( PHVOBJ hObj, int evnt  )
{
	if ( evnt != EVT_LIVE )
		TRACE_LOG((HANDLE)hObj, "�����¼���� %d  (%s) ��Ӧ�ó���.\n", evnt, GetEventText(evnt) );

	if ( hObj->cbfxc )  hObj->cbfxc((HANDLE)hObj, evnt);
#ifdef WIN32
	if ( hObj->hWnd )
		PostMessage( hObj->hWnd, hObj->nMsgNo, (DWORD)hObj, evnt );
#endif
}

static void ReleaseData(PHVOBJ pHvObj)
{
	Mutex_Lock(pHvObj);
	FREE_BUFFER ( pHvObj->pBigImage );
	FREE_BUFFER ( pHvObj->pSmallImage );
	FREE_BUFFER ( pHvObj->pBinImage );
	FREE_BUFFER( pHvObj->pHeadImage );
	FREE_BUFFER(pHvObj->pT3DImage);
	FREE_BUFFER( pHvObj->pQuadImage );
	pHvObj->szBigImage = 0;
	pHvObj->szBinImage = 0;
	pHvObj->szSmallImage = 0;
	pHvObj->szHeadImage = 0;
	pHvObj->szT3DImage = 0;
	pHvObj->szQuadImage = 0;
	pHvObj->strPlate[0] = '\0';
	memset( &pHvObj->plateInfo, 0, sizeof(PlateInfo) );
	Mutex_Unlock(pHvObj);
}

#ifdef PROBE_BEFORE_CONNECT
static BOOL TestCameraReady( DWORD dwIP )
{
	DataHeader header;
	int len;
	BOOL bFound = FALSE;
	SOCKET sock = sock_udp_open();

	sock_udp_timeout( sock, 500 );
	SEARCH_HEADER_INIT( header );
	len = sock_udp_send0( sock, dwIP, PORT_SEARCH, (char *)&header, sizeof(DataHeader) );
	if ( (len=sock_udp_recv( sock, (char *)&header, sizeof(DataHeader), &dwIP)) == sizeof(DataHeader) && 
		header.DataId == DID_ANSWER )
		bFound = TRUE;
	sock_close( sock );
	return bFound;
}
#endif

DLLAPI HANDLE CALLTYPE LPNR_Init( const char *strIP )
{
	PHVOBJ hObj = NULL;

#ifdef linux
	struct sockaddr_in 	my_addr;
	if ( inet_aton( strIP, (struct in_addr *)&my_addr)==0 )
#else
	DWORD threadId;

	WINSOCK_START();
	if ( inet_addr(strIP) == INADDR_NONE )
#endif
	{
		return NULL;
	}
#ifdef PROBE_BEFORE_CONNECT
	if ( !TestCameraReady(inet_addr(strIP)) )
	{
		// ʶ���ûӦ��
		WINSOCK_STOP();
		return NULL;
	}
#endif
	// 1. ��������
	hObj = (PHVOBJ)malloc(sizeof(HVOBJ));
	memset( hObj, 0, sizeof(HVOBJ) );
	strcpy( hObj->strIP, strIP );
	hObj->strIP[15] = '\0';
	hObj->sock = INVALID_SOCKET;
	hObj->dwMagicNum = MAGIC_NUM;

	// 2. ����Mutex�� and ���������߳�

#ifdef linux
	pthread_mutex_init(&hObj->hMutex, NULL );
	pthread_mutex_init(&hObj->hMutexRing, NULL );
	pthread_create( &hObj->hThread, NULL, lpnr_workthread_fxc, (void *)hObj);
#else
	hObj->hMutex = CreateMutex(
							NULL,				// default security attributes
							FALSE,			// initially not owned
							NULL);				// mutex name (NULL for unname mutex)

	hObj->hMutexRing = CreateMutex(
							NULL,				// default security attributes
							FALSE,			// initially not owned
							NULL);				// mutex name (NULL for unname mutex)

	hObj->hThread = CreateThread(
						NULL,						// default security attributes
						0,								// use default stack size
(LPTHREAD_START_ROUTINE)lpnr_workthread_fxc,		// thread function
						hObj,						// argument to thread function
						0,								// thread will be suspended after created.
						&threadId);				// returns the thread identifier
#endif
	TRACE_LOG(hObj,"��LPNR_Init(%s)�� - OK\n", strIP );
	return hObj;
}

DLLAPI BOOL CALLTYPE LPNR_Terminate(HANDLE h)
{
	//int i;
	PHVOBJ pHvObj = (PHVOBJ)h;
	
	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;
		
	TRACE_LOG(pHvObj, "��LPNR_Terminate��\n");
	if ( pHvObj->hThread )
	{
		pHvObj->status = STAT_END;
#ifndef linux		
		//for(i=0; i<10 && pHvObj->status!=STAT_EXIT; i++ ) Sleep(10);
		WaitForSingleObject(pHvObj->hThread, 100);
		if (  pHvObj->hThread )
		{
			// work thread not terminated. kill it
			TerminateThread( pHvObj->hThread, 0 );
		}
		CloseHandle(pHvObj->hMutex);
		CloseHandle(pHvObj->hMutexRing);
#else
		pthread_cancel( pHvObj->hThread );
		pthread_join(pHvObj->hThread, NULL);
		pthread_mutex_destroy(&pHvObj->hMutex );
		pthread_mutex_destroy(&pHvObj->hMutexRing );
#endif		
	}
	ReleaseData(pHvObj);
	//DeleteObject(pHvObj->hMutex);
	if ( pHvObj->sock != INVALID_SOCKET )
 		closesocket( pHvObj->sock);
	free( pHvObj );	
	WINSOCK_STOP();
	return TRUE;
}


// ����Ӧ�ó�����¼��ص�����
DLLAPI BOOL CALLTYPE LPNR_SetCallBack(HANDLE h, LPNR_callback mycbfxc )
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	
	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;
		
	TRACE_LOG(pHvObj,"��LPNR_SetCallBack��\n");
	pHvObj->cbfxc = mycbfxc;
	return TRUE;
}
#if defined _WIN32 || defined _WIN64
// ����Ӧ�ó��������Ϣ�Ĵ��ھ������Ϣ���
DLLAPI BOOL CALLTYPE LPNR_SetWinMsg( HANDLE h, HWND hwnd, int msgno)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;

	TRACE_LOG(pHvObj,"��LPNR_SetWinMsg�� - msgno = %d\n", msgno);
	pHvObj->hWnd = hwnd;
	pHvObj->nMsgNo = msgno;
	return TRUE;
}
#endif

DLLAPI BOOL CALLTYPE LPNR_GetPlateNumber(HANDLE h, char *buf)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;
	TRACE_LOG(pHvObj, "��LPNR_GetPlateNumber��- Ӧ�ó����ȡ���ƺ��� (%s)\n", pHvObj->strPlate);
	// NOTE:
	// let API user program decide to lock or not to prevent dead lock (User invoke LPNR_Lock then call this function will cause dead lock in Linux 
	// as we dont use recusive mutex. application try to lock a mutex which already owned by itself will cause self-deadlock.
	// same reasone for other functions that comment out the Mutex_Lock/Mutex_Unlock
	// It is OK in Windows as Windows's mutex is recursively.
	//Mutex_Lock(pHvObj);		
	strcpy(buf, pHvObj->strPlate );
	//Mutex_Unlock(pHvObj);
	return TRUE;
}

DLLAPI BOOL CALLTYPE LPNR_GetPlateAttribute(HANDLE h, BYTE *attr)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;
	TRACE_LOG(pHvObj, "��LPNR_GetPlateAttribute��- Ӧ�ó����ȡ���ƺ��� (%s)\n", pHvObj->strPlate);
	memcpy(attr, pHvObj->plateInfo.MatchRate, sizeof(pHvObj->plateInfo.MatchRate));
	return TRUE;
}

DLLAPI int CALLTYPE LPNR_GetPlateColorImage(HANDLE h, char *buf)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;
	TRACE_LOG(pHvObj,"��LPNR_GetPlateColorImage��- Ӧ�ó����ȡ���Ʋ�ɫͼƬ size=%d\n", pHvObj->szSmallImage);
	//Mutex_Lock(pHvObj);
	if ( pHvObj->szSmallImage > 0 )
		memcpy(buf, pHvObj->pSmallImage, pHvObj->szSmallImage );
	//Mutex_Unlock(pHvObj);
	return pHvObj->szSmallImage;
}

DLLAPI int CALLTYPE LPNR_GetPlateBinaryImage(HANDLE h, char *buf)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;
	TRACE_LOG(pHvObj,"��LPNR_GetPlateBinaryImage��- Ӧ�ó����ȡ���ƶ�ֵͼƬ size=%d\n", pHvObj->szBinImage);
	//Mutex_Lock(pHvObj);
	if ( pHvObj->szBinImage > 0 )
		memcpy(buf, pHvObj->pBinImage, pHvObj->szBinImage );
	//Mutex_Unlock(pHvObj);
	return pHvObj->szBinImage;
}

DLLAPI int CALLTYPE LPNR_GetCapturedImage(HANDLE h, char *buf)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	if ( !IS_VALID_OBJ(pHvObj) )
		return 0;
	TRACE_LOG(pHvObj,"��LPNR_GetCapturedImage��- Ӧ�ó����ȡץ��ͼƬ size=%d\n", pHvObj->szBigImage );
	//Mutex_Lock(pHvObj);
	if ( pHvObj->szBigImage > 0 )
		memcpy(buf, pHvObj->pBigImage, pHvObj->szBigImage );
	//Mutex_Unlock(pHvObj);
	return pHvObj->szBigImage;
}

DLLAPI int CALLTYPE LPNR_GetHeadImage(HANDLE h, char *buf)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	if ( !IS_VALID_OBJ(pHvObj) )
		return 0;
	TRACE_LOG(pHvObj,"��LPNR_GetHeadImage��- Ӧ�ó����ȡ��ͷͼƬ size=%d\n", pHvObj->szHeadImage );
	//Mutex_Lock(pHvObj);
	if ( pHvObj->szHeadImage > 0 )
		memcpy(buf, pHvObj->pHeadImage, pHvObj->szHeadImage );
	//Mutex_Unlock(pHvObj);
	return pHvObj->szHeadImage;
}

DLLAPI int CALLTYPE LPNR_GetMiddleImage(HANDLE h, char *buf)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	if ( !IS_VALID_OBJ(pHvObj) )
		return 0;
	TRACE_LOG(pHvObj,"��LPNR_GetMiddleImage��- Ӧ�ó����ȡ4/9ͼƬ size=%d\n", pHvObj->szT3DImage );
	//Mutex_Lock(pHvObj);
	if ( pHvObj->szT3DImage > 0 )
		memcpy(buf, pHvObj->pT3DImage, pHvObj->szT3DImage );
	//Mutex_Unlock(pHvObj);
	return pHvObj->szT3DImage;
}

DLLAPI int CALLTYPE LPNR_GetQuadImage(HANDLE h, char *buf)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	if ( !IS_VALID_OBJ(pHvObj) )
		return 0;
	TRACE_LOG(pHvObj,"��LPNR_GetQuadImage��- Ӧ�ó����ȡ1/4ͼƬ size=%d\n", pHvObj->szQuadImage );
	//Mutex_Lock(pHvObj);
	if ( pHvObj->szQuadImage > 0 )
		memcpy(buf, pHvObj->pQuadImage, pHvObj->szQuadImage );
	//Mutex_Unlock(pHvObj);
	return pHvObj->szQuadImage;
}

DLLAPI int CALLTYPE LPNR_GetLiveFrame(HANDLE h, char *buf)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	if ( !IS_VALID_OBJ(pHvObj) )
		return 0;
	//Mutex_Lock(pHvObj);
	if ( pHvObj->nLiveSize > 0 )
		memcpy(buf, pHvObj->pLiveImage, pHvObj->nLiveSize );
	//Mutex_Unlock(pHvObj);
	return pHvObj->nLiveSize;
}

DLLAPI int CALLTYPE LPNR_GetLiveFrameSize(HANDLE h)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	if ( !IS_VALID_OBJ(pHvObj) )
		return 0;
	return pHvObj->nLiveSize;
}

DLLAPI BOOL CALLTYPE LPNR_IsOnline(HANDLE h)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;
	//TRACE_LOG(pHvObj,"��LPNR_IsOnline�� - %s\n", pHvObj->enLink == NORMAL ? "yes" : "no");
	return pHvObj->enLink == NORMAL;
}

DLLAPI int CALLTYPE LPNR_GetPlateColorImageSize(HANDLE h)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	if ( !IS_VALID_OBJ(pHvObj) )
		return 0;
	TRACE_LOG(pHvObj,"��LPNR_GetPlateColorImageSize��- return %d\n", pHvObj->szSmallImage);
	return pHvObj->szSmallImage;
}

DLLAPI int CALLTYPE LPNR_GetPlateBinaryImageSize(HANDLE h)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	if ( !IS_VALID_OBJ(pHvObj) )
		return 0;

	TRACE_LOG(pHvObj, "��LPNR_GetPlateBinaryImageSize�� - return %d\n",  pHvObj->szBinImage);
	return pHvObj->szBinImage;
}

DLLAPI int CALLTYPE LPNR_GetCapturedImageSize(HANDLE h)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	if ( !IS_VALID_OBJ(pHvObj) )
		return 0;
	TRACE_LOG(pHvObj, "��LPNR_GetCapturedImageSize�� - return %d\n", pHvObj->szBigImage);
	return pHvObj->szBigImage;
}

DLLAPI int CALLTYPE LPNR_GetHeadImageSize(HANDLE h)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	if ( !IS_VALID_OBJ(pHvObj) )
		return 0;
	TRACE_LOG(pHvObj, "��LPNR_GetHeadImageSize�� - return %d\n", pHvObj->szHeadImage);
	return pHvObj->szHeadImage;
}

DLLAPI int CALLTYPE LPNR_GetMiddleImageSize(HANDLE h)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	if ( !IS_VALID_OBJ(pHvObj) )
		return 0;
	TRACE_LOG(pHvObj, "��LPNR_GetMiddleImageSize�� - return %d\n", pHvObj->szT3DImage);
	return pHvObj->szT3DImage;
}

DLLAPI int CALLTYPE LPNR_GetQuadImageSize(HANDLE h)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	if ( !IS_VALID_OBJ(pHvObj) )
		return 0;
	TRACE_LOG(pHvObj, "��LPNR_GetQuadImageSize�� - return %d\n", pHvObj->szQuadImage);
	return pHvObj->szQuadImage;
}

DLLAPI BOOL CALLTYPE LPNR_ReleaseData(HANDLE h)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;
	TRACE_LOG(pHvObj,"��LPNR_ReleaseData��\n" );
	if ( pHvObj->enOper != OP_IDLE )
	{
		TRACE_LOG(pHvObj,"\t - ��ǰ������δ��ɣ��������ͷ�����\n");
		return FALSE;
	}
	ReleaseData(pHvObj);
	return TRUE;
}

DLLAPI BOOL CALLTYPE LPNR_SoftTrigger(HANDLE h)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	DataHeader header;
	
	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;
	TRACE_LOG(pHvObj,"��LPNR_SoftTrigger��\n");
	if ( pHvObj->enLink != NORMAL )
	{
		TRACE_LOG(pHvObj,"\t - ץ�Ļ�����!\n");
		return FALSE;
	}
	if ( pHvObj->enOper != OP_IDLE )
	{
		TRACE_LOG(pHvObj,"\t - ץ�Ļ�æ����ǰ������δ���!\n");
		return FALSE;
	}
	CTRL_HEADER_INIT( header, CTRL_TRIGGER, 0 );
	sock_write( pHvObj->sock, (const char*)&header, sizeof(header) );
	return TRUE;
}


DLLAPI BOOL CALLTYPE LPNR_SoftTriggerEx(HANDLE h, int Id)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	DataHeader header;
	
	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;

	TRACE_LOG(pHvObj,"[LPNR_SoftTriggerEx]- Id=%d\n", Id);
/*	
	if ( pHvObj->enLink != NORMAL )
	{
		MTRACE_LOG(pHvObj->hLog,"\t - Camera offline!\n");
		return FALSE;
	}
	if ( pHvObj->enOper != OP_IDLE )
	{
		MTRACE_LOG(pHvObj->hLog,"\t - Camera busy, current recognition has not finished yet!\n");
		return FALSE;
	}
*/	
	CTRL_HEADER_INIT( header, CTRL_TRIGGER, Id );
	sock_write( pHvObj->sock, (const char*)&header, sizeof(header) );
	return TRUE;
}

DLLAPI BOOL CALLTYPE LPNR_IsIdle(HANDLE h)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	
	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;
//	TRACE_LOG(pHvObj,"��LPNR_IsIdle��\n");
	if ( pHvObj->enLink != NORMAL )
	{
		TRACE_LOG(pHvObj,"\t - ץ�Ļ�����!\n");
		return FALSE;
	}
	return ( pHvObj->enOper == OP_IDLE );
}

DLLAPI BOOL CALLTYPE LPNR_GetTiming(HANDLE h, int *elaped, int *processed )
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	
	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;
	if ( pHvObj->enLink != NORMAL )
	{
		TRACE_LOG(pHvObj,"\t - ץ�Ļ�����!\n");
		return FALSE;
	}
	*elaped = pHvObj->elapsed_time;
	*processed = pHvObj->process_time;
	return TRUE;
}

DLLAPI BOOL CALLTYPE LPNR_EnableLiveFrame(HANDLE h, int nSizeCode)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	DataHeader header;
	
	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;
	TRACE_LOG(pHvObj,"��LPNR_EnableLiveFrame(%d)��\n", nSizeCode );
	if ( pHvObj->enLink != NORMAL )
	{
		TRACE_LOG(pHvObj,"\t - ץ�Ļ�����!\n");
		return FALSE;
	}
	CTRL_HEADER_INIT( header, CTRL_LIVEFEED, nSizeCode );
	sock_write( pHvObj->sock, (const char*)&header, sizeof(header) );
	return TRUE;
}

DLLAPI BOOL CALLTYPE LPNR_TakeSnapFrame(HANDLE h, BOOL bFlashLight)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	DataHeader header;
	
	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;
	if ( pHvObj->enLink != NORMAL )
	{
		TRACE_LOG(pHvObj,"LPNR_TakeSnapFrame-  ʧ�ܣ�ʶ�������!\n");
		return FALSE;
	}
	if ( pHvObj->enOper != OP_IDLE )
	{
		TRACE_LOG(pHvObj,"LPNR_TakeSnapFrame- ʶ������ڴ���ʶ���У�������ץ��ͼƬ����\n");
		return FALSE;
	}
	// �������
	TRACE_LOG(pHvObj,"LPNR_TakeSnapFrame- ����ʶ���ץ�Ĳ���ȡͼ��\n");
	ReleaseData(pHvObj);
	CTRL_HEADER_INIT(header, CTRL_SNAPCAP, bFlashLight ? 1 : 0 );
	sock_write( pHvObj->sock, (char *)&header, sizeof(header) );
/*
	for(i=0; i<100 && pHvObj->szBigImage==0; i++)
		Sleep(10);
	if ( pHvObj->szBigImage==0 )
	{
		TRACE_LOG(pHvObj,"\t--> ʶ�����ʱû�з���ץ��ͼ��\n" );
		return FALSE;
	}	
*/
	return TRUE;
}

DLLAPI BOOL CALLTYPE LPNR_Lock(HANDLE h)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;
	//TRACE_LOG(pHvObj,"��LPNR_Lock��\n");
	Mutex_Lock(pHvObj);
	return TRUE;
}

DLLAPI BOOL CALLTYPE LPNR_Unlock(HANDLE h)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;
	//TRACE_LOG(pHvObj, "��LPNR_Unlock��\n");
	Mutex_Unlock(pHvObj);
	return TRUE;
}

DLLAPI BOOL CALLTYPE LPNR_SyncTime(HANDLE h)
{
	DataHeader header;
#ifdef WIN32
	SYSTEMTIME tm;
#else
	struct tm *ptm;
	struct timeval tv;
#endif
	PHVOBJ pHvObj = (PHVOBJ)h;

	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;
	if ( pHvObj->enLink != NORMAL )
	{
		TRACE_LOG(pHvObj,"LPNR_SyncTime - ʧ�ܣ�ʶ�������!\n");
		return FALSE;
	}
	// set camera system time
#ifdef WIN32
	// set camera system time
	GetLocalTime( &tm );
	TIME_HEADER_INIT( header, tm.wYear, tm.wMonth, tm.wDay, tm.wHour, tm.wMinute, tm.wSecond, tm.wMilliseconds );
#else
	gettimeofday( &tv, NULL );
	// set camera system time
	ptm = localtime(&tv.tv_sec);
	TIME_HEADER_INIT( header, ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday,
						 ptm->tm_hour, ptm->tm_min, ptm->tm_sec, tv.tv_usec/1000 );
#endif
	sock_write( pHvObj->sock, (char *)&header, sizeof(header) );	
	return TRUE;
}

DLLAPI BOOL CALLTYPE LPNR_ResetHeartBeat(HANDLE h)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;

	TRACE_LOG(pHvObj, "[LPNR_ResetHeartBeat]\n");
	pHvObj->tickLastHeared = 0;
	return TRUE;
}

DLLAPI BOOL CALLTYPE LPNR_GetMachineIP(HANDLE h, LPSTR strIP)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;
	strcpy( strIP, pHvObj->strIP	);
	return TRUE;
}

DLLAPI BOOL CALLTYPE LPNR_GetExtDIO(HANDLE h, short dio_val[])
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;
	dio_val[0] = pHvObj->dio_val[0];
	dio_val[1] = pHvObj->dio_val[1];
	return TRUE;
}

DLLAPI LPCTSTR CALLTYPE LPNR_GetCameraLabel(HANDLE h)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	if ( !IS_VALID_OBJ(pHvObj) )
			return "";
	TRACE_LOG(pHvObj, "��LPNR_GetCameraLabel��- return '%s'\n", pHvObj->label );
	return (LPCTSTR)pHvObj->label;
}

DLLAPI BOOL CALLTYPE LPNR_GetModelAndSensor(HANDLE h, int *modelId, int *sensorId)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;

	*modelId = pHvObj->verAttr.param[1] >> 16;
	*sensorId = pHvObj->verAttr.param[1] & 0x0000ffff;
	return TRUE;
}
DLLAPI BOOL CALLTYPE LPNR_GetVersion(HANDLE h, DWORD *version, int *algver)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;

	*version = pHvObj->verAttr.param[0];
	*algver = pHvObj->verAttr.algver;
	return TRUE;
}

DLLAPI BOOL CALLTYPE LPNR_GetCapability(HANDLE h, DWORD *cap)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;

	*cap = pHvObj->verAttr.param[2];
	return TRUE;
}


DLLAPI BOOL CALLTYPE LPNR_QueryPlate( IN LPCTSTR strIP, OUT LPSTR strPlate, int tout )
{
#if 0
	DataHeader header, rplyHeader;
	int len;
	BOOL bFound = FALSE;
	SOCKET sock;
	DWORD dwIP;

	TRACE_LOG(NULL, "��LPNR_QueryPlate(%s,...)��\n", strIP);
	if ( (dwIP=inet_addr(strIP)) == INADDR_NONE )
	{
		TRACE_LOG(NULL, "==> IP��ַ����!\n");
		return FALSE;
	}
	sock = sock_udp_open();
	if ( sock == INVALID_SOCKET )
	{
		TRACE_LOG(NULL, "--> UDP socket open error -- %d\n", WSAGetLastError() );
		return FALSE;
	}
	//sock_udp_timeout( sock, tout );
	QUERY_HEADER_INIT( header, QID_PKPLATE );
	len = sock_udp_send0( sock, dwIP, PORT_SEARCH, (char *)&header, sizeof(DataHeader) );
	if ( sock_dataready(sock, tout) &&
	    (len=sock_udp_recv( sock, (char *)&rplyHeader, sizeof(DataHeader), &dwIP)) == sizeof(DataHeader) )
	{
		if ( rplyHeader.DataType == DTYP_REPLY && rplyHeader.DataId == QID_PKPLATE )
		{
			strcpy(strPlate, rplyHeader.plateInfo.chNum);
			bFound = TRUE;
		}
		else
		{
			TRACE_LOG(NULL, "��λ���Ӧ��֡����(0x%x)����ID(%d)����\n", rplyHeader.DataType, rplyHeader.DataId);
		}
	}
	else
	{
		TRACE_LOG(NULL, "--> ��λ���Ӧ��ʱ!\n");
	}

	sock_close( sock );
	return bFound;
#endif
}

#if defined _WIN32 || defined _WIN64
DLLAPI BOOL CALLTYPE LPNR_WinsockStart( BOOL bStart )
{
	if ( bStart )
		WINSOCK_START();
	else
		WINSOCK_STOP();
	return TRUE;		// ������Զ��ɹ�
}
#endif

DLLAPI BOOL CALLTYPE LPNR_SetExtDO(HANDLE h, int pin, int value)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	DataHeader ctrlHdr;
	int len;

	if ( !IS_VALID_OBJ(pHvObj) ||  pin < 0 || pin >= 4 )
		return FALSE;

	CTRL_HEADER_INIT( ctrlHdr, CTRL_WRITEEXTDO, 1);
	ctrlHdr.ctrlAttr.option[0] = pin;
	ctrlHdr.ctrlAttr.option[1] = value;
	pHvObj->acked_ctrl_id = 0;
	len = sock_write( pHvObj->sock, (const char*)&ctrlHdr, sizeof(ctrlHdr) );
	return len==sizeof(DataHeader);
}

DLLAPI BOOL CALLTYPE LPNR_PulseOut(HANDLE h, int pin, int period, int count)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	DataHeader ctrlHdr;
	int len, param;

	if ( !IS_VALID_OBJ(pHvObj) ||  pin < 0 || pin >= 4 )
		return FALSE;

	param = pin + ((count & 0xff) << 8) + ((period & 0xffff) << 16);
	pHvObj->acked_ctrl_id = 0;
	CTRL_HEADER_INIT( ctrlHdr, CTRL_DOPULSE, param);
	len = sock_write( pHvObj->sock, (const char*)&ctrlHdr, sizeof(ctrlHdr) );
	return len==sizeof(DataHeader);
}

DLLAPI BOOL CALLTYPE LPNR_LightCtrl(HANDLE h, int onoff, int msec)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	DataHeader ctrlHdr;
	int len;

	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;

	CTRL_HEADER_INIT( ctrlHdr, CTRL_LIGHT, onoff);
	memcpy(ctrlHdr.ctrlAttr.option, &msec, 4);
	len = sock_write( pHvObj->sock, (const char*)&ctrlHdr, sizeof(ctrlHdr) );
	return len==sizeof(DataHeader);
}

DLLAPI BOOL CALLTYPE LPNR_IRCut(HANDLE h, int onoff)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	DataHeader ctrlHdr;
	int len;

	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;

	CTRL_HEADER_INIT( ctrlHdr, CTRL_IRCUT, onoff);
	len = sock_write( pHvObj->sock, (const char*)&ctrlHdr, sizeof(ctrlHdr) );
	return len==sizeof(DataHeader);
}

DLLAPI BOOL CALLTYPE LPNR_SetOSDTimeStamp(HANDLE h, BOOL bEnable, int x, int y)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	DataHeader header;
	int len;

	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;

	if ( bEnable )
		pHvObj->osdConf.enabler |= OSD_EN_TIMESTAMP;
	else
		pHvObj->osdConf.enabler &= ~OSD_EN_TIMESTAMP;
	OSD_HEADER_INIT(header, &pHvObj->osdConf);
	header.size = sizeof(OSDPayload);
	if ( x!=0 && y!=0 )
		pHvObj->osdPayload.enabler |= OSD_EN_TIMESTAMP;
	else
		pHvObj->osdPayload.enabler &= ~OSD_EN_TIMESTAMP;
	pHvObj->osdPayload.x[OSDID_TIMESTAMP] = x;
	pHvObj->osdPayload.y[OSDID_TIMESTAMP] = y;
	len = sock_write( pHvObj->sock, (const char*)&header, sizeof(header) );
	len += sock_write( pHvObj->sock, (const char*)&pHvObj->osdPayload, sizeof(OSDPayload) );
	return len==sizeof(DataHeader) + sizeof(OSDPayload);
}

DLLAPI BOOL CALLTYPE LPNR_SetOSDLabel(HANDLE h, BOOL bEnable, const char *label)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	DataHeader header;
	int len;

	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;

	if ( label != NULL )
	{
		memset( &header, 0, sizeof(header) );
		header.DataType = DTYP_EXCONF;
		header.DataId = 0;
		memcpy(pHvObj->extParamConf.label, label, SZ_LABEL);
		pHvObj->extParamConf.label[SZ_LABEL-1] = '\0';
		memcpy( &header.extParamConf, &pHvObj->extParamConf, sizeof(ExtParamConf) );
		sock_write (pHvObj->sock, (char *)&header, sizeof(header) );
	}

	if ( bEnable )
		pHvObj->osdConf.enabler |= OSD_EN_LABEL;
	else
		pHvObj->osdConf.enabler &= ~OSD_EN_LABEL;
	OSD_HEADER_INIT(header, &pHvObj->osdConf);
	len = sock_write( pHvObj->sock, (const char*)&header, sizeof(header) );

	return len > 0;
}

DLLAPI BOOL CALLTYPE LPNR_SetOSDLogo(HANDLE h, BOOL bEnable)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	DataHeader header;
	int len;

	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;

	if ( bEnable )
		pHvObj->osdConf.enabler |= OSD_EN_LOGO;
	else
		pHvObj->osdConf.enabler &= ~OSD_EN_LOGO;
	OSD_HEADER_INIT(header, &pHvObj->osdConf);
	len = sock_write( pHvObj->sock, (const char*)&header, sizeof(header) );
	return len == sizeof(header);
}

DLLAPI BOOL CALLTYPE LPNR_SetOSDROI(HANDLE h, BOOL bEnable)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	DataHeader header;
	int len;

	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;

	if ( bEnable )
		pHvObj->osdConf.enabler |= OSD_EN_ROI;
	else
		pHvObj->osdConf.enabler &= ~OSD_EN_ROI;
	OSD_HEADER_INIT(header, &pHvObj->osdConf);
	len = sock_write( pHvObj->sock, (const char*)&header, sizeof(header) );
	return len == sizeof(header);
}

DLLAPI BOOL CALLTYPE LPNR_SetOSDPlate(HANDLE h, BOOL bEnable, int loc, int dwell, BOOL bFadeOut)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	DataHeader header;
	int len;

	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;
	
	if ( bEnable )
	{
		pHvObj->osdConf.enabler |= OSD_EN_PLATE;
		if ( loc != 1 && loc != 2 && loc != 3 ) loc = 2;		// Ĭ��������
		pHvObj->osdConf.x = -loc - 6;		// 1~3 --> -7 ~ -9
		pHvObj->osdConf.param[OSD_PARM_DWELL] = dwell;
		pHvObj->osdConf.param[OSD_PARM_FADEOUT] = bFadeOut ? 1 : 0;
	}
	else
		pHvObj->osdConf.enabler &= ~OSD_EN_PLATE;
	OSD_HEADER_INIT(header, &pHvObj->osdConf);
	len = sock_write( pHvObj->sock, (const char*)&header, sizeof(header) );
	return len == sizeof(header);
}

DLLAPI BOOL CALLTYPE LPNR_COM_init(HANDLE h, int Speed)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	DataHeader ctrlHdr;
	int len;

	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;

	CTRL_HEADER_INIT( ctrlHdr, CTRL_COMSET, Speed);
	len = sock_write( pHvObj->sock, (const char*)&ctrlHdr, sizeof(ctrlHdr) );
	return len==sizeof(DataHeader);
}

DLLAPI BOOL CALLTYPE LPNR_UserOSDOn(HANDLE h, int x, int y, int align, int fontsz, int text_color, int opacity, const char *text)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	DataHeader header;
	int len;

	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;

	OSDTEXT_HEADER_INIT(header, NULL);
	header.osdText.x  = x;
	header.osdText.y  = y;
	header.osdText.nFontId = 0;		// ����
	if ( fontsz==24 || fontsz==32 || fontsz==40 || fontsz==48 || fontsz==56 )
		header.osdText.nFontSize = fontsz;
	else
		header.osdText.nFontSize = 32;
	header.osdText.RGBForgrund = text_color;
	header.osdText.alpha[0] = opacity * 128 / 100;		// 0~100 --> 0 ~ 128
	header.osdText.alpha[1] = 0;
	if ( 0 < align && align < 4 )
		header.osdText.param[OSD_PARM_ALIGN] = align - 1;		// 1 ~ 3 --> 0 ~ 2
	header.osdText.param[OSD_PARM_SCALE] = 1;
	header.size = strlen(text) + 1;
	len = sock_write( pHvObj->sock, (const char *)&header, sizeof(header) );
	len += sock_write(pHvObj->sock, text, strlen(text)+1);
	return len == sizeof(header) + header.size;
}

DLLAPI BOOL CALLTYPE LPNR_UserOSDOff(HANDLE h)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	DataHeader ctrlHdr;

	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;

	OSDTEXT_HEADER_INIT(ctrlHdr, NULL);
	sock_write( pHvObj->sock, (const char *)&ctrlHdr, sizeof(ctrlHdr) );
	return TRUE;
}

DLLAPI BOOL CALLTYPE LPNR_SetStream(HANDLE h,  int encoder, BOOL bSmallMajor, BOOL bSmallMinor)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	DataHeader header;
	int  nStreamSize = 0;
	int  len;

	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;

	if ( bSmallMinor )
		nStreamSize = 1;
	if ( bSmallMajor )
		nStreamSize |= 0x02;

	pHvObj->h264Conf.u8Param[1] = nStreamSize;
	pHvObj->h264Conf.u8Param[2] = encoder;

	H264_HEADER_INIT( header, pHvObj->h264Conf);
	len = sock_write( pHvObj->sock, (const char *)&header, sizeof(header) );
	return len==sizeof(header);
}

DLLAPI BOOL CALLTYPE LPNR_SetCaptureImage(HANDLE h, BOOL bDisFull, BOOL bEnMidlle, BOOL bEnSmall, BOOL bEnHead)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	DataHeader header;
	int len;

	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;

	if (bDisFull)
		pHvObj->paramConf.enabler |= PARM_DE_FULLIMG;
	else
		pHvObj->paramConf.enabler &= ~PARM_DE_FULLIMG;

	if ( bEnMidlle )
			pHvObj->paramConf.enabler |= PARM_EN_T3RDIMG;
	else
			pHvObj->paramConf.enabler &= ~PARM_EN_T3RDIMG;

	if ( bEnHead )
		pHvObj->paramConf.enabler  |= PARM_EN_HEADIMG;
	else
		pHvObj->paramConf.enabler  &= ~PARM_EN_HEADIMG;

	if ( bEnSmall )
			pHvObj->paramConf.enabler |= PARM_EN_QUADIMG;
	else
			pHvObj->paramConf.enabler &= ~PARM_EN_QUADIMG;

	memset( &header, 0, sizeof(header) );
	header.DataType = DTYP_CONF;
	header.DataId = 0;
	memcpy( &header.paramConf, &pHvObj->paramConf, sizeof(ParamConf) );
	len = sock_write (pHvObj->sock, (char *)&header, sizeof(header) );

	return len==sizeof(header);
}

DLLAPI BOOL CALLTYPE LPNR_COM_aync(HANDLE h, BOOL bEnable)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	DataHeader ctrlHdr;
	int len, param = bEnable ? 1 : 0;

	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;

	CTRL_HEADER_INIT( ctrlHdr, CTRL_COMASYN, param);
	len = sock_write( pHvObj->sock, (const char*)&ctrlHdr, sizeof(ctrlHdr) );
	return len==sizeof(DataHeader);
}

DLLAPI BOOL CALLTYPE LPNR_COM_send(HANDLE h, const char *data, int size)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	DataHeader ctrlHdr;
	int len;

	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;

	CTRL_HEADER_INIT( ctrlHdr, CTRL_COMTX, 0);
	ctrlHdr.size = size;
	len = sock_write( pHvObj->sock, (const char*)&ctrlHdr, sizeof(ctrlHdr) );
	len += sock_write(pHvObj->sock, data, size);
	return len==sizeof(DataHeader);
}

DLLAPI int CALLTYPE LPNR_COM_iqueue(HANDLE h)
{
	PHVOBJ pHvObj = (PHVOBJ)h;

	if ( !IS_VALID_OBJ(pHvObj) )
		return 0;
	return RingElements(pHvObj);
}

DLLAPI int CALLTYPE LPNR_COM_peek(HANDLE h, char *RxData, int size)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	int  nb, i, pos;

	if ( !IS_VALID_OBJ(pHvObj) )
		return 0;

	Ring_Lock(pHvObj);
	nb = RingElements(pHvObj);
	if ( size > nb )
		size = nb;
	pos = pHvObj->ring_head;
	for(i=0; i<nb; i++)
	{
		RxData[i] = pHvObj->ring_buf[pos];
		pos = NextPosit(pHvObj,pos);
	}
	Ring_Unlock(pHvObj);
	return nb;
}

DLLAPI int CALLTYPE LPNR_COM_read(HANDLE h, char *RxData, int size)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	int  nb, i;

	if ( !IS_VALID_OBJ(pHvObj) )
		return 0;

	Ring_Lock(pHvObj);
	nb = RingElements(pHvObj);
	if ( size > nb )
		size = nb;
	for(i=0; i<nb; i++)
	{
		RxData[i] = pHvObj->ring_buf[pHvObj->ring_head];
		pHvObj->ring_head = NextHeadPosit(pHvObj);
	}
	Ring_Unlock(pHvObj);
	return nb;
}

DLLAPI int CALLTYPE LPNR_COM_remove(HANDLE h, int size)
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	int  nb, i;

	if ( !IS_VALID_OBJ(pHvObj) )
		return 0;

	Ring_Lock(pHvObj);
	nb = RingElements(pHvObj);
	if ( size > nb )
		size = nb;
	for(i=0; i<nb; i++)
		pHvObj->ring_head = NextHeadPosit(pHvObj);
	Ring_Unlock(pHvObj);
	return nb;
}

DLLAPI BOOL CALLTYPE LPNR_COM_clear(HANDLE h)
{
	PHVOBJ pHvObj = (PHVOBJ)h;

	if ( !IS_VALID_OBJ(pHvObj) )
		return FALSE;

	Ring_Lock(pHvObj);
	pHvObj->ring_head = pHvObj->ring_tail = 0;
	Ring_Unlock(pHvObj);
	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// WORKING THREAD
#define IMG_BUFSIZE	(1920*1080*3)		// 6MB
#ifdef linux
void *lpnr_workthread_fxc(void *lpParameter)
#else
DWORD WINAPI lpnr_workthread_fxc(PVOID lpParameter)
#endif
{
	PHVOBJ pHvObj = (PHVOBJ) lpParameter;
	PVOID payload = malloc( IMG_BUFSIZE );
	int	   rlen;
	long   imgWidth, imgHeight, imgType;
	fd_set set;
	struct timeval val;
	int ret;
	DataHeader  dataHeader;
	DataHeader header;
	SOCKET m_sock = INVALID_SOCKET;
	BYTE soh[3] = { 0xaa, 0xbb, 0xcc };
	char *liveFrame = NULL;
	int	  nLiveSize = 0;
	int	  nLiveAlloc = 0;
	DWORD tickHeartBeat=0;
	DWORD tickStartProc = 0;

	TRACE_LOG(pHvObj,"<<< ===================   W O R K   T H R E A D   S T A R T   =====================>>>\n");
	pHvObj->status = STAT_RUN;
	pHvObj->tickLastHeared = 0;
	for(; pHvObj->status==STAT_RUN; )
	{
		if ( m_sock == INVALID_SOCKET )
		{
			TRACE_LOG(pHvObj,"reconnect camera IP %s port 6008...\n", pHvObj->strIP );
			pHvObj->enLink = RECONNECT;
			m_sock = sock_connect( pHvObj->strIP, 6008 );
			TRACE_LOG(pHvObj, "--> socket = 0x%x (%d)\n", m_sock, m_sock );
			if ( m_sock != INVALID_SOCKET )
			{
				DataHeader header;
				pHvObj->enLink = NORMAL;
				pHvObj->sock = m_sock;
				LPNR_SyncTime((HANDLE)pHvObj);
				// disable intermidiate files
				CTRL_HEADER_INIT( header, CTRL_IMLEVEL, 0  );
				sock_write( m_sock, (char *)&header, sizeof(header) );
				TRACE_LOG(pHvObj,"Connection established. download system time to camera.\n");
				NoticeHostEvent(pHvObj, EVT_ONLINE ); 
				pHvObj->tickLastHeared = 0;			// reset last heared time
				pHvObj->nLiveSize = 0;
			}
			else
			{
				Sleep(1000);
				continue;
			}
		}

		// check for sending heart-beat packet
		if ( GetTickCount() >= tickHeartBeat )
		{
			DataHeader hbeat;
			HBEAT_HEADER_INIT(hbeat);
			//TRACE_LOG(pHvObj,"send heartbeat packet to camera.\n");
			sock_write( m_sock, (const char *)&hbeat, sizeof(hbeat) );
			tickHeartBeat = GetTickCount() + 1000;
		}

		// check for input from camera
		FD_ZERO( &set );
		FD_SET( m_sock, &set );
		val.tv_sec = 0;
		val.tv_usec = 10000;
		ret = select( m_sock+1, &set, NULL, NULL, &val );
		if ( ret > 0 && FD_ISSET(m_sock, &set) )
		{
			int rc;
			if ( (rc=sock_read_n_bytes( m_sock, (char *)&dataHeader, sizeof(dataHeader) )) != sizeof(dataHeader) )
			{
				if ( rc<=0 )
				{
					TRACE_LOG(pHvObj,"Socket broken, close and reconnect...wsaerror=%d\n", WSAGetLastError() );
					sock_close( m_sock );
					m_sock = pHvObj->sock = INVALID_SOCKET;
					pHvObj->enLink = DISCONNECT;
					pHvObj->enOper = OP_IDLE;
					tickStartProc = 0;
					ReleaseData(pHvObj);
					NoticeHostEvent( pHvObj, EVT_OFFLINE );
					continue;
				}
				else
				{
					int nskip;
					TRACE_LOG(pHvObj, "��ȡ����֡ͷ��õĳ�����%d, ��Ҫ%d�ֽڡ�����ͬ������һ��֡ͷ��\n",
							rc, sizeof(dataHeader) );
					nskip = sock_drain_until( m_sock, soh, 3 );
					TRACE_LOG(pHvObj,"--> ���� %d �ֽ�!\n", nskip);
				}
			}
			pHvObj->tickLastHeared = GetTickCount();
			if ( !IsValidHeader(dataHeader) )
			{
				int nskip;
				TRACE_LOG(pHvObj,"Invalid packet header received 0x%08X, re-sync to next header\n", dataHeader.DataType );
				nskip = sock_drain_until( m_sock, soh, 3 );
				TRACE_LOG(pHvObj,"--> skip %d bytes\n", nskip);
				continue;
			}	
			// read payload after header (if any)
			if ( dataHeader.size > 0 )
			{
				if ( dataHeader.size > IMG_BUFSIZE )
				{
					int nskip;
					// image size over buffer size ignore
					TRACE_LOG(pHvObj, "payload size %d is way too large. re-sync to next header\n", dataHeader.size);
					nskip = sock_drain_until( m_sock, soh, 3 );
					TRACE_LOG(pHvObj, "--> skip %d bytes\n", nskip);
					continue;
				}
				if ( (rlen=sock_read_n_bytes( m_sock, payload, dataHeader.size )) != dataHeader.size  )
				{
					int nskip;
					TRACE_LOG(pHvObj, "read payload DataType=0x%x, DataId=%d, expect %d bytes, only get %d bytes. --> ignored!\n", 
							dataHeader.DataType, dataHeader.DataId, dataHeader.size, rlen );
					nskip = sock_drain_until( m_sock, soh, 3 );
					TRACE_LOG(pHvObj,"--> drop this packet, skip %d bytes\n", nskip);
					//sock_close( m_sock );
					//m_sock = pHvObj->sock = INVALID_SOCKET;
					//pHvObj->enOper = OP_IDLE;
					continue;
				}
			}
			// receive a image file (JPEG or BMP)
			if ( dataHeader.DataType == DTYP_IMAGE )
			{
				// acknowledge it
				ACK_HEADER_INIT( header );
				sock_write( m_sock, (const char *)&header, sizeof(header) );
				// then process
				imgWidth = dataHeader.imgAttr.width;
				imgHeight = dataHeader.imgAttr.height;
				imgType = dataHeader.imgAttr.format;
				// IMAGE is a live frame
				if (dataHeader.DataId == IID_LIVE )
				{
					Mutex_Lock(pHvObj);
					if ( dataHeader.size > pHvObj->nLiveAlloc )
					{
						pHvObj->nLiveAlloc = (dataHeader.size + 1023)/1024 * 10240;		// roundup to 10K boundry
						pHvObj->pLiveImage = realloc( pHvObj->pLiveImage, pHvObj->nLiveAlloc );
					}
					pHvObj->nLiveSize = dataHeader.size;
					memcpy( pHvObj->pLiveImage, payload, dataHeader.size );
					Mutex_Unlock(pHvObj);
					NoticeHostEvent(pHvObj, EVT_LIVE );
				}
				else // if (dataHeader.DataId != IID_LIVE ) - other output images
				{	
					// Processed image
					// printf("received processed image: Id=%d, size=%d\r\n", dataHeader.DataId, dataHeader.size );
					switch( dataHeader.DataId )
					{
					case IID_CAP:
						Mutex_Lock(pHvObj);
						pHvObj->szBigImage = dataHeader.size;
						pHvObj->pBigImage = malloc( pHvObj->szBigImage );
						memcpy( pHvObj->pBigImage, payload, pHvObj->szBigImage );
						Mutex_Unlock(pHvObj);
						TRACE_LOG(pHvObj,"==> ���յ�ץ�Ĵ�ͼ - %s (%d bytes)\n", dataHeader.imgAttr.basename, dataHeader.size);
						if ( strcmp(dataHeader.imgAttr.basename,"vsnap.jpg") == 0 )
							NoticeHostEvent(pHvObj, EVT_SNAP );
						break;
					case IID_HEAD:
						Mutex_Lock(pHvObj);
						pHvObj->szHeadImage = dataHeader.size;
						pHvObj->pHeadImage = malloc( pHvObj->szHeadImage );
						memcpy( pHvObj->pHeadImage, payload, pHvObj->szHeadImage );
						Mutex_Unlock(pHvObj);
						TRACE_LOG(pHvObj,"==> ���յ���ͷͼ (%d bytes)\n", dataHeader.size);
						break;
					case IID_T3RD:
						Mutex_Lock(pHvObj);
						pHvObj->szT3DImage = dataHeader.size;
						pHvObj->pT3DImage = malloc( pHvObj->szT3DImage );
						memcpy( pHvObj->pT3DImage, payload, pHvObj->szT3DImage );
						Mutex_Unlock(pHvObj);
						TRACE_LOG(pHvObj,"==> ���յ�4/9�����ͼ (%d bytes)\n", dataHeader.size);
						break;
					case IID_QUAD:
						Mutex_Lock(pHvObj);
						pHvObj->szQuadImage = dataHeader.size;
						pHvObj->pQuadImage = malloc( pHvObj->szQuadImage );
						memcpy( pHvObj->pQuadImage, payload, pHvObj->szQuadImage );
						Mutex_Unlock(pHvObj);
						TRACE_LOG(pHvObj,"==> ���յ�1/4�����ͼ (%d bytes)\n", dataHeader.size);
						break;
					case IID_PLRGB:
						Mutex_Lock(pHvObj);
						pHvObj->szSmallImage = dataHeader.size;
						pHvObj->pSmallImage = malloc( pHvObj->szSmallImage );
						memcpy( pHvObj->pSmallImage, payload, pHvObj->szSmallImage );
						Mutex_Unlock(pHvObj);
						TRACE_LOG(pHvObj, "==> ���յ����Ʋ�ɫͼ (%d bytes)\n", dataHeader.size);
						break;
					case IID_PLBIN:
						Mutex_Lock(pHvObj);
						pHvObj->szBinImage = dataHeader.size;
						pHvObj->pBinImage = malloc( pHvObj->szBinImage );
						memcpy( pHvObj->pBinImage, payload, pHvObj->szBinImage );
						Mutex_Unlock(pHvObj);
						TRACE_LOG(pHvObj, "==> ���յ����ƶ�ֵͼ (%d bytes)\n", dataHeader.size);
						break;
					default:
						// ignore for other images
						;
					}
				}  // else if not live image
			}
			else if ( dataHeader.DataType == DTYP_DATA )
			{
				switch (dataHeader.DataId )
				{
				case DID_BEGIN:
					ReleaseData(pHvObj);
					memset( &pHvObj->plateInfo, 0, sizeof(PlateInfo) );
					strcpy( pHvObj->strPlate, "  ���ƾ�ʶ" );
					pHvObj->enOper = OP_RREPORT;
					break;
				case DID_END:
					pHvObj->enOper = OP_IDLE;
					tickStartProc = 0;
					// �����������ƽ������ֵ
					// 0 Ϊ ���Ķ� 0~100
					// 1 Ϊ ��ͷ��β��Ϣ 1 ��ͷ��0xff ��β��0 δ֪
					// 2 Ϊ������ɫ���� ������ɿ���
					pHvObj->plateInfo.MatchRate[3] = (BYTE)dataHeader.dataAttr.val[2];	// ����
					pHvObj->plateInfo.MatchRate[4] = (BYTE)dataHeader.dataAttr.val[1];	// ����Դ
					TRACE_LOG(pHvObj, "==>ʶ����������ݽ������!\n");
					NoticeHostEvent(pHvObj, EVT_DONE );
					break;
				case DID_PLATE:
					Mutex_Lock(pHvObj);
					memcpy( &pHvObj->plateInfo, &dataHeader.plateInfo, sizeof(PlateInfo) );
					switch( pHvObj->plateInfo.plateCode & 0x00ff )
					{
					case PLC_BLUE:
						strcpy( pHvObj->strPlate, "��" );
						break;
					case PLC_YELLOW:
						strcpy( pHvObj->strPlate, "��" );
						break;
					case PLC_WHITE:
						strcpy( pHvObj->strPlate, "��" );
						break;
					case PLC_BLACK:
						strcpy( pHvObj->strPlate, "��" );
						break;
					case PLC_GREEN:
						strcpy( pHvObj->strPlate, "��" );
						break;
					case PLC_YELGREEN:
						strcpy( pHvObj->strPlate, "��" );
						break;
					default:
						strcpy( pHvObj->strPlate, "��" );
					}
					strcat( pHvObj->strPlate, pHvObj->plateInfo.chNum );
					TRACE_LOG(pHvObj, "==> ���յ�ʶ��ĳ��� ��%s��\n", pHvObj->strPlate );
					pHvObj->plateInfo.MatchRate[5] = (BYTE)pHvObj->plateInfo.plateCode & 0x00ff;	// ������ɫ����
					pHvObj->plateInfo.MatchRate[6] = (BYTE)((pHvObj->plateInfo.plateCode>>8) & 0xff);	// �������ʹ���
					Mutex_Unlock(pHvObj);
					break;
				case DID_TIMING:
					pHvObj->process_time = dataHeader.timeInfo.totalProcess;
					pHvObj->elapsed_time = dataHeader.timeInfo.totalElapsed;
					break;
				case DID_COMDATA:
					if ( dataHeader.size > 0 )
					{
						int  i=0;
						BYTE *rx_data = (BYTE *)payload;
						TRACE_LOG(pHvObj, "Receive COM port RX data (%d bytes) - save to ring buffer. Current ring elements=%d\n", dataHeader.size, RingElements(pHvObj));
						Ring_Lock(pHvObj);
						for( ; !IsRingFull(pHvObj) && dataHeader.size > 0; )
						{
							pHvObj->ring_buf[pHvObj->ring_tail] = rx_data[i++];
							pHvObj->ring_tail = NextTailPosit(pHvObj);
							dataHeader.size--;
						}
						Ring_Unlock(pHvObj);
					}
					break;
				case DID_EXTDIO:
					pHvObj->dio_val[0] = dataHeader.dataAttr.val[0] & 0xffff;
					pHvObj->dio_val[1] = dataHeader.dataAttr.val[1] & 0xffff;
					break;
				case DID_VERSION:
					memcpy(&pHvObj->verAttr, &dataHeader.verAttr, sizeof(VerAttr));
					NoticeHostEvent(pHvObj, EVT_VERINFO );
					break;
				case DID_CFGDATA:
					memcpy(&pHvObj->dataCfg, &dataHeader.dataAttr, sizeof(DataAttr));
					break;
				}	// switch (dataHeader.DataId )
			}	// else if ( dataHeader.DataType == DTYP_DATA )
			else if ( dataHeader.DataType == DTYP_EVENT )
			{
				switch(dataHeader.DataId)
				{
				case EID_TRIGGER:
					pHvObj->enOper = OP_PROCESS;
					tickStartProc = GetTickCount();
					pHvObj->enTriggerSource = (TRIG_SRC_E)dataHeader.evtAttr.param;
					TRACE_LOG(pHvObj, "==> ʶ�������ʶ�� (%s)����ʼ����!\n", GetTriggerSourceText(pHvObj->enTriggerSource) );
					NoticeHostEvent(pHvObj, EVT_FIRED );
					break;
				case EID_VLDIN:
					TRACE_LOG(pHvObj, "==> ��������������Ȧʶ����!\n");
					NoticeHostEvent(pHvObj, EVT_VLDIN );
					break;
				case EID_VLDOUT:
					TRACE_LOG(pHvObj, "==> �����뿪������Ȧʶ����!\n");
					NoticeHostEvent(pHvObj, EVT_VLDOUT );
					break;
				case EID_EXTDI:
					pHvObj->diParam = dataHeader.evtAttr.param;
					pHvObj->dio_val[0] = pHvObj->diParam & 0xffff;
					TRACE_LOG(pHvObj, "==> EXT DI ״̬�仯 0x%x -> 0x%x\n", 
							(pHvObj->diParam >> 16) & 0xffff, (pHvObj->diParam & 0xffff));
					NoticeHostEvent(pHvObj, EVT_EXTDI );
					break;
				}
			}
			else 	if ( dataHeader.DataType == DTYP_CONF )
			{
				memcpy(&pHvObj->paramConf, &dataHeader.paramConf, sizeof(ParamConf));
			}
			else 	if ( dataHeader.DataType == DTYP_EXCONF )
			{
				memcpy(&pHvObj->extParamConf, &dataHeader.extParamConf, sizeof(ExtParamConf));
				memcpy( pHvObj->label, &dataHeader.extParamConf.label, SZ_LABEL);
				pHvObj->label[SZ_LABEL-1] = '\0';
				TRACE_LOG(pHvObj, "�յ���չ���ò��� - label=%s\n", pHvObj->label);
			}
			else 	if ( dataHeader.DataType == DTYP_CONF )
			{
				memcpy(&pHvObj->paramConf, &dataHeader.paramConf, sizeof(ParamConf));
			}
			else if ( dataHeader.DataType == DTYP_H264CONF )
			{
				memcpy( &pHvObj->h264Conf, &dataHeader.h264Conf, sizeof(H264Conf) );
			}
			else if ( dataHeader.DataType == DTYP_OSDCONF )
			{
				memcpy( &pHvObj->osdConf, &dataHeader.osdConf, sizeof(OSDConf) );
				if ( dataHeader.size==sizeof(OSDPayload) )
					sock_read_n_bytes(m_sock, &pHvObj->osdPayload, sizeof(OSDPayload));
			}
			else if ( dataHeader.DataType==DTYP_TEXT &&  dataHeader.DataId==TID_PLATENUM )
			{
				// �յ���ǰ���͵ĳ��ƺ���
				char *ptr;
				// ������ƺ����к�׺ (��β)�����磺"����E12345(��β)"����Ҫ�Ѻ�׺����
				if ( (ptr=strchr(dataHeader.textAttr.text,'('))!=NULL)
				{
					*ptr = '\0';
					pHvObj->plateInfo.MatchRate[1] = 0xff;
				}
				strcpy(pHvObj->strPlate ,dataHeader.textAttr.text);
				NoticeHostEvent(pHvObj, EVT_PLATENUM );
				TRACE_LOG(pHvObj, "�յ���ǰ���͵ĳ��ƺ���: %s\n", pHvObj->strPlate);
			}
			else if ( dataHeader.DataType==DTYP_ACK && dataHeader.DataId!=0 )
			{
				// �յ�����Ӧ��֡
				pHvObj->acked_ctrl_id = dataHeader.DataId;
				NoticeHostEvent(pHvObj, EVT_ACK );
				TRACE_LOG(pHvObj, "�յ��������� %d��Ӧ��֡!\n", pHvObj->acked_ctrl_id);
			}
		}	// if ( ret > 0 && FD_ISSET(pHvObj->sock, &set) )
		if ( pHvObj->tickLastHeared && (GetTickCount() - pHvObj->tickLastHeared > 10000) )
		{
			// over 10 seconds not heared from LPNR camera. consider socket is broken 
			sock_close( m_sock );
			m_sock = pHvObj->sock = INVALID_SOCKET;
			pHvObj->enLink = DISCONNECT;
			pHvObj->nLiveSize = 0;
			ReleaseData(pHvObj);
			NoticeHostEvent(pHvObj, EVT_OFFLINE );
			TRACE_LOG(pHvObj,"Camera heart-beat not heared over 10 sec. reconnect.\n");
		}
		if ( pHvObj->enOper==OP_PROCESS && (GetTickCount() - tickStartProc) > 2000 )
		{
			TRACE_LOG(pHvObj,"LPNR ����ͼƬʱ�䳬��2�룬��λ״̬������!\n");
			pHvObj->enOper = OP_IDLE;
		}
	}
	pHvObj->status = STAT_EXIT;
	closesocket( m_sock );
	m_sock = pHvObj->sock = INVALID_SOCKET;
	pHvObj->enLink = DISCONNECT;
	pHvObj->hThread = 0;
	FREE_BUFFER(pHvObj->pLiveImage);
	ReleaseData(pHvObj);
	free(payload);
	TRACE_LOG(pHvObj,"-----------------    W O R K    T H R E A D   E X I T   ------------------\n");
	return 0;
 }
 
#if defined linux && defined D_ENABLE_LPNR_TESTCODE
#include <termios.h>

static struct termios tios_save;

static int ttysetraw(int fd)
{
	struct termios ttyios;
	if ( tcgetattr (fd, &tios_save) != 0 )
		return -1;
	memcpy(&ttyios, &tios_save, sizeof(ttyios) );
	ttyios.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP
                      |INLCR|IGNCR|ICRNL|IXON);
	ttyios.c_oflag &= ~(OPOST|OLCUC|ONLCR|OCRNL|ONOCR|ONLRET);
	ttyios.c_lflag &= ~(ECHO|ECHONL|ICANON|ISIG|IEXTEN);
	ttyios.c_cflag &= ~(CSIZE|PARENB);
	ttyios.c_cflag |= CS8;
	ttyios.c_cc[VMIN] = 1;
	ttyios.c_cc[VTIME] = 0;
	return tcsetattr (fd, TCSANOW, &ttyios);
}

static int ttyrestore(int fd)
{
	return tcsetattr(fd, TCSANOW, &tios_save);
}

int tty_ready( int fd )
{
	int	n=0;
 	fd_set	set;
	struct timeval tval = {0, 10000};		// 10 msec timed out

	FD_ZERO(& set);
	FD_SET(fd, &set);
	n = select( fd+1, &set, NULL, NULL, &tval );

	return n;
}

void lpnr_event_handle( HANDLE h, int evnt )
{
	PHVOBJ pHvObj = (PHVOBJ)h;
	int i, n, size;
	char *buf;
	char chIP[16];
	char rx_buf[1024];

	LPNR_GetMachineIP(h, chIP);
	switch (evnt)
	{
	case EVT_ONLINE:
		printf("LPNR camera (IP %s) goes ONLINE.\r\n", chIP );
		break;
	case EVT_OFFLINE:
		printf("LPNR camera IP %s goes OFFLINE.\r\n", pHvObj->strIP );
		break;
	case EVT_FIRED:
		printf("LPNR camera IP %s FIRED a image process.\r\n", chIP );
		break;
	case EVT_DONE:
		printf("LPNR camera IP %s processing DONE\r\n", pHvObj->strIP );
		size = LPNR_GetCapturedImageSize(h);
		if ( size == 0 ) size = 1024*1024;
		buf = malloc(size);
		LPNR_GetPlateNumber(h, buf);
		printf("\t plate number = %s\r\n", buf );
		n = LPNR_GetCapturedImage(h, buf );
		printf("\t captured image size is %d bytes.\r\n", n );
		n = LPNR_GetPlateColorImage(h, buf );
		printf("\t plate color image size is %d bytes.\r\n", n );
		n = LPNR_GetPlateBinaryImage(h, buf );
		printf("\t plate binary image size is %d bytes.\r\n", n );
		free( buf );
		LPNR_ReleaseData((HANDLE)h);
		break;
	case EVT_LIVE:
		LPNR_Lock(h);
		size = LPNR_GetLiveFrameSize(h);
		buf = malloc(size);
		LPNR_GetLiveFrame(h, buf);
		LPNR_Unlock(h);
		// do some thing for live frame (render on screen for example)., but do not do it in this thread context
		// let other render thread do the job. We don't want to block LPNR working thread for too long.
		printf("reveive a live frame size=%d bytes\r\n", size );
		free(buf);
		break;
	case EVT_SNAP:
		printf("snap image reveived, size=%d bytes\r\n", LPNR_GetCapturedImageSize(h));
		break;
	case EVT_ASYNRX:
		printf("Serial port data Rx (%d bytes) -- ", (n=LPNR_COM_iqueue(h)));
		LPNR_COM_read(h, rx_buf, sizeof(rx_buf));
		rx_buf[n] = '\0';
		printf("%s\r\n", rx_buf);
	}
}

/*
 *  after launched, user use keyboard to control the opeation. stdin is set as raw mode.
 *  therefore, any keystroke will be catched immediately without have to press <enter> key.
 *  'q' - quit
 *  'l' - soft trigger.
 */
int generate_random_string(char buf[], int size)
{
	int i;
	for(i=0; i<size; i++)
	{
		buf[i] = 32 + (random() % 95);
	}
	return size;
}

int main( int argc, char *const argv[] )
{
	int ch;
	BOOL bQuit = FALSE;
	BOOL bOnline = FALSE;
	int		 lighton = 0;
	int     IRon = 0;
	int		fd;
	HANDLE hLPNR;
	int    n, pin=0, value=0, period;
	char tx_buf[64];

	if ( argc != 2 )
	{
		fprintf(stderr, "USUAGE: %s <ip addr>\n", argv[0] );
		return -1;
	}
	fd = 0;		// stdin
	ttysetraw(fd);
	if ( (hLPNR=LPNR_Init(argv[1])) == NULL )
	{
		ttyrestore(fd);
		fprintf(stderr, "Invalid LPNR IP address: %s\r\n", argv[1] );
		return -1;
	}
	LPNR_SetCallBack( hLPNR, lpnr_event_handle );

	srandom(time(NULL));

	for(;!bQuit;)
	{
		BOOL bOn = LPNR_IsOnline(hLPNR);
		if ( bOn != bOnline )
		{
			bOnline = bOn;
			printf("camera become %s\r\n", bOn ? "online" : "offline" );
		}
		if ( tty_ready(fd) && read(fd, &ch, 1)==1 )
		{
			ch &= 0xff;
			switch (ch)
			{
			case '0':
			case '1':
			case '2':
				pin = ch - '0';
				printf("choose pin number %d\r\n", pin);
				break;
			case 'a':		// must issue 'i' command first (once only)
				LPNR_COM_aync(hLPNR,TRUE);
				printf("enable COM port async Rx\r\n");
				break;
			case 'A':
				LPNR_COM_aync(hLPNR,FALSE);
				printf("disable COM port async Rx\r\n");
				break;
			case 'l':
				printf("manual trigger a recognition...\r\n");
				LPNR_SoftTrigger( hLPNR );
				break;
			case 'd':	// disable live
				printf("disable live feed.\r\n");
				LPNR_EnableLiveFrame( hLPNR, FALSE );
				break;
			case 'e':	// enable live
				printf("enable live feed.\r\n");
				LPNR_EnableLiveFrame( hLPNR, TRUE );
				break;
			case 'f':		// 'f' - flash light
				lighton = 1 - lighton;
				printf("light control, on/off=%d\r\n", lighton);
				LPNR_LightCtrl(hLPNR, lighton, 0);
				break;
			case 'F':		// IR cut
				IRon = 1 - IRon;
				printf("IR-cut control, on/off=%d\r\n", IRon);
				LPNR_IRCut(hLPNR, IRon);
				break;
			case 'i':		// initial COM port
				LPNR_COM_init(hLPNR,9600);
				printf("initial transparent COM port!\r\n");
				break;
			case 'n':
				printf("camera name (label) = %s\r\n",  LPNR_GetCameraLabel(hLPNR));
				break;
			case 'o':		// Ext DO
				value = 1 - value;
				LPNR_SetExtDO(hLPNR, pin, value);
				printf("outpput DO pin=%d, value=%d\r\n", pin, value);
				break;
			case 'p':		// pulse
			case 'P':
				n = random() % 10 + 1;
				period = ch=='p' ? 250 : 500;
				printf("output pulse at %d msec period for %d times on pin %d.\r\n",  period, n,  pin);
				LPNR_PulseOut(hLPNR, pin, period, n);
				break;
			case 'q':
				printf("Quit test program...\r\n");
				bQuit = TRUE;
				break;
			case 's':
			case 'S':
				printf("take s snap frame...\r\n");
				LPNR_TakeSnapFrame(hLPNR,ch=='S');
				break;
			case 't':
				printf("time sync with camera.\r\n");
				LPNR_SyncTime(hLPNR);
				break;
			case 'x':
				// generate random string Tx to COM
				n = generate_random_string(tx_buf, random()%60+1);
				LPNR_COM_send(hLPNR, tx_buf, n);
				tx_buf[n] = '\0';
				printf("Tx %d bytes: %s\r\n",  n, tx_buf);
				break;
			case 'u':	// turn off user OSD
				LPNR_UserOSDOff(hLPNR);
				break;
			case 'U':	// turn on user OSD
				LPNR_UserOSDOn(hLPNR,-5,0,2,40,0xFFFF00,80,"Overlay Line 1\nMiddle Line\nLast Line is the longest");
				break;
			case 't':		 // turn off time stamp OSD
				LPNR_SetOSDTimeStamp(hLPNR,FALSE,0,0);
				break;
			case 'T':
				LPNR_SetOSDTimeStamp(hLPNR,TRUE,0,0);
				break;
			default:
				printf("ch=%c (%d), ignored.\r\n", ch, ch );
				break;
			}
		}
	}	
	printf("terminate working thread and destruct hLPNR...\r\n");
	LPNR_Terminate(hLPNR);
	ttyrestore(fd);
	return 0;
}

#endif


void demo_event_handle( HANDLE h, int evnt )
{
    PHVOBJ pHvObj = (PHVOBJ)h;
    char chIP[16];

	LPNR_GetMachineIP(h, chIP);
	switch (evnt)
	{
	case EVT_DONE:
		printf("LPNR camera IP %s processing DONE\r\n", pHvObj->strIP );
		printf("\t will to sync_packet .\r\n" );

        sync_packet(h);

		LPNR_ReleaseData((HANDLE)h);
		break;

    default:
        break;
	}
}