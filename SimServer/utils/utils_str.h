/***************************************************************************
                          utils_str.h  -  description
 ***************************************************************************/

#ifndef _UTILS_STR_H_
#define _UTILS_STR_H_

#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * trim: trim the leading and trailing white space character of input string.
 *        white space characters includes: ' ', '\t', '\r\', '\n'
 *  on return, str has stored with trimed string already and return value is address of str itself.
 */
extern char* trim( char *str );

extern int strisnumber( const char *string, int *number );
extern int strisfloat( const char *string, float *num );
extern int strisboolean( const char *string, int *boolval );
extern int strstartwith(const char *string, const char *prefix, int minmatch, char ** pleft);
extern int strcasestartwith(const char *string, const char *prefix, int minmatch, char ** pleft);
extern int strcasestartwith(const char *string, const char *prefix, int minmatch, char ** pleft);
extern int strgettoken(const char *string, char *buf, char **pleft);
extern int strgetword( const char *string, char *buf, int size, char **pleft);
extern int strgettokenpair( const char *src, char *keyword, char *operand, char **pleft );
extern int strgettokenpair2( const char *src, char *keyword, char *operand, int *pVal1, int *pVal2, char **pleft );
extern int strgettokenpairN( const char *src, char *keyword, char *operand, int Val[], int *vlimit, char **pleft );
extern int strbreak(const char *src, char *args[], int arg_limit);
extern void strrelease(int argc, char *args[]);
extern int stridxinargs( const char *key, int minmatch,...);
extern int stridxinarray( const char *key, int minmatch, const char *array[], int size);
extern int strhanzi2pinying( const char *platenum, char *out  );
extern char *strdupline( const char *src, char **next_pos );
extern int strrowcol( const char *src, int *cols );

extern int str_b64enc(const char *src, char *buf, int bufsize );
extern int str_b64dec(const char* string, char *buf, int bufsize);

#ifdef __cplusplus
};
#endif

#endif /* _UTILS_STR_H_ */
