
#ifndef __DATADEF_HEAD_H
#define __DATADEF_HEAD_H

#ifdef __cplusplus
extern "C" {
#endif


typedef unsigned short Uint16;
typedef unsigned int Uint32;
typedef int Int32;
typedef unsigned char Uint8;
typedef char Int8;



#define LISTEN_PORT 58002
#define MAX_USE_COUNT	5 // �������ӵ�����û�����

#define WAN_VERSION	0x00010000

#if 1
#define WAN_CMD_MAGIC		0x0FF055AA // ����ͷ
#define WAN_FEILD_MAGIC		0xAA55A55A	// Field��ͷMagic
#define WAN_MAIN_PACK_MAGIC	0x88776655	// �Ӱ�Magic
#define WAN_SING_MAGIC		0x44332211	// ����Magic
#define WAN_PACKAGE_TAIL	0x550FF0AA
#else
#define WAN_CMD_MAGIC		0x0FFF55FF // ����ͷ
#define WAN_FEILD_MAGIC		0xAA55A55A	// Field��ͷMagic
#define WAN_MAIN_PACK_MAGIC	0x88776655	// �Ӱ�Magic
#define WAN_SING_MAGIC		0x44332211	// ����Magic
#define WAN_PACKAGE_TAIL	0x550FFFFF
#endif

/* 
 * Э���е����ݽṹΪ���������ݰ�������Ϊ ��������������������Ϣͷ+n��Field��
 * ÿ��Field��������N���Ӱ���ÿ���Ӱ����а�ͷ + n���������ɣ��ṹ���Ը���
 * */

#define DATA_TYPE_IMAGE		0x00000004
#define DATA_TYPE_NORMAL	0x00000000

// #define DATA_ID_CAPTURE		0x00010003	// ץ������֡
#define DATA_ID_CAPTURE		0x00040a01	// ץ������֡
#define DATA_ID_HBEAT		0x00010001  // ����֡
#define DATA_ID_SYNC		0x00080007  // ϵͳУʱ
#define DATA_ID_IMAGE		0x01000000	// ��������ͼƬ���������ݣ��Ƚϸ���
#define DATA_ID_CONFIG		0x01050000	// ��������֡
#define DATA_ID_VERSION		0x01040000 //�豸�汾��Ϣ




typedef struct 
{
	Uint32	ui32Header;				//ָ��ͷ
	Uint32 	ui32Version;			//ָ��汾
	Uint32 	ui32MessageLength;		//ָ���
	//������Ϣ��Ҫ�����Ŀ�ĵ�ַ(��Σ�����㡢��Ƶ����㡢���ݴ���㡢PC��)
	Uint16	ui16ReceiveId;			//����ID��		
	Uint16	ui16SendId;				//����ID��
	Uint32	ui32MessageId;			//��Ϣ���к�
	Uint32	ui32ResponseId;			//������Ϣ���к�
	Uint32	ui32MessageType;		//��Ϣ����
	Int32	i32FBTimeOutms;			//��ʱʱЧ,����Ч������
	Uint32	ui32FieldCount;			//��Ϣ����
	Uint8	aui8Field[4];			//Ϊ�˱��ڲ����������ָ��(ʵ��Ϊ����)
}StCommandPacketHeader;

#define SETHEADER_TOTAL_LEN( num ) header.ui32MessageLength = num
#define HEADER_INIT( ) do {\
	header.ui32Header = WAN_CMD_MAGIC;\
	header.ui32Version = 0x00010000;\
	header.ui16ReceiveId = 0x01;\
	header.ui16SendId = 0x02;\
	header.ui32MessageId = 0x00;\
	header.ui32FieldCount = 1;\
	header.ui32MessageType = 0x00000004;\
}while( 0 )

typedef struct
{
	Uint8	ui8CmdAttribute;
	Uint8	ui8CmdType;
	Uint8	ui8CmdSub;
	Uint8	ui8CmdMain;
}StCmdFieldHeader;

typedef union
{
	Uint32	ui32CmdFiled;
	StCmdFieldHeader strcCommandField;
}UnCommandField;

//���嵥��fieldָ�ͷ
typedef struct 
{
	Uint32	ui32MagicNumber;
	UnCommandField unionCommandField;
	Uint32	ui32CommandId;
	Uint32	ui32FDCode;
	Uint32 	ui32CommandLength;
	Uint8	aui8Data[4];//Ϊ�˱��ڲ����������ָ��(ʵ��Ϊ����)
}StCommandContentHeader;

//������Ϣ
typedef struct 
{
	Uint32 ui32MagicNum;
	Uint32 ui32Version;
	Uint32 ui32IP;
	Uint32 ui32NetMask;
	Uint32 ui32Dns;
	Uint32 ui32GateWay;
	Uint8  ui8Mac[8];
	Int8   i8DeviceCode[32];
	Int8   i8Reserve[64];
	Uint32 ui32CheckNum;
}StIfconfigInfo;

//�û���Ϣ  
typedef struct 
{
	Int8 i8FixAddr[64];
	Int8 i8FixDirection[64];
	Int8 i8UserDefCode[64];
	Int8 i8UserDefInfo[64];
}StDeamonUserInfo;

//�豸������Ϣ
typedef struct _stCamInfo
{
	StIfconfigInfo     strcIpInfo;
	StDeamonUserInfo   strcUserInfo;
}stCamInfo;

//�¼���Ϣ��������ͷ
typedef struct _stMainPacketHeader
{
	Uint32	ui32Header;				//��ͷ 0x88776655
	Uint32	ui32Version;			//Э��汾
	Uint32	ui32Type;				//���ͱ���
	Uint32	ui32Length;				//���ݳ���
	Uint32	ui32ObjNum;				//Ŀ�����
	Uint8	aui8Data[4];			//ָ��OBJ Ŀ���Ӱ�
}stMainPacketHeader;


//�Ӱ�����
typedef struct _stObjPacketAttr
{	
	Uint32	ui32SubPacketType;				//�Ӱ�����
	Uint32	ui32SubPacketOffset;			//�Ӱ�ƫ��λ��
	Uint32	ui32SubPacketSize;				//�Ӱ���С������
}stObjPacketAttr;

typedef struct _stSystemClock
{
	Uint8 		ui8Year;		//��
	Uint8 		ui8Month;		//��
	Uint8 		ui8Day;			//��
	Uint8 		ui8Hour;		//ʱ
	Uint8 		ui8Minite;		//��
	Uint8 		ui8Second;		//��
	Uint16 		ui16MiliSecond;	//����
}stSystemClock;

#define DEFAULT_SUBPACKET 16
//����Ŀ��������Ӱ���Ϣ��������������ͷ��
typedef struct _stSingleOBJPacket
{
	Uint32	ui32Header;				//��ʶ  0x44332211
	Uint32	ui32TableLen;			//��Ŀ�����ݼ�������  Ĭ��216����ʶstSingleOBJPacket �ṹ���С  
	Uint32 	ui32Serial;				//Ŀ�����к�
	stSystemClock 	strcClock;		//����ʱ�䣬8���ֽ�
	Uint32	ui32SubPacketCnt;		//�Ӱ�����
	stObjPacketAttr  strcSubPacketAttr[DEFAULT_SUBPACKET];		//�Ӱ�����
}stSingleOBJPacket;

#define SINGLEPACK_INIT() do{\
	singPack->ui32Header = 0x44332211;\
	singPack->ui32TableLen = 216;\
	singPack->ui32Serial  = 0;\
	}while(0)

//�����Ӱ�ͷ��Ϣ
typedef struct _stSubPacketHeader
{
	Uint32	ui32Header;				//��ͷ 0xddccbbaa
	Uint32	ui32Version;			//Э��汾 0x00010001
	Uint32	ui32Type;				//�Ӱ�����
	Uint32	ui32Length;				//�Ӱ�����
	Uint32	ui32Serial;				//�Ӱ����к�
	Uint8	ui8ImgAttr[8];			//ͼ�����ԣ�Ԥ��
	Uint32	ui32Bak;				//Ԥ��
	void 	*data;
}stSubPacketHeader;

#define SUBPACK_INIT() do{\
	subPack->ui32Header = 0xddccbbaa;\
	subPack->ui32Version = 0x00010002;\
	}while(0)

#define SUBPACK_IMAGEWIDTH(h) do{\
	subPack->ui8ImgAttr[4] = (h&0xff);\
	subPack->ui8ImgAttr[5] = (h&0xff00)>>8;\
}while(0)

#define SUBPACK_IMAGHEIGHT(h) do{\
	subPack->ui8ImgAttr[6] = (h&0xff);\
	subPack->ui8ImgAttr[7] = (h&0xff00)>>8;\
}while(0)


// ȫ����ͼ
#define SUBPACK_SETJPEG() do{\
	subPack->ui8ImgAttr[0] = 0x00;\
	subPack->ui8ImgAttr[1] = 0x00;\
	}while(0)
// ��ֵͼ
#define SUBPACK_SETBIN() do{\
	subPack->ui8ImgAttr[0] = 0x04;\
	subPack->ui8ImgAttr[1] = 0x00;\
	}while(0)

// ����ͼ
#define SUBPACK_SETPLATE() do{\
	subPack->ui8ImgAttr[0] = 0x00;\
	subPack->ui8ImgAttr[1] = 0x01;\
	}while(0)
#define SUBPACK_SETDATALEN(h) subPack->ui32Length = h


typedef struct _RecongizeData{
	Uint32 ui32MagicNum;
	Uint32 ui32Version;
	Uint32 ui32DataLen;
	Uint32 ui32EventType;
	Uint32 ui32ObjType;
	Uint8 ui8OCRResult[16];
	Uint32 ui32PlateColor;
	Uint16 ui16VCRResult;
	Uint16 ui16VCRDepth;
	Uint32 ui32ObjSpeed;
	Uint32 ui32ObjWidth;
	Uint32 ui32ObjHeight;
	Uint32 ui32ObjStartx;
	Uint32 ui32ObjStarty;
	Uint32 ui32ObjLength;
	Uint32 ui32ObjImgType;
	Uint32 ui32ObjAddressY;
	Uint32 ui32ObjAddressU;
	Uint32 ui32ObjAddressV;
	Uint16 ui16PlateStartX;
	Uint16 ui16PlateStartY;
	Uint16 ui16PlateWidth;
	Uint16 ui16PlateHeight;
	Uint16 ui16ObjId;
	Uint8 ui8ObjLaneId;
	Uint8 ui8RunMode;
	Uint8 ui8SingleCharCredibility[10];
	Uint8 ui8OverallCredibility;
	Uint8 ui8ui8PlateLocImgNo;
	Uint16 ui16Shutter;
	Uint8 ui8Gain;
	Uint8 ui8Brightness;
	Uint32 ui32ViolationInfo;
	Uint32 ui32OcrSucceed;
	Uint32 ui32CarType;
	Uint8 ui8NumCapFrms;
	Uint8 ui8CapImgIndex;
	Uint16 ui16CapNum;
	Uint8 ui8MD5[4][32];
//	Uint8 ui8CaptureTime[4][8];
	stSystemClock stColock;
	Uint8 ui8RedSTime[8];	
	Uint8 ui8RedETime[8];
	Uint32 ui32Rsved3[16];
}StRecongizeData1;

typedef struct _StRecongizeData2{
	Uint32 ui32MagicNum;
	Uint32 ui32Version;
	Uint32 ui32DataLen;
	Uint32 ui32EventType;
	Uint32 ui32ObjType;
	Uint8 ui8OCRResult[16];
	Uint32 ui32PlateColor;
	Uint16 ui16VCRResult;
	Uint16 ui16VCRDepth;
	Uint32 ui32ObjSpeed;
	Uint32 ui32ObjWidth;
	Uint32 ui32ObjHeight;
	Uint32 ui32ObjStartx;
	Uint32 ui32ObjStarty;
	Uint32 ui32ObjLength;
	Uint32 ui32ObjImgType;
	Uint32 ui32ObjAddressY;
	Uint32 ui32ObjAddressU;
	Uint32 ui32ObjAddressV;
	Uint16 ui16PlateStartX;
	Uint16 ui16PlateStartY;
	Uint16 ui16PlateWidth;
	Uint16 ui16PlateHeight;
	Uint16 ui16ObjId;
	Uint8 ui8ObjLaneId;
	Uint8 ui8RunMode;
	Uint8 ui8SingleCharCredibility[10];
	Uint8 ui8OverallCredibility;
	Uint8 ui8ui8PlateLocImgNo;
	Uint16 ui16Shutter;
	Uint8 ui8Gain;
	Uint8 ui8Brightness;
	Uint32 ui32ViolationInfo;
	Uint32 ui32OcrSucceed;
	Uint32 ui32CarType;
	Uint8 ui8NumCapFrms;
	Uint8 ui8CapImgIndex;
	Uint16 Rsved2;
	Uint32 ui32Rsved3[8];
}StRecongizeData2;
	
#define CAR_COLOR_WHITE	1
#define CAR_COLOR_GRAY	2
#define CAR_COLOR_YELLO	3
#define CAR_COLOR_PINK	4
#define CAR_COLOR_RED	5
#define CAR_COLOR_PURPLE 6 //��ɫ
#define CAR_COLOR_GREEN	7
#define CAR_COLOR_BLUE	8
#define CAR_COLOR_BROWN	9 // ��ɫ
#define CAR_COLOR_BLACK	10


#ifdef __cplusplus	
}
#endif

#endif
