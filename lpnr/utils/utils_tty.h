/*
 *	utils_tty.h
 *
 *  TTY i/o and control functions
 */
#ifndef _UTILS_TTY_H_
#define _UTILS_TTY_H_

#include <termios.h>

#ifdef __cplusplus
extern "C" {
#endif


/*   fd_tty: the fd for a tty.
 *   p_tty:  point to caller's buffer to store the original setting of the fd.
 *   baud:   the baud rate setting. 0 means do not change the orignal line speed.
 */
int tty_raw( int fd_tty, struct termios *p_tty, int baud );

/*
 * tty_set: set the tty device termios. Ususally, resume the orginal setting get from tty-set
 */
void tty_set( int fd_tty, struct termios *p_tty );

int tty_ready( int fd, int tout );

/*
 * tty_getc: read one raw character from tty device. Special key with escape sequence
 *           will be converted to single key code.
 *   fd_tty: fd of tty (must call term_raw once, before use this function)
 *   tout: time out setting.
 *	 > 0 time out in mini-seconds.
 *         0 no time out, return immediately if no input in buffer.
 *        -1 wait until a character is read.
 *   esc_on: enable the escape sequence interpret. 0 will turn it off.
 *    return value:
 *       > 0: character received from tty
 *         0: no data.
 *        -1: I/O error
 */
int tty_getc( int fd, int tout, int esc_on );
int tty_putc(int fd, int ch);

/* tty_gets: read one line which terminated with specified character
 * parameters:
 *   tout:
 *	0: No wait. Get until no character or 'eol' is received.
 *     -1: Get until buffer full or eol is get.
 *     >0: 'tout' msec after last character is received.
 *    eol: end of line character. If not 0 then receive until this 'eol' is encounted
 *         (or tout, or buffer full).
 *     ec: address to hold the end code (0 normal ending, -1 tout, -2 buffer full)
 * return value:
 *    >0: number of bytes received.
 */
int tty_gets( int fd, char *buf, int bufsize, int tout, int eol, int *ec );

// Followung function uses the terminal's Escape Sequence to control the
// display effect. Don't set tty to raw mode. Just open it then control.
// This is a easy way to do the simple control w/o the huge curset.

// get binary data. return number of bytes read.
// tout is time out in msec to wait for next byte. (tout between bytes).
//   0: no wait
//  -1: wait forever until 'bufsize' bytes are received.
//  >0: msec to wait for next byte.
int tty_read_tout( int fd, char *buf, int bufsize, int tout );

// discard specified queue (input or output)
#define TTY_IQUEUE	TCIFLUSH
#define TTY_OQUEUE	TCOFLUSH
#define tty_flush(fd, qsel)		tcflush(fd,qsel)
#define tty_flush_in(fd)			tcflush(fd,TCIFLUSH)
#define tty_flush_out(fd)			tcflush(fd,TCOFLUSH)
#define tty_flush_io(fd)			tcflush(fd,TCIFLUSH|TCOFLUSH)
 
// wait for output queue drain (send out)
#define tty_drain(fd)					tcdrain(fd)
// tty read/write
#define tty_write(fd,buf,sz)	write(fd,buf,sz)
#define tty_read(fd,buf,sz)		read(fd,buf,sz)

int tty_skip_until(int fd, unsigned char *soh, int size, int *found );
int tty_skip_until_oneof(int fd, unsigned char *soh[], int nh, int size, int *soh_index );
int tty_read_n_bytes(int fd, void *buffer, int len);
int tty_skip_n_bytes( int fd, int n );

#define tty_close(fd)		close(fd)

#ifdef ENABLE_TTYESCSEQ
/*
 * tty cursor control
 */
void tty_goto( int tty, int row, int col );

/* display attributes setting */
void tty_attribute( int tty, ... );

#define KEY_ESC		'\033'
#define	KEY_BS		'\177'
/*
 * following code is used to replace the escape sequence of special key stroke with
 * a single byte key code. Please noted that it works based on the assumption of
 * the escape sequence define in bellow. This sequence is true for most of TTY
 * (console, telnet psuedo, VTxxx) but not the old fashion terminal such as HPxxx
 */
#define KEY_UP		(0x80|'A')
#define KEY_DOWN	(0x80|'B')
#define KEY_RIGHT	(0x80|'C')
#define KEY_LEFT	(0x80|'D')
#define KEY_PAUSE	(0x80|'P')
#define KEY_HOME	(0x80|'1')
#define KEY_INS		(0x80|'2')
#define KEY_DEL		(0x80|'3')
#define KEY_END		(0x80|'4')
#define KEY_PGUP	(0x80|'5')
#define KEY_PGDN	(0x80|'6')
#define KEY_FN(n)	(0x80|('f'+(n)))		/* F1 ~ F10 (n is 1 ~ 10) */
#define KEY_SFN(n)	(0x80|('F'+(n)))		/* Shift-F1 ~ Shift-F10 (n is 1 ~ 10) */

/* escape sequence for tty control */

#define ESC_CTRL_UP	"\033[A"
#define ESC_CTRL_DOWN	"\033[B"
#define ESC_CTRL_RIGHT	"\033[C"
#define ESC_CTRL_LEFT	"\033[D"

#define ESC_CTRL_CLEAR		"\033[2J"	// clear screen
#define ESC_CTRL_CLEAR2BOS	"\033[1J"	// clear to home
#define ESC_CTRL_CLEAR2EOS	"\033[0J"	// clear to end of screen
#define ESC_CTRL_CLEAR2EOL	"\033[K"	// clear to end of line
#define ESC_CTRL_SAVE_CURSOR	"\033[s"	// save cursor
#define ESC_CTRL_RESTOTE_CURSOR	"\033[u"	// restore cursor

/* TTY text display attribute */
#define TTY_TA_OFF		0		// Off all except color
#define TTY_TA_HIGHLIGHT	1		// High Intensity
#define TTY_TA_DEEM		2		// low intensity
#define TTY_TA_ITALIC		3		// Italic (most of terminal does not support)
#define TTY_TA_UNDERLINE	4		// Underline
#define TTY_TA_BLINK		5		// Blinking
#define TTY_TA_RAPIDBLINK	6		// Rapid blinking
#define TTY_TA_REVERSE		7		// Reversed video
	/* forground color */
#define TTY_FC_BLACK		30
#define TTY_FC_RED		31
#define TTY_FC_GREEN		32
#define TTY_FC_YELLOW		33
#define TTY_FC_BLUE		34
#define TTY_FC_MEGENTA		35
#define TTY_FC_CYAN		36
#define TTY_FC_WHITE		37
	/* background color */
#define TTY_BC_BLACK		40
#define TTY_BC_RED		41
#define TTY_BC_GREEN		42
#define TTY_BC_YELLOW		43
#define TTY_BC_BLUE		44
#define TTY_BC_MEGENTA		45
#define TTY_BC_CYAN		46
#define TTY_BC_WHITE		47

#define tty_esccmd( tty, escseq )	write( tty, escseq, strlen(escseq) )
#define tty_clear(tty)			tty_esccmd(tty, ESC_CTRL_CLEAR)
#define tty_clear2bos(tty)		tty_esccmd(tty, ESC_CTRL_CLEAR2BOS)
#define tty_clear2eos(tty)		tty_esccmd(tty, ESC_CTRL_CLEAR2EOS)
#define tty_clear2eol(tty)		tty_esccmd(tty, ESC_CTRL_CLEAR2EOL)

	/* cursor movement control */
#define tty_home(tty)			tty_esccmd(tty, "\033[1;1H")
#define tty_up(tty)			tty_esccmd(tty, ESC_CTRL_UP)
#define tty_down(tty)			tty_esccmd(tty, ESC_CTRL_DOWN)
#define tty_right(tty)			tty_esccmd(tty, ESC_CTRL_RIGHT)
#define tty_left(tty)			tty_esccmd(tty, ESC_CTRL_LEFT)
#define tty_save_curpos(tty)		tty_esccmd(tty, ESC_CTRL_SAVE_CURSOR )
#define tty_restore_curpos(tty)		tty_esccmd(tty, ESC_CTRL_RESTOTE_CURSOR )

/* common used display attribute control */
#define tty_highlight(tty)		tty_esccmd(tty, "\033[1m" )
#define tty_deem(tty)			tty_esccmd(tty, "\033[2m" )
#define tty_italic(tty)			tty_esccmd(tty, "\033[3m" )
#define tty_underline(tty)		tty_esccmd(tty, "\033[4m" )
#define tty_blink(tty)			tty_esccmd(tty, "\033[5m" )
#define tty_quickblink(tty)		tty_esccmd(tty, "\033[6m" )
#define tty_reverse(tty)		tty_esccmd(tty, "\033[7m" )
#define tty_normal(tty)			tty_esccmd(tty, "\033[0m" )

	/* set forground color */
#define tty_text_black(tty)		tty_esccmd(tty, "\033[30m" )
#define tty_text_red(tty)		tty_esccmd(tty, "\033[31m" )
#define tty_text_green(tty)		tty_esccmd(tty, "\033[32m" )
#define tty_text_yellow(tty)		tty_esccmd(tty, "\033[33m" )
#define tty_text_blue(tty)		tty_esccmd(tty, "\033[34m" )
#define tty_text_magenta(tty)		tty_esccmd(tty, "\033[35m" )
#define tty_text_cyan(tty)		tty_esccmd(tty, "\033[36m" )
#define tty_text_white(tty)		tty_esccmd(tty, "\033[37m" )

	/* set background color */
#define tty_bg_black(tty)		tty_esccmd(tty, "\033[40m" )
#define tty_bg_red(tty)			tty_esccmd(tty, "\033[41m" )
#define tty_bg_green(tty)		tty_esccmd(tty, "\033[42m" )
#define tty_bg_yellow(tty)		tty_esccmd(tty, "\033[43m" )
#define tty_bg_blue(tty)		tty_esccmd(tty, "\033[44m" )
#define tty_bg_magenta(tty)		tty_esccmd(tty, "\033[45m" )
#define tty_bg_cyan(tty)		tty_esccmd(tty, "\033[46m" )
#define tty_bg_white(tty)		tty_esccmd(tty, "\033[47m" )

	/* common used color combination black/white text on color background */
#define tty_white_on_blue(tty)		tty_esccmd(tty, "\033[1;37;44m" )
#define tty_white_on_red(tty)		tty_esccmd(tty, "\033[1;37;41m" )
#define tty_black_on_yellow(tty)	tty_esccmd(tty, "\033[0;30;43m" )
#define tty_black_on_green(tty)		tty_esccmd(tty, "\033[0;30;42m" )
#define tty_black_on_cyan(tty)		tty_esccmd(tty, "\033[0;30;46m" )
#define tty_black_on_magenta(tty)	tty_esccmd(tty, "\033[0;30;45m" )
#define tty_white_on_magenta(tty)	tty_esccmd(tty, "\033[1;37;45m" )
#define tty_yellow_on_blue(tty)		tty_esccmd(tty, "\033[1;33;44m" )
#endif

#ifdef __cplusplus
};
#endif

#endif

