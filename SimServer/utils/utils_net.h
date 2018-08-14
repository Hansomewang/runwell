/***************************************************************************
                          utils_net.h  -  description
                             -------------------
    begin                : Thu Dec 20 2001
    copyright            : (C) 2001 by Liming Xie
    email                : liming_xie@yahoo.com
    
    Revised by		: Thomas Chang
    						  : Many revisions
 ***************************************************************************/

#ifndef _UTILS_NET_H_
#define _UTILS_NET_H_

#ifdef ENABLE_IPV6
#include "utils_net6.h"
#else

#define		MSEC_PER_SEC		1000
#define		USEC_PER_SEC		1000000

#ifdef __cplusplus
extern "C" {
#endif

#ifdef linux
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr_in SOCKADDR_STORAGE;
typedef struct sockaddr	SOCKADDR;
typedef struct in_addr  IN_ADDR;		// sin_addr member of SOCKADDR_IN
#define INET_ADDRSTRLEN	16
#define SOCKADDR_LEN(ss)	sizeof(SOCKADDR_IN)
#define INVALID_SOCKET		(0xffffffff)
#define SOCKET_ERROR			(-1)
#endif

/* ------------- common socket related input/output ---------- */

extern int sock_connect( const char *host, int port );
extern int sock_connect0( unsigned long ul_addr, int port );

/* ipbind can be NULL, for any address to bind */
extern int sock_listen( int port, const char *ipbind, int backlog );
extern int sock_accept( int fd_listen );

/* ensure safe read/write, without EAGAIN/EINTR */

extern int sock_set_timeout( long time_ms );

extern int sock_read(int fd, void* buffer, int buf_size);

extern int sock_write(int fd, const void* buffer, int buf_size);

/* 
 * skip/read/peek input queue until 'eol' byte is encounted ('eol' is also read/peek into buffer)
 * return value is number of bytes has been read. buffer is terminated by '\0'
 */
extern int sock_drain_until( int fd, unsigned char *soh, int ns );
extern int sock_peek_until(int fd, char* buffer, int buf_size, int eol);
extern int sock_read_until(int fd, char *buffer, int buf_size, int eol);
#define sock_read_line(fd,buf,sz)	sock_read_until(fd,buf,sz,(int)'\n')
#define sock_peek_line(fd,buf,sz)	sock_read_until(fd,buf,sz,(int)'\n')
extern int sock_getc(int fd, int tout);

/* read EXACT n bytes from socket,
 * return: bytes read, or -1 when failed */
extern int sock_read_n_bytes(int fd, void* buffer, int n);
extern int sock_write_n_bytes(int fd, void *buffer, int n);

extern int sock_skip_n_bytes(int fd, int n);

extern int sock_is_connected( int fd );

extern int sock_udp_open();
extern int sock_udp_timeout( int sock, int nTimeOut );
extern int sock_udp_bindhost(int port, const char *host);
#define sock_udp_bind(p)	sock_udp_bindhost(p,NULL)
extern int sock_udp_broadcast( int fd, int enable );	// enable: 1 yes, 0 no
// create a UDP socket, send messgae then close it. ip==NULL is broadcasting
extern int sock_udp_send( const char *ip, int port, const void* msg, int len );
// send to single dstination addr. in 'paddr' ('port' is ignored) or broadcast to 'port' if 'paddr' is NULL.
extern int sock_udp_send0( int fd, SOCKADDR_IN *paddr, int port, const void *msg, int len );
// send to one or multiple destination. ndst==0 for broadcasting
extern int sock_udp_sendX( int fd, SOCKADDR_IN *paddr, int ndst, const void *msg, int len );
// receive a udp packet, fd should be bind to specific port. saIn is sender's net address.
extern int sock_udp_recv2( int fd, void *buf, int size, SOCKADDR *saIn, int *addrlen );
#define sock_udp_recv(fd,buf,sz,saddr)	sock_udp_recv2(fd,buf,sz,(SOCKADDR *)saddr,NULL)

// tout in msec
//  -1 means wait until data is ready.
//   0 means no wait at all. Just poll the status
extern int sock_dataready( int fd, int tout );
extern int sock_iqueue(int fd);		// any queued input data (length of data)
extern int sock_oqueue(int fd);		// any pending output data (bytes of pending data)

// clean all input data in socket.
// return: number of bytes eaten.
extern int sock_drain( int fd );
#define sock_close(s)		close(s)

/* get host IP address -- first non-loop (127.0.0.1) bound IP */
/* hostname: NULL for local host. Otherwise, the target host name
 *      buf: Output buffer of IP dot number string.
 * return:
 *      packed IP address
 */
extern unsigned long get_host_ip( const char *hostname, char *buf );
/* 
 * in following functions
 * if ifa_name is NULL, then return address of first adapter (exclude "lo")
 */
extern const char *get_ifadapter_name();
extern unsigned long get_ifadapter_ip( const char *ifa_name, char *buf, SOCKADDR_IN *netmask, SOCKADDR_IN *ip_bcast );
/*
 * ifconfig: config network interface adapter 
 * ifname: name of adapter, NULL mean first non lo interface (usually eth0)
 * ip, netmask, gwip: ip address, netmask and defaut gateway IP. NULL means do not change
 */
extern int ifconfig(const char *ifname, const char *ipaddr, const char *netmask, const char *gw);
extern int get_netaddr( const char *ifname, SOCKADDR_IN *netaddr );
extern int set_netaddr( const char *ifname, SOCKADDR_IN *netaddr );
extern int get_netmask( const char *ifname, SOCKADDR_IN *netmask );
extern int set_netmask( const char *ifname, SOCKADDR_IN *netmask );
// default gateway operation:
// 
// 'get_gatewayaddr'- return value is IPv4 gateway IP in network byte order
//    this addr. also assigned to 'buf' in readable format if 'buf' is not null.
// 'add_defgateway' - add a default gateway
// 'del_defgateway' - delete a default gateway (gwaddr can be INARRY_ANY, for any defaut gateway)
extern unsigned long get_gatewayaddr( const char *ifname, char *buf );
extern int add_default_gateway(const char *ifname, SOCKADDR_IN *pgwaddr);
extern int del_default_gateway(const char *ifname, SOCKADDR_IN *pgwaddr);
#define del_defgateway(af)	del_default_gateway(NULL,NULL)

extern int get_mac_addr( const char *ifname, unsigned char *hwaddr );		// hwaddr: 6 bytes mac addr.
extern int set_mac_addr( const char *ifname, unsigned char *hwaddr );
extern const char *mac2string(unsigned char *hwaddr);

/* get peer name (usually, IP adddress, unless host name for the ip is given in dns server or
 * 'hosts' file.
 *
 */
extern const char* sock_getpeername( int fd, int *port );
extern int sock_getpeeraddr(int fd, SOCKADDR *saddr);
extern const char *sockaddr_in_tostring(SOCKADDR_IN *addr, int withport);
#define sockaddr_tostring(addr,withport)	sockaddr_in_tostring((SOCKADDR_IN*)addr,withport)
int is_sockaddr_equal(void *ss1, void *ss2);

// helper - same as inet_addr. but caller does not have to include many socket header files.
extern unsigned long INET_ATON( const char *ipstr );
extern const char *INET_NTOA( unsigned long ip );
extern const char *INET_NTOA2( unsigned long ip, char *numdot );
#define IP_to_str(ip,str)		INET_NTOA2(ip,str)		// backward compatible. Try not to use in new code
extern const char *SOCKADDR_IN_TOSTRING(SOCKADDR_IN addr);

#define SOCKADDR_IN_ADDR(saddr)		( saddr.sin_addr.s_addr )
#define SOCKADDR_IN_PORT(saddr)		( saddr.sin_port )
#define SOCKADDR_IN_FAMILY(saddr)	( saddr.sin_family )

#define SOCKADDR_IN_SET(saddr,ip,port) \
	do { \
			saddr.sin_family = AF_INET; \
			saddr.sin_addr.s_addr = ip; \
			saddr.sin_port = port; \
	} while(0)

#define SOCKADDR_IN_GET(saddr,ip,port) \
	do { \
			ip = saddr.sin_addr.s_addr; \
			port = saddr.sin_port; \
	} while(0)

#define SOCKADDR_IN_SET_FROMHOST(saddr,ip,port) \
	do { \
			saddr.sin_family = AF_INET; \
			saddr.sin_addr.s_addr = htonl(ip); \
			saddr.sin_port = htons(port); \
	} while(0)

#define SOCKADDR_IN_GET_TOHOST(saddr,ip,port) \
	do { \
			ip = ntohl(saddr.sin_addr.s_addr); \
			port = ntohs(saddr.sin_port); \
	} while(0)
		
// following macro makes source code IPV6 and IPV4 compatible.
#define is_sockaddr_v4(saddr)				(1)
#define is_sockaddr_v4mapped(saddr)	(0)
#define sockaddr_v6to4(v6addr,v4addr)			// do nothing

#ifdef __cplusplus
};
#endif

#endif		// #ifdef ENABLE_IPV6

#endif /* _UTILS_NET_H_ */
