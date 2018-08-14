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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include "utils_file.h"
#include "utils_str.h"


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
	long offset;
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
		offset = ftell(fp);
		ftruncate(fileno(fp),offset);
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
	char buf[2048], *ptr, *q;
	FILE *fp = fopen( path, "r+" );
	if ( fp != NULL )
	{
		int len = fread( buf, 1, sizeof(buf), fp );
		buf[len] = '\0';
		if ( (!bcaseignore && (ptr=strstr( buf, str_prefix )) != NULL) ||
			   (bcaseignore && (ptr=strcasestr( buf, str_prefix )) != NULL) )
		{
			ptr += strlen(str_prefix);
			rewind( fp );
			fwrite( buf, 1, ptr-buf, fp );
			fwrite( str_new, 1, strlen(str_new), fp );
			// skip leading white-space after str_prefix
			for(q=ptr; *q && (*q==' ' || *q=='\t' || *q=='\n'); q++);
			// then skip a token (string replaced by str_new
			for(; *q && *q!=' ' && *q!='\t' && *q!='\n'; q++);
			fwrite( q, 1, strlen(q), fp );
			*q = '\0';
			printf("file_replace_str_after - string \"%s\" replaced by \"%s\"\n", ptr, str_new );
		}
		else
		{
			printf("file_replace_str_after - prefix string %s not found in file %s\n", str_prefix, path );
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

#define MAX_EXTENSION		10
#define MAX_FILE2DELETE	1000

int delete_files(const char *dir,...)
{
	DIR *hdir;
	const char *ext2del[MAX_EXTENSION];
	int i, num_extension = 0;
	struct dirent *entry;
	va_list va;
	int num2delete = 0;
	int numdeleted = 0;
	char *file2delete[MAX_FILE2DELETE];
	char path[PATH_MAX];
	
	// retrieve all file extensions that would be deleted.
	va_start(va,dir);
	while ( num_extension < MAX_EXTENSION && (ext2del[num_extension]=va_arg(va,char *)) != NULL )
		num_extension++;
	va_end(va);
	
	if ( (hdir=opendir(dir)) == NULL ) return -1;
	while( num2delete<MAX_FILE2DELETE && (entry=readdir(hdir)) != NULL )
	{
		char *ptr_ext;
		if ( entry->d_type == DT_REG && (num_extension==0 || (ptr_ext = strrchr(entry->d_name,'.')) != NULL) )
		{
			int catched = num_extension>0 ? 0 : 1;
			ptr_ext++;
			
			for(i=0; i<num_extension && !catched; i++ )
			{
				if ( strcmp(ptr_ext,ext2del[i])==0 )
					catched = 1;
			}
			if ( catched )
			{
				file2delete[num2delete++] = strdup(entry->d_name);
			}
		}
	}
	closedir(hdir);
	// delete all catched files
	for(i=0; i<num2delete; i++)
	{
		strcpy(path, dir);
		strcat(path, file2delete[i]);
		if ( unlink(path)== 0 )
			numdeleted++;
		free(file2delete[i]);
	}
	return numdeleted;
}

/*
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

*/
