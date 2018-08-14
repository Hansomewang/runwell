/***************************************************************************
                          utils_file.h  -  description
                             -------------------
    begin                : Thu Dec 20 2001
    copyright            : (C) 2001 by Liming Xie
    email                : liming_xie@yahoo.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _UTILS_FILE_H_
#define _UTILS_FILE_H_

#ifdef __cplusplus
extern "C" {
#endif

extern int get_filename( const char *path, char* buffer, int buf_size );

extern int get_dirname( const char *path, char* buffer, int buf_size );

/* create directory, and create all the parent directory if dose not exist.
 * return: 0 when okay, < 0 when failed. */
extern int make_full_dir( const char * path );

extern int get_abs_path( const char * dirname, const char * rel_path, char * buf, int bufsize );

extern int get_filesize( const char *path );
extern int fget_filesize( int fd );

extern char* read_text_file( const char * filepath );

extern int tmpdump( FILE* dest, FILE* src );

extern int tmpdumpfd( int fd, FILE* src );

extern int touch_file( const char *path );
extern int file_replace_str( const char *path, const char *str_old, const char *str_new, int bcaseignore );
extern int file_replace_str_after( const char *path, const char *str_prefix, const char *str_new, int bcaseignore );
//extern int get_freespace( const char *mntpoint );		// return # of KB
//extern int get_usedspace( const char *dir );				// return # of KB
int delete_files(const char *dir,...);

#ifdef __cplusplus
};
#endif

#endif /* _UTILS_FILE_H_ */
