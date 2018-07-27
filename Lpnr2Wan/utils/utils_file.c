/***************************************************************************
                          utils_file.c  -  description
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
#define _GNU_SOURCE		// enable some GNU specific string function prototype declaration in string.h

#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include "utils_file.h"
#include "utils_str.h"

#ifdef linux
#include <dirent.h>
#include <unistd.h>
#else
#include "windows/types.h"
#endif

int get_filename( const char *path, char* buffer, int buf_size )
{
	int			len = -1;
	const char	*ptr;

	for(ptr=path+strlen(path); ptr>=path; ptr--) {
		if(*ptr=='/') {
			ptr ++;
			len = strlen(ptr);
			if(len >= buf_size) return -1;
			strcpy( buffer, ptr );
			break;
		}
	}

	return len;
}

int get_dirname( const char *path, char* buffer, int buf_size )
{
	int 			len = -1;
	const char		*ptr;

	for(ptr=path+strlen(path); ptr>=path; ptr--) {
		if(*ptr=='/') {
			len = ptr - path;
			if(len >= buf_size) return -1;
			memcpy(buffer, path, len);
			buffer[len] = '\0';
			break;
		}
	}

	return len;
}

int make_full_dir( const char * path )
{
	struct stat 	finfo;
	char 		updir[PATH_MAX];

	if( get_dirname(path, updir, PATH_MAX) <= 0 ) {
		return -1;
	}

//	if( (0 == lstat(updir,&finfo)) ||
	if( (0 == stat(updir,&finfo)) ||
		(0 == make_full_dir(updir)) ) {
		return mkdir(path, 0755);
	} else {
		return -1;
	}
}

int get_abs_path( const char * dirname, const char * rel_path, char * buf, int bufsize )
{
	int dlen, flen;

	if( (! dirname) ||
		(! rel_path) ||
		(dlen = strlen(dirname)) + (flen = strlen(rel_path)) > (bufsize -2)
		) {
		return -EINVAL;
	}

	if( *rel_path == '/' ) {
		strcpy( buf, rel_path );
		return flen;
	}

	memcpy( buf, dirname, dlen );
	if( buf[dlen-1] != '/' ) {
		buf[dlen++] = '/';
		buf[dlen] = '\0';
	}
	memcpy( buf + dlen, rel_path, flen );
	*(buf + dlen + flen) = '\0';

	return (dlen + flen);
}

int get_filesize( const char *path )
{
	struct stat	fs;
	int	rc = -1;

	if ( stat(path, &fs) == 0 )
		rc = fs.st_size;
	return rc;
}

int fget_filesize( int fd )
{
	struct stat	fs;
	int	rc = -1;

	if ( fstat(fd, &fs) == 0 )
		rc = fs.st_size;
	return rc;
}

char* read_text_file( const char * filepath )
{
	char* text = NULL;
	FILE* fp = NULL;

	if( (fp = fopen(filepath,"rb")) != NULL ) {
		int filelen, readlen;

		fseek(fp, 0L, SEEK_END);
		filelen = ftell(fp);
		if( (filelen>0) && ((text = malloc(sizeof(char)*(filelen+1)))!=NULL)) {
			fseek(fp, 0L, SEEK_SET);
			if( (readlen = fread(text, sizeof(char), filelen, fp)) > 0 ) {
				text[ readlen ] = '\0';
			} else {
				free(text);
				text = NULL;
			}
		}

		fclose(fp);
	}

	return text;
}

int tmpdump( FILE* dest, FILE* src )
{
	char buf[ 4096 ];
	int  nread = 0, nwrite = 0, ntotal = 0;

	fseek(src, 0, SEEK_SET);
	while( ((nread = fread(buf, sizeof(char), 4096, src)) > 0) &&
		   ((nwrite = fwrite(buf, sizeof(char), nread, dest)) == nread) ) {
		ntotal += nwrite;
	}
	return ntotal;
}

int tmpdumpfd( int fd, FILE* src )
{
	char buf[ 4096 ];
	int  nread = 0, nwrite = 0, ntotal = 0;

	fseek(src, 0, SEEK_SET);
	while( ((nread = fread(buf, sizeof(char), 4096, src)) > 0) &&
		   ((nwrite = write(fd, buf, nread)) == nread) ) {
		ntotal += nwrite;
	}
	return ntotal;
}

int touch_file( const char *path )
{
	int fd = open( path, O_CREAT|O_RDWR, 0666 );
	if ( fd != -1 )
		close(fd);
	return fd >= 0 ? 0 : -1;
}

int file_replace_str( const char *path, const char *str_old, const char *str_new, int bcaseignore )
{
	char buf[2048], *ptr;
	FILE *fp = fopen( path, "r+" );
	if ( fp != NULL )
	{
		int len = fread( buf, 1, sizeof(buf), fp );
		buf[len] = '\0';
		if ( (!bcaseignore && (ptr=strstr( buf, str_old )) != NULL) ||
			   (bcaseignore && (ptr=strcasestr( buf, str_old )) != NULL) )
		{
			rewind( fp );
			fwrite( buf, 1, ptr-buf, fp );
			fwrite( str_new, 1, strlen(str_new), fp );
			ptr += strlen( str_old );
			fwrite( ptr, 1, strlen(ptr), fp );
		}
		fclose( fp );
		return ptr==NULL ? -1 : 0;
	}
	else
	{
		printf("file_replace_str - file %s open for update error.\n", path );
		return -1;
	}
}

int file_replace_str_after( const char *path, const char *str_prefix, const char *str_new, int bcaseignore )
{
	char buf[2048], *ptr, *str_old;
	FILE *fp = fopen( path, "r+" );
	if ( fp != NULL )
	{
		int len = fread( buf, 1, sizeof(buf), fp );
		buf[len] = '\0';
		if ( (!bcaseignore && (ptr=strstr( buf, str_prefix )) != NULL) ||
			   (bcaseignore && (ptr=strcasestr( buf, str_prefix )) != NULL) )
		{
			ptr += strlen(str_prefix);
			while ( *ptr==' ') ptr++;
			rewind( fp );
			// resize the buffer size
			truncate(path,0);
			fwrite( buf, 1, ptr-buf, fp );
			fwrite( str_new, 1, strlen(str_new), fp );
			str_old = ptr;
			while ( *ptr != ' ' && *ptr!='\t' && *ptr!='\n' ) ptr++;		// skip old string
			fwrite( ptr, 1, strlen(ptr), fp );
			*ptr = '\0';
			printf("file_replace_str_after - string %s replaced by %s\n", str_old, str_new );
		}
		else
		{
			printf("file_replace_str_after - prefix string %s not foun in file %s\n", str_prefix, path );
		}
		fclose( fp );
		return ptr==NULL ? -1 : 0;
	}
	else
	{
		printf("file_replace_str_after - file %s open for update error.\n", path );
		return -1;
	}
}

int get_freespace( const char *mntpoint )
{
	char buf[128];
	int	 nfreeKB=0;
	FILE *fp = popen("df", "r");
	
	if ( fp == NULL )
		return -1;
	
	while( fgets(buf, sizeof(buf),fp) != NULL )
	{
		if ( strstr(buf, mntpoint) != NULL )
		{
			int i;
			const char *token;
			token = strtok(buf, " \t");		// first token
			for(i=0; i<3; i++)
				token = strtok(NULL, " \t");
			nfreeKB = atoi(token);
			break;
		}
	}
	pclose(fp);
	printf("%s - free space=%dK\n", mntpoint, nfreeKB );
	return nfreeKB;
}

int get_usedspace( const char *dir )
{
	char buf[128];
	const char *token;
	int	 nusedKB=0;
	FILE *fp;
	
	sprintf(buf, "du -s %s", dir );
	fp = popen(buf, "r");
	
	if ( fp == NULL )
		return -1;
	
	while( fgets(buf, sizeof(buf),fp) != NULL )
	{
		if ( (token=strtok(buf," \t")) != NULL &&  strisnumber(token, &nusedKB) )
		{
			break;
		}
	}
	pclose(fp);
	printf("%s - used space=%dK\n", dir, nusedKB );
	return nusedKB;	
}

