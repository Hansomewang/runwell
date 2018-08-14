/***********************************************************
 * 作者:梁俊辉                                             *
 * 时间:2001年10月                                         *
 * 名称：myping.c                                          *
 * 说明:本程序用于演示ping命令的实现原理                   *
 ***********************************************************/
#include <stdio.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <string.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netdb.h>
#include <setjmp.h>
#include <errno.h>

#define PACKET_SIZE     4096
#define MAX_WAIT_TIME   8 
#define MAX_NO_PACKETS  3

static char sendpacket[PACKET_SIZE];
static char recvpacket[PACKET_SIZE];
static int sockfd,datalen=56;
static int nsend=0,nreceived=0;
static struct sockaddr_in dest_addr;
static pid_t pid;
static struct sockaddr_in from;
static struct timeval tvrecv;

static void statistics(int signo);
static unsigned short cal_chksum(unsigned short *addr,int len);
static int pack(int pack_no);
static void send_packet(void);
static void recv_packet(void);
static int unpack(char *buf,int len);
static void tv_sub(struct timeval *out,struct timeval *in);
static int online = 0;

void statistics(int signo)
{   
    printf("\n--------------------PING statistics-------------------\n");
	printf("receive id:%d \r\n", nreceived );
	if( nreceived > 0 )
		online = 1;
	else
		online = 0;
	close(sockfd);
}
/*校验和算法*/
static unsigned short cal_chksum(unsigned short *addr,int len)
{       
	int nleft=len;
        int sum=0;
        unsigned short *w=addr;
        unsigned short answer=0;
		
/*把ICMP报头二进制数据以2字节为单位累加起来*/
        while(nleft>1)
        {       sum+=*w++;
                nleft-=2;
        }
		/*若ICMP报头为奇数个字节，会剩下最后一字节。把最后一个字节视为一个2字节数据的高字节，这个2字节数据的低字节为0，继续累加*/
        if( nleft==1)
        {       *(unsigned char *)(&answer)=*(unsigned char *)w;
                sum+=answer;
        }
        sum=(sum>>16)+(sum&0xffff);
        sum+=(sum>>16);
        answer=~sum;
        return answer;
}
/*设置ICMP报头*/
static int pack(int pack_no)
{
	int packsize;
    struct icmp *icmp;
    struct timeval *tval;
    icmp=(struct icmp*)sendpacket;
    icmp->icmp_type=ICMP_ECHO;
    icmp->icmp_code=0;
    icmp->icmp_cksum=0;
    icmp->icmp_seq=pack_no;
    icmp->icmp_id=pid;
    packsize=8+datalen;
    tval= (struct timeval *)icmp->icmp_data;
    gettimeofday(tval,NULL);    /*记录发送时间*/
    icmp->icmp_cksum=cal_chksum( (unsigned short *)icmp,packsize); /*校验算法*/
    return packsize;
}

/*发送三个ICMP报文*/
static void send_packet()
{       
	int packetsize;
     while( nsend<MAX_NO_PACKETS)
     {
		 nsend++;
         packetsize=pack(nsend); /*设置ICMP报头*/
         if( sendto(sockfd,sendpacket,packetsize,0,
                (struct sockaddr *)&dest_addr,sizeof(dest_addr) )<0  )
          {
			  perror("sendto error");
              continue;
          }
          usleep( 100000 ); /*每隔一秒发送一个ICMP报文*/
        }
}

/*接收所有ICMP报文*/
static void recv_packet()
{ 
	int n,fromlen;
    extern int errno;
    signal(SIGALRM, statistics);
    fromlen=sizeof(from);
	int receive = 0;
    while( receive < nsend )
    {
		alarm(MAX_WAIT_TIME);
        if( (n=recvfrom(sockfd,recvpacket,sizeof(recvpacket),0, (struct sockaddr *)&from,(socklen_t*)&fromlen)) <= 0 )
		{
			if( errno == SIGINT )
			{
				printf("recve SIGINT Signal\r\n");
				break;
			}
		}
		if( errno == SIGINT )
		{
			printf("receive int signal!\r\n");
			break;
		}
		receive++;
		if( n < 8 )
			continue;
 	    gettimeofday(&tvrecv,NULL);  /*记录接收时间*/
        if( unpack(recvpacket,n)==-1)
			continue;
		nreceived++;
     }
	close( sockfd );
}

/*剥去ICMP报头*/
static int unpack(char *buf,int len)
{
	int iphdrlen;
   	struct ip *ip;
    struct icmp *icmp;
    struct timeval *tvsend;
    double rtt;

    ip=(struct ip *)buf;
    iphdrlen=ip->ip_hl<<2;    /*求ip报头长度,即ip报头的长度标志乘4*/
    icmp=(struct icmp *)(buf+iphdrlen);  /*越过ip报头,指向ICMP报头*/
        len-=iphdrlen;            /*ICMP报头及ICMP数据报的总长度*/
        if( len < 8 )                /*小于ICMP报头长度则不合理*/
        {       printf("ICMP packets\'s length:%d  is less than 8\n", len );
                return -1;
        }
        /*确保所接收的是我所发的的ICMP的回应*/
        if( (icmp->icmp_type==ICMP_ECHOREPLY) && (icmp->icmp_id==pid) )
        {       tvsend=(struct timeval *)icmp->icmp_data;
                tv_sub(&tvrecv,tvsend);  /*接收和发送的时间差*/
                rtt=tvrecv.tv_sec*1000+tvrecv.tv_usec/1000;  /*以毫秒为单位计算rtt*/
                /*显示相关信息*/
                printf("%d byte from %s: icmp_seq=%u ttl=%d rtt=%.3f ms\n",
                        len,
                        inet_ntoa(from.sin_addr),
                        icmp->icmp_seq,
                        ip->ip_ttl,
                        rtt);
 			online = 1; 
        }
        else    return -1;
		return 0;
}

/*两个timeval结构相减*/
static void tv_sub(struct timeval *out,struct timeval *in)
{       if( (out->tv_usec-=in->tv_usec)<0)
        {       --out->tv_sec;
                out->tv_usec+=1000000;
        }
        out->tv_sec-=in->tv_sec;
}

int iponline( const char *strip)
{
	if( NULL == strip || inet_addr(strip ) == INADDR_NONE )
		return -1;
	struct protoent *protocol;
	int size=50*1024;
	if( (protocol=getprotobyname("icmp") )==NULL)  
		return -1;
	/*生成使用ICMP的原始套接字,这种套接字只有root才能生成*/
	if( (sockfd=socket(AF_INET,SOCK_RAW,protocol->p_proto) )<0)
	    return -1;
	setsockopt(sockfd,SOL_SOCKET,SO_RCVBUF,&size,sizeof(size) );
	memset( (void*)&dest_addr, 0, sizeof( dest_addr ));
    dest_addr.sin_family = AF_INET;
	dest_addr.sin_addr.s_addr = inet_addr( strip );
	send_packet();  /*发送所有ICMP报文*/
	recv_packet();  /*接收所有ICMP报文*/
	return online;
}

#if 0
int main(int argc, char *argv[])
{
	printf("%s online ? :%d \r\n",argv[1], iponline( argv[1] ) );
	return 0;
}
#endif
