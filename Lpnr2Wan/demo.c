#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "demo.h"
#include "lpnr/rwlpnrapi.h"
#include "utils/utils_net.h"

extern void demo_event_handle( HANDLE h, int evnt );

static int serv_sock = 0; 
typedef struct _sev_client
{
    int data;
    struct client_node *next; 
}client_node;

static client_node *cli_head = NULL; //link head

void init_SubPacket_0(HANDLE h,TestDataObj *pTestDataObj) // 识别子包
{
    char buf[16];
    pTestDataObj->TestSubPacketHeader0.ui32Header = 0xddccbbaa;    
    pTestDataObj->TestSubPacketHeader0.ui32Version = 0x00010001;
    pTestDataObj->TestSubPacketHeader0.ui32Type = 0x00010003;
    pTestDataObj->TestSubPacketHeader0.ui32Length = sizeof(stSubPacketHeader) + sizeof(CarDataPacket);
    pTestDataObj->TestSubPacketHeader0.ui32Serial = 0;

    pTestDataObj->CarDataPacket.MagicNum = 0x0FF055AA;
    pTestDataObj->CarDataPacket.Version = 0x00010002;
    pTestDataObj->CarDataPacket.DataLength = sizeof(stSubPacketHeader) + sizeof(CarDataPacket);
    pTestDataObj->CarDataPacket.ObjType = 0;

    if(LPNR_GetPlateNumber(h,buf))
    {
        strcpy(pTestDataObj->CarDataPacket.OCRResult,buf); //车牌识别结果
    }
    
    //车牌颜色

}

void init_SubPacket_1(HANDLE h,TestDataObj *pTestDataObj) //车牌二值图
{
    pTestDataObj->BinImagesz = LPNR_GetPlateBinaryImageSize(h);
    pTestDataObj->TestBinImage = malloc(pTestDataObj->BinImagesz);
    memset(pTestDataObj->TestBinImage,0,pTestDataObj->BinImagesz);
    
    pTestDataObj->TestSubPacketHeader1.ui32Header = 0xddccbbaa;    
    pTestDataObj->TestSubPacketHeader1.ui32Version = 0x00010001;
    pTestDataObj->TestSubPacketHeader1.ui32Type = 0x00010001 ;
    pTestDataObj->TestSubPacketHeader1.ui32Length = sizeof(stSubPacketHeader) + LPNR_GetPlateBinaryImageSize(h);
    pTestDataObj->TestSubPacketHeader1.ui32Serial = 1;
    pTestDataObj->TestSubPacketHeader1.ui8ImgAttr[0] = 0x04;
    pTestDataObj->TestSubPacketHeader1.ui8ImgAttr[1] = 0x00;

    LPNR_GetPlateBinaryImage(h,pTestDataObj->TestBinImage);
}

void init_SubPacket_2(HANDLE h,TestDataObj *pTestDataObj) // 全景图
{
    pTestDataObj->BigImagesz = LPNR_GetCapturedImageSize(h);
    pTestDataObj->TestBigImage = malloc(pTestDataObj->BigImagesz);
    memset(pTestDataObj->TestBigImage,0,pTestDataObj->BigImagesz);

    pTestDataObj->TestSubPacketHeader2.ui32Header = 0xddccbbaa;    
    pTestDataObj->TestSubPacketHeader2.ui32Version = 0x00010001;
    pTestDataObj->TestSubPacketHeader2.ui32Type = 0x00010001 ;
    pTestDataObj->TestSubPacketHeader2.ui32Length = sizeof(stSubPacketHeader) + LPNR_GetCapturedImageSize(h);
    pTestDataObj->TestSubPacketHeader2.ui32Serial = 2;

    LPNR_GetCapturedImage(h,pTestDataObj->TestBigImage);

}

void init_SubPacket_3(HANDLE h,TestDataObj *pTestDataObj) //// 车牌彩色图
{
    pTestDataObj->SmallImagesz = LPNR_GetPlateColorImageSize(h);
    pTestDataObj->TestSmallImage = malloc(pTestDataObj->SmallImagesz);
    memset(pTestDataObj->TestSmallImage,0,pTestDataObj->SmallImagesz);

    pTestDataObj->TestSubPacketHeader3.ui32Header = 0xddccbbaa;    
    pTestDataObj->TestSubPacketHeader3.ui32Version = 0x00010001;
    pTestDataObj->TestSubPacketHeader3.ui32Type = 0x00010001 ;
    pTestDataObj->TestSubPacketHeader3.ui32Length = sizeof(stSubPacketHeader) + LPNR_GetPlateColorImageSize(h);
    pTestDataObj->TestSubPacketHeader3.ui32Serial = 3;

    LPNR_GetPlateColorImage(h,pTestDataObj->TestSmallImage);
}

int ready_and_send(TestDataObj *pTestDataObj)
{
    void *packet_buf = malloc(1024*1024*2);
    int len_head;
    int ret = 0;

    if(packet_buf == NULL)
    {
        printf("malloc packet_buf fail!\n");
        return -1;
    }

    memset(packet_buf,0,(1024*1024*2));

    len_head = sizeof(StCommandPacketHeader)+sizeof(StCommandContentHeader)+sizeof(stMainPacketHeader) \
        +sizeof(stSingleOBJPacket)+sizeof(stSubPacketHeader)+sizeof(CarDataPacket)+sizeof(stSubPacketHeader);

    memcpy(packet_buf,pTestDataObj,len_head);

    memcpy(packet_buf+len_head,pTestDataObj->TestBinImage,pTestDataObj->BinImagesz);

    len_head = len_head + pTestDataObj->BinImagesz;
    memcpy(packet_buf+len_head,&(pTestDataObj->TestSubPacketHeader2),sizeof(stSubPacketHeader));

    len_head = len_head + sizeof(stSubPacketHeader);
    memcpy(packet_buf+len_head,pTestDataObj->TestBigImage,pTestDataObj->BigImagesz);

    len_head = len_head + pTestDataObj->BigImagesz;
    memcpy(packet_buf+len_head,&(pTestDataObj->TestSubPacketHeader3),sizeof(stSubPacketHeader));

    len_head = len_head + sizeof(stSubPacketHeader);
    memcpy(packet_buf+len_head,pTestDataObj->TestSmallImage,pTestDataObj->SmallImagesz);

    len_head = len_head + pTestDataObj->SmallImagesz;
    memcpy(packet_buf+len_head,&(pTestDataObj->Tail),4);

    len_head = len_head+4;

    printf("BinImagesz = %d\n",pTestDataObj->BinImagesz);
    printf("BigImagesz = %d\n",pTestDataObj->BigImagesz);
    printf("SmallImagesz = %d\n",pTestDataObj->SmallImagesz);
    printf("len_head = %d\n",len_head);
    printf("pTestDataObj->CarDataPacket.OCRResult = %s \n",pTestDataObj->CarDataPacket.OCRResult);

    if(serv_sock)
    {
        printf("send data!\n");
        //ret = sock_write(serv_sock,(void *)packet_buf,len_head);
       ret =  sock_write_n_bytes(serv_sock,(char *)packet_buf,len_head);
       printf("ret = %d\n",ret);
        
    }

    free(packet_buf);
    
}

int sync_packet(HANDLE h)
{

    int packet_len = 0;
	TestDataObj *pTestDataObj = (TestDataObj *)malloc(sizeof(TestDataObj));	
    client_node *node = NULL;

    if(pTestDataObj == NULL)
    {
        printf("malloc TestDataObj fail!\n");
        return -1;
    }

    memset(pTestDataObj,0,sizeof(TestDataObj));

    packet_len = sizeof(StCommandPacketHeader)+sizeof(StCommandContentHeader)+sizeof(stMainPacketHeader)+sizeof(stSingleOBJPacket)+4*sizeof(stSubPacketHeader)+sizeof(CarDataPacket)+LPNR_GetPlateBinaryImageSize(h)+LPNR_GetCapturedImageSize(h)+LPNR_GetPlateColorImageSize(h)+4;

    printf("packet_len = %d\n",packet_len);
    //初始化大包头
    pTestDataObj->TestPacketHeader.ui32Header = bigpacket_header;
    pTestDataObj->TestPacketHeader.ui32Version = bigpacket_version;
    pTestDataObj->TestPacketHeader.ui32MessageLength = packet_len;
    pTestDataObj->TestPacketHeader.ui16ReceiveId = 0x08;
    pTestDataObj->TestPacketHeader.ui16SendId = 0x01;
    pTestDataObj->TestPacketHeader.ui32MessageId = 1;
    pTestDataObj->TestPacketHeader.ui32ResponseId = 0;
    pTestDataObj->TestPacketHeader.ui32MessageType = 0x00000004;

    pTestDataObj->TestPacketHeader.ui32FieldCount = 1;

    //初始化 field 头
    pTestDataObj->TestContentHeader.ui32MagicNumber = 0xAA55A55A;
    pTestDataObj->TestContentHeader.unionCommandField.ui32CmdFiled = 0x01000000;
    pTestDataObj->TestContentHeader.ui32CommandId = 0x00000000;
    pTestDataObj->TestContentHeader.ui32FDCode = 0x00000000;
    pTestDataObj->TestContentHeader.ui32CommandLength = sizeof(TestDataObj) - sizeof(StCommandPacketHeader) - sizeof(StCommandContentHeader);

    //事件信息数据主包头
    pTestDataObj->TestMainPacketHeader.ui32Header = 0x88776655;
    pTestDataObj->TestMainPacketHeader.ui32Version = 0x00010002;
    pTestDataObj->TestMainPacketHeader.ui32Type = 0x00000001;
    pTestDataObj->TestMainPacketHeader.ui32Length = pTestDataObj->TestContentHeader.ui32CommandLength - sizeof(stMainPacketHeader);
    pTestDataObj->TestMainPacketHeader.ui32ObjNum = 1;  //一个目标索引

    //单个目标的所有子包信息  这里只有一个目标
    pTestDataObj->TestSingleOBJPacket.ui32Header = 0x44332211;
    pTestDataObj->TestSingleOBJPacket.ui32TableLen =  72;  
    pTestDataObj->TestSingleOBJPacket.ui32Serial = 1;  //只有一个索引
    pTestDataObj->TestSingleOBJPacket.ui32SubPacketCnt = 4;  //4个子包

    pTestDataObj->TestSingleOBJPacket.strcSubPacketAttr[0].ui32SubPacketType = 0x03;  // 识别子包
    pTestDataObj->TestSingleOBJPacket.strcSubPacketAttr[0].ui32SubPacketOffset = 0;
    pTestDataObj->TestSingleOBJPacket.strcSubPacketAttr[0].ui32SubPacketSize = sizeof(CarDataPacket);

    pTestDataObj->TestSingleOBJPacket.strcSubPacketAttr[1].ui32SubPacketType = 0x01;  // 车牌二值图
    pTestDataObj->TestSingleOBJPacket.strcSubPacketAttr[1].ui32SubPacketOffset = &(pTestDataObj->TestSubPacketHeader1)-&(pTestDataObj->TestSubPacketHeader0);
    pTestDataObj->TestSingleOBJPacket.strcSubPacketAttr[1].ui32SubPacketSize = LPNR_GetPlateBinaryImageSize(h);

    pTestDataObj->TestSingleOBJPacket.strcSubPacketAttr[2].ui32SubPacketType = 0x01;  // 全景图
    pTestDataObj->TestSingleOBJPacket.strcSubPacketAttr[2].ui32SubPacketOffset = &(pTestDataObj->TestSubPacketHeader2)-&(pTestDataObj->TestSubPacketHeader1);
    pTestDataObj->TestSingleOBJPacket.strcSubPacketAttr[2].ui32SubPacketSize = LPNR_GetCapturedImageSize(h);

    pTestDataObj->TestSingleOBJPacket.strcSubPacketAttr[3].ui32SubPacketType = 0x01;  // 车牌彩色图
    pTestDataObj->TestSingleOBJPacket.strcSubPacketAttr[3].ui32SubPacketOffset = &(pTestDataObj->TestSubPacketHeader3)-&(pTestDataObj->TestSubPacketHeader2);
    pTestDataObj->TestSingleOBJPacket.strcSubPacketAttr[3].ui32SubPacketSize = LPNR_GetPlateColorImageSize(h);

    //初始化 子包0
    init_SubPacket_0(h,pTestDataObj);

    //初始化 子包1
    init_SubPacket_1(h,pTestDataObj);

    //初始化 子包2
    init_SubPacket_2(h,pTestDataObj);

    //初始化 子包3
    init_SubPacket_3(h,pTestDataObj);

    pTestDataObj->Tail = 0x550FF0AA;

    ready_and_send(pTestDataObj);

    free(pTestDataObj);
    free(pTestDataObj->TestBinImage);
    free(pTestDataObj->TestBigImage);
    free(pTestDataObj->TestSmallImage );
    printf("free pTestDataObj!\n");

    return 0;

}


int main(int argc, char const *argv[])
{
    
	int		fd;
    //int serv_sock;
	HANDLE hLPNR;

	if ( argc != 2 )
	{
		fprintf(stderr, "USUAGE: %s <ip addr>\n", argv[0] );
		return -1;
	}
	
	if ( (hLPNR=LPNR_Init(argv[1])) == NULL )
	{
		fprintf(stderr, "Invalid LPNR IP address: %s\r\n", argv[1] );
		return -1;
	}
	LPNR_SetCallBack( hLPNR, demo_event_handle );

    fd = sock_listen( 58002, NULL, 5 );
    printf("init socket!\n");
    //配置server socket
    while( 1 )
    {
        if( sock_dataready(fd,0) > 0 )
        {
            serv_sock = sock_accept( fd );
            printf("socket accept successful!\n");    
        }

        //TODO 

    } 

	LPNR_Terminate(hLPNR);
    return 0;
}
