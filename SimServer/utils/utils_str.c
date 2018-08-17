/***************************************************************************
                          utils_str.c  -  description
                             -------------------
    begin                : Thu Dec 20 2003
    copyright            : (C) 2003 by Thomas Chang
    email                : thomasc1958@gmail.com
 ***************************************************************************/
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include "utils_str.h"

typedef enum {
    HI_FALSE = 0,
    HI_TRUE  = 1,
} HI_BOOL;

char* trim( char *str )
{
	int len;
	char *ptr, *q;

	for(ptr=str; *ptr==' ' || *ptr=='\t'; ptr++);
	for(q=str+strlen(str)-1; *q==' ' || *q=='\t' || *q=='\r' || *q=='\n'; q--);
	len = q+1 - ptr;
	memmove(str, ptr, len);
	str[len] = '\0';

	return str;
}

int strisnumber( const char *string, int *number )
{
	const char *ptr = string;
	HI_BOOL bIsHex=HI_FALSE;
	HI_BOOL bIsNegative=HI_FALSE;
	
	if ( *ptr == '\0' )  return 0;
	if ( *ptr=='-' ) 
	{
		bIsNegative = HI_TRUE;
		ptr++;		// leading '-' sign
	}
	if ( ptr[0]=='0' && ptr[1]=='x' )
	{
		if ( !bIsNegative )
		{
			bIsHex = HI_TRUE;
			ptr += 2;
		}
		else
			return 0;
	}
	if ( bIsHex )
		for( ; *ptr && isxdigit(*ptr) ; ptr++ );
	else
		for( ; *ptr && isdigit(*ptr) ; ptr++ );
	if ( *ptr != '\0' && *ptr!=' ' && *ptr != '\t' && *ptr != '\n' ) return 0;
	
	if ( number ) 
		*number = strtol( string, NULL, bIsHex ? 16 : 10 );
	return 1;
}

int strisfloat( const char *string, float *number )
{
	const char *ptr = string;
	
	if ( *ptr == '\0' )  return 0;
	if ( *ptr=='-' ) ptr++;				// leading '-' sign
	for( ; *ptr && isdigit(*ptr) ; ptr++ );
	if ( *ptr=='.' ) ptr++;
	for( ; *ptr && isdigit(*ptr) ; ptr++ );
	if ( *ptr != '\0' ) return 0;
	
	if ( number )  *number = atof( string );
	return 1;
}

int strisboolean( const char *string, int *boolval )
{
	int idx;
	if ( (idx=stridxinargs( string, 3, "no", "yes", "off", "on", "disable", "enable", "stop", "start", (char *)0 )) == -1 )
		return 0;
	if ( boolval )
		*boolval = idx % 2;
	return 1;
}

int strstartwith(const char *string, const char *prefix, int minmatch, char ** pleft)
{
	int len = strlen(prefix);
	int n = strlen( string );
	if ( n < len && n >=minmatch && minmatch ) len = n;
	
	if( 0 == strncmp(string, prefix, len) ) {
		if(pleft) *pleft = (char*)(string + len);
		return 1;
	}
	if(pleft) *pleft = NULL;
	return 0;
}

int strcasestartwith(const char *string, const char *prefix, int minmatch, char ** pleft)
{
	int len = strlen(prefix);
	int n = strlen( string );
	if ( n < len && n >=minmatch && minmatch ) len = n;
	if( 0 == strncasecmp(string, prefix, len) ) {
		if(pleft) *pleft = (char*)(string + len);
		return 1;
	}
	if(pleft) *pleft = NULL;
	return 0;
}

/* get a token which is:
 *  a numeric constant like 1.23, -32
 *  a alpha-numeric word (composed by alphabetic, number and '_' but not started with digit)
 *  a double quoted string (quote will be removed) like "This is a long token" then the token will be: This is a long token
 *  white space (' ', \t, \n) are treated as token separators
 *  return value:
 *	length of token. Unless input is all white space to end of string. Otherwise
 *      return value must be >= 1.
 *      On return, *pleft will be point to the next non white space character after
 *      the token.
 */
 
int strgettoken(const char *string, char *buf, char **pleft)
{
	const char *ptr, *q;
	int len;

	/* skip leading white-space */
//	for(ptr=string; *ptr && isspace(*ptr); ptr++);
	for(ptr=string; *ptr && (*ptr==' ' || *ptr=='\t' || *ptr=='\n'); ptr++);
	if ( *ptr=='\0' ) return 0;
		
	if(*ptr == '\"')
	{
		/* obtain quoted words - terminated by closing " or end of line */
		ptr ++;
		for(q=ptr; *q!='\"' && *q!='\r' && *q!='\n' && *q!='\0'; q++);
		len = q - ptr;
		if( *q=='\"' ) q++;	// skip the closing double quote
	}
	else
	{
		if ( (*ptr == '-' && isdigit(*(ptr+1))) || isdigit(*ptr) )
		{
			q = (*ptr == '-') ? ptr + 1 : ptr;
			for ( ; isdigit(*q); q++ );
			if ( *q == '.' )
				for ( q=q+1; isdigit(*q); q++ );
		}
		else if ( isalpha(*ptr) || *ptr=='_' )
			for(q=ptr; isalnum(*q) || *q=='_'  /*|| *q=='.'*/; q++);		// a qualified token will be alpanumeric character + '_' 
		else
			q = ptr + 1;
		len = q - ptr;
	}
	memcpy( buf, ptr, len );
	buf[len] = '\0';
	/* skip white-space after token, also skip the trailing '\\' + '\n' which means concatenation to next line  */
//	for(; *q && isspace(*q); q++);
	for(; *q==' ' || *q=='\t' || (*q=='\\' && (*q+1)=='\n') || (*(q-1)=='\\' && (*q)=='\n'); q++);
	if ( pleft )
		*pleft=(char*)q;
	return len;
}

/* 
 * simplified version of strgettoken. It get a 'word' from input string. 'word' only separated
 * by white space (' ', '\t' and '\n').
/*/
int strgetword( const char *string, char *buf, int size, char **pleft)
{
	const char *ptr, *q;
	int len;

	/* skip leading white-space */
	for(ptr=string; *ptr && (*ptr==' ' || *ptr=='\t' || *ptr=='\n'); ptr++);
	if ( *ptr=='\0' ) return 0;
	for(q=ptr; *q && *q!=' ' && *q!='\t' && *q!='\n'; q++);
	len = q - ptr;
	if ( len < size )
	{
		memcpy( buf, ptr, len );
		buf[len] = '\0';
		if ( pleft )
			*pleft=(char*)q;
		return len;
	}
	return 0;
}

/* 
 * parse input stream 'src' and find first token (alpanumeric word) as keyword, 
 * if '=' presents after keyword, then next token after '=' as operand of keyword.
 * on return
 *  return value is length of keyword.
 * keyword will be stored in user supplied buffer 'keyword'
 * operand will be stored in user supplied buffer 'operand'. If no operand is found after 'keyword', operand will 
 * be returned as empty string. pleft will be point to next non-whitespace character in 'src' string
 * so that caller can continue to parse the input.
 */
int strgettokenpair( const char *src, char *keyword, char *operand, char **pleft )
{
	int rc;
	char *ptr;
	
	operand[0] = '\0';
	if ( (rc=strgettoken( src, keyword, &ptr )) > 0 && *ptr=='=' )
		strgettoken( ptr+1, operand, &ptr );
	if ( pleft != NULL )
		*pleft = ptr;
	
	return rc;	
}

/* 
 * parse input stream 'src' find a pattern of 
 * case 1: <keyword>
 * case 2: <keyword>=<operand>
 * case 3: <keyword>=<value1>,<value2>
 * on return
 * return value is length of keyword.
 * keyword will be stored in user supplied buffer 'keyword'
 * characters after '=' will be stored in user supplied buffer 'operand'. for case 1, operand will be empty.
 * pVal1 will be stored with value1 (for case 2 when operand is a number, and case 3), 
 *  		 otherwise, stored with -1.
 * pVal2 will be stored with value2 (for case 3), otherwise, stored with -1.
 * pleft will be point to the first non-whitespace character after pattern group 
 * so that caller can continue to parse the input.
 */
int strgettokenpair2( const char *src, char *keyword, char *operand, int *pVal1, int *pVal2, char **pleft )
{
	int rc;
	char *ptr;
	char oprnd1[16], oprnd2[16];
	
	operand[0] = '\0';
	*pVal1 = *pVal2 = -1;
	if ( (rc=strgettoken( src, keyword, &ptr )) > 0 && *ptr=='=' )
	{
		if ( strgettoken( ptr+1, oprnd1, &ptr )>0 )
		{
			if ( *ptr==',' && strisnumber(oprnd1,pVal1) )
			{
				char *ptr0 = ptr + 1;
				if (strgettoken( ptr0, oprnd2, &ptr )>0 && strisnumber(oprnd2,pVal2) )
				{
					strcpy( operand, oprnd1 );
					strcat( operand, "," );
					strcat( operand, oprnd2 );
				}
				else
				{
					// missing oprnd2 or oprnd2 is not an integer - terminated after ","
					*pVal2 = *pVal1;
					ptr = ptr0;		// terminate the parsing right after ','
				}
			}
			else
			{
				// after oprnd1 is not a ',' or is ',' but token after ',' is not a integer value. 
				// skip ',' and terminate the parsing.
				strcpy( operand, oprnd1 );
				if ( *ptr==',' ) 
					ptr++;
				else
					strisnumber(oprnd1,pVal1);	// if operand is a integer, stored in pVal1
			}
		}
	}	
	if ( pleft != NULL )
		*pleft = ptr;
	return rc;	
}

/* 
 * parse input stream 'src' find a pattern of 
 * case 1: <keyword>
 * case 2: <keyword>=<operand>
 * case 3: <keyword>=<value1>,<value2>,<value3>...
 * on return
 * return value is length of keyword.
 * keyword will be stored in user supplied buffer 'keyword'
 * characters after '=' will be stored in user supplied buffer 'operand'. for case 1, operand will be empty.
 * Val array will be stored with value1 ~ valueN (for case 2 when operand is a number, and case 3), 
 *  	 stored with -1.
 * vlmit is I/O parameter. On input it is upper limit of number of values after '='
 *       on return, it is actual number of values stored in Val. 
 * pleft will be point to the first non-whitespace character after pattern group 
 * so that caller can continue to parse the input.
 */
int strgettokenpairN( const char *src, char *keyword, char *operand, int Val[], int *vlimit, char **pleft )
{
	int i, rc;
	char *ptr, *ptr1=NULL;
	char oprnd[16];
	
	operand[0] = '\0';
	for(i=0; i<*vlimit; i++) Val[i] = -1;
	i = 0;
	if ( (rc=strgettoken( src, keyword, &ptr )) > 0 && *ptr=='=' )
	{
		// TODO - it is a little dangerous as we assume operand point to a space of 32 bytes.
		// caller may only provide smaller one. Be allert on this. 
		strgetword( ptr+1, operand, 32, &ptr1 );
		while ( strgettoken( ptr+1, oprnd, &ptr )>0 && strisnumber(oprnd,Val+i) )
		{
			if (++i == *vlimit ) break;
			if ( *ptr!=',' ) break;
		}
		ptr = ptr1;
	}	
	*vlimit = i;
	if ( pleft != NULL )
		*pleft = ptr;
	return rc;	
}

/*
 * strbreak - break input string into many tokens which separated by white spaces
 * return value:
 *   number of token
 * arguments:
 *  
 */
int strbreak(const char *src, char *args[], int arg_limit)
{
	char buf[256];
	int argc = 0;
	char *ptr = (char *)src;
	
	while ( argc < arg_limit && strgetword(ptr, buf, sizeof(buf), &ptr)>0 )
	{
		args[argc++] = strdup(buf);
	}
	return argc;
}

void strrelease(int argc, char *args[])
{
	int i;
	for(i=0; i<argc; i++)
		free(args[i]);
}

int stridxinargs( const char *key, int minmatch,...)
{
	va_list va;
	char *matcharg;
	int  matched_arg = -1;
	int arg_index = 0;
	
	va_start( va, minmatch );
	while ( (matcharg = va_arg( va, char *)) != NULL )
	{
		if ( (minmatch==0 && strcmp(key, matcharg)==0) ||
		    (minmatch && strstartwith( key, matcharg, minmatch, NULL) ) )
		{
			matched_arg = arg_index;
			break;
		}
		arg_index++;
	}
	va_end(va);
	return matched_arg;
}

extern int stridxinarray( const char *key, int minmatch, const char * array[], int size)
{
	int i;
	if ( size==0 )
		for(i=0; array[i]!=(char *)0; i++,size++ );
	for( i=0; i<size; i++ )
	{
		if ((minmatch==0 && strcmp(key, array[i])==0) ||
		    (minmatch && strstartwith( key, array[i], minmatch, NULL) ) )
			return i;
	}
	return -1;
}

char *strdupline( const char *src, char **next_pos )
{
	const char *ptr;
	char *line;
	int len;
	
	if ( *src == '\0' ) return NULL;
	if ( (ptr=strchr(src,'\n')) != NULL )
	{
		len = ptr - src;
		if ( *(ptr-1)=='\r' ) len--;
		*next_pos = (char *)(ptr + 1);
	}
	else
	{
		len = strlen(src);
		*next_pos = (char *)(src + len);
	}
	line = malloc( len + 1 );
	if ( len > 0 )
		memcpy( line, src, len );
	line[len] = '\0';
	return line;
}

int strrowcol( const char *src, int *cols )
{
	int lines=0, len, max_cols=0;
	const char *ptr=src, *plend;
	while( ptr && *ptr )
	{
		if ( (plend=strchr(ptr,'\n')) != NULL )
		{
			len = plend - ptr;
			if ( *(plend-1)=='\r' ) len--;
			ptr = plend + 1;
		}
		else
		{
			len = strlen(ptr);
			ptr += len;
		}
		if ( len > max_cols )
			max_cols = len;
		lines++;
	}
	if ( cols )
		*cols = max_cols;
	return lines;
}

/*
static char int2code( int n )
{
	char	ch = '?';

	if ( 0 <= n && n < 10 )
		ch = n + '0';
	else if ( 10 <= n && n < 36 )
		ch = (n - 10) + 'A';
	else if ( 36 <= n && n < 62 )
		ch = (n - 36) + 'a';
	else if (n == 62)
		ch = '+';
	else if (n==63)
		ch = '=';
	return ch;
}

static int code2int( char ch )
{
	int	rc=0;

	if ( '0' <= ch && ch <= '9' )
		rc = ch - '0';
	else if ( 'A' <= ch && ch <= 'Z' )
		rc = ch - 'A' + 10;
	else if ( 'a' <= ch && ch <= 'z' )
		rc = ch - 'a' + 36;
	else if ( ch == '+' )
		rc = 63;
	else if ( ch == '=' )
		rc = 63;
	return rc;
}


char *bitset_to_str( unsigned long bits, int nbit, char *str, int size )
{
	int	i;
	char	*p, *p0;

	p0 = str;
	p = p0;
	*p0 = '\0';
	for (i=0; i<nbit; i++)
	{
		if ( bits & (1<<i) )
		{
			sprintf( p, "%02d ", i );
			p += 3;
			if ( p - p0 + 3 > size ) break;
		}
	}
	if ( p != p0 ) *(--p) = '\0';
	return p0;
}

unsigned long str_to_bitset( const char* str )
{
	unsigned long 	bits = 0;
	char		buf[ 512 ];
	char		*token, *ptr;
	int		n;

	strcpy( buf, str );
	token = strtok_r( buf, " \t,", &ptr );
	while ( token )
	{
		n = atoi( token );
		if ( 0 <= n && n < 32 )
			bits |= ( 1 << n );
		token = strtok_r( NULL, " \t,", &ptr );
	}
	return bits;
}

char *bitset1_to_str( unsigned long bits, int nbit, char *str, int size )
{
	int	i;
	char	*p, *p0;

	p0 = str;
	p = p0;
	for (i=0; i<nbit; i++)
	{
		if ( bits & (1<<i) )
		{
			sprintf( p, "%02d ", i+1 );
			p += 3;
			if ( p - p0 + 3 > size ) break;
		}
	}
	if ( p != p0 ) *(--p) = '\0';
	return p0;
}

unsigned long str_to_bitset1( const char* str )
{
	unsigned long 	bits = 0;
	char		buf[ 512 ];
	char		*token, *ptr;
	int		n;

	strcpy( buf, str );
	token = strtok_r( buf, " \t,", &ptr );
	while ( token )
	{
		n = atoi( token );
		if ( 0 < n && n <= 32 )
			bits |= ( 1 << (n-1) );
		token = strtok_r( NULL, " \t,", &ptr );
	}
	return bits;
}

// str like "1,3-8,12" -- 1-based, 1 is bit0 
char *bitset1_to_str_ext( unsigned long bits, int nbit )
{
	int	i, j;
	char	*p, *p0;
	static char _mybuf[128];
	
	p = p0 = _mybuf;
	*p0 = '\0';
	for (i=0; i<nbit; i++)
	{
		if ( bits & (1<<i) )
		{
			if ( (i==nbit-1) || (bits & (1<<(i+1))) == 0 )
			{	// isolated '1' bit 
				sprintf( p, "%d,", i+1 );
				p += strlen(p);
			}
			else
			{	// cluster '1' bits 
				for( j=i+1; j<nbit && (bits & (1<<j)); j++ );
				sprintf( p, "%d-%d,", i+1, j );
				p += strlen( p );
				i = j + 1;
			}
		}
	}
	if ( p != p0 ) *(--p) = '\0';
	return p0;
}

unsigned long str_to_bitset1_ext( const char *str )
{
	int	i, b1, b2;
	unsigned long	bits = 0UL;
	const char	*ptr = str;
	
	while ( *ptr )
	{
		for( ; *ptr==' ' ;  ) ptr++;
		b1 = atoi( ptr );
		for( ; isdigit(*ptr); ) ptr++;
		for( ; *ptr==' ' ;  ) ptr++;
		if ( *ptr==',' || *ptr == '\0' || isdigit(*ptr) )
		{
			// isolate bit - set it 
			if ( b1 <= 32 )
				bits |= ( 1 << (b1-1) );
		}
		else if ( *ptr == '-' )
		{
			ptr++;
			for( ; *ptr==' ' ;  ) ptr++;
			if ( *ptr == '\0' )	// 18- means 18 to last bit
				b2 = 32;
			else
			{
				b2 = atoi( ptr );
				if ( b2 > 32 ) b2 = 32;
			}
			for( ; isdigit(*ptr); ) ptr++;
			for( ; *ptr==' ' ;  ) ptr++;
			if ( b1 <= b2 )
			{
				for( i=b1-1; i<b2; i++ )
					bits |= ( 1 << i );
			}
		}
		if ( *ptr == ',' ) ptr++;
	}
	return bits;
}			
*/
		
static const char* to_b64 =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	"abcdefghijklmnopqrstuvwxyz"
	"0123456789+/";

/* encode 72 characters	per	line */
#define	CHARS_PER_LINE	72

typedef unsigned char	byte;

/* return value:
 * >0: encoded string length, -1 buf too small
 * Encoded string is terminated by '\0'.
 */
int str_b64enc(const char *src, char *buf, int bufsize )
{
	int	size	= strlen( src );
	int	div	= size / 3;
	int	rem	= size % 3;
	int	chars 	= div*4 + rem + 1;
	int	newlines = (chars + CHARS_PER_LINE - 1)	/ CHARS_PER_LINE;
	int	outsize  = chars + newlines;
	const byte*	data = (const byte *)src;
	byte*		enc = (byte *)buf;

	if ( bufsize < outsize + 1 ) return -1;
	chars =	0;
	while (div > 0)
	{
		enc[0] = to_b64[ (data[0] >> 2)	& 0x3f];
		enc[1] = to_b64[((data[0] << 4)	& 0x30)	+ ((data[1] >> 4) & 0xf)];
		enc[2] = to_b64[((data[1] << 2)	& 0x3c)	+ ((data[2] >> 6) & 0x3)];
		enc[3] = to_b64[ data[2] & 0x3f];
		data +=	3;
		enc  += 4;
		div--;
		chars += 4;
		if (chars == CHARS_PER_LINE)
		{
			chars =	0;
//			*(enc++) = '\n';	/* keep the encoded string in single line */
		}
	}

	switch (rem)
	{
		case 2:
			enc[0] = to_b64[ (data[0] >> 2)	& 0x3f];
			enc[1] = to_b64[((data[0] << 4)	& 0x30)	+ ((data[1] >> 4) & 0xf)];
			enc[2] = to_b64[ (data[1] << 2)	& 0x3c];
			enc[3] = '=';
			enc   += 4;
			chars += 4;
			break;
		case 1:
			enc[0] = to_b64[ (data[0] >> 2)	& 0x3f];
			enc[1] = to_b64[ (data[0] << 4)	& 0x30];
			enc[2] = '=';
			enc[3] = '=';
			enc   += 4;
			chars += 4;
			break;
	}

	*enc = '\0';
	return strlen(buf);		// exclude the tail '\0'
}

/*
 * decode a base64 encoded string.
 * return -1: bufsize too small, \
 *         0: string content error,
 *       > 0: decoded string (null terminated).
 */
int  str_b64dec(const char* string, char *buf, int bufsize)
{
	register int length = string ? strlen(string) : 0;
	register byte* data = (byte *)buf;

	/* do a	format verification first */
	if (length > 0)
	{
		register int count = 0,	rem	= 0;
		register const char* tmp = string;

		while (length >	0)
		{
			register int skip = strspn(tmp,	to_b64);
			count += skip;
			length -= skip;
			tmp += skip;
			if (length > 0)
			{
				register int i,	vrfy = strcspn(tmp, to_b64);

				for (i = 0; i < vrfy; i++)
				{
					if (isspace(tmp[i])) continue;
					if (tmp[i] == '=')
					{
						/* we should check if we're close to the end of	the string */
						if ( (rem = count % 4) < 2 )
							/* rem must be either 2	or 3, otherwise	no '=' should be here */
							return 0;
						/* end-of-message recognized */
						break;
					}
					else
					{
						/* Invalid padding character. */
						return 0;
					}
				}
				length -= vrfy;
				tmp    += vrfy;
			}
		}
		if ( bufsize < (count/4 * 3 + (rem ? (rem-1) : 0)) )
			return -1;

		if (count > 0)
		{
			register int i,	qw = 0;

			length = strlen(string);
			for (i = 0; i < length; i++)
			{
			register char ch = string[i];
				register byte bits;

				if (isspace(ch)) continue;

				bits = 0;
				if ((ch	>= 'A')	&& (ch <= 'Z'))
				{
					bits = (byte) (ch - 'A');
				}
				else if	((ch >=	'a') &&	(ch <= 'z'))
				{
					bits = (byte) (ch - 'a'	+ 26);
				}
				else if	((ch >=	'0') &&	(ch <= '9'))
				{
					bits = (byte) (ch - '0'	+ 52);
				}
				else if	(ch == '=')
					break;

				switch (qw++)
				{
					case 0:
						data[0] = (bits << 2) & 0xfc;
						break;
					case 1:
						data[0] |= (bits >> 4) & 0x03;
						data[1] = (bits << 4) & 0xf0;
						break;
					case 2:
						data[1] |= (bits >> 2) & 0x0f;
						data[2] = (bits << 6) & 0xc0;
						break;
					case 3:
						data[2] |= bits & 0x3f;
						break;
				}
				if (qw == 4)
				{
					qw = 0;
					data += 3;
				}
			}
			data += qw;
			*data = '\0';
		}
	}
	return data - (unsigned char *)buf;
}
