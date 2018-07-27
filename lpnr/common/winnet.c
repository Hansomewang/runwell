/*
 * winnet.c
 *	windows socket utility functions
 */
//#include <stdio.h>
//#include <sys/types.h>
//#include <sys/stat.h>
//#include <fcntl.h>
//#include <stdlib.h>
//#include <string.h>
//#include <ctype.h>
//#include <errno.h>
//#include <time.h>
#include "winnet.h"

#ifndef _WINTYPES_H
#define MSG_NOSIGNAL		0
#define MSG_DONTWAIT		0
#endif

//#define _CRT_SECURE_NO_WARNINGS 		// disable the annoying warning 4996

int winsock_startup( void )
{
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;
 
	wVersionRequested = MAKEWORD( 2, 2 );
 
	err = WSAStartup( wVersionRequested, &wsaData );
	if ( err != 0 ) 
	{
		/* Tell the user that we could not find a usable */
		/* WinSock DLL.                                 */
		return err;
	}
 
    return 0; 
}

int winsock_cleanup( void )
{
    return WSACleanup( );
}

const char *sock_ntoa( DWORD dwIP )
{
	struct in_addr  iaddr;
	static char strIP[ 16 ];

	iaddr.s_addr = htonl(dwIP);
	strcpy( strIP, inet_ntoa( iaddr ) );
	return strIP;
}

unsigned long sock_aton( const char *str )
{
	unsigned long ip = inet_addr(str);
	ip = ntohl(ip);
	return ip;
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

SOCKET sock_connect0( DWORD dwIP, int port )
{
	struct sockaddr_in	destaddr;
	SOCKET 			fd;

	memset( & destaddr, 0, sizeof(destaddr) );
	destaddr.sin_family = AF_INET;
	destaddr.sin_port = htons( (short)port );
	destaddr.sin_addr.S_un.S_addr = dwIP;

	if ( (fd = socket(PF_INET, SOCK_STREAM, 0)) != INVALID_SOCKET )
	{
		if ( connect(fd, (struct sockaddr *)&destaddr, sizeof(destaddr)) == SOCKET_ERROR )
		{
			sock_close( fd );
			fd = INVALID_SOCKET;
		}
		else
		{
			int tmp=1;
			setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (const char*)&tmp, sizeof(tmp));
		}
	}
	return fd;
}

SOCKET sock_connect( const char *host, int port )
{
/*
	struct sockaddr_in	destaddr;
    struct hostent 		*hp;
	SOCKET 			fd = INVALID_SOCKET;

	memset( & destaddr, 0, sizeof(destaddr) );
	destaddr.sin_family = AF_INET;
	destaddr.sin_port = htons( (short)port );
   	if ((destaddr.sin_addr.s_addr=INET_ATON(host)) ==INADDR_NONE)
   	{
	        hp = gethostbyname(host);
	        if(! hp) return INVALID_SOCKET;
	        memcpy (& destaddr.sin_addr, hp->h_addr, sizeof(destaddr.sin_addr));
    }

	if ( (fd = socket(PF_INET, SOCK_STREAM, 0)) != INVALID_SOCKET )
	{
		if ( connect(fd, (struct sockaddr *)&destaddr, sizeof(destaddr)) == SOCKET_ERROR )
		{
			sock_close( fd );
			fd = INVALID_SOCKET;
		}
	}
	return fd;
*/
	return sock_connect0(inet_addr(host), port);
}

SOCKET sock_listen( const char *ipbind, int port )
{
		struct sockaddr_in 	my_addr;
		SOCKET		fd;

    	if ( (fd = socket(AF_INET,SOCK_STREAM,0)) != INVALID_SOCKET )
		{
			BOOL 		bReuseAddr = TRUE;
   			setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&bReuseAddr, sizeof(bReuseAddr));

    		memset(&my_addr, 0, sizeof(my_addr));
			my_addr.sin_family = AF_INET;
			my_addr.sin_port = htons ((short)port);
    		if( ipbind != NULL ) 
			{
    			my_addr.sin_addr.s_addr = INET_ATON(ipbind);
    		} 
			else 
			{
	    		my_addr.sin_addr.s_addr = htonl (INADDR_ANY);
			}

    		if ( bind (fd, (struct sockaddr *) &my_addr, sizeof (my_addr)) == 0 )
    		{
    			if( listen(fd, 5) == 0 ) 
				{
    				return fd;
    			}
    		}
		}
		if ( fd != INVALID_SOCKET )
			sock_close(fd);
		return INVALID_SOCKET;
}

SOCKET sock_accept( SOCKET sock_listen )
{
	struct sockaddr_in	fromaddr;
	int		addrlen = sizeof(fromaddr);
	SOCKET	sock;
	int		tmp=1;

	if ( (sock = accept( sock_listen, (struct sockaddr *) & fromaddr, & addrlen )) != INVALID_SOCKET )
		setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (const char*)&tmp, sizeof(tmp));
	return sock;
}


int sock_getc( SOCKET sock, int tout )
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

int sock_read_until(SOCKET fd, char* buffer, int bufsize, int eol)
{
	int		len, size, buf_left;
	char	*ptr;

	if ( eol != 0 )
	{
		int i, found=0;
		char *cmpptr;

		for (ptr=buffer,size=0,buf_left=bufsize; buf_left>0; /**/) 
		{
			/* peek the data, but not remove the data from the queue */
			if ( (len = recv(fd, ptr, buf_left, MSG_PEEK)) == SOCKET_ERROR )
				return -1;
			else if ( len == 0 )
				return 0;

			/* try to find EOL in the data */
			for(i=0, cmpptr=ptr; i<len; i++, cmpptr++) {
				if(*cmpptr == eol) 
				{
					len = i+1;
					found = 1;
					break;
				}
			}

			/* get the data we need, and remove them from the socket input queue */
			if ( sock_skip_n_bytes( fd, len ) != len )
				return -1;

			ptr += len;
			size += len;
			buf_left -= len;

			if( found ) break;
		}
	}
	else
	{
		for( ptr=buffer, size=0, buf_left = bufsize; buf_left > 0; )
		{
			if ( (len = recv( fd, ptr, buf_left, 0 )) == SOCKET_ERROR )
				return -1;
			ptr += len;
			size += len;
			buf_left -= len;
		}
	}
	buffer[size] = '\0';
	return size;
}

// copy w/o remove data from socket buffer
int sock_peek_until(SOCKET fd, char* buffer, int buf_size, int eol)
{
	int len, size, buf_left, i, found=0;
	char *ptr, *cmpptr;

	for (ptr=buffer,size=0,buf_left=buf_size-1; buf_left>0; /**/) 
	{
		/* peek the data, but not remove the data from the queue */
		if ( (len = recv(fd, ptr, buf_left, MSG_PEEK)) == SOCKET_ERROR )
			return -1;
		else if ( len == 0 )
			return 0;

		/* try finding '\n' in the data */
		for(i=0, cmpptr=ptr; i<len; i++, cmpptr++) 
		{
			if(*cmpptr == eol) 
			{
				len = i+1;
				found = 1;
				break;
			}
		}
		ptr += len;
		size += len;
		buf_left -= len;

		if( found ) 
		{
			buffer[size] = '\0';
			return size;
		}
	}
	/* buffer not enough , line too long */
	return 0;
}

int sock_read_n_bytes(SOCKET fd, void* buffer, int n)
{
	char *ptr = (char *)buffer;
	int len;
	while( n > 0 ) 
	{
		//len = recv(fd, ptr, n, MSG_WAITALL);
		len = recv(fd, ptr, n, 0);
		if( len == 0 ) break;
		if( len == SOCKET_ERROR )
			return -1;
		ptr += len;
		n -= len;
	}
	return (int)(ptr-(char*)buffer);
}

int sock_write_n_bytes(SOCKET fd, void* buffer, int size)
{
	int len;
	char *ptr = (char *)buffer;
	
	while ( size > 0 )
	{
		len = send(fd, ptr, min(size,1024), MSG_NOSIGNAL);
		if( len <= 0)	return -1;
		ptr += len;
		size -= len;
	}
	return (int)(ptr - (char *)buffer);
}

int sock_skip_n_bytes(SOCKET fd, int n)
{
	char buffer[ 1024 ];
	int len;
	int left = n;

	while( left > 0 ) 
	{
		//len = recv(fd, ptr, n, MSG_WAITALL);
		len = recv(fd, buffer, min( n, sizeof( buffer ) ), 0);
		if( len == 0 ) break;
		if( len == SOCKET_ERROR )
			return -1;
		left -= len;
	}
	return (n-left);
}
// drain all data currently in socket input buffer
int sock_drain( SOCKET fd )
{
	int	n = 0;
	char	buf[ 1024 ];

	while ( sock_dataready( fd, 0 ) )
		n += recv(fd, buf, sizeof(buf), 0);
	return n;
}

int sock_drain_until( SOCKET fd, void *soh, int ns )
{
	char buffer[1024];
	char *ptr;
	int i, len, nskip=0;
	while ( sock_dataready(fd,100) )
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

int sock_is_connected( SOCKET fd )
{
	fd_set set;
	struct timeval val;
	int ret;

	FD_ZERO( & set );
	FD_SET( fd, & set );
	val.tv_sec = 0;
	val.tv_usec = 0;
	ret = select( (int)fd +1, & set, NULL, NULL, & val );
	if( ret > 0 ) 
	{
		// try to peek data
		char buf[64];
		ret = recv( fd, buf, sizeof(buf), MSG_NOSIGNAL | MSG_PEEK );
		if( ret <= 0 ) return 0;
	} 
	else if ( ret < 0 ) 
	{
		// select error
		return 0;
	} else {
		// select timeout
	}

	return 1;
}


//================================ U D P ==============================
SOCKET sock_udp_open()
{
	return socket( AF_INET, SOCK_DGRAM, 0 );
}

SOCKET sock_udp_bindLocalIP( unsigned long ulIP, int port )
{
	struct sockaddr_in 	my_addr;
	SOCKET		udp_fd;

	if ( (udp_fd = socket( AF_INET, SOCK_DGRAM, 0 )) != INVALID_SOCKET  )
	{
		BOOL bReuseAddr = FALSE;
		setsockopt( udp_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&bReuseAddr, sizeof(bReuseAddr));

		memset(&my_addr, 0, sizeof(my_addr));
		my_addr.sin_family = AF_INET;
		my_addr.sin_port = htons ((short)port);
//		my_addr.sin_addr.s_addr = htonl (ulIP);
		my_addr.sin_addr.s_addr = ulIP;

		if (bind ( udp_fd, (struct sockaddr *) &my_addr, sizeof (my_addr)) != 0)
		{
	        	sock_close( udp_fd );
	        	udp_fd = INVALID_SOCKET;
	    }
	}
    return udp_fd;
}

int sock_getlocalname( SOCKET sock, DWORD *myIP, WORD *myPort )
{
	struct sockaddr_in 	my_addr;
	int			tmp;

	tmp = sizeof(my_addr);
	if ( getsockname( sock, (struct sockaddr *)&my_addr, &tmp ) == 0 )
	{
		*myIP = ntohl(my_addr.sin_addr.s_addr);
		*myPort = ntohs(my_addr.sin_port);
		return 0;
	}
	else
		return SOCKET_ERROR;
}

// enable/disable the broadcast transmission on given UDP socket
// return value: 0 OK, -1 error (call WSAGetLastError() for reason of failure)
int sock_udp_broadcast( SOCKET sock, BOOL bEnable )
{
	return setsockopt( sock, SOL_SOCKET, SO_BROADCAST, (CHAR *)&bEnable, sizeof ( BOOL ));
}

// Set UDP receiving time out. nTimeOut in msec.
int sock_udp_timeout( SOCKET sock, int nTimeOut )
{
	return setsockopt( sock, SOL_SOCKET, SO_RCVTIMEO, (CHAR *)&nTimeOut, sizeof ( nTimeOut ));
}
#if 1
// after bind to a broadcast socket, use sock_udp_send/sock_udp_send0 (with target IP as IPADDR_BROADCAST)
// use sock_udp_recv to receive reply from server.
SOCKET sock_udp_bind_broadcast( int port, int nTimeOut )
{
	BOOL	bBroadcast = TRUE;
	BOOL 	bReuseAddr = FALSE;
	SOCKADDR_IN saUdpClient;
	SOCKET sock_bc = socket(AF_INET,SOCK_DGRAM,0); 
	if ( setsockopt( sock_bc, SOL_SOCKET, SO_BROADCAST, (CHAR *)&bBroadcast, sizeof ( BOOL )) == SOCKET_ERROR )
	{
		closesocket( sock_bc );
		return INVALID_SOCKET;
	}
	if ( setsockopt ( sock_bc, SOL_SOCKET, SO_RCVTIMEO, (CHAR *) &nTimeOut, sizeof (nTimeOut)) == SOCKET_ERROR )
	{
		closesocket( sock_bc );
		return INVALID_SOCKET;
	}
	if ( setsockopt(sock_bc, SOL_SOCKET, SO_REUSEADDR, (const char*)&bReuseAddr, sizeof(bReuseAddr)) == SOCKET_ERROR )
	{
		closesocket( sock_bc );
		return INVALID_SOCKET;
	}
	saUdpClient.sin_family = AF_INET; 
	saUdpClient.sin_port = htons((u_short)port); 
	saUdpClient.sin_addr.s_addr = htonl(INADDR_ANY); 
	if(bind( sock_bc, (SOCKADDR *)&saUdpClient, sizeof(SOCKADDR_IN)) != 0) 
	{
		closesocket( sock_bc );
		return INVALID_SOCKET;
	}
	return sock_bc;
}
#endif

int sock_udp_send( SOCKET udp_fd, const char * ip, int port, const char * msg, int len )
{
	SOCKADDR_IN saddr, *paddr;

	if ( ip == NULL )
		paddr = NULL;
	else
	{
		paddr = &saddr;
		saddr.sin_family = AF_INET;
		saddr.sin_port = htons((short)port);
		saddr.sin_addr.s_addr = inet_addr(ip);
	}
	return sock_udp_send0( udp_fd, paddr, port, msg, len );
}

int sock_udp_send0( SOCKET fd, SOCKADDR_IN *paddr, int port, const void* msg, int len )
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
		len = (int)strlen( (char *)msg );
	return sendto( fd, (const char *)msg, len, 0, (const struct sockaddr *)paddr, sizeof(SOCKADDR_IN) );
}

int sock_udp_recv( SOCKET sock, char *buf, int size, DWORD *IPSender )
{
	SOCKADDR_IN  sender;
	int		addrlen = sizeof( sender );
	int		ret;

	if ( (ret = recvfrom( sock, buf, size, 0, (struct sockaddr *)&sender, &addrlen )) > 0 )
	{
		if ( IPSender != NULL )
			*IPSender = sender.sin_addr.S_un.S_addr;
	}
	return ret;
}

int sock_udp_recv0( SOCKET sock, char *buf, int size, SOCKADDR_IN *saIn )
{
	SOCKADDR_IN  sender;
	int		addrlen = sizeof( sender );
	int		ret;

	if ( (ret = recvfrom( sock, buf, size, 0, (struct sockaddr *)&sender, &addrlen )) > 0 )
	{
		if ( saIn != NULL )
		{
			sender.sin_port = ntohs( sender.sin_port );
			memcpy( (void *)saIn, (void *)&sender, sizeof(SOCKADDR_IN) );
		}
	}
	return ret;
}

//========================== c o m m o n ==============================
int sock_dataready( SOCKET fd, int tout )
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
		tv.tv_sec = tout / 1000;
		tv.tv_usec = (tout % 1000) * 1000;
		ptv = &tv;
	}
	nsel = select( (int)fd+1, &rfd_set, NULL, NULL, ptv );
	if ( nsel > 0 && FD_ISSET( fd, &rfd_set ) )
		return 1;
	return 0;
}

unsigned long sock_iqueue( SOCKET sock )
{
	unsigned long ul;

	if ( ioctlsocket( sock, FIONREAD, &ul ) == 0 )
		return ul;
	return 0;
}

//////////////////////////////////////////////////////////////
#if _WIN32_WINNT < 0x0600	
#include <stdio.h>

CONST IN6_ADDR in6addr_any = { { { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 } } };

const char *inet_ntop( int family, void *saddr, char *buf, int size)
{
	static char addrstr[ INET6_ADDRSTRLEN ];

	if ( buf == NULL ) return NULL;

	if ( family==AF_INET )
	{
		IN_ADDR  iaddr;
		IN_ADDR *paddr = (IN_ADDR *)saddr;
		iaddr.s_addr = paddr->s_addr;
		strcpy(addrstr, inet_ntoa( iaddr ) );
	} 
	else if ( family==AF_INET6 )
	{
		IN6_ADDR *paddr = (IN6_ADDR *)saddr;
		WORD waddr[8];
		int i, i1, i2, nzero=0;
		int j, j1, j2;
		// find longest continues 0

		memcpy(waddr, paddr->s6_addr16, sizeof(waddr));
		for(i=0; i<8; i++)
			waddr[i] = ntohs(waddr[i]);
		for(i=0; i<8; i++)
		{
			if ( waddr[i]==0 )
			{
				for(j=i+1; j<8; j++)
					if ( waddr[j] != 0 ) break;
				j1 = i; j2 = waddr[j]==0 ? j : j-1;
				if ( j2-j1+1 > nzero )
				{
					i1 = j1; i2 = j2; nzero = j2-j1+1;
				}
				i = j2;
			}
		}
		if ( nzero > 0 )
		{
			char *ptr = addrstr;
			if ( j1==0 ) *(ptr++) = ':';
			for(i=0; i<j1; i++)
			{
				sprintf(ptr, "%x:", waddr[i] );
				ptr += strlen(ptr);
			}
			*(ptr++) = ':';
			if ( j2 < 7 )
			{
				for(i=j2+1; i<8; i++)
				{
					sprintf(ptr, "%x:", waddr[i] );
					ptr += strlen(ptr);
				}
				*(--ptr) = '\0';
			}
			else
			{
				*ptr = '\0';
			}
		}
		else
			sprintf(addrstr, "%x:%x:%x:%x:%x:%x:%x:%x", 
				waddr[0],waddr[1],waddr[2],waddr[3],
				waddr[4],waddr[5],waddr[6],waddr[7]);
		
	}
	else
		return NULL;
	// addstr contains IP address string. copy to user buffer
	if ( (int)strlen(addrstr) < size )
		strcpy(buf, addrstr);
	else
	{
		strncpy(buf, addrstr, size-1);
		buf[size-1] = '\0';
	}
	return buf;
}

int inet_pton( int family,  const char *addrstr, void *saddr)
{
	if ( family == AF_INET )
	{
		IN_ADDR *paddr = (IN_ADDR *)saddr;
		paddr->s_addr = inet_addr(addrstr);
		return paddr->s_addr==INADDR_NONE ? -1 : 1;
	}
	else if ( family == AF_INET6 )
	{
		IN6_ADDR *paddr = (IN6_ADDR *)saddr;
		int i, icc;
		const char *pb, *pe;
		pb = addrstr;
		pe = addrstr + strlen(addrstr);
		if ( *pb=='[' ) pb++;
		for(i=0; i<8; i++)
		{
			if ( *pb==':' && *(pb+1)==':')
			{
				icc = i;
				pb += 2;
			}
			else if ( *pb == ':' )
				pb++;
			if ( pb == pe || !isxdigit(*pb) ) break;
			paddr->s6_addr16[i] = (short)strtol(pb, (char **)&pb, 16);
			paddr->s6_addr16[i] = htons(paddr->s6_addr16[i]);
		}
		if ( i < 8 )
		{
			int nzero = 8 - i;
			// icc~(i-1) move to (icc+nzero)~(i+nzero-1)
			memmove(&paddr->s6_addr16[icc+nzero], &paddr->s6_addr16[icc], sizeof(short)*(i-icc));
			// zero word from icc ~ icc+nzero-1
			for(i=icc; i<icc+nzero; i++)
				paddr->s6_addr16[i] = 0;
		}
		return 0;
	}
	else
		return -1;
}

const char *in6addr_tostring( IN6_ADDR *in6addr, int prefix)
{
	static char straddr[INET6_ADDRSTRLEN+8];

	if ( inet_ntop(AF_INET6, in6addr, straddr, sizeof(straddr)) != NULL )
		sprintf(straddr+strlen(straddr), "/%d", prefix);
	else
		straddr[0] = '\0';		
	return straddr;
}

#endif
