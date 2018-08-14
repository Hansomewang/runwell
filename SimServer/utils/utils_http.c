/***************************************************************************
  utils_http.c  -  description

 * implement http protocol parsing 
 *
 ***************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "dbg_printf.h"
#include "utils_http.h"
#include "utils_str.h"
#include "utils_net.h"
#include "http_code.h"
#ifndef PATH_MAX
#define PATH_MAX 512
#endif

const StrStrPair mime_type_pair[] = {
{"text/html", ".html .htm .jsp"},
{"text/css", ".css"},
{"text/plain", ".txt .log" },
{"video/mpeg4", ".mp4 .h264 .264"},
{"image/jpeg", ".jpg .jpeg" },
{"image/png", ".png"},
{"application/zip", ".zip"},
{"application/pdf", ".pdf"},
{"application/octet-stream", ".bin .exe .class"}
};

typedef struct tagIntStrPair {
	int		n;
	char	*str;
} IntStrPair;

int mimetype_initialize()
{
	// not used. we hard code all supported mime type in above mime_type_pair declaration
	return 0;
}

void mimetype_terminate( void )
{
}

const char* guess_mimetype_by_filename( const char *filename )
{
	const char	*ext;
	int extlen;

	const StrStrPair	*item;
	int i;
	const char 	*ptr, *end;

	ext = strrchr( filename, '.' );
	if( ! ext ) return NULL;
	extlen = strlen(ext);
	if( ! extlen ) return NULL;

	for(i=0; i<sizeof(mime_type_pair)/sizeof(mime_type_pair[0]); i++)
	{
		item = &mime_type_pair[i];

		end = item->value + strlen(item->value) - extlen;
		for( ptr=item->value; ptr<=end; ptr++ ) {
			if( (0 == strncasecmp(ext, ptr, extlen)) &&
				(ptr[extlen]==',' || ptr[extlen]==';' || isspace(ptr[extlen]) || ptr[extlen]=='\0') ) {
				return item->key;
			}
		}
	}

	return NULL;
}

const char* guess_mimetype_by_content( char *data, int size )
{
	char *ptr;
	int i;

	for( i=0, ptr=data; i<size; i++, ptr++ ) {
		if(*ptr < '\t')
			return "application/octet-stream";
	}

	return "text/plain";
}

/* ------------------------------------------------------------------ */

/*enum {
	HTTP_ERROR	= -1,
	HTTP_NONE	= 0,
	HTTP_GET,
	HTTP_PUT,
	HTTP_POST,
	HTTP_DELETE,
	HTTP_HEAD,
	HTTP_TRACE,
	HTTP_CONNECT,
	HTTP_LINK,
	HTTP_UNLINK,
	HTTP_METHODMAX,
};*/

const char * _http_method_string[] = {
	"NONE",
	"GET",
	"PUT",
	"POST",
	"DELETE",
	"HEAD",
	"TRACE",
	"CONNECT",
	"LINK",
	"UNLINK",
};

int http_method_int( const char * method_string )
{
	int i;
	for(i=0; i<HTTP_METHODMAX; i++) {
		if( 0 == strcasecmp( _http_method_string[i], method_string ) ) {
			return i;
		}
	}
	return HTTP_ERROR;
}

const char * http_method_string( int method_int )
{
	if( method_int>=HTTP_NONE && method_int<HTTP_METHODMAX )
		return _http_method_string[ method_int ];
	else
		return "ERROR";
}


int http_receive_headers( int fd, StrMap * strmap )
{
	int n = 0;
	char buffer[LARGEBUFFER_SIZE], key[PATH_MAX], value[PATH_MAX];
	const char *ptr, *q;
	int len;

	PtrList		* list = strmap;
	StrStrPair	* header;

	/* go on to parse headers */
	for(n=0; n<100; n++) { /* we assume 100 as the max number */

		len = sock_read_line(fd, buffer, LARGEBUFFER_SIZE);
		if(len <= 0) { 	/* bad request, interrupted by client? */
			StrMap_remove_all( list );
			return -1;
		}
		buffer[len] = '\0';

		/* we meet a blank line --- end line of request,
		 * we return with headers */
		if( buffer[0]=='\n' || (buffer[0]=='\r' && buffer[1]=='\n') )
			break;
		ptr = buffer;

		/* parse the first word as key */
		for(q=ptr; *q!=':' && *q!='\0'; q++);
		len = q - ptr;
		if(len >= PATH_MAX) continue;
		memcpy(key, ptr, len);
		key[len] = '\0';

		for(ptr=q; *ptr==':' || *ptr==' ' || *ptr=='\t'; ptr++);

		/* parse the other till end of line as value */
		for(q=ptr; *q!='\r' && *q!='\n' && *q!='\0'; q++);
		len = q - ptr;
		if(len >= PATH_MAX) continue;
		memcpy(value, ptr, len);
		value[len] = '\0';

		/* we got a new header, add to the list */
		header = malloc( sizeof(StrStrPair) );
		header->key = strdup(key);
		header->value = strdup(value);
		PtrList_append( list, header );
	}

	return n;
}

int http_skip_headers( int fd )
{
	int n = 0;
	char buffer[LARGEBUFFER_SIZE];
	int len;

	/* go on to parse headers */
	for(n=0; n<100; n++) { /* we assume 100 as the max number */

		len = sock_read_line(fd, buffer, LARGEBUFFER_SIZE);
		if(len <= 0) { 	/* bad request, interrupted by client? */
			return -1;
		}
		buffer[len] = '\0';

		/* we meet a blank line --- end line of request,
		 * we return with headers */
		if( buffer[0]=='\n' || (buffer[0]=='\r' && buffer[1]=='\n') )
			break;
		buffer[0] = 0;

	}

	return n;
}

/* ---------------- Response ----------------------- */
static int trace_response = 0;
int http_trace_response( int onoff )
{
	int	old_value = trace_response;
	trace_response = onoff;
	return old_value;
}

int http_response_ex( int fd, int code, const char* shortmsg, StrMap * headers, const char *textmsg )
{
	char 		buf[BUFFER_SIZE], *ptr;
	int		buf_left = BUFFER_SIZE -1;

	PtrList		* list = headers;
	POSITION 	pos;
	StrStrPair 	* header;

	if( code && shortmsg ) {
		ptr = buf;
		sprintf(ptr, "HTTP/1.0 %d %s\r\n", code, shortmsg );
		while(*ptr!='\0') ptr++;
		buf_left += ptr - buf;

		for(pos=list->head; pos!=NULL; pos=pos->next)
		{
			header = (StrStrPair *)pos->ptr;
			if(buf_left < strlen(header->key) + strlen(header->value) + strlen("%s: %s\r\n")) break;
			sprintf(ptr, "%s: %s\r\n", header->key, header->value);
			while(*ptr!='\0') ptr++;
		}
		sprintf(ptr, "\r\n");

		send( fd, buf, strlen(buf), 0 );
		if ( trace_response )
			PRINTF3( "HTTP Response_ex: => {%s", buf );
	}

	if(textmsg) {
		send( fd, textmsg, strlen(textmsg), 0 );
	}
	return code;
}

int http_response(int fd, int code, const char *shortmsg, const char *extra_header, const char *textmsg )
{
	char 	header[BUFFER_SIZE];
	int	extra_len = 0;

	if( extra_header && (extra_len=strlen(extra_header)) > 0 )
	{
		/* should contain "Content-Type: xxx" */
	} else {
		extra_header = "Content-Type: text/html\r\n";
		extra_len = strlen(extra_header);
	}

	/* send http header */
	if( code && shortmsg && extra_header )
	{
		sprintf(header,
			"HTTP/1.0 %d %s\r\n"
			"Server: " SOFTWARE_NAME "\r\n",
			code, shortmsg );
		send( fd, header, strlen(header), 0 );
		send( fd, extra_header, extra_len, 0 );

		if ( trace_response )
		{
			PRINTF3( "HTTP response => \r\n"
					 "%s", header);
			if ( extra_len > 0 )
				PRINTF3( "%s", extra_header );
		}

		strcpy(header,	"Connection: close\r\n\r\n" );
		send(fd, header, strlen(header), 0);
		if ( trace_response )
			PRINTF3( "%s", header );
	}

	/* send http text if there is */
	if(textmsg && strlen(textmsg)>0)
	{
		send(fd, textmsg, strlen(textmsg), 0);
		if ( trace_response )
			PRINTF3(textmsg);
	}
	return code;
}

IntStrPair http_default_messages[] = {
	{ 200, "OK" },
	{ 201, "Created" },
	{ 202, "Accepted" },
	{ 203, "Non-Authoritative Information" },
	{ 204, "No Content" },
	{ 205, "Reset Content" },
	{ 206, "Partial Content" },
	{ 300, "Multiple Choices" },
	{ 301, "Moved Permanently" },
	{ 302, "Found" },
	{ 303, "See Other" },
	{ 304, "Not Modified" },
	{ 305, "Use Proxy" },
	{ 306, "(Unused)" },
	{ 307, "Temporary Redirect" },
	{ 400, "Bad Request" },
	{ 401, "Unauthorized" },
	{ 402, "Payment Required" },
	{ 403, "Forbidden" },
	{ 404, "Not Found" },
	{ 405, "Method Not Allowed" },
	{ 406, "Not Acceptable" },
	{ 407, "Proxy Authentication Required" },
	{ 408, "Request Timeout" },
	{ 409, "Conflict" },
	{ 410, "Gone" },
	{ 411, "Length Required" },
	{ 412, "Precondition Failed" },
	{ 413, "Request Entity Too Large" },
	{ 414, "Request-URI Too Long" },
	{ 415, "Unsupported Media Type" },
	{ 416, "Requested Range Not Satisfiable" },
	{ 417, "Expectation Failed" },
	{ 500, "Internal Server Error" },
	{ 501, "Not Implemented" },
	{ 502, "Bad Gateway" },
	{ 503, "Service Unavailable" },
	{ 504, "Gateway Timeout" },
	{ 505, "HTTP Version Not Supported" },
	{ 0, NULL },
};

const char * http_code_to_string( int code )
{
	IntStrPair *inm;
	char *str = "Unknown Status";

	for(inm = & http_default_messages[0]; inm->n != 0; inm++ ) {
		if( code == inm->n ) {
			str = inm->str;
			break;
		}
	}

	return str;
}

int http_response_default(int fd, int code)
{
	char buffer[BUFFER_SIZE];
	const char * str = http_code_to_string( code );
	struct sockaddr_in myaddr;
	socklen_t addrlen = sizeof(myaddr);
	
	getsockname(fd, (struct sockaddr *)&myaddr, &addrlen);

	sprintf( buffer,
			"<HTML>\r\n"
			"<HEAD><TITLE>%d %s</TITLE></HEAD>\r\n"
			"<BODY>\r\n"
			"<H1>%s</H1>\r\n"
			"<BR><HR><ADDRESS>HTTPD (" SOFTWARE_NAME ") %s:%d</ADDRESS>\r\n"
			"</BODY>\r\n"
			"</HTML>\r\n",
			code, str, str,
			inet_ntoa(myaddr.sin_addr), ntohs(myaddr.sin_port) );

	return http_response( fd, code, str, NULL, buffer );
}

typedef struct tagHttpConnection {
	int			fd;
	int			code;
	char		msg[ PATH_MAX ];
	StrMap		headers;
} HttpConnection;

static inline char bin2bcd(char bin)
{
	return bin <= 9 ? bin + '0' : bin-10 + 'A';
}
int http_url_encode( const char *str, int len, char *buf, int bufsize )
{
	const char	*src, *src_end;
	char *dst, *eostr;

	src = str;
	src_end = str + strlen(str);
	for( dst=buf, eostr=buf+bufsize-3; src<src_end; src++ ) {
		if( dst>=eostr ) return -1;
		switch( *src ) {
//		case ' ':	*dst++ ='+';	break;
		case '\\':	*dst++ ='/';	break;
		case '*':
		case '@':
		case '.':
		case '-':	*dst ++ = *src;	break;
		default:
			if( (*src>='0' && *src<='9') ||
				(*src>='A' && *src<='z') ) {
				*dst ++ = *src;
			} else {
				*dst++ = '%';
				*dst++ = bin2bcd(*src >> 4);
				*dst++ = bin2bcd(*src & 0x0f);
			}
			break;
		}
	}
	*dst = '\0';

	return dst-buf;
}

int http_url_decode( const char *str, int len, char *buf, int bufsize )
{
	const char	*src, *src_end;
	char	*dst, *eostr, hex[3];

	src = str;
	src_end = src + len;

	for( dst=buf, eostr=buf+bufsize-1; src<src_end; dst++ ) {
		if( dst>=eostr ) return -1;
		switch( *src ) {
		case '%':
			src ++;
			hex[0] = * src ++;
			hex[1] = * src ++;
			hex[2] = 0;
			* dst = ( strtol((char *)hex,NULL,16) & 0xFF );
			break;
//		case '+':
//			src ++;
//			*dst = ' ';
		default:
			*dst = *src;
			src++;
			break;
		}
	}
	*dst = '\0';

	return dst-buf;
}

int http_decode_authorization_string( const char *auth_string, char *user, char *passwd )
{
	const char	*ptr;
	int		rc, len;
	char		buf[ 64 ];

	if ( auth_string != NULL && (ptr = strstr(auth_string, "Basic" )) != NULL )
	{
		ptr += 5;
		while ( isspace(*ptr) ) ptr++;
		rc = str_b64dec( ptr, buf, sizeof(buf) );
		if( rc > 0 )
		{
			ptr = strchr( buf, ':' );
			if ( ptr && (len = (ptr - buf)) > 0 )
			{
				memcpy( user, buf, len  );
				user[ len ] = '\0';
				strcpy( passwd, ++ptr );
				return 0;
			}
		}
	}
	else
	{
		user[0] = '\0';
		passwd[0] = '\0';
	}
	return -1;
}

/* return value is buf */
char *http_encode_authorization_string( const char *user, const char *passwd, char *buf )
{
	char	tmp[ 128 ];

	strcpy( buf, "Basic " );
	buf += strlen( buf );
	sprintf( tmp, "%s:%s", user, passwd );
	str_b64enc( tmp, buf, 128 );
	return 	buf;
}
