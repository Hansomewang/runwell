#ifndef __DEMO_H
#define __DEMO_H

typedef unsigned short Uint16;
typedef unsigned int Uint32;
typedef int Int32;
typedef unsigned char Uint8;
typedef char Int8;

#define DEFAULT_SUBPACKET 4

//定义命令包集合的报头
typedef struct 
{
	Uint32	ui32Header;				//指令头
	Uint32 	ui32Version;			//指令版本
	Uint32 	ui32MessageLength;		//指令长度
	Uint16	ui16ReceiveId;			//接收ID号		//代表消息需要到达的目的地址(层次：网络层、视频服务层、数据处理层、PC层)
	Uint16	ui16SendId;				//发送ID号
	Uint32	ui32MessageId;			//消息序列号
	Uint32	ui32ResponseId;			//反馈消息序列号
	Uint32	ui32MessageType;		//消息类型
	Int32	i32FBTimeOutms;			//超时时效,暂无效，保留
	Uint32	ui32FieldCount;			//消息个数
}StCommandPacketHeader;

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

//子包属性
typedef struct _stObjPacketAttr
{	
	Uint32	ui32SubPacketType;				//子包类型
	Uint32	ui32SubPacketOffset;			//子包偏移位置
	Uint32	ui32SubPacketSize;				//子包大小，长度
}stObjPacketAttr;

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
}stSubPacketHeader;

#define bigpacket_header 0x0FF055AA    //大包包头
#define bigpacket_version 0x00010000 

#define subpacket_header 0xddccbbaa   //小包包头
#define subpacket_type_jpg 0x00010001  //图片数据小包
#define subpacket_type_cardata 0x00010003 //识别信息小包
#define subpacket_type_version 0x00010002 //小包version
#define IMG_SIZE 1024*200//(1920*1080)		// 6MB

typedef struct _cardatapacket
{
    Uint32 MagicNum;
    Uint32 Version;
    Uint32 DataLength; //识别数据包长度
    Uint32 EventType;
    Uint32 ObjType;    //目标类型，0-车辆、1-行人、2-事件、3-人脸
    Uint8 OCRResult[16]; //单辆车牌识别结果，如京A12345
    Uint32 PlateColor;   //车牌颜色(0为未知色，1为蓝色，2为白色，3为黑色，4为黄色)
    Uint16 VCRResult;    //车身颜色
    Uint16 VCRDepth;   //车身颜色深浅
    Uint32 ObjSpeed;  //目标速度
    Uint32 ObjWidth;  //目标宽度(若目标类型为车辆，则为车头参数/车脸图参数)
    Uint32 ObjHeight; //目标高度(若目标类型为车辆，则为车头参数/车脸图参数)
    Uint32 ObjStartx;
    Uint32 ObjStarty;
    Uint32 ObjLength;  //目标长度(车长)
    Uint32 ObjImgType;
    Uint32 ObjAddressY;
    Uint32 ObjAddressU;
    Uint32 ObjAddressV;
    Uint16 PlateStartX;
    Uint16 PlateStartY;
    Uint16 PlateWidth;
    Uint16 PlateHeight;
    Uint16 ObjId;      //目标ID号，0-65535
    Uint8 ObjLaneId;  //目标所占车道号
    Uint8 RunMode;
    Uint8 SingleCharCredibility[10];
    Uint8 OverallCredibility;
    Uint8 ui8PlateLocImgNo;
    Uint16 Shutter;  //当前图像抓怕快门值(us)
    Uint8 Gain;  //当前图像抓怕增益值
    Uint8 Brightness;  //当前图像亮度值
    Uint32 ViolationInfo;  //车辆违法信息，0表示正常行驶，非0表示违法
    Uint32 OcrSucceed;  //当前车牌有无识别结果，0-无识别结果、1-有识别结果
    Uint32 CarType;  //车型信息
    Uint8 NumCapFrms;  //当前目标采图帧数
    Uint8 CapImgIndex;
    Uint16 Rsved2; //预留
    Uint32 Rsved3[8]; //预留
}CarDataPacket;

typedef struct _testdataobj //事件信息结构体
{
    StCommandPacketHeader TestPacketHeader; //主包头
    StCommandContentHeader TestContentHeader; //field包头

    stMainPacketHeader TestMainPacketHeader; //数据主包头

    stSingleOBJPacket TestSingleOBJPacket; //子包信息

    stSubPacketHeader TestSubPacketHeader0; //子包1
    CarDataPacket CarDataPacket; //识别信息

    stSubPacketHeader TestSubPacketHeader1; //子包2
    int BinImagesz;
    void *TestBinImage; //车牌二值图

    stSubPacketHeader TestSubPacketHeader2; //子包头3
    int BigImagesz;
    void *TestBigImage; //全景图

    stSubPacketHeader TestSubPacketHeader3; //子包头4
    int SmallImagesz;
    void *TestSmallImage; //车牌彩色图

    Uint32 Tail;

}TestDataObj;

typedef struct _testsetobj //配置信息结构体
{
    StCommandPacketHeader TestPacketHeader; //主包头
    StCommandContentHeader TestContentHeader; //field包头

    stCamInfo stCamInfo; 

}TestSetObj, *PTestSetObj;

#endif
