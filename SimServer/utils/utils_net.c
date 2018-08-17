/***************************************************************************
                          utils_net.c  -  description
                             -------------------
    begin                : Thu Dec 20 2001
    copyright            : (C) 2001 by Liming Xie
    email                : liming_xie@yahoo.com

    Version: 	1.0.1
    Date:	2005-03-06
    Author:	Thomas Chang

    1. Add the "get_host_ip" and "get_ifadapter_ip" functions

 ***************************************************************************/
#ifndef ENABLE_IPV6
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/poll.h>
#include <sys/socket.h>   
#include <linux/sockios.h>		// for sock_oqueue SIOCOUTQ
#include <sys/ioctl.h>   
#include <netinet/in.h>
#include <ifaddrs.h>
//#include <net/if.h>
#include <net/ethernet.h>
#include <arpa/inet.h>
#include <linux/if_arp.h>
#include <linux/route.h>
#ifndef min
#define min( x, y ) ( (x) > (y) ? (y) : (x) )
#endif
#ifndef max
#define max( x, y ) ( (x) < (y) ? (y) : (x) )
#endif
#include "utils_net.h"
#include "longtime.h"

//#define NET_TIMEDOUT_ENABLE

#define LOCAL_HOST	0x0100007fL		// this is 127.0.0.1

static	long	NET_TIMEDOUT_MSEC = 60000;		/* 60 sec == 1 min */

int sock_set_timeout( long time_ms )
{
	long	old = NET_TIMEDOUT_MSEC;

	NET_TIMEDOUT_MSEC = time_ms;

	return old;
}

int sock_connect( const char *host, int port )
{
	struct sockaddr_in	destaddr;
  struct hostent 		*hp;
	int 			fd = 0;

	memset( & destaddr, 0, sizeof(destaddr) );
	destaddr.sin_family = AF_INET;
	destaddr.sin_port = htons( (short)port );
  if ((inet_aton(host, & destaddr.sin_addr)) == 0)
  {
      hp = gethostbyname(host);
      if(! hp) return -1;
      memcpy (& destaddr.sin_addr, hp->h_addr, sizeof(destaddr.sin_addr));
  }

	fd = socket(PF_INET, SOCK_STREAM, 0);
	if (fd < 0)   return -1;

	if ( connect(fd, (struct sockaddr *)&destaddr, sizeof(destaddr)) < 0 )
	{
		close(fd);
		return -1;
	}
	return fd;
}

int sock_connect0( unsigned long ul_addr, int port )
{
	struct sockaddr_in	destaddr;
  struct hostent 		*hp;
	int 			fd = 0;

	memset( & destaddr, 0, sizeof(destaddr) );
	destaddr.sin_family = AF_INET;
	destaddr.sin_port = htons( (short)port );
	destaddr.sin_addr.s_addr = ul_addr;
	
	fd = socket(PF_INET, SOCK_STREAM, 0);
	if (fd < 0)   return -1;

	if ( connect(fd, (struct sockaddr *)&destaddr, sizeof(destaddr)) < 0 )
	{
		close(fd);
		return -1;
	}
	return fd;
}

int sock_listen( int port, const char *ipbind, int backlog )
{
	struct sockaddr_in 	my_addr;
	int 			fd, tmp = 1;

	fd = socket(AF_INET,SOCK_STREAM,0);
	if (fd < 0) 
  	 return -1;

 setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &tmp, sizeof(tmp));

	memset(&my_addr, 0, sizeof(my_addr));
  my_addr.sin_family = AF_INET;
  my_addr.sin_port = htons ((short)port);
	if( ipbind != NULL ) {
		inet_aton( ipbind, & my_addr.sin_addr );
	} else {
  	my_addr.sin_addr.s_addr = htonl (INADDR_ANY);
	}

	if( 0 == bind (fd, (struct sockaddr *) &my_addr, sizeof (my_addr)) )
	{
		if( 0 == listen(fd, backlog) ) {
			return fd;
		}
	}

	close(fd);
	return -1;
}

int sock_accept( int fd_listen )
{
	struct sockaddr_in	fromaddr;
	socklen_t		addrlen = sizeof(fromaddr);
	int		fd;
	int		tmp=1;

	if ( (fd = accept( fd_listen, (struct sockaddr *) & fromaddr, & addrlen )) != -1 )
		setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &tmp, sizeof(tmp));
	return fd;
}

const char* sock_getpeername( int fd, int *port )
{
	struct sockaddr_in	peer_addr;
	socklen_t		addrlen = sizeof(peer_addr);
	
	if ( getpeername( fd,  (struct sockaddr *) &peer_addr, &addrlen ) == 0 )
	{
		if ( port ) *port = ntohs( peer_addr.sin_port );
		return inet_ntoa( peer_addr.sin_addr );
	}
	else
		return "";			
}

int sock_getpeeraddr(int fd, struct sockaddr *saddr)
{
	SOCKADDR_STORAGE	ss;
	socklen_t		addrlen = sizeof(ss);
	int rc = -1;
	
	if ( getpeername( fd,  (struct sockaddr *) &ss, &addrlen ) == 0 )
	{
		memcpy(saddr, &ss, addrlen);
		rc = AF_INET;
	}
	return rc;
}

const char *sockaddr_in_tostring(SOCKADDR_IN *addr, int withport)
{
	static char straddr[INET_ADDRSTRLEN+6];
	unsigned short port;
	
	struct in_addr inaddr;
	inaddr.s_addr = addr->sin_addr.s_addr;
	port = addr->sin_port;
	port = ntohs(port);
	strcpy(straddr, inet_ntoa(inaddr));
	if ( withport )
		sprintf(straddr+strlen(straddr), ":%u", (unsigned short)port);
	return straddr;
}

int is_sockaddr_equal(void *ss1, void *ss2)
{
	SOCKADDR_IN *sa1 = (SOCKADDR_IN *)ss1;
	SOCKADDR_IN *sa2 = (SOCKADDR_IN *)ss2;
	return sa1->sin_addr.s_addr == sa2->sin_addr.s_addr;
}


#ifdef linux
int sock_wait_for_io_or_timeout(int fd, int for_read, long timeout)
{
	struct pollfd 	poll_table[1], *poll_entry = & poll_table[0];
	int				ret;

	poll_entry->fd = fd;
	poll_entry->events = for_read ? POLLIN : POLLOUT;
    do {
		ret = poll(poll_table, 1, timeout);
	} while (ret == -1);

	return (poll_entry->revents>0) ? 0 : -1;
}
#else
int sock_wait_for_io_or_timeout(int fd, int for_read, long timeout)
{
	fd_set fs;
	struct timeval tv;
	int				ret;
	tv.tv_sec = timeout / 1000;
	tv.tv_usec = timeout % 1000 * 1000;
	FD_ZERO( &fs );
	FD_SET( fd, &fs );

	do {
		if( for_read )
			ret = select( fd + 1, &fs, NULL, NULL, &tv );
		else
			ret = select( fd + 1, NULL, &fs, NULL, &tv );
	} while( ret == -1 );

	return (ret) ? 0 : -1;
}
#endif

int sock_read( int fd, void* buffer, int buf_size )
{
#ifdef NET_TIMEDOUT_ENABLE
	if( 0 != sock_wait_for_io_or_timeout(fd, 1, NET_TIMEDOUT_MSEC) ) return -1;
#endif

	return recv(fd, buffer, buf_size, 0);
}

int sock_write( int fd, const void* buffer, int buf_size )
{
#ifdef NET_TIMEDOUT_ENABLE
	if( 0 != sock_wait_for_io_or_timeout(fd, 0, NET_TIMEDOUT_MSEC) ) return -1;
#endif
	
	return send(fd, buffer, buf_size, MSG_NOSIGNAL);
}

int sock_drain_until( int fd, unsigned char *soh, int ns )
{
	char buffer[1024];
	char *ptr;
	int i, len, nskip=0;
	while ( sock_dataready(fd,100) )
	{
		/* peek the data, but not remove the data from the queue */
		if ( (len = recv(fd, buffer, sizeof(buffer), MSG_PEEK)) == -1 || len<=0 )
			return -1;

		/* try to locate soh sequence in buffer */
		for(i=0, ptr=buffer; i<=len-ns; i++, ptr++) 
			if ( 0==memcmp( ptr, soh, ns) )
				break;
		nskip += ptr - buffer;
		if ( i > len-ns )
			recv( fd, buffer, len, 0 );
		else
			recv( fd, buffer, (ptr-buffer), 0 );
		if ( i <= len-ns )
			break;
	}
	return nskip;
}

int sock_getc( int sock, int tout )
{
	int		rc;
	unsigned char	ch = 0;

    if ( (rc = sock_dataready( sock, tout )) > 0 )
    {
    	rc = sock_read( sock, &ch, 1 );
        return rc == 1 ? ch : -2;
    }
    /* timed out or I/O error */
   	return rc-1;	/* 0 -> -1, -1 -> -2 */
}

int sock_read_until(int fd, char* buffer, int buf_size, int eol)
{
	int len, size, buf_left, i, found=0;
	char *ptr, *cmpptr;

	for (ptr=buffer,size=0,buf_left=buf_size-1; buf_left>0; /**/) 
	{
#ifdef NET_TIMEDOUT_ENABLE
		if( 0 != sock_wait_for_io_or_timeout(fd, 1, NET_TIMEDOUT_MSEC) ) return -1;
#endif
		/* peek the data, but not remove the data from the queue */
		len = recv(fd, ptr, buf_left, MSG_PEEK);
		if( (len == 0) || (len < 0 && (errno != EAGAIN || errno != EINTR) ) )
			return -1;
		else if( len < 0 )
			continue;

		/* try finding 'eol' in the data */
		for(i=0, cmpptr=ptr; i<len; i++, cmpptr++) 
		{
			if(*cmpptr == eol) 
			{
				len = i+1;
				found = 1;
				break;
			}
		}

		/* get the data we need, and remove from the queue */
		len = recv(fd, ptr, len, 0);
		if( (len == 0) || (len < 0 && (errno != EAGAIN || errno != EINTR) ) )
			return -1;
		else if( len < 0 )
			continue;

		ptr += len;
		size += len;
		buf_left -= len;

		if( found ) {
			buffer[size] = '\0';
			return size;
		}
	}
	return 0;
}

// copy w/o remove data from socket buffer
int sock_peek_until(int fd, char* buffer, int buf_size, int eol)
{
	int len, size, buf_left, i, found=0;
	char *ptr, *cmpptr;

	for (ptr=buffer,size=0,buf_left=buf_size-1; buf_left>0; /**/) 
	{
		/* peek the data, but not remove the data from the queue */
		len = recv(fd, ptr, buf_left, MSG_PEEK);
		if( (len == 0) || (len < 0 && (errno != EAGAIN || errno != EINTR) ) )
			return -1;
		else if( len < 0 )
			continue;

		/* try finding 'eol' in the data */
		for(i=0, cmpptr=ptr; i<len; i++, cmpptr++) 
		{
			if(*cmpptr == eol) {
				len = i+1;
				found = 1;
				break;
			}
		}
		ptr += len;
		size += len;
		buf_left -= len;

		if( found ) {
			buffer[size] = '\0';
			return size;
		}
	}
	return 0;
}

int sock_read_n_bytes(int fd, void* buffer, int n)
{
	char *ptr = buffer;
	int len;
	while( n > 0 ) {
#ifdef NET_TIMEDOUT_ENABLE
		if( 0 != sock_wait_for_io_or_timeout(fd, 1, NET_TIMEDOUT_MSEC) ) return -1;
#endif

		//len = recv(fd, ptr, n, MSG_WAITALL);
		len = recv(fd, ptr, n, 0);
		if( len==0)			// socket broken. sender close it or directly connected device (computer) power off
			return -1;
		if( len < 0 ) {
			if( errno == EAGAIN || errno == EINTR ) continue;
			else {
				return -1;
			}
		}
		ptr += len;
		n -= len;
	}
	return (ptr-(char*)buffer);
}

int sock_write_n_bytes(int fd, void* buffer, int size)
{
	int len;
	char *ptr = (char *)buffer;
	
	while ( size > 0 )
	{
		len = send(fd, ptr, min(size,1024), MSG_NOSIGNAL);
		if( (len == 0) || (len < 0 && (errno != EAGAIN || errno != EINTR) ) )
			return -1;
		else if( len < 0 )		// this muust be due to EAGAIN or EINTR (send is interrupted)
			continue;

		ptr += len;
		size -= len;
	}
	return ptr - (char *)buffer;
}

int sock_skip_n_bytes(int fd, int n)
{
	char buffer[ 1024 ];
	int len;
	int left = n;
	while( left > 0 ) {
#ifdef NET_TIMEDOUT_ENABLE
		if( 0 != sock_wait_for_io_or_timeout(fd, 1, NET_TIMEDOUT_MSEC) ) return -1;
#endif
		//len = recv(fd, ptr, n, MSG_WAITALL);
		len = recv(fd, buffer, min( n, sizeof( buffer ) ), 0);
		if( len == 0 ) break;
		if( len < 0 )
		{
			if( errno == EAGAIN || errno == EINTR )
				continue;
			else
				return -1;
		}
		left -= len;
	}
	return (n-left);
}

// drain all data currently in socket input buffer
int sock_drain( int fd )
{
	int	rlen, n = 0;
	char	buf[ 1024 ];

	while ( sock_dataready( fd, 0 ) )
	{
		rlen = recv(fd, buf, sizeof(buf), 0);
		if ( rlen > 0 )
			n += rlen;
		else
			return -1;
	}
	return n;
}

int sock_is_connected( int fd )
{
	fd_set set;
	struct timeval val;
	int ret;

	FD_ZERO( & set );
	FD_SET( fd, & set );
	val.tv_sec = 0;
	val.tv_usec = 0;
	ret = select( fd +1, & set, NULL, NULL, & val );
	if( ret > 0 ) {
		// try to peek data
		char buf[8];
		ret = recv( fd, buf, 8, MSG_NOSIGNAL | MSG_PEEK );
		if( ret <= 0 ) return 0;
	} else if ( ret < 0 ) {
		// select error
		return 0;
	} else {
		// select timeout
	}

	return 1;
}

//================================ U D P ===========================
int sock_udp_open()
{
	return socket(AF_INET,SOCK_DGRAM,0);		
}

int sock_udp_timeout( int sock, int nTimeOut )
{
	return setsockopt( sock, SOL_SOCKET, SO_RCVTIMEO, &nTimeOut, sizeof ( nTimeOut ));
}

int sock_udp_bindhost(int port,const char *host)
{
	struct sockaddr_in 	my_addr;
	int			tmp=0;
	int			udp_fd;

	// signal init, to avoid app quit while pipe broken
//	signal(SIGPIPE, SIG_IGN);

	if ( (udp_fd = socket( AF_INET, SOCK_DGRAM, 0 )) >= 0 )
	{
		setsockopt( udp_fd, SOL_SOCKET, SO_REUSEADDR, &tmp, sizeof(tmp));

		memset(&my_addr, 0, sizeof(my_addr));
		my_addr.sin_family = AF_INET;
		my_addr.sin_port = htons ((short)port);
		if ( host != NULL )
			my_addr.sin_addr.s_addr = inet_addr(host);
		else
			my_addr.sin_addr.s_addr = INADDR_ANY;
		if (bind ( udp_fd, (struct sockaddr *) &my_addr, sizeof (my_addr)) < 0)
		{
    	close( udp_fd );
    	udp_fd = -EIO;
  	}
	}
  return udp_fd;	
}

int sock_udp_broadcast( int fd, int enable )
{
	return setsockopt( fd, SOL_SOCKET, SO_BROADCAST, &enable, sizeof(enable) );
}
	
int sock_udp_send( const char *ip, int port, const void* msg, int len )
{
	SOCKADDR_IN	udp_addr;
	int	sockfd, ret;

	sockfd = socket( AF_INET, SOCK_DGRAM, 0 );
	if( sockfd <= 0 ) return -1;

  //setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &tmp, sizeof(tmp));

	memset( & udp_addr, 0, sizeof(udp_addr) );
	udp_addr.sin_family = AF_INET;
	if ( ip==NULL )
	{
		udp_addr.sin_addr.s_addr = INADDR_BROADCAST;
		sock_udp_broadcast(sockfd,1);
	}
	else
		inet_aton( ip, &udp_addr.sin_addr );
	udp_addr.sin_port = htons( (short)port );

	ret = sendto( sockfd, msg, len, 0, (const struct sockaddr *)&udp_addr, sizeof(udp_addr) );
	close( sockfd );
	return ret;
}

int sock_udp_send0( int fd, SOCKADDR_IN *paddr, int port, const void* msg, int len )
{
 SOCKADDR_IN	udp_addr;

	if ( paddr == NULL )
	{
		paddr = &udp_addr;
		memset( & udp_addr, 0, sizeof(udp_addr) );
		udp_addr.sin_family = AF_INET;
		udp_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);		
		udp_addr.sin_port = htons( (short)port );
	}

	if ( len == 0 )
		len = strlen( (char *)msg );
	return sendto( fd, msg, len, 0, (const struct sockaddr *)paddr, sizeof(SOCKADDR_IN) );
}

int sock_udp_sendX( int fd, SOCKADDR_IN *paddr, int ndst, const void* msg, int len )
{
	int i;
	int total_len = 0;
	if ( len == 0 )
		len = strlen( (char *)msg );
	for(i=0; i<ndst; i++)
		total_len += sendto( fd, msg, len, 0, (const struct sockaddr *)&paddr[i], sizeof(SOCKADDR_IN) );
	return total_len;
}

int sock_udp_recv2( int fd, void *buf, int size, SOCKADDR *src,int *paddrlen )
{
	SOCKADDR_IN  sender;
	socklen_t		addrlen = sizeof( sender );
	int		ret;

	if ( (ret = recvfrom( fd, buf, size, 0, (struct sockaddr *)&sender, &addrlen )) > 0 )
	{
		if ( src != NULL )
			memcpy( (void *)src, (void *)&sender, sizeof(SOCKADDR_IN) );
		if ( paddrlen != NULL )
			*paddrlen = addrlen;
	}
	return ret;
}

//========================== c o m m o n ==============================
int sock_dataready( int fd, int tout )
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
	nsel = select( fd+1, &rfd_set, NULL, NULL, ptv );
	if ( nsel > 0 && FD_ISSET( fd, &rfd_set ) )
		return 1;
	return 0;
}

int sock_iqueue(int fd)
{
	unsigned long ul;

	if ( ioctl( fd, FIONREAD, &ul ) == 0 )
		return (int)ul;
	return 0;
	
}

int sock_oqueue(int fd)
{
	size_t left;
	if (ioctl(fd, SIOCOUTQ, &left) == 0 )
		return (int)left;
	return 0;
}

//========================== o t h e r =================================

unsigned long get_host_ip( const char *hostname, char *buf )
{
	char	name[ 256 ];
	struct	hostent	*host;
	unsigned long	ipaddr = 0UL;
	int	i;

	if ( hostname == NULL )
	{
		gethostname( name, sizeof(name) );
		hostname = name;
	}

	if ( (host = gethostbyname( hostname )) != NULL )
	{
		for (i=0; host->h_addr_list[i]!=NULL; i++)
		{
			memcpy( &ipaddr, host->h_addr_list[0], sizeof(unsigned long) );
			if ( ipaddr != LOCAL_HOST ) break;
		}
		if ( buf )
			INET_NTOA2( ipaddr, buf );
	}
	return ipaddr;
}

unsigned long get_ifadapter_ip( const char *ifa_name, char *buf, SOCKADDR_IN *netmask, SOCKADDR_IN *ip_bcast  )
{
	struct	ifaddrs	*ifaddr, *ifaddr_head;
	unsigned long	ipaddr = 0UL;

	if ( getifaddrs( &ifaddr_head ) == 0 )
	{
		if ( ifa_name == NULL )
		{
			ifaddr = ifaddr_head;
			while ( ifaddr )
			{
				if ( ifaddr->ifa_addr->sa_family == AF_INET &&
				     strcmp( ifaddr->ifa_name, "lo" ) )
				{
					ipaddr = ((struct sockaddr_in *)(ifaddr->ifa_addr))->sin_addr.s_addr;
					if ( netmask )  memcpy( netmask,ifaddr->ifa_netmask, sizeof(SOCKADDR_IN) );
					if ( ip_bcast )  memcpy( ip_bcast,ifaddr->ifa_ifu.ifu_broadaddr, sizeof(SOCKADDR_IN) );
					break;
				}
				ifaddr = ifaddr->ifa_next;
			}
		}
		else
		{
			ifaddr = ifaddr_head;
			while ( ifaddr )
			{
				if ( ifaddr->ifa_addr->sa_family == AF_INET &&
				     !strcmp( ifaddr->ifa_name, ifa_name ) )
				{
					ipaddr = ((struct sockaddr_in *)(ifaddr->ifa_addr))->sin_addr.s_addr;
					if ( netmask )  memcpy( netmask,ifaddr->ifa_netmask, sizeof(SOCKADDR_IN) );
					if ( ip_bcast )  memcpy( ip_bcast,ifaddr->ifa_ifu.ifu_broadaddr, sizeof(SOCKADDR_IN) );
					break;
				}
				ifaddr = ifaddr->ifa_next;
			}
		}
		freeifaddrs( ifaddr_head );
	}
	if ( buf && ipaddr )
		INET_NTOA2( ipaddr, buf );
	return ipaddr;
}

const char *get_ifadapter_name()
{
	static char if_name[8] = "";
	struct	ifaddrs	*ifaddr, *ifaddr_head;

	if_name[0] = '\0';
	if ( getifaddrs( &ifaddr_head ) == 0 )
	{
		ifaddr = ifaddr_head;
		while ( ifaddr )
		{
			if ( ifaddr->ifa_addr->sa_family == AF_INET &&
			     strcmp( ifaddr->ifa_name, "lo" ) )
			{
				strcpy(if_name, ifaddr->ifa_name);
				break;
			}
			ifaddr = ifaddr->ifa_next;
		}	
		freeifaddrs( ifaddr_head );
	}
	return if_name;
}

int get_netmask( const char *ifname, SOCKADDR_IN *netmask )
{
	int rc, fd;
  struct ifreq ifr;

  if((fd = socket(AF_INET,SOCK_DGRAM, 0)) < 0)
		return -1;
  bzero((char *)&ifr, sizeof(ifr));
  strcpy(ifr.ifr_name,ifname==NULL ? get_ifadapter_name() : ifname);
  rc = ioctl(fd,SIOCGIFNETMASK,&ifr);
  close(fd);
  if ( rc==0 )
  {
  	memcpy(netmask, &ifr.ifr_ifru.ifru_netmask, sizeof(struct sockaddr_in) );
  }
  return rc;	
}

int set_netmask( const char *ifname, SOCKADDR_IN *netmask )
{
	int rc, fd;
  struct ifreq ifr;
  struct sockaddr_in netmask_addr; 
  if((fd = socket(AF_INET,SOCK_DGRAM, 0)) < 0)
		return -1;
  bzero((char *)&ifr, sizeof(ifr));
  strcpy(ifr.ifr_name,ifname==NULL ? get_ifadapter_name() : ifname);
  bzero(&netmask_addr, sizeof(struct sockaddr_in));
  netmask_addr.sin_family = AF_INET;
  netmask_addr.sin_addr = netmask->sin_addr;
  memcpy((char*)&ifr.ifr_ifru.ifru_netmask, (char*)&netmask_addr, sizeof(struct sockaddr_in));
  rc = ioctl(fd,SIOCSIFNETMASK,&ifr);
  close(fd);
  return rc;	
}

int get_netaddr( const char *ifname, SOCKADDR_IN *netaddr )
{
	int rc, fd;
  struct ifreq ifr;
	
  if((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
  	return -1;
  bzero((char *)&ifr, sizeof(ifr));
  strcpy(ifr.ifr_name,ifname==NULL ? get_ifadapter_name() : ifname);
  rc = ioctl(fd, SIOCGIFADDR, &ifr);
  close(fd);
  if (rc==0 )
  {
  	memcpy(netaddr, &ifr.ifr_ifru.ifru_addr, sizeof(struct sockaddr_in) );
  }
  return rc;
}

int set_netaddr( const char *ifname, SOCKADDR_IN *netaddr )
{
	int rc, fd;
  struct ifreq ifr;
  struct sockaddr_in addr;
	
  if((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
  	return -1;
  bzero((char *)&ifr, sizeof(ifr));
  strcpy(ifr.ifr_name,ifname==NULL ? get_ifadapter_name() : ifname);
  bzero(&addr, sizeof(struct sockaddr_in));
  addr.sin_family = AF_INET;
  addr.sin_addr = netaddr->sin_addr;
  memcpy((char*)&ifr.ifr_ifru.ifru_addr, (char*)&addr, sizeof(struct sockaddr_in));
  rc = ioctl(fd, SIOCSIFADDR, &ifr);
  close(fd);
  return rc;
}

int get_mac_addr( const char *ifname, unsigned char *ifaddr )
{
	int		rc=-1, skfd;
	struct ifreq 	ifr;

	if ( ifname==NULL )
		strcpy( ifr.ifr_name, get_ifadapter_name() );
	else
		strcpy( ifr.ifr_name, ifname );
		
	if( (skfd = socket(AF_INET, SOCK_DGRAM, 0)) >= 0 )
	{
		//printf("get_mac_addr - if_name = %s\n", ifr.ifr_name );
		if (ioctl(skfd, SIOCGIFHWADDR, &ifr) != -1 )
		{
			memcpy(ifaddr, ifr.ifr_hwaddr.sa_data, 6);
  		rc = 0;
		} 
		close( skfd );
	}
	return rc;
}

int set_mac_addr( const char *ifname, unsigned char *ifaddr )
{
	int		rc=-1, skfd;
	struct ifreq 	ifr;

	if ( ifname==NULL )
		strcpy( ifr.ifr_name, get_ifadapter_name() );
	else
		strcpy( ifr.ifr_name, ifname );
		
	if( (skfd = socket(AF_INET, SOCK_DGRAM, 0)) >= 0 )
	{
		//printf("set_mac_addr - if_name = %s\n", ifr.ifr_name );
		ifr.ifr_addr.sa_family = ARPHRD_ETHER;
		memcpy(ifr.ifr_hwaddr.sa_data, ifaddr, 6);
		if ((rc=ioctl(skfd, SIOCSIFHWADDR, &ifr)) != 0 )
			perror("SIOCSIFHWADDR");
		close( skfd );
	}
	return rc;
}

const char *mac2string(unsigned char *hwaddr)
{
	static char macaddr[20];
	sprintf(macaddr,"%02x:%02x:%02x:%02x:%02x:%02x", 
		hwaddr[0],hwaddr[1],hwaddr[2],hwaddr[3],hwaddr[4],hwaddr[5]);
	return macaddr;
}

unsigned long get_gatewayaddr( const char *ifname, char *buf )
{
	FILE *fp;
	char iface[16];
	char line[128];
	unsigned long gw_addr=INADDR_NONE, dest_addr, gate_addr;
	struct in_addr inaddr;
	
	fp = fopen("/proc/net/route", "r");
	if (fp == NULL)
		return gw_addr;
	if ( ifname==NULL )
		ifname = get_ifadapter_name();
		
  /* skip title line */
  fgets(line, sizeof(line),fp);
  while (fgets(line, sizeof(line), fp))
  {
  	if ( strlen(line)<10 ) continue;
		if (sscanf(line, "%s\t%lX\t%lX", iface,&dest_addr,&gate_addr) != 3 || 
					dest_addr != 0 ||
					(ifname!=NULL && strcmp(iface, ifname)!=0) )
				continue;
		gw_addr = gate_addr;
		break;
	}
	fclose(fp);
	if ( buf != NULL )
	{
		inaddr.s_addr = gw_addr;
		strcpy( buf, inet_ntoa(inaddr) );
	}
	return gw_addr;
}

int add_default_gateway(const char *ifname, SOCKADDR_IN *gwaddr )
{
	int fd, rc;
	struct rtentry rm;
	SOCKADDR_IN sin;
	
	 	if((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
  		return -1;
		if ( ifname==NULL )	ifname = get_ifadapter_name();
    bzero(&rm,   sizeof(struct rtentry));
    rm.rt_dst.sa_family = AF_INET;
    rm.rt_gateway.sa_family = AF_INET;
    rm.rt_genmask.sa_family = AF_INET;
    memset(&sin,0,sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = gwaddr->sin_addr.s_addr;
    memcpy(&rm.rt_gateway, &sin, sizeof(sin));
    rm.rt_dev = (char *)ifname;
    rm.rt_flags = RTF_UP | RTF_GATEWAY ;
    rc = ioctl(fd, SIOCADDRT, &rm );
   	close(fd);
   	return rc;
}

int del_default_gateway(const char *ifname, SOCKADDR_IN *gwaddr)
{
	int fd, rc;
  struct rtentry rm;
    
	 	if((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
  		return -1;
		if ( ifname==NULL )	ifname = get_ifadapter_name();
    bzero(&rm,   sizeof(struct rtentry));
    rm.rt_dst.sa_family = AF_INET;
    ((SOCKADDR_IN*)&rm.rt_dst)->sin_addr.s_addr = INADDR_ANY;
    rm.rt_genmask.sa_family = AF_INET;
    if ( gwaddr!=NULL )
    {
    	gwaddr->sin_family = AF_INET;
  		memcpy(&rm.rt_gateway, gwaddr, sizeof(SOCKADDR_IN));
  	}
    rm.rt_dev = (char *)ifname;
    rm.rt_flags = RTF_GATEWAY;
    rc = ioctl(fd, SIOCDELRT, &rm );
		if (rc != 0)
			perror("del_afgateway (SIOCDELRT)");
		close(fd);
		return rc;
}

int ifconfig(const char *ifname, const char *ipaddr, const char *netmask, const char *gwip)
{
    struct   sockaddr_in   sin;
    struct   ifreq   ifr;
    int   fd;

    bzero(&ifr,sizeof(struct ifreq));
    if (ifname == NULL)
        ifname = get_ifadapter_name();
    fd = socket(AF_INET,SOCK_DGRAM,0);
    if (fd==-1)
        return  -1;

		// set IP address
		if ( ipaddr != NULL )
		{
	    strncpy(ifr.ifr_name, ifname,IFNAMSIZ);
	    ifr.ifr_name[IFNAMSIZ-1] = 0;
	    memset(&sin, 0, sizeof(sin));
	    sin.sin_family = AF_INET;
	    sin.sin_addr.s_addr = inet_addr(ipaddr);
	    memcpy(&ifr.ifr_addr, &sin, sizeof(sin));
	    if (ioctl(fd,SIOCSIFADDR,&ifr)<0)
	    {
	    	close(fd);
	      return -1;
	    }
	  }
    // set netmask
    if (netmask != NULL)
    {
	    bzero(&ifr, sizeof(struct ifreq));
	    strncpy(ifr.ifr_name,   ifname,   IFNAMSIZ);
	    ifr.ifr_name[IFNAMSIZ   -   1]   =   0;
	    memset(&sin,   0,   sizeof(sin));
	    sin.sin_family   =   AF_INET;
	    sin.sin_addr.s_addr   =   inet_addr(netmask);
	    memcpy(&ifr.ifr_addr,   &sin,   sizeof(sin));
	    if(ioctl(fd, SIOCSIFNETMASK, &ifr ) < 0)
	    {
	    	close(fd);
        return -1;
      }
	  }
    /////////////////////////////////
    if ( gwip != NULL )
    {
	    struct rtentry rm;
	    bzero(&rm,   sizeof(struct rtentry));
	    rm.rt_dst.sa_family = AF_INET;
	    rm.rt_gateway.sa_family = AF_INET;
	    rm.rt_genmask.sa_family = AF_INET;
	    memset(&sin,0,sizeof(sin));
	    sin.sin_family = AF_INET;
	    sin.sin_addr.s_addr = inet_addr(gwip);
	    memcpy(&rm.rt_gateway, &sin,   sizeof(sin));
	    rm.rt_dev = (char *)ifname;
	    rm.rt_flags = RTF_UP | RTF_GATEWAY ;
	    if(ioctl(fd, SIOCADDRT, &rm ) < 0)
	    {
	    	close(fd);
	     	return -1;
	    }
	  }
    /////////////////////////////////
    // bring up adapter
    ifr.ifr_flags |= IFF_UP|IFF_RUNNING;
    ioctl(fd,SIOCSIFFLAGS,&ifr);
    close(fd);
    return 0;
}

unsigned long INET_ATON( const char *ipstr )
{
	return (unsigned long) inet_addr( ipstr );
}

const char *INET_NTOA( unsigned long ip )
{
	struct in_addr inaddr;
	inaddr.s_addr = ip;
	return inet_ntoa( inaddr );
}

const char *INET_NTOA2( unsigned long ip, char *numdot )
{
	char	*ptr = numdot;
	unsigned int	a, b, c, d;

	a = ip & 0x000000ff;
	b = (ip & 0x0000ff00) >> 8;
	c = (ip & 0x00ff0000) >> 16;
	d = (ip & 0xff000000) >> 24;

	sprintf( ptr, "%d.%d.%d.%d", a, b, c, d );
	return ptr;
}

const char *SOCKADDR_IN_TOSTRING(SOCKADDR_IN addr)
{
	return sockaddr_in_tostring(&addr,1);
}

#endif
