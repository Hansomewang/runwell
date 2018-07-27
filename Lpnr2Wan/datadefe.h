
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
#define MAX_USE_COUNT	5 // 可以连接的最大用户数量

#define WAN_VERSION	0x00010000

#if 1
#define WAN_CMD_MAGIC		0x0FF055AA // 主包头
#define WAN_FEILD_MAGIC		0xAA55A55A	// Field包头Magic
#define WAN_MAIN_PACK_MAGIC	0x88776655	// 子包Magic
#define WAN_SING_MAGIC		0x44332211	// 单包Magic
#define WAN_PACKAGE_TAIL	0x550FF0AA
#else
#define WAN_CMD_MAGIC		0x0FFF55FF // 主包头
#define WAN_FEILD_MAGIC		0xAA55A55A	// Field包头Magic
#define WAN_MAIN_PACK_MAGIC	0x88776655	// 子包Magic
#define WAN_SING_MAGIC		0x44332211	// 单包Magic
#define WAN_PACKAGE_TAIL	0x550FFFFF
#endif

/* 
 * 协议中的数据结构为，单个数据包的内容为 主包，主包包含主包信息头+n个Field包
 * 每个Field包包含了N个子包，每个子包含有包头 + n个单包构成，结构稍显复杂
 * */

#define DATA_TYPE_IMAGE		0x00000004
#define DATA_TYPE_NORMAL	0x00000000

// #define DATA_ID_CAPTURE		0x00010003	// 抓拍请求帧
#define DATA_ID_CAPTURE		0x00040a01	// 抓拍请求帧
#define DATA_ID_HBEAT		0x00010001  // 心跳帧
#define DATA_ID_SYNC		0x00080007  // 系统校时
#define DATA_ID_IMAGE		0x01000000	// 包含车牌图片和其他内容，比较复杂
#define DATA_ID_CONFIG		0x01050000	// 配置配置帧
#define DATA_ID_VERSION		0x01040000 //设备版本信息




typedef struct 
{
	Uint32	ui32Header;				//指令头
	Uint32 	ui32Version;			//指令版本
	Uint32 	ui32MessageLength;		//指令长度
	//代表消息需要到达的目的地址(层次：网络层、视频服务层、数据处理层、PC层)
	Uint16	ui16ReceiveId;			//接收ID号		
	Uint16	ui16SendId;				//发送ID号
	Uint32	ui32MessageId;			//消息序列号
	Uint32	ui32ResponseId;			//反馈消息序列号
	Uint32	ui32MessageType;		//消息类型
	Int32	i32FBTimeOutms;			//超时时效,暂无效，保留
	Uint32	ui32FieldCount;			//消息个数
	Uint8	aui8Field[4];			//为了便于操作而定义的指针(实际为数组)
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

//定义单条field指令报头
typedef struct 
{
	Uint32	ui32MagicNumber;
	UnCommandField unionCommandField;
	Uint32	ui32CommandId;
	Uint32	ui32FDCode;
	Uint32 	ui32CommandLength;
	Uint8	aui8Data[4];//为了便于操作而定义的指针(实际为数组)
}StCommandContentHeader;

//网络信息
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

//用户信息  
typedef struct 
{
	Int8 i8FixAddr[64];
	Int8 i8FixDirection[64];
	Int8 i8UserDefCode[64];
	Int8 i8UserDefInfo[64];
}StDeamonUserInfo;

//设备配置信息
typedef struct _stCamInfo
{
	StIfconfigInfo     strcIpInfo;
	StDeamonUserInfo   strcUserInfo;
}stCamInfo;

//事件信息数据主包头
typedef struct _stMainPacketHeader
{
	Uint32	ui32Header;				//表头 0x88776655
	Uint32	ui32Version;			//协议版本
	Uint32	ui32Type;				//类型备用
	Uint32	ui32Length;				//数据长度
	Uint32	ui32ObjNum;				//目标个数
	Uint8	aui8Data[4];			//指向OBJ 目标子包
}stMainPacketHeader;


//子包属性
typedef struct _stObjPacketAttr
{	
	Uint32	ui32SubPacketType;				//子包类型
	Uint32	ui32SubPacketOffset;			//子包偏移位置
	Uint32	ui32SubPacketSize;				//子包大小，长度
}stObjPacketAttr;

typedef struct _stSystemClock
{
	Uint8 		ui8Year;		//年
	Uint8 		ui8Month;		//月
	Uint8 		ui8Day;			//日
	Uint8 		ui8Hour;		//时
	Uint8 		ui8Minite;		//分
	Uint8 		ui8Second;		//秒
	Uint16 		ui16MiliSecond;	//毫秒
}stSystemClock;

#define DEFAULT_SUBPACKET 16
//单个目标的所有子包信息，属于数据主包头里
typedef struct _stSingleOBJPacket
{
	Uint32	ui32Header;				//标识  0x44332211
	Uint32	ui32TableLen;			//单目标数据检索长度  默认216，标识stSingleOBJPacket 结构体大小  
	Uint32 	ui32Serial;				//目标序列号
	stSystemClock 	strcClock;		//触发时间，8个字节
	Uint32	ui32SubPacketCnt;		//子包个数
	stObjPacketAttr  strcSubPacketAttr[DEFAULT_SUBPACKET];		//子包属性
}stSingleOBJPacket;

#define SINGLEPACK_INIT() do{\
	singPack->ui32Header = 0x44332211;\
	singPack->ui32TableLen = 216;\
	singPack->ui32Serial  = 0;\
	}while(0)

//数据子包头信息
typedef struct _stSubPacketHeader
{
	Uint32	ui32Header;				//表头 0xddccbbaa
	Uint32	ui32Version;			//协议版本 0x00010001
	Uint32	ui32Type;				//子包类型
	Uint32	ui32Length;				//子包长度
	Uint32	ui32Serial;				//子包序列号
	Uint8	ui8ImgAttr[8];			//图像属性，预留
	Uint32	ui32Bak;				//预留
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


// 全景大图
#define SUBPACK_SETJPEG() do{\
	subPack->ui8ImgAttr[0] = 0x00;\
	subPack->ui8ImgAttr[1] = 0x00;\
	}while(0)
// 二值图
#define SUBPACK_SETBIN() do{\
	subPack->ui8ImgAttr[0] = 0x04;\
	subPack->ui8ImgAttr[1] = 0x00;\
	}while(0)

// 车牌图
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
#define CAR_COLOR_PURPLE 6 //紫色
#define CAR_COLOR_GREEN	7
#define CAR_COLOR_BLUE	8
#define CAR_COLOR_BROWN	9 // 棕色
#define CAR_COLOR_BLACK	10


#ifdef __cplusplus	
}
#endif

#endif
