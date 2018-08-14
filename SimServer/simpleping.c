/***********************************************************
 * ����:������                                             *
 * ʱ��:2001��10��                                         *
 * ���ƣ�myping.c                                          *
 * ˵��:������������ʾping�����ʵ��ԭ��                   *
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
/*У����㷨*/
static unsigned short cal_chksum(unsigned short *addr,int len)
{       
	int nleft=len;
        int sum=0;
        unsigned short *w=addr;
        unsigned short answer=0;
		
/*��ICMP��ͷ������������2�ֽ�Ϊ��λ�ۼ�����*/
        while(nleft>1)
        {       sum+=*w++;
                nleft-=2;
        }
		/*��ICMP��ͷΪ�������ֽڣ���ʣ�����һ�ֽڡ������һ���ֽ���Ϊһ��2�ֽ����ݵĸ��ֽڣ����2�ֽ����ݵĵ��ֽ�Ϊ0�������ۼ�*/
        if( nleft==1)
        {       *(unsigned char *)(&answer)=*(unsigned char *)w;
                sum+=answer;
        }
        sum=(sum>>16)+(sum&0xffff);
        sum+=(sum>>16);
        answer=~sum;
        return answer;
}
/*����ICMP��ͷ*/
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
    gettimeofday(tval,NULL);    /*��¼����ʱ��*/
    icmp->icmp_cksum=cal_chksum( (unsigned short *)icmp,packsize); /*У���㷨*/
    return packsize;
}

/*��������ICMP����*/
static void send_packet()
{       
	int packetsize;
     while( nsend<MAX_NO_PACKETS)
     {
		 nsend++;
         packetsize=pack(nsend); /*����ICMP��ͷ*/
         if( sendto(sockfd,sendpacket,packetsize,0,
                (struct sockaddr *)&dest_addr,sizeof(dest_addr) )<0  )
          {
			  perror("sendto error");
              continue;
          }
          usleep( 100000 ); /*ÿ��һ�뷢��һ��ICMP����*/
        }
}

/*��������ICMP����*/
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
 	    gettimeofday(&tvrecv,NULL);  /*��¼����ʱ��*/
        if( unpack(recvpacket,n)==-1)
			continue;
		nreceived++;
     }
	close( sockfd );
}

/*��ȥICMP��ͷ*/
static int unpack(char *buf,int len)
{
	int iphdrlen;
   	struct ip *ip;
    struct icmp *icmp;
    struct timeval *tvsend;
    double rtt;

    ip=(struct ip *)buf;
    iphdrlen=ip->ip_hl<<2;    /*��ip��ͷ����,��ip��ͷ�ĳ��ȱ�־��4*/
    icmp=(struct icmp *)(buf+iphdrlen);  /*Խ��ip��ͷ,ָ��ICMP��ͷ*/
        len-=iphdrlen;            /*ICMP��ͷ��ICMP���ݱ����ܳ���*/
        if( len < 8 )                /*С��ICMP��ͷ�����򲻺���*/
        {       printf("ICMP packets\'s length:%d  is less than 8\n", len );
                return -1;
        }
        /*ȷ�������յ����������ĵ�ICMP�Ļ�Ӧ*/
        if( (icmp->icmp_type==ICMP_ECHOREPLY) && (icmp->icmp_id==pid) )
        {       tvsend=(struct timeval *)icmp->icmp_data;
                tv_sub(&tvrecv,tvsend);  /*���պͷ��͵�ʱ���*/
                rtt=tvrecv.tv_sec*1000+tvrecv.tv_usec/1000;  /*�Ժ���Ϊ��λ����rtt*/
                /*��ʾ�����Ϣ*/
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

/*����timeval�ṹ���*/
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
	/*����ʹ��ICMP��ԭʼ�׽���,�����׽���ֻ��root��������*/
	if( (sockfd=socket(AF_INET,SOCK_RAW,protocol->p_proto) )<0)
	    return -1;
	setsockopt(sockfd,SOL_SOCKET,SO_RCVBUF,&size,sizeof(size) );
	memset( (void*)&dest_addr, 0, sizeof( dest_addr ));
    dest_addr.sin_family = AF_INET;
	dest_addr.sin_addr.s_addr = inet_addr( strip );
	send_packet();  /*��������ICMP����*/
	recv_packet();  /*��������ICMP����*/
	return online;
}

#if 0
int main(int argc, char *argv[])
{
	printf("%s online ? :%d \r\n",argv[1], iponline( argv[1] ) );
	return 0;
}
#endif
