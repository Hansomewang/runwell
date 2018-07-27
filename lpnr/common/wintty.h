/*
 *	wintty.h
 *
 *  TTY i/o and control functions
 */
#ifndef _WINTTY_H_
#define _WINTTY_H_
#include <tchar.h>

#ifdef __cplusplus
extern "C" {
#endif

/* tty_open: open the COM port device and return file handler.
 *   device: device file like "/dev/ttyS0" in linux, or "COM1" in Windows.
 * return value:
 *   >0: file descriptor (fd) of tty port. 
 *   -1: open failure
 */
extern int tty_open( TCHAR *device );	// "COM1", "COM2"..."COM8", or "\\\\.\\COM9" ...
extern int tty_openPort( int port );		// 1 for COM1, 2 for COM2... 

// tty_close: close the COM port
extern int tty_close( int fd_tty );

/*
 *  tty_raw: set tty to raw mode at gievn baudrate.
 *	   baud: baudrate (256000, 128000, 115200, 57600, 56000, 38400, 19200, 14400, 9600, 4800, 2400, 1200, 600, 300, 110)
 *	   nbit: 8, 7, 6.
 *	   parity: 0 (none), 1 odd, 2 even, 3 Mark 4 Space
 *	  Note: stop bit is always 1 
 */
extern int tty_raw( int fd_tty, int baud, int nbit, int parity );
#define PRTY_NONE	0
#define PRTY_ODD	1
#define PRTY_EVEN	2
#define PRTY_MARK	3
#define PRTY_SPACE	4
/*
 * tty_raw only set default number of stop bits as 1.If you need other setting, use tty_stopbit 
 * nStopBits can be 0 for 1 stopbit, 1 for 1.5 stopbit, 2 for 2 stopbits. 
 * Note: This function shall be used after tty_raw. Otherwise, will be reset to 1 stopbit.
 * return value:
 * 0: successful
 * -1: failed
 */
#define STOPBITS_ONE          0
#define STOPBITS_ONEHALF      1
#define STOPBITS_TWO	      2

extern int tty_stopbit( int fd_tty, int nStopBits );	
/*
 * tty_setReadTimeOuts:
 *	Set the time out parameter for tty read.
 *	Parameters: (all in msec)
 *    fd:            File handle returned by tty_open
 *	  tout_interval: time out constant between any two characters. (0 for no interval timeout)
 *	  tout_tm:       total timeout multiplier.
 *	  tout_tc:		 total timeout constant. 
 *	Note: for a nBytes read operation, total elapsed time out = tout_tc + tout_tm * nByte
 *		  both tout_tm and tout_tc are 0 means no total read timeout.
 *  Defualt setting by tty_raw is:
 *	  tout_interval = 10
 *	  tout_tc = 50
 *	  tout_tm = 1
 */
extern int tty_setReadTimeouts( int fd, int tout_interval, int tout_tm, int tout_tc );

/*
 *  tty_ready: check if any data in tty driver's input buffer.
 *		tout: time out setting.
 *		 > 0 time out in milli-seconds.
 *         0 no time out, return immediately if no input in buffer.
 *        -1 wait until a character is read.
 *  return value:
 *		1: data ready
 *		0: no data
 *	   -1: I/O error.
 *  note: For performance issue, when 'tout' > 0, we only check the input availability on 5 msec interval
 *		  except last time slice which is the remainder of 'tout' divided by 5.
 *		  So if tout is 8 msec, and data is received at 3 msec, then this function return 1 after 5 msec. 
 *		  If data is received at 6 msec, then this function return 1 after 8 msec.
 */
extern int tty_ready( int fd, int tout );
extern int tty_iqueue( int fd );			// return number of bytes in input queue. -1 is I/O error 

/*
 * tty_getc: read one raw character from tty device. Special key with escape sequence
 *           will be converted to single key code.
 *     fd_tty: fd of tty (must call term_raw once, before use this function)
 *     tout: same as 'tout' parameter in tty_ready
 *  return value:
 *      >= 0: character received from tty
 *        -1: no data.
 *        -2: I/O error
 */
extern int tty_getc( int fd, int tout );
extern int tty_putc(int fd, int code );

/* tty_gets - read until speciied bytes or termination character in received.
 * parameters:
 *    buf: caller's buffer
 *	  len: max. number of bytes to read.
 *    eol: end of line character. If 0 then read 'len' bytes (unless time out)
 * return value:
 *    >0: number of bytes received.
 *     0: nothing is read (time out or TTY I/O error before any character is receied)
 *  NOTE: 
 *	  (1) buf is null terminated. So sizeof(buf) shall large enough to hold the ending '\0' (>len);
 *    (2) time out is depends on current tty read time out setting.
 *	  (3) read is terminated either 'len' bytes is received or 'eol' character is encountered - whichever comes first.
 *	  (4) tty_gets may terminated abnormally (before read is satisfied) - time out or I/O error.
 */
extern int tty_gets( int fd, char *buf, int len, int eol );

// skip until soh byte sequence is found (which is 'size' bytes). return value is number of bytes skipped.
// if 'found' is true. means that 'soh' byte sequence is found and read out of input buffer.
extern int tty_skip_until(int fd, unsigned char *soh, int size, int *found );
// skip until one of soh is found. For multiple version of protocols with different SOH.
// soh is an array of pointer, each entry point to one SOH sequence, total 'nh' entries.
// each header must have same length 'size'.
// on return,  return value is number of bytes skipped and if 'soh_index' is not null
// it's content will be stored with which SOH is encounted (0-based index). -1 means
// none of SOH is found.
extern int tty_skip_until_oneof(int fd, unsigned char *soh[], int nh, int size, int *soh_index );
// get all bytes skipped in 'tty_skip_until' or 'tty_skip_until_oneof'. Please noticed that
// skipped bytes are stored in a static buffer with fixed size. If skipped number of bytes
// is more than is buffer size, extra data are lost. And since it is a static, so it won't  be
// thread safe. Event different tty port share same buffer. So it is not 100% sure skipped
// data will be survived un-touched from tty_skip_until to tty_get_skipped.
extern const unsigned char *tty_get_skipped();
extern int tty_read_n_bytes(int fd, void *buffer, int len);

extern int tty_read( int fd_tty, void *buffer, int len );
extern int tty_write( int fd_tty, void *buffer, int len );
extern int tty_clear( int fd );			// clear Tx/Rx buffer. All pending characters are dropped.

#ifdef __cplusplus
}
#endif

#endif

