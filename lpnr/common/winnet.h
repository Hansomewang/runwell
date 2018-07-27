/*
 *	winnet.h
 *
 *  Socket i/o and control functions
 */
#ifndef _WINNET_H_
#define _WINNET_H_

#if !(defined(_WINSOCKAPI_) || defined(_WINSOCKAPI2_))
#include <winsock2.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

// winsock startup and clean
extern int winsock_startup( void );
extern int winsock_cleanup( void );
extern const char *sock_ntoa( DWORD dwIP );		// 【重要: dwIP是 host byte order】
extern unsigned long sock_aton( const char *str );	// 【重要】这个函数返回的是host byte order
extern unsigned long INET_ATON( const char *ipstr );	//【重要】返回的是network byte order
extern const char *INET_NTOA( unsigned long ip );		// 【重要: ip是 network byte order】

// create a TCP socket with server and return socket
extern SOCKET sock_connect( const char *host, int port );
extern SOCKET sock_connect0( DWORD dwIP, int port );

// server side listen to a port on given IP (ipbind). If ipbind == NULL, listen to all IP in local machine.
extern SOCKET sock_listen( const char *ipbind, int port );
extern SOCKET sock_accept( SOCKET sock_listen );

#define sock_read( s, buf, size )	recv( s, (char *)buf, size, 0 )
#define sock_write(s, buf, size )	send( s, (char *)buf, size, 0 )
#define sock_close( s )				closesocket( s )

/* read one character from socket stream with tout option.
 * tout: 0 no wait
 *       -1 wait indefinitely
 *       >0 milli-seconds, either timeout or 
 */
extern int sock_getc( SOCKET, int tout );

/* read one line from socket (until EOL character or specified bytes count is filled). buf is null terminated.
 * return value:
 *   >0:  line length. include the eol character.
 *    0:  Nothing is read (only possible when len == 0)
 *   -1: socket error.
 */
/* 
 * read/peek input queue until 'eol' byte is encounted ('eol' is also read/peek into buffer)
 * return value is number of bytes has been read. buffer is terminated by '\0'
 */
extern int sock_peek_until(SOCKET fd, char* buffer, int buf_size, int eol);
extern int sock_read_until(SOCKET fd, char *buffer, int buf_size, int eol);
#define sock_read_line(fd,buf,sz)	sock_read_until(fd,buf,sz,(int)'\n')
#define sock_peek_line(fd,buf,sz)	sock_read_until(fd,buf,sz,(int)'\n')

/* read EXACT n bytes from socket,
 * return: bytes readed, or -1 when failed */
extern int sock_read_n_bytes(SOCKET fd, void* buffer, int n);
/* write exact n bytes. return number of bytes written */
extern int sock_write_n_bytes(SOCKET fd, void *buffer, int n);
extern int sock_skip_n_bytes(SOCKET fd, int n);


/* clean all buffered input data in driver level */
extern int sock_drain( SOCKET fd );
extern int sock_drain_until( SOCKET fd, void *soh, int ns );

/* check if socket is still connected (return 1 yes, 0 no) */
extern int sock_is_connected( SOCKET fd );

/* obtain and bind a socket to local address - if bind failed, socket created is not closed */
extern SOCKET sock_udp_open();
extern SOCKET sock_udp_bindLocalIP( unsigned long ulIP, int port );
#define sock_udp_bind( port )	sock_udp_bindLocalIP( INADDR_ANY, port )

/* set UDP socket options. return value 0: OK, 0-1 Failed. */
extern int sock_udp_broadcast( SOCKET sock, BOOL bEnable );
extern int sock_udp_timeout( SOCKET sock, int nTimeOut );

/* obtain and bind a socket for UDP broadcasting */
extern SOCKET sock_udp_bind_broadcast( int port, int nTimeOut );

/* send a UDP packet to gievn IP and PORT via a binded UDP socket
 * if sock == -1,sock_udp_send will create a UDP sock and close it after packet is sent.
 * len is msg length (0 means 'msg' is a null terminated string).
 * return value:
 *  0:OK, -1: Error
 */
extern int sock_udp_send( SOCKET sock, const char * ip, int port, const char * msg, int len );
//extern int sock_udp_send0( SOCKET sock, DWORD dwIP, int port, const char * msg, int len );
extern int sock_udp_send0( SOCKET fd, SOCKADDR_IN *paddr, int port, const void* msg, int len );

/* receive a UDP datagram 
 *   'sock' shall be a socket bind to UDP port
 *   'buf' is the caller's storage for data received. 
 *    'size' is size of buf in byte. If size less then UDP packet data, extra data will be lost
 *	         maximum UDP data size is 1024 bytes.
 *    'IPSender' is the address to a DWORD to hold the UDP sender IP. Can be NULL if caller does
 *    not care the sender IP.
 *  return value:
 *    >0:  Number of bytes received.
 *     0:  socket 'sock' is closed.
 *    -1:  Error. To obtain detail error code, invoke WSAGetLastError()
 */
extern int sock_udp_recv( SOCKET sock, char *buf, int size, DWORD *IPSender );
extern int sock_udp_recv0( SOCKET sock, char *buf, int size, SOCKADDR_IN *saIn );

/* check if there are any incoming data queued in socket driver level.
 * parameter:
 *	 sock: a TCP or UDP socket.
 *	 tout: time out value in number if millisecond. (0 return immediately, -1 wait until data ready)
 *  return value:
 *	    1: data is ready
 *		0: no data (or any socket error)
 */
extern int sock_dataready( SOCKET sock, int tout );
/* 
 * obtain number of bytes available in input buffer - workable for both TCP and UDP.
 * for TCP, return value is number of bytes can be read by a single 'recv' call.
 * for UDP, return value is size of first data gram packet in buffer.
 */
extern unsigned long sock_iqueue( SOCKET sock );

// get host IP address by domain name. dotnum IP address string is also copy to buf.
extern unsigned long get_host_ip( const char *hostname, char *buf );
extern int sock_getlocalname( SOCKET sock, DWORD *myIP, WORD *myPort );

#define SOCKADDR_IP( sa )	( (sa).sin_addr.S_un.S_addr )		// sa must be SOCKADDR_IN
#define SOCKADDR_PORT( sa )	( (sa).sin_port )
#define SOCKADDR_FAMILY( sa ) ( (sa).sin_family )

#if _WIN32_WINNT < 0x0600	
#define INET6_ADDRSTRLEN 65
#define AF_INET6        23              // Internetwork Version 6
typedef struct in6_addr {
    union {
        UCHAR       Byte[16];
        USHORT      Word[8];
    } u;
} IN6_ADDR;
#define s6_addr			u.Byte
#define s6_addr16		u.Word

extern CONST IN6_ADDR in6addr_any;

const char *inet_ntop( int family, void *saddr, char *buf, int size);
int inet_pton( int family,  const char *addrstr, void *saddr);
const char *in6addr_tostring( IN6_ADDR *in6addr, int prefix);

#endif

#ifdef __cplusplus
}
#endif

#endif

