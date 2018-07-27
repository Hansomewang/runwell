/*
 * wintty.c
 *
 *	Utility functions for Windows COM port I/O.
 */
#include <stdio.h>
#include <io.h>
#include <windows.h>
#include "wintty.h"

int tty_read( int fd_tty, void *buffer, int len )
{
	HANDLE	hFile = (HANDLE)_get_osfhandle( fd_tty );
	DWORD	dwBytesRead;

	if ( ReadFile( hFile, buffer, len, &dwBytesRead, NULL ) )
		return dwBytesRead;
	else
		return 0;
}

int tty_write( int fd_tty, void *buffer, int len )
{
	HANDLE	hFile = (HANDLE) _get_osfhandle( fd_tty );
	DWORD   dwBytesWritten ;

	if (WriteFile( hFile, buffer, len, &dwBytesWritten, NULL ) )
		return dwBytesWritten;
	else
		return 0;
}

int tty_openPort( int nPort )
{
	TCHAR	strDevice[32];

	_stprintf( strDevice, _T("\\\\.\\COM%d"), nPort );
	return tty_open( strDevice );
}

int tty_open( TCHAR *device )
{
	HANDLE hFile;
	hFile = CreateFile( device, GENERIC_READ | GENERIC_WRITE,
						0,                  // exclusive access
						NULL,               // no security attrs
						OPEN_EXISTING,
						0,					// no overlapped I/O
						NULL );
	if( hFile == INVALID_HANDLE_VALUE )
		return ( -1 ) ;
	return _open_osfhandle( (long)hFile, 0 );
}

int tty_close( int fd_tty )
{
	HANDLE hFile = (HANDLE)_get_osfhandle( fd_tty );
	PurgeComm(hFile, PURGE_TXABORT|PURGE_RXABORT|PURGE_TXCLEAR|PURGE_RXCLEAR);
	return CloseHandle( hFile );
}

int tty_raw( int fd_tty, int baud, int nbit, int parity )
{
	HANDLE hFile = (HANDLE)_get_osfhandle( fd_tty );
	COMMTIMEOUTS CommTimeOuts;
	DCB dcb;

	// only enable the event of RxChar
	SetCommMask( hFile, EV_RXCHAR ) ;

	// setup device buffers
	SetupComm( hFile, 4096, 4096 ) ;

	// purge any information in the buffer
	PurgeComm( hFile, PURGE_TXABORT | PURGE_RXABORT |  PURGE_TXCLEAR | PURGE_RXCLEAR ) ;

	GetCommState(hFile, &dcb);
	dcb.BaudRate = baud;
	dcb.ByteSize = nbit;
	dcb.StopBits = ONESTOPBIT;
	dcb.fAbortOnError = TRUE;
	dcb.fDtrControl = DTR_CONTROL_DISABLE;
	dcb.fDsrSensitivity = FALSE;
	dcb.fOutX = FALSE;
	dcb.fInX = FALSE;
	dcb.fRtsControl = RTS_CONTROL_DISABLE;
	switch ( parity )
	{
	case PRTY_ODD:
		dcb.Parity = ODDPARITY;
		break;
	case PRTY_EVEN:
		dcb.Parity = EVENPARITY;
		break;
	case PRTY_MARK:
		dcb.Parity = MARKPARITY;
		break;
	case PRTY_SPACE:
		dcb.Parity = SPACEPARITY;
		break;
	case PRTY_NONE:
	default:
		dcb.Parity = NOPARITY;
		break;
	}

	if( !SetCommState(hFile, &dcb) )
	{
		//CloseHandle(hFile);   do not close handle. let tty_close do the job
		return -1;
	}

	// set default time out */
	// for read. two time out is govern - interval time out is time between two characters.
	// total time out is max. elasped time to read nBytes. 
	//    total time out = ReadTotalTimeoutConstant + nBytes * ReadTotalTimeoutMultiplier
	CommTimeOuts.ReadIntervalTimeout = 10;
	CommTimeOuts.ReadTotalTimeoutConstant = 100;
	CommTimeOuts.ReadTotalTimeoutMultiplier = 1;
	// for write. real total time out = 'WriteTotalTimeoutMultiplier' * bytes-to-write + 'WriteTotalTimeoutConstant'
	CommTimeOuts.WriteTotalTimeoutMultiplier = 10;	// character interval time out
	CommTimeOuts.WriteTotalTimeoutConstant = 50;	// 50 msec total time out 
	SetCommTimeouts(hFile, &CommTimeOuts);

	// assert DTR
//	EscapeCommFunction( hFile, SETDTR ) ;
	return 0;
}

int tty_stopbit( int fd_tty, int nStopBits )
{
	HANDLE hFile = (HANDLE)_get_osfhandle( fd_tty );
	DCB dcb;
	// get current Comm state
	GetCommState(hFile, &dcb);
	dcb.StopBits = nStopBits;
	return SetCommState(hFile, &dcb) ? 0 : -1;
}

int tty_setReadTimeouts( int fd_tty, int tout_interval, int tout_tm, int tout_tc )
{
	HANDLE hFile = (HANDLE)_get_osfhandle( fd_tty );
	COMMTIMEOUTS CommTimeOuts;
	
	GetCommTimeouts( hFile, &CommTimeOuts );
	CommTimeOuts.ReadIntervalTimeout = tout_interval;
	CommTimeOuts.ReadTotalTimeoutMultiplier = tout_tm;
	CommTimeOuts.ReadTotalTimeoutConstant = tout_tc;
	return SetCommTimeouts(hFile, &CommTimeOuts) ? 0 : -1;
}

/*
 * check if any character is in the input buffer within specified time (msec)
 * return value:
 *   1: data ready to read
 *   0: no data
 *  -1: TTY I/O error (fd is not a valid COM port)
 */
int tty_ready( int fd, int tout )
{
	HANDLE		hFile = (HANDLE)_get_osfhandle( fd );
	DWORD		dwEvent;
	COMSTAT		ComStat ;
	DWORD		dwErrorFlags;
	int			rc = -1;
	
	if ( tout == -1 )
	{
		// wait for Receving character event.
		if( WaitCommEvent( hFile, &dwEvent, NULL ) )
			rc = (dwEvent & EV_RXCHAR) != 0;
	}
	else if ( tout == 0 )
	{
			if ( ClearCommError( hFile, &dwErrorFlags, &ComStat ) )
				rc = ComStat.cbInQue > 0;
	}
	else // tout > 0
	{
		LONGLONG ft0, ft1;
		tout *= 10000;			// convert to 100 nsec

		GetSystemTimeAsFileTime( (LPFILETIME)&ft0 );
		rc = 0;
		do
		{
			if ( ClearCommError( hFile, &dwErrorFlags, &ComStat ) )
			{
				rc = ComStat.cbInQue > 0;
			}
			else // if ( GetLastError() != ERROR_ACCESS_DENIED )
			{
				rc = -1;
			}
			if ( rc == 0 )
				Sleep( 5 );
			GetSystemTimeAsFileTime( (LPFILETIME)&ft1 );
		} while ( rc==0 && (int)(ft1 - ft0) < tout );
	}
	return rc;
}

/*
 * tty_getc: read one raw or cooked character from tty device.
 * fd_tty: fd of tty (must call term_raw once before use this function)
 * tout: time out setting.
 *	 > 0 return either character received or no input within this 'tout' time period.
 *     0 read w/o wait, return immediately if no input in buffer.
 *    -1 wait until a character is read.
 *  return value:
 *      >= 0: character received from tty (0 could be a valid COM port input)
 *        -1: no data. (timed out)
 *		  -2: I/O error.
 */

int tty_getc( int fd, int tout )
{
		int		rc;
		unsigned char	ch = 0;

    	if ( (rc = tty_ready( fd, tout )) > 0 )
    	{
    		rc = tty_read( fd, &ch, 1 );
        	return rc == 1 ? ch : -2;
        }
        /* timed out or I/O error */
   		return rc-1;	/* 0 -> -1, -1 -> -2 */
}

int tty_putc(int fd, int code )
{
	return tty_write(fd, &code, 1);
}

/* tty_gets - read until speciied bytes or termination character in received.
 * parameters:
 *    buf: caller's buffer
 *	  len: max. number of bytes to read.
 *    eol: end of line character. If 0 then read 'len' bytes (unless time out)
 * return value:
 *    >0: number of bytes received.
 *     0: nothing is read (time out or TTY I/O error)
 *  NOTE: 
 *	  (1) buf is null terminated. So sizeof(buf) shall large enough to hold the ending '\0';
 *    (2) time out is depends on current tty read time out setting.
 *	  (3) read is terminated either 'len' bytes is received or 'eol' character is encountered - whichever comes first.
 *	  (4) tty_gets may terminated abnormally (before read is satisfied) - time out or I/O error.
 */
int tty_gets( int fd, char *buf, int len, int eol )
{
	int	nget = 0;
	int	ch=0, rc;

	while ( nget < len && (rc=tty_read( fd, &ch, 1 ))==1 )
	{
		*buf++ = ch;
		nget++;
		if ( eol!=0 && eol==ch ) break;
	}
	*buf = '\0';
	return nget;
}

int tty_clear( int fd )
{
	HANDLE		hFile = (HANDLE)_get_osfhandle( fd );
	return PurgeComm( hFile, PURGE_TXCLEAR | PURGE_RXCLEAR ) == 0 ? -1 : 0;
}

int tty_iqueue( int fd )
{
	HANDLE		hFile = (HANDLE)_get_osfhandle( fd );
	COMSTAT		ComStat ;
	DWORD		dwErrorFlags;

	if ( ClearCommError( hFile, &dwErrorFlags, &ComStat ) )
		return ComStat.cbInQue;
	return -1;
}

#define MAX_SKIP_COUNT	4096
static unsigned char byte_skip[MAX_SKIP_COUNT];
const unsigned char *tty_get_skipped()
{
	return byte_skip;
}

int tty_skip_until(int fd, unsigned char *soh, int size, int *found )
{
	unsigned char ch;
	int i, rc;
	int nskip = 0;
	*found = 0;
catch_again:
	while ( (rc=tty_read(fd, &ch,1))==1 && ch != soh[0] ) 
	{
		if ( nskip < MAX_SKIP_COUNT )
			byte_skip[nskip] = ch;
		nskip++;
	}
	if ( rc != 1 ) return nskip;
	for(i=1; i<size; i++)
	{
		if ( (rc=tty_read(fd,&ch,1))!=1 || ch!=soh[i] )
		{
			if ( rc==1 ) 
			{
				if ( nskip < MAX_SKIP_COUNT )
					byte_skip[nskip] = ch;
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
	unsigned char ch;
	int i, rc;
	int nskip = 0;
	int index;

	*soh_index = -1;
catch_again:
	index = -1;
	// get first byte in any one of SOH sequence
	while ( (rc=tty_read(fd, &ch,1))==1 &&  (index=byte_in_oneof(ch,soh,0,nh))==-1 )
	{
		if ( nskip < MAX_SKIP_COUNT )
			byte_skip[nskip] = ch;
		nskip++;
	}
	if ( rc != 1 ) return nskip;
	for(i=1; i<size; i++)
	{
		if ( (rc=tty_read(fd,&ch,1))!=1 || byte_in_oneof(ch,soh,i,nh)!=index)
		{
			if ( rc==1 ) 
			{
				if ( nskip < MAX_SKIP_COUNT )
					byte_skip[nskip] = ch;
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
			Sleep(10);
			len = tty_read(fd, ptr, n);
		}
		if ( len==0 ) break;
		if( len == -1 )
			return -1;
		ptr += len;
		n -= len;
	}
	return (int)(ptr-(char*)buffer);	
}
