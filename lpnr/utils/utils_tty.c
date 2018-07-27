#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include "utils_tty.h"
#include "longtime.h"

#ifdef linux
#include <unistd.h>
#endif
/* init terminal so that we can grab keys */
void tty_set( int fd, struct termios *p_tty )
{
#ifdef linux
    tcsetattr (fd, TCSADRAIN, p_tty);
#endif
}

int tty_raw( int fd_tty, struct termios *p_tty, int baud )
{
#ifdef linux
   	 static int baudrate[] = {600, 1200, 1800, 2400, 4800, 9600, 19200, 38400, /* 0~7*/
   	 	57600, 115200, 230400, 460800, 500000, 576000, 921600,	/* 8~14 */
   	 	1000000, 1152000, 1500000, 2000000 };		/* 15 ~ 18 */
    	struct termios tty;

    	if ( tcgetattr (fd_tty, &tty) != 0 )
		return -1;;
    	if ( p_tty != NULL )
    		memcpy( p_tty, &tty, sizeof( struct termios ) );
#if 1
    	tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP
                          |INLCR|IGNCR|ICRNL|IXON);
    	tty.c_oflag &= ~(OPOST|OLCUC|ONLCR|OCRNL|ONOCR|ONLRET);
    	tty.c_lflag &= ~(ECHO|ECHONL|ICANON|ISIG|IEXTEN);
    	tty.c_cflag &= ~(CSIZE|PARENB);
    	tty.c_cflag |= CS8;
    	tty.c_cc[VMIN] = 1;
    	tty.c_cc[VTIME] = 0;
#else
    	if (cfmakeraw( &tty ) != 0 );		// Same effect as code in above disabled codes
    		return -1;
#endif
    	if ( baud > 0 )
    	{
				speed_t line_speed = B9600;
				int i, n = sizeof(baudrate)/sizeof(int);
		
				for (i=0; i<n; i++)
	    		if ( baud == baudrate[i] ) break;
				if ( i < 8 )
	    		line_speed = B600 + i;
	    	else if (i < 15)
	    		line_speed = B57600 + i - 8;
	    	else if ( i < n )
	    		line_speed = B1000000 + i - 15;
	    	if ( cfsetspeed( &tty, line_speed ) != 0 )
	    		return -1;
     	}
    	return tcsetattr (fd_tty, TCSADRAIN, &tty);
#else
	return 0;
#endif
}

/*
 * check if any character is in the input buffer within specified time (msec)
 */
int tty_ready( int fd, int tout )
{
	int	n=0;
  fd_set	set;
	struct timeval tval, *ptval;

	ptval = &tval;
	if ( tout > 0 )
	{
		tval.tv_sec = tout / 1000;
		tval.tv_usec = (tout % 1000) * 1000;
	}
	else if ( tout == 0 )
	{
		tval.tv_sec = 0;
		tval.tv_usec = 0;
	}
	else	/* < 0, blocked until data available to read */
		ptval = NULL;

	FD_ZERO(& set);
	FD_SET(fd, & set);
	n = select( fd +1, & set, NULL, NULL, ptval );

	return n==1 && FD_ISSET(fd, &set);
}

/*
 * read one raw character from tty device.
 * fd_tty: fd of tty (must call term_raw once before use this function)
 * tout: time out setting.
 *	 > 0 time out in milli-seconds.
 *         0 no time out, return immediately if no input in buffer.
 *        -1 wait until a character is read.
 *  return value:
 *      >= 0: character received from tty
 *       -1: no data.
 */
#define MAX_SEQ		3
#define MSBCH		10		// 10 ms delay time between characters

int tty_getc( int fd, int tout, int esc_on )
{
	int		rc=-1;
	unsigned char	ch = 0;

	if ( (rc = tty_ready( fd, tout )) > 0 )
	{
		rc = read( fd, &ch, 1 );
#ifdef ENABLE_TTYESCSEQ		
		/* if this is a <Esc>, check for escape sequence
		 * If input is not a valid escape sequence. The char has been
		 * read and checked for will be lost. Sorry, I cannot unget the chars.
		 */
		if ( ch == KEY_ESC && esc_on && tty_ready(fd,MSBCH) )
		{
  		char	ch1, ch2;
			int	key;

			read(fd, &ch, 1);
			if ( ch != '[' )
				return ch;	// Sorry, Esc is gone.

			read(fd, &ch1, 1);
			switch( ch1 )
			{
				case 'A':	return KEY_UP;
				case 'B':	return KEY_DOWN;
				case 'C':	return KEY_RIGHT;
				case 'D':	return KEY_LEFT;
				case 'P':	return KEY_PAUSE;
				case '1':	key = KEY_HOME;		break;
				case '2':	key = KEY_INS;		break;
				case '3':	key = KEY_DEL;		break;
				case '4':	key = KEY_END;		break;
				case '5':	key = KEY_PGUP;		break;
				case '6':	key = KEY_PGDN;		break;
				default:
					key = 0;
			}
			if ( key != 0 && tty_ready(fd,MSBCH) )
			{
				read( fd, &ch2, 1 );
				if ( ch2 == '~' ) return key;
				if ( key == KEY_HOME )		// <Esc>[1x~
				{	/* check for F1 ~ F12 */
					if ( ch2 >= '1' && ch2 <= '5' )
						key = KEY_FN(ch2-'0');		// F1 ~ F5
					else if ( ch2 >= '7' && ch2 <= '9' )
						key = KEY_FN(ch2-'1');		// F6 ~ F8
					else
						key = 0;
				}
				else if ( key == KEY_INS )	// <Esc>[2x~
				{
					if ( ch2 == '0' || ch2 == '1' )
						key = KEY_FN(ch2-'0'+9);	// F9, F10
					else if ( '3' <= ch2 && ch2 <= '6' )
						key = KEY_SFN(ch2-'2');		// Shift-F1 ~ Shift-F4
					else if ( ch2=='8' || ch2=='9' )
						key = KEY_SFN(ch2-'3');		// Shift-F5, Shift-F6
					else
						key = 0;
				}
				else if ( key == KEY_DEL )	// <Esc>[3x~
				{
					if ( '1' <= ch2 && ch2 <= '4' )
						key = KEY_SFN(ch2-'1'+7);
					else
						key = 0;
				}
				if ( key != 0 )
				{
					read( fd, &ch2, 1 );		// eat last '~'
					if ( ch2 != '~' )
						key = 0;
				}
			}
 			return key == 0 ? ch1 : key;	// sorry drop few characters.
		}
#endif   		
	}
 	return rc == 1 ? ch : -1;
}

int tty_putc(int fd, int ch)
{
	return write(fd, &ch, 1);
}

/*
 * parameters:
 *   tout:
 *	0: No wait. Get until no character or 'eol' is received.
 *     -1: Get until buffer full or eol is get.
 *     >0: 'tout' msec after last character is received.
 *    eol: end of line character. If not 0 then receive until this 'eol' is encounted
 *         (or tout, or buffer full).
 *     ec: adress to hold the end code (0 normal ending, -1 tout, -2 buffer full)
 * return value:
 *    >0: number of bytes received.
 */
int tty_gets( int fd, char *buf, int bufsize, int tout, int eol, int *ec )
{
	int	nget = 0;
	int	ch=0;

	while ( nget < bufsize && (ch=tty_getc( fd, tout, 0 ))>=0 )
	{
		*buf++ = ch;
		nget++;
		if ( eol!=0 && eol==ch ) break;
	}
	if ( ch < 0 && ec )
		*ec = -1;
	else if ( ch!=eol && nget==bufsize && ec )
		*ec = -2;
	return nget;
}

// read N bytes. return number of bytes read.
// tout is timeout value.
//   0: no wait.
//  -1: wait until request # of character is filled
//  >0: msec to wait for next character. If timed-out, terminate the read
// return value:
//	 number of bytes read. read terminated when specified number of bytes 
//   has received or time out.
int tty_read_tout( int fd, char *buf, int bufsize, int tout )
{
	int	n, nc = 0;
	
	if ( tout == -1 )
	{
		while ( bufsize > 0 )
		{
			n = read( fd, buf, bufsize );
			nc += n;
			bufsize -= n;
			buf += n;
		}
	}
	else if ( tout == 0 )
	{
		nc = read(fd, buf, bufsize);
	}
	else
	{
		_longtime lt_end = timeGetLongTime() + tout;
		do
		{
			n = read( fd, buf, bufsize );
			nc += n;
			bufsize -= n;
			buf += n;
		} while ( bufsize>0 && timeGetLongTime() < lt_end );
	}
	return nc;
}

#ifdef WIN32
#define MAX_SKIP_COUNT	4096
static unsigned char byte_skip[MAX_SKIP_COUNT];
const unsigned char *tty_get_skipped()
{
	return byte_skip;
}
#endif

int tty_skip_until(int fd, unsigned char *soh, int size, int *found )
{
	int i, rc;
	int nskip = 0;
	*found = 0;
catch_again:
	while ( (rc=tty_getc(fd,size,0))>=0 && (unsigned char)rc != soh[0] ) 
	{
#ifdef WIN32		
		if ( nskip < MAX_SKIP_COUNT )
			byte_skip[nskip] = (unsigned char)rc;
#endif			
		nskip++;
	}
	if ( rc == -1 ) return nskip;
	for(i=1; i<size; i++)
	{
		if ( (rc=tty_getc(fd,size,0))<0 || (unsigned char)rc!=soh[i] )
		{
			if ( rc>=0 ) 
			{
#ifdef WIN32				
				if ( nskip < MAX_SKIP_COUNT )
					byte_skip[nskip] = (unsigned char)rc;
#endif					
				nskip++;
				goto catch_again;
			}
			else
				return nskip;
		}
	}
	*found = 1;
	return nskip;
}

static int byte_in_oneof(unsigned char ch, unsigned char *soh[],int index,int num_head)
{
	int i;
	for(i=0; i<num_head; i++)
		if ( ch==soh[i][index] )
			return i;
	return -1;
}

int tty_skip_until_oneof(int fd, unsigned char *soh[], int nh, int size, int *soh_index )
{
	int i, rc;
	int nskip = 0;
	int index;

	*soh_index = -1;
catch_again:
	index = -1;
	// get first byte in any one of SOH sequence
	while ( (rc=tty_getc(fd,size,0))>=0 &&  (index=byte_in_oneof((unsigned char)rc,soh,0,nh))==-1 )
	{
#ifdef WIN32		
		if ( nskip < MAX_SKIP_COUNT )
			byte_skip[nskip] = (unsigned char)rc;
#endif			
		nskip++;
	}
	if ( rc == -1 ) return nskip;
	for(i=1; i<size; i++)
	{
		if ( (rc=tty_getc(fd,size,0))<0 || byte_in_oneof((unsigned char)rc,soh,i,nh)!=index)
		{
			if ( rc>=0 ) 
			{
#ifdef WIN32				
				if ( nskip < MAX_SKIP_COUNT )
					byte_skip[nskip] = ch;
#endif					
				nskip++;
				goto catch_again;
			}
			else
				return nskip;
		}
	}
	*soh_index = index;
	return nskip;
}

int tty_read_n_bytes(int fd, void *buffer, int n)
{
	char *ptr = (char *)buffer;
	int len;
	while( n > 0 ) 
	{
		len = tty_read(fd, ptr, n);
		if ( len == 0 )
		{
			usleep(5000);
			len = tty_read(fd, ptr, n);
		}
		if ( len==0 ) break;
		if( len == -1 )
			return -1;
		ptr += len;
		n -= len;
	}
	return (ptr-(char*)buffer);	
}



#ifdef ENABLE_TTYESCSEQ
//-------------------  c o n t r o l ---------------------
void tty_goto( int fd, int x, int y )
{
	char *escape="\033[%d;%dH";
	char cmd[64];

	sprintf( cmd, escape, x, y );
	write( fd, cmd, strlen(cmd) );
}

// attribute list argument must be terminated by -1
// ex.: tty_attribute( 1, 1, 5, 34, 40, -1 );

void tty_attribute( int tty, ... )
{
	va_list	va;
	int	attr, narg = 0;
	char	cmd[80] = "\033[";
	char	*ptr = cmd + 2;

	va_start( va, tty );

	while ( ( attr=va_arg(va,int) ) != -1 )
	{
		if ( narg == 0 )
			sprintf( ptr, "%d", attr );
		else
			sprintf( ptr, ";%d", attr );
		ptr += strlen( ptr );
		narg++;
	}

	va_end( va );

	if ( narg )
	{
		*ptr ++ = 'm';
		*ptr ++ = '\0';
		write( tty, cmd, strlen(cmd) );
	}
}
#endif
