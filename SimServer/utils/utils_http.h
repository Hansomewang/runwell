/***************************************************************************
    utils_http.h  -  description
 ***************************************************************************/

#ifndef _UTILS_HTTP_H_
#define _UTILS_HTTP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "utils_ptrlist.h"

#undef SOFTWARE_NAME
#define SOFTWARE_NAME	"Runwell InViso Camera Engine"
#define		BUFFER_SIZE				4096			// 4KB
#define		LARGEBUFFER_SIZE	65536			// 64KB
#define		HUGEBUFFER_SIZE		1048576		// 1MB

extern int mimetype_initialize( );
extern void mimetype_terminate( void );

extern const char* guess_mimetype_by_filename( const char *filename );
extern const char* guess_mimetype_by_content( char *data, int size );

enum {
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
};

extern int http_method_int( const char * method_string );
extern const char * http_method_string( int method_int );

extern const char * http_code_to_string( int code );

/* param: 'list' point to address of an initialized PtrList;
 * return: number of headers parsed, -1 when failed. */
extern int http_receive_headers( int fd, StrMap * strmap );
extern int http_skip_headers( int fd );

extern int http_response(int fd, int code, const char *shortmsg, const char *extra_header, const char *textmsg );
extern int http_response_default(int fd, int code);
extern int http_response_ex( int fd, int code, const char* shortmsg, StrMap * headers, const char *textmsg );
extern int http_trace_response( int onoff );

extern int http_url_encode( const char *str, int len, char *buf, int bufsize );
extern int http_url_decode( const char *str, int len, char *buf, int bufsize );

extern int http_decode_authorization_string( const char *auth_string, char *user, char *passwd );
extern char *http_encode_authorization_string( const char *user, const char *passwd, char *buf );

#ifdef __cplusplus
};
#endif


#endif /*  _UTILS_HTTP_H_ */
