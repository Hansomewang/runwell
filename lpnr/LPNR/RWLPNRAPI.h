#ifndef _RWLPNRAPI_INCLDED_
#define _RWLPNRAPI_INCLDED_

#if defined _WIN32 || defined _WIN64
#ifdef BUILD_DLL
#define DLLAPI		__declspec(dllexport)
#define CALLTYPE	__stdcall
#else
#define DLLAPI
#define CALLTYPE
#endif
#else  // [linux]
#define TRUE		1
#define FALSE	0
#define INVALID_SOCKET	(~0)
#define SOCKET_ERROR    (-1)
#define DLLAPI
#define CALLTYPE
typedef int						BOOL;
typedef int						SOCKET;
typedef unsigned char BYTE;
typedef unsigned long	DWORD;
typedef void *				PVOID;
typedef void *				HANDLE;
typedef const char *		LPCTSTR;
typedef char *				LPSTR;
#define	IN
#define OUT
#endif

#define EVT_ONLINE			1		// ʶ�������
#define EVT_OFFLINE		2		// ʶ�������
#define EVT_FIRED			3		// ʶ�𴥷�
#define EVT_DONE			4		// ʶ��������������
#define EVT_LIVE				5		// ʵʱͼ�����
#define EVT_VLDIN			6		// �г�����������Ȧ
#define EVT_VLDOUT		7		// �����뿪������Ȧʶ����
#define EVT_EXTDI			8		// DI ״̬�仯 ��һ����Ĳ����ã�����չDI�㣩
#define EVT_SNAP			9		// �յ�ץ�� ͼƬ��ʹ��LPNR_TakeSnapFrame��ȡץ��ͼƬ�Ż��յ�����Ϣ)
#define EVT_ASYNRX		10		// �յ�͸����������

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*LPNR_callback)(void *, int );

// �������������TCP���ӣ����������̡߳�
DLLAPI HANDLE CALLTYPE LPNR_Init( const char *strIP );
// �ر����ӣ����������߳�
DLLAPI BOOL CALLTYPE LPNR_Terminate(HANDLE h);
// ���ûص����� ��for Windows, LPNR_SetWinMsg is recommended��
DLLAPI BOOL CALLTYPE LPNR_SetCallBack(HANDLE h, LPNR_callback mycbfxc );
#if defined _WIN32 || defined _WIN64
DLLAPI BOOL CALLTYPE LPNR_SetWinMsg( HANDLE h, HWND hwnd, int msgno);
#endif
// ѯ��������Ƿ�online
DLLAPI BOOL CALLTYPE LPNR_IsOnline(HANDLE h);
// ��ȡ���ƺ��룬������յ�EVT_DONE����òſ��Ի�ȡ����ʶ��ĳ��ƺ��롣
DLLAPI BOOL CALLTYPE LPNR_GetPlateNumber(HANDLE h, char *buf);
DLLAPI BOOL CALLTYPE LPNR_GetPlateAttribute(HANDLE h, BYTE *attr);
/* LPNR_GetPlateAttribute: ��ȡ����������Ϣ��ͬ����Ҫ���յ�EVT_DONE�����
 * plate attribute:
 * attr������8���ֽڡ�ÿ���ֽ�����Ϊ��
 * 0�� ���Ŷ� ��0~100����reserved��
 * 1�� ��ͷ���ǳ�β�ĳ��ƣ�reserved��1����ͷ��0xff ��β��0 δ֪��
 * 2��������ɫ�� ��reserved��
 * 3������ ��ֻ�г��ٴ���ʱ���ٶ�ֵ�������壩
 * 4������Դ (0��ͼƬ���ͣ�1����λ��������2����Ȧ������3����ʱ������4��������Ȧ������5�����ٴ���)
 * 5~7������
 */
// ��ȡlive jpeg֡���յ�EVT_LIVE�¼�����á�buf������������һ��JPEG��ʽ��ͼƬ���ݡ�buf��С������ڵ���LPNR_GetLiveFrameSize���ص�ֵ��
DLLAPI int CALLTYPE LPNR_GetLiveFrameSize(HANDLE h);
DLLAPI int CALLTYPE LPNR_GetLiveFrame(HANDLE h, char *buf);
// ���º�����ȡץ��ͼƬ��һ�����յ�EVT_DONE�����
// ��ȡ���Ʋ�ɫͼ��buf���ݾ���������һ��bmp��ʽ��ͼƬ����С������ڵ���LPNR_GetPlateColorImageSize���ص�ֵ
DLLAPI int CALLTYPE LPNR_GetPlateColorImageSize(HANDLE h);
DLLAPI int CALLTYPE LPNR_GetPlateColorImage(HANDLE h, char *buf);
// ��ȡ���ƶ�ָͼ��binary image����buf���ݾ���������һ��bmp��ʽ��ͼƬ����С������ڵ���LPNR_GetPlateBinaryImageSize���ص�ֵ
DLLAPI int CALLTYPE LPNR_GetPlateBinaryImageSize(HANDLE h);
DLLAPI int CALLTYPE LPNR_GetPlateBinaryImage(HANDLE h, char *buf);
// ��ȡץ��ͼƬ�������������ƺ����ͼƬ����buf���ݾ��Ǹ�ͼƬjpeg��ʽ���������ݣ���С������ڵ���LPNR_GetCapturedImageSize���ص�ֵ
// ���������û�н�ֹ����ץ��ȫͼ�Ż��յ����ͼƬ
DLLAPI int CALLTYPE LPNR_GetCapturedImageSize(HANDLE h);
DLLAPI int CALLTYPE LPNR_GetCapturedImage(HANDLE h, char *buf);
// ��ȡץ��ͼƬ�ĳ�ͷͼ�������������ƺ����ͼƬ����buf���ݾ��Ǹ�ͼƬjpeg��ʽ���������ݣ���С������ڵ���LPNR_GetHeadImageSize���ص�ֵ
// ���������ʹ�ܷ��ͳ�ͷͼ�Ż��ڷ����공�ƺ��յ����ͼƬ
DLLAPI int CALLTYPE LPNR_GetHeadImageSize(HANDLE h);
DLLAPI int CALLTYPE LPNR_GetHeadImage(HANDLE h, char *buf);
// ��ȡץ��ͼƬ1/4����ȵ�ͼ�������������ƺ����ͼƬ����buf���ݾ��Ǹ�ͼƬjpeg��ʽ���������ݣ���С������ڵ���LPNR_GetQuadImageSize���ص�ֵ
// ���������ʹ�ܷ���1/4�����ͼ�Ż��ڷ����공�ƺ��յ����ͼƬ
DLLAPI int CALLTYPE LPNR_GetQuadImageSize(HANDLE h);
DLLAPI int CALLTYPE LPNR_GetQuadImage(HANDLE h, char *buf);
// ͬ�������ʱ�䣬�ú����·������ʱ�����������������������ϵͳʱ�䡣
DLLAPI BOOL CALLTYPE LPNR_SyncTime(HANDLE h);
// ������������֡��ʱ������λ������ʱ�����Ҫ����������������⹤���߳�����Ϊ��ʱ��δ�յ�����֡���ر����ӡ�
DLLAPI BOOL CALLTYPE LPNR_ResetHeartBeat(HANDLE h);
// ��ȡ���������ݺ��ͷű����ͼƬ���ݡ����Բ����á������߳����´ο�ʼʶ�����ʱ�����Զ��ͷš�
DLLAPI BOOL CALLTYPE LPNR_ReleaseData(HANDLE h);
// ��λ������һ��ʶ������������������������idle״̬��û�����ڷ��������У�
// LPNR_SoftTriggerEx ��һ������Id������һ����ţ�ÿ�δ��������������ʹ��ͬһ��Id�Ŷ�η��ͣ�ֻ�ᴥ��һ�Ρ����������粻�ȶ�����η���ȷ�����Դ���ʱʹ�á�
DLLAPI BOOL CALLTYPE LPNR_SoftTrigger(HANDLE h);
DLLAPI BOOL CALLTYPE LPNR_SoftTriggerEx(HANDLE h, int Id);
// �������������߳�״̬�Ƿ������ã��ȴ��´δ�����
DLLAPI BOOL CALLTYPE LPNR_IsIdle(HANDLE h);
// ��ȡ���һ�γ��Ʒ����Ĵ���ʱ�䡣elaped time��wall clock��processed time��CPU time. ��λ��msec
DLLAPI BOOL CALLTYPE LPNR_GetTiming(HANDLE h, int *elaped, int *processed );
// ����ʵʱJPEG֡��nSizeCodeȡֵ��Χ 0~3
// 0 - �ر�ʵʱ֡����
// 1 - ����ȫ�����
// 2 - ����1/4�����
// 3 - ����1/16�����
// ע�ͣ���Ҫ���ʶ��������汾 1.3.5.18 �����ϵİ汾���;ɰ汾���ӵĻ���nSIzeCode 0 һ���ǹرգ�!=0 ����������ȸ��ݵ�ǰ���õĲ������͡�
// ÿ���ͻ��˿��������Լ���Ҫ�Ľ���ȣ����ụ��Ӱ�졣
// 1.3.5.18�汾֮�ϣ�֡�ʻ��ܶࡣ������1/4, 1/16�����������ܵ���֡�ʣ�25���ϣ�
// ���ڲ���Ҫlive jpeg֡��application, ����ر�live jpeg, ���Խ�ʡ���������������ÿ���յ�EVT_ONLINE��Ϣ�󣬾͵���
// LPNR_EnableLiveFrame(h,0)���Ա�֤�ر�live jpeg. ��������LPNR_Init�����̵��ã���Ϊ��ʱ��TCP���ӻ�û������
DLLAPI BOOL CALLTYPE LPNR_EnableLiveFrame(HANDLE h, int nSizeCode);
// ��ȡһ��ץ��֡�����ǲ����г��Ʒ������������ֻ�Ǵ���һ��ץ�ģ�ͼƬҪ���յ�EVT_SNAP��Ϣ���¼����󣬵���LPNR_GetCapturedImage��ȡ
// bFlashLight TRUE��ץ��ʱ���⣬FALSE�ǲ�Ҫ���⡣
DLLAPI BOOL CALLTYPE LPNR_TakeSnapFrame(HANDLE h, BOOL bFlashLight);
// LPNR_Lock/LPNR_Unlock - ��ͼƬ���ݺͳ������ݼ���/�����������ڶ�ȡ�����б������߳�ˢ�����ݡ�
// �ж�ȡlive jpeg frameʱ����Ҫ�õ����������������п����ڶ�ȡ�Ĺ����У��µ�һ��ʵʱ֡��ˢ��buffer����ɶ��������ݴ���
// ����ͼƬ���Բ���������Ϊ����ֻ������ʶ����ʱ�Żᱻˢ�¡�
DLLAPI BOOL CALLTYPE LPNR_Lock( HANDLE h );
DLLAPI BOOL CALLTYPE LPNR_Unlock(HANDLE h);
// helper function
// ��ȡ�����IP�����Ǹ�һ��application���Ӷ��ʶ���ʱ�����յ�һ����Ϣ��Ҫ֪�������Ϣ������̨ʶ������͵�ʱ�򣬿��Ե������������
DLLAPI BOOL CALLTYPE LPNR_GetMachineIP(HANDLE h, LPSTR strIP);
// ��ȡ�����label��name of camera configured��
DLLAPI LPCTSTR CALLTYPE LPNR_GetCameraLabel(HANDLE h);

// control camera's lighting control output. 
// onoff - 1 ON, 0 OFF
// msec - if 'onoff'==1, this argument set 'msec' to turn on. if 'msec' is 0 means turn on until explicitly turned off.
DLLAPI BOOL CALLTYPE LPNR_LightCtrl(HANDLE h, int onoff, int msec);
// control camaera's IR-cut lens.  'onoff' 1 apply IR-cut filter, 0 turn off IR-cut filter
DLLAPI BOOL CALLTYPE LPNR_IRCut(HANDLE h, int onoff);
// ������ƵOSD���ӵ�ʱ���ǩλ�ã�bEnableΪTRUEʹ�ܣ�FALSE���ܵ���; x, yΪ����λ�ã���Ƶ���Ͻ�Ϊԭ�㣩����(0,0)ʹ��Ĭ��ֵ�����Ͻǣ�
DLLAPI BOOL CALLTYPE LPNR_SetOSDTimeStamp(HANDLE h, BOOL bEnable, int x, int y);

// query function (use UDP, ����Ҫ�Ƚ�������) - �����������Ƹ�һ��������ϵͳ�������м��ٸ������ʹ�õġ�����Ҫ����
// ��ÿ̨���߻���TCP���ӣ��������ÿ̨�������ѯ��
// note: 'tout' is response timed out setting in msec 
DLLAPI BOOL CALLTYPE LPNR_QueryPlate( IN LPCTSTR strIP, OUT LPSTR strPlate, int tout );
#if defined _WIN32 || defined _WIN64
// ���APû���Լ�����Winsock��ϵͳ��Ҳû�е���LPNR_Init, ����Ҫʹ�������������Winsock��ſ��Ե���ǰ���UDP��ѯ���
DLLAPI BOOL CALLTYPE LPNR_WinsockStart( BOOL bStart );
#endif

// extended DI/DO functions
DLLAPI BOOL CALLTYPE LPNR_GetExtDI(HANDLE h, int *diLastnThis);
DLLAPI BOOL CALLTYPE LPNR_SetExtDO(HANDLE h, int pin, int value);
// output 50% duty cycle pulse train with 'period' (msec). count is pulse count. if count==0 means forever until stop explicitly (period 0 means stop).
DLLAPI BOOL CALLTYPE LPNR_PulseOut(HANDLE h, int pin, int period, int count);		

// ͸������ - ������д����ô��ڵģ�����ʹ�����´���͸���������������ݸ���������������ת�������ڡ���������յ��Ĵ�������
//                  ���Ի��͸������������൱���ڴ����豸������LED��ʾ�����ͼ����֮��һ������Ĵ��ڡ�����������ᴫ�������
//                  ֻ�������ݵ���ת��
// ��/�ر�͸������, speed�ǲ�����, Speed=0 Ϊ�رմ���, >0 �򿪴������øò����ʡ�ֻ����8bit data, 1-stop bit, no parity
DLLAPI BOOL CALLTYPE LPNR_COM_init(HANDLE h, int Speed);
// ʹ�ܴ�������Ľ��գ�Ĭ���ǲ�ʹ�ܵģ�ʶ����յ��������벻�ᷢ�͸�û��ʹ�ܵĿͻ��ˡ�������ʹ�ܵĿͻ��˶����յ�ͬ�������ݣ�
// ��̬���յ��������ݺ󣬻ᷢ���¼�EVT_ASYNRX��Windows��Ϣ���ǻص�������
DLLAPI BOOL CALLTYPE LPNR_COM_aync(HANDLE h, BOOL bEnable);
// ���ʹ�������
DLLAPI BOOL CALLTYPE LPNR_COM_send(HANDLE h, const char *data, int len);
// �յ��Ĵ������ݣ���̬�Ᵽ�����ڲ�1K��ring buffer���棬�û�ʹ�����º�����ȡRX����ѶϢ
DLLAPI int CALLTYPE LPNR_COM_iqueue(HANDLE h);		// ��ȡ���յ��Ĵ����ֽ�������δ��ȡ�ߣ�
// ���ƶ�̬�Ᵽ��Ĵ���RX���ݣ���size�ֽڡ���������ݲ����Ƴ���
// ����ֵ: ʵ�ʸ��Ƶ��ֽ��������size > ring buffer�ڿɶ����ֽ���������ֵ��ʵ�ʸ��Ƶ��ֽ���(Ҳ����ring buffer�ڿɶ��ֽ���)��
// ���size <= ring buffer ������ֽ���������ֵ����size.
DLLAPI int CALLTYPE LPNR_COM_peek(HANDLE h, char *RxData, int size);
// �����ϸ����������������ڶ�ȡ����ring buffer���Ƴ���
DLLAPI int CALLTYPE LPNR_COM_read(HANDLE h, char *RxData, int size);
// ��ring buffer���Ƴ�ǰ��'size'�ֽ�
DLLAPI int CALLTYPE LPNR_COM_remove(HANDLE h, int size);
// �����̬���ڱ�������д��ڽ�������
DLLAPI BOOL CALLTYPE LPNR_COM_clear(HANDLE h);
// note:
// LPNR_COM_peek + LPNR_COM_remove = LPNR_COM_read

#ifdef __cplusplus
}
#endif

#endif
