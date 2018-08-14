/***************************************************************************
                          utils_conf.c  -  description
                             -------------------
 ***************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include "utils_conf.h"

#define MAX_STRING	128

//#define DEBUGCONF

ConfUnit* ConfUnit_new( const char *key )
{
	ConfUnit *cf;

	cf = (ConfUnit *)malloc( sizeof(*cf) );
	if( ! cf ) return NULL;
	memset(cf, 0, sizeof(*cf) );

	cf->key = strdup(key);
	PtrList_initialize( & cf->items );
	PtrList_initialize( & cf->subunits );

	return cf;
}

void ConfUnit_delete( ConfUnit *cf )
{
	ConfUnit_terminate( cf );
	free(cf);
}

void ConfUnit_initialize( ConfUnit *cf, const char *key )
{
	cf->key = strdup(key);
	PtrList_initialize( & cf->items );
	PtrList_initialize( & cf->subunits );
}

void ConfUnit_clean( ConfUnit *cf )
{
	StrStrPair	*item;
	ConfUnit 	*subunit;

	while( cf->items.count > 0 )
	{
		item = (StrStrPair*) PtrList_remove_head(& cf->items);
		free(item->key);
		free(item->value);
		free(item);
	}

	while( cf->subunits.count > 0 )
	{
		subunit = (ConfUnit*) PtrList_remove_head(& cf->subunits);
		ConfUnit_terminate( subunit );
		free( subunit );
	}
}
	
void ConfUnit_terminate( ConfUnit *cf )
{
	ConfUnit_clean( cf );
	free(cf->key);
}

void ConfUnit_set_item( ConfUnit *cf, const char *key, const char *value )
{
	StrStrPair	*item = NULL;
	PtrList		*list = & cf->items;
	POSITION 	pos, pos_this;

//	Cannot work like this. PtrList_remove will free the "pos" and cause
//	trouble in pos=PtrList_get_next(..., pos)
//	Same changes are made in ConfUnit_delete_item and ConfUnit_delet_subunit.
//	for(pos=list->head; pos!=NULL; pos=PtrList_get_next(list,pos) ) {
	for(pos=list->head; pos!=NULL; ) 
	{
		// item = (StrStrPair*) PtrList_get(list, pos);
		item = (StrStrPair*) PtrList_get(list, pos); // modify 
		pos_this = pos;
		pos = pos->next;
		if(0 == strcmp(key, item->key)) {
			if( value != NULL ) {
				free(item->value);
				item->value = strdup(value);
			} else {
				item = (StrStrPair *)PtrList_remove(list,pos_this);
				free(item->key);
				free(item->value);
				free(item);
			}
			return;
		}
	}
	// not found, then add new item
	if(value!=NULL) ConfUnit_add_item(cf, key, value);
}

const char* ConfUnit_get_item( ConfUnit *cf, const char *key )
{
	StrStrPair	*item;
	PtrList		*list = & cf->items;
	POSITION 	pos;

	for(pos=list->head; pos!=NULL; pos=pos->next ) {
		item = (StrStrPair*) pos->ptr;
		if(0 == strcmp(key, item->key)) {
			return item->value;
		}
	}
	return NULL;
}

const char* ConfUnit_get_item_safe( ConfUnit *cf, const char *key, const char *def_value )
{
	StrStrPair	*item;
	PtrList		*list = & cf->items;
	POSITION 	pos;

	for(pos=list->head; pos!=NULL; pos=pos->next ) {
		item = (StrStrPair*) pos->ptr;
		if(0 == strcmp(key, item->key)) {
			return item->value;
		}
	}
	return def_value;
}

void ConfUnit_set_item_as_enum( ConfUnit *cf, const char *key, int value, const char **EnumString )
{
#ifdef DEBUG
	int i;

	for( i = 0; EnumString && EnumString[ i ]; i++ );
	if( EnumString && i <= value )
	{
		fprintf( stderr, "Error in set_item_as_int( %s, %s, %d ).\n", cf ->key, key, value );
		return;
	]
#endif

	if( EnumString )
	{
		ConfUnit_set_item( cf, key, EnumString[ value ] );
	}
	else
	{
		char string_value[ 20 ];

		sprintf( string_value, "%d", value );
		ConfUnit_set_item( cf, key, string_value );
	}

}

int ConfUnit_get_item_as_enum( ConfUnit *cf, const char *key, int defval, const char **EnumString )
{
	const char* strval;
	int i;

	strval = ConfUnit_get_item(cf, key);
	if(! strval) return defval;

	for( i = 0; EnumString && EnumString[ i ]; i++ )
	{
		if( 0 == strcasecmp( strval, EnumString[ i ] ) )
			return i;
	}

	if( EnumString && ! isdigit( *strval ) && *strval != '-' )
		return defval;

	return strtol(strval,NULL,0);
}

int ConfUnit_get_item_as_int( ConfUnit *cf, const char *key, int defval )
{
	const char* strval;
	strval = ConfUnit_get_item(cf, key);
	if(! strval) return defval;

	return strtol(strval,NULL,0);
}

int ConfUnit_get_item_as_bool( ConfUnit *cf, const char *key, int defval )
{
	const char* strval;
	const char* truestr[] = { "true", "yes", "enable", "on", "1" };
	const char* falsestr[] = { "false", "no", "disable", "off", "0" };
	int i;

	strval = ConfUnit_get_item(cf, key);
	if(! strval) return defval;

	for(i=0; i<sizeof(truestr)/sizeof(char*); i++) {
		if( 0 == strcasecmp(strval, truestr[i]) ) return 1;
	}

	for(i=0; i<sizeof(truestr)/sizeof(char*); i++) {
		if( 0 == strcasecmp(strval, falsestr[i]) ) return 0;
	}

	return defval;
}

double ConfUnit_get_item_as_double( ConfUnit *cf, const char *key, double defval )
{
	const char* strval;
	strval = ConfUnit_get_item(cf, key);
	if(! strval) return defval;

	return strtod(strval,NULL);
}

/* get sub unit */
ConfUnit* ConfUnit_get_subunit( ConfUnit *cf, const char *key )
{
	ConfUnit	*subunit;
	PtrList		*list = & cf->subunits;
	POSITION 	pos;

	for(pos=list->head; pos!=NULL; pos=pos->next ) {
		subunit = (ConfUnit*) pos->ptr;
		if( 0 == strcmp(key, subunit->key) ) return subunit;
	}
	return NULL;
}

int ConfUnit_delete_item( ConfUnit *cf, const char *key )
{
	StrStrPair	*item;
	PtrList		*list = & cf->items;
	POSITION 	pos, pos_this;

	for(pos=list->head; pos!=NULL; )
	{
		item = (StrStrPair*) pos->ptr;
		pos_this = pos;
		pos = pos->next;
		if(0 == strcmp(key, item->key))
		{
			PtrList_remove(list, pos_this);
			free(item->value);
			free(item->key);
			free(item);
			return 0;
		}
	}
	return -1;
}

int ConfUnit_delete_subunit( ConfUnit *cf, const char *key )
{
	ConfUnit	*subunit;
	PtrList		*list = & cf->subunits;
	POSITION 	pos, pos_this;

	for(pos=list->head; pos!=NULL; )
	{
		subunit = (ConfUnit*) pos->ptr;
		pos_this = pos;
		pos = pos->next;
		if( 0 == strcmp(key, subunit->key) )
		{
			PtrList_remove(list, pos_this);
			ConfUnit_delete(subunit);
			return 0;
		}
	}
	return -1;
}

/* set item & subunit */
void ConfUnit_add_item( ConfUnit *cf, const char *key, const char *value )
{
	StrStrPair	*newitem;
	newitem = (StrStrPair *)malloc( sizeof(*newitem) );
	if(! newitem) return;

	newitem->key = strdup(key);
	newitem->value = strdup(value);

	PtrList_append(& cf->items, (void*)newitem);
}

void ConfUnit_add_subunit( ConfUnit *cf, ConfUnit *subcf )
{
	PtrList_append(& cf->subunits, (void*)subcf);
}

/* serialize string */
const char* ConfUnit_parse_text( ConfUnit *cf, const char* text )
{
	const char *ptr, *q, *tmp;
	char newkey[MAX_STRING], newvalue[MAX_STRING];
	int unitkeylen, keylen, valuelen;
	ConfUnit *newunit;

	unitkeylen = strlen(cf->key);

	ptr = text;

	while(*ptr!='\0') {
		for(; *ptr==' ' || *ptr=='\t'; ptr++);
		if(*ptr=='<') {
			ptr++;
			if(*ptr=='/') {
				ptr++;
				if( (0 == memcmp(cf->key,ptr,unitkeylen)) &&
					('>' == *(ptr+unitkeylen)) ) {
					/* meet end of this section */
					ptr += unitkeylen+1;
					while(*ptr!='\n' && *ptr!='\0') ptr++;
					if(*ptr=='\n') ptr++;
#if defined(DEBUGCONF)
					fprintf( stderr, "</%s>\n", cf->key );
#endif
					return ptr;
				} else {
					/* a bad </ flag */
#ifdef DEBUGCONF
					fprintf( stderr, "Bad </ flag, expecting </%s>.\n", cf->key );
#endif
					return NULL;
				}
			} else {
				/* meet a sub section */
				for(q=ptr; *q!='>' && *q!='\0' && *q!='\n'; q++);
				keylen = q-ptr;
				if( keylen >= MAX_STRING ) return NULL;
				memcpy(newkey, ptr, keylen);
				newkey[keylen]='\0';
#if defined(DEBUGCONF)
				fprintf( stderr, "<%s>\n", newkey );
#endif
				newunit = ConfUnit_new( newkey );
				if(! newunit) return NULL;

				for(ptr=q; *ptr!='\n' && *ptr!='\0'; ptr++);
				if(*ptr=='\n') ptr++;

				tmp = ConfUnit_parse_text( newunit, ptr );
				if(tmp) {
					/* trying to parse new sub section okay */
					ConfUnit_add_subunit( cf, newunit );
					ptr = tmp;
				} else {
					/* trying to parse new sub section failed */
					ConfUnit_delete( newunit );
					return NULL;
				}
			}
		} else {
			/* seek a "key value" or "key = value", # start a comment */
			q = ptr;
			if ( *q == '"' )
				for ( q++, ptr++;*q != '"' && *q != '='; q++ );
			else
				for(;*q!=' ' && *q!='\t' && *q!='=' && *q!='#' && *q!='\n' && *q!='\0'; q++);
			if(*q==' ' || *q=='\t' || *q=='=' || *q=='"') {
				keylen = q-ptr;
				if(keylen >= MAX_STRING) return NULL;
				memcpy(newkey, ptr, keylen);
				newkey[keylen] = '\0';

				for(ptr=q; *ptr==' ' || *ptr=='\t' || *ptr=='=' || *ptr=='"'; ptr++);
				for(q=ptr; *q!='#' && *q!='\r' && *q!='\n' && *q!='\0'; q++);
				q--;
				for(; q>=ptr && (*q==' ' || *q=='\t'); q--);
				q ++;
				valuelen = q-ptr;
				if(valuelen >= MAX_STRING) return NULL;
				memcpy(newvalue, ptr, valuelen);
				newvalue[valuelen]='\0';

				ConfUnit_add_item(cf, newkey, newvalue);
#if defined(DEBUGCONF)
				fprintf( stderr, "%s\t\t=\t%s\n", newkey, newvalue );
#endif
			}
			for(ptr=q; *ptr!='\n' && *ptr!='\0'; ptr++);
			if(*ptr=='\n') ptr++;
		}
	}

	return ptr;
}

/* serialize file */
int ConfUnit_load_from( ConfUnit *cf, const char *filename )
{
	FILE *fp;
	char *text;
	const char *ptr;
	long len, readlen;
	int ret;

	/* open the file to read in configuration data */
	if( (fp = fopen(filename, "r")) ) {
		fseek(fp, 0L, SEEK_END);
		len = ftell(fp);

		/* max length: 64K bytes */
		if(len < MAX_CONFUNIT_TEXT) {
			text = (char *)malloc( len+1 );
			if( text ) {

				fseek(fp, 0L, SEEK_SET);
				readlen = fread(text, 1, len, fp);
#ifdef linux
				if(readlen == len) 
#else
				if ( 1 )		/* Windows will convert \r\n to \n and result in readlen < len */
#endif
				{
					
					text[readlen] = '\0';

					ptr = ConfUnit_parse_text( cf, text );
					if( ptr ) {
						ret = 0;
					} else {
						ret = -EINVAL;
					}

				} else ret = -EIO;

				free(text);
			} else ret = -ENOMEM;
		} else ret = -EINVAL;
		fclose(fp);
	} else ret = -EIO;

	return ret;
}

int ConfUnit_calculate_text_size( ConfUnit *cf )
{
	int n = 0;

	StrStrPair 	*item;
	ConfUnit	*subcf;
	POSITION	pos;

	for( pos=cf->items.head; pos!=NULL; pos=pos->next ) {
		item = (StrStrPair*) pos->ptr;

		n += strlen(item->key) + strlen(item->value) + strlen("%s\t= %s\r\n");
	}

	for( pos=cf->subunits.head; pos!=NULL; pos=pos->next ) {
		subcf = (ConfUnit*) pos->ptr;

		n += strlen(subcf->key) + strlen("<%s>\r\n");
		n += ConfUnit_calculate_text_size( subcf );
		n += strlen(subcf->key) + strlen("</%s>\r\n");
	}

	return n;
}

int ConfUnit_to_text( ConfUnit *cf, char * buffer, int buf_size )
{
	StrStrPair 	*item;
	ConfUnit	*subcf;
	POSITION	pos;
	char		*ptr = buffer, *buf_end = buffer + buf_size;

	for( pos=cf->items.head; pos!=NULL; pos=pos->next ) {
		item = (StrStrPair*) pos->ptr;

		if( ptr + strlen(item->key) + strlen(item->value) + strlen("%s\t= %s\r\n") >= buf_end ) break;
		if ( strchr( item->key, ' ' ) != NULL )
			sprintf(ptr, "\"%s\"\t= %s\r\n", item->key, item->value );
		else
			sprintf(ptr, "%s\t= %s\r\n", item->key, item->value );
		while(*ptr!='\0') ptr++;
	}

	for( pos=cf->subunits.head; pos!=NULL; pos=pos->next ) {
		subcf = (ConfUnit*) pos->ptr;

		if( ptr + strlen(subcf->key) + strlen("<%s>\r\n") >= buf_end ) break;
		sprintf( ptr, "<%s>\r\n", subcf->key );
		while(*ptr!='\0') ptr++;

		ptr += ConfUnit_to_text( subcf, ptr, buf_size - (ptr-buffer) );

		if( ptr + strlen(subcf->key) + strlen("</%s>\r\n") >= buf_end ) break;
		sprintf( ptr, "</%s>\r\n", subcf->key );
		while(*ptr!='\0') ptr++;
	}

	return (ptr - buffer);
}

char* ConfUnit_get_text( ConfUnit *cf )
{
	char	* buf = NULL;
	int		bufsize, realsize;

	bufsize = ConfUnit_calculate_text_size( cf );
	buf = (char *)malloc( bufsize );
	if(! buf) return NULL;

	realsize = ConfUnit_to_text( cf, buf, bufsize );
	return buf;
}

void ConfUnit_save_to_fp( ConfUnit *cf, FILE *fp )
{
	StrStrPair 	*item;
	ConfUnit	*subcf;
	POSITION	pos;

	for( pos=cf->items.head; pos!=NULL; pos=pos->next ) {
		item = (StrStrPair*) pos->ptr;
		if ( strchr( item->key, ' ' ) != NULL )
			fprintf( fp, "\"%s\"\t= %s\r\n", item->key, item->value );
		else
			fprintf( fp, "%s\t= %s\r\n", item->key, item->value );
	}

	for( pos=cf->subunits.head; pos!=NULL; pos=pos->next ) {
		subcf = (ConfUnit*) pos->ptr;
		fprintf( fp, "<%s>\r\n", subcf->key );
		ConfUnit_save_to_fp( subcf, fp );
		fprintf( fp, "</%s>\r\n", subcf->key );
	}
}

int ConfUnit_save_to( ConfUnit *cf, const char *filename )
{
	FILE *fp;

	fp = fopen(filename, "w");
	if(! fp) return -EIO;

	ConfUnit_dump( cf, 0, fp );
	fclose(fp);

	return 0;
}

void ConfUnit_dump( ConfUnit *cf, int deepth, FILE *fp )
{
	StrStrPair 	*item;
	ConfUnit	*subcf;
	POSITION	pos;
	int		indent;
	char		space[ MAX_STRING ];

	indent = deepth * 4;
	if ( indent >= MAX_STRING ) indent = MAX_STRING - 1;
	memset( space, ' ', indent );
	space[indent] = '\0';
//	fprintf( fp, "%s<%s>\n", space, cf->key );
	for( pos=cf->items.head; pos!=NULL; pos=pos->next )
	{
		item = (StrStrPair*) pos->ptr;
		if ( strchr( item->key, ' ' ) != NULL )
			fprintf( fp, "%s\"%-18s\" = %s\n", space, item->key, item->value );
		else
			fprintf( fp, "%s%-20s = %s\n", space, item->key, item->value );
	}

	for( pos=cf->subunits.head; pos!=NULL; pos=pos->next )
	{
		subcf = (ConfUnit*) pos->ptr;
		fprintf( fp, "%s<%s>\n", space, subcf->key );
		ConfUnit_dump( subcf, deepth+1, fp );
		fprintf( fp, "%s</%s>\n", space, subcf->key );
	}
//	fprintf( fp, "%s</%s>\n", space, cf->key );
}


char **ConfUnit_subunit_list( ConfUnit *conf )
{
	int	count, len, i;
	char 	**array=NULL;
	ConfUnit	*cf;
	POSITION	pos;

	count = conf->subunits.count;
	array = (char **)malloc( (count+1) * sizeof(char *) );
	for (i=0,pos=conf->subunits.head; pos!=NULL; pos=pos->next, i++ )
	{
		cf = (ConfUnit *)pos->ptr;
		len = strlen( cf->key );
		array[i] = (char *)malloc( len + 1);
		strcpy( array[i], cf->key );
	}
	array[i] = NULL;
	return array;
}

char **ConfUnit_item_list( ConfUnit *conf )
{
	int	count, len, i;
	char 	**array=NULL;
	StrStrPair	*item;
	POSITION	pos;

	count = conf->items.count;
	array = (char **)malloc( (count+1) * sizeof(char *) );
	for (i=0,pos=conf->items.head; pos!=NULL; pos=pos->next, i++ )
	{
		item = (StrStrPair *)pos->ptr;
		len = strlen( item->key );
		array[i] = (char *)malloc( len + 1);
		strcpy( array[i], item->key );
	}
	array[i] = NULL;
	return array;
}

char **ConfUnit_itemvalue_list( ConfUnit *conf )
{
	int	count, len, i;
	char 	**array=NULL;
	StrStrPair	*item;
	POSITION	pos;

	count = conf->items.count;
	array = (char **)malloc( (count+1) * sizeof(char *) );
	for (i=0,pos=conf->items.head; pos!=NULL; pos=pos->next, i++ )
	{
		item = (StrStrPair *)pos->ptr;
		len = strlen( item->value );
		array[i] = (char *)malloc( len + 1);
		strcpy( array[i], item->value );
	}
	array[i] = NULL;
	return array;
}

static int item_cmp( void * ptr1, void *ptr2 )
{
	StrStrPair	*item1 = (StrStrPair *)ptr1;
	StrStrPair	*item2 = (StrStrPair *)ptr2;

	return strcmp( item1->key, item2->key );
}
static int unit_cmp( void * ptr1, void *ptr2 )
{
	ConfUnit	*conf1 = (ConfUnit *)ptr1;
	ConfUnit	*conf2 = (ConfUnit *)ptr2;

	return strcmp( conf1->key, conf2->key );
}

void ConfUnit_sort( ConfUnit *conf )
{
	StrStrPair	**pItems, *item;
	ConfUnit	**pConf, *subconf;
	int		nitem = conf->items.count;
	int		nconf = conf->subunits.count;
	int		i;
	POSITION	pos;

	/* sort key */
	PtrList_set_compare( &conf->items, (__compar_fn_t)item_cmp );
	PtrList_set_compare( &conf->subunits, (__compar_fn_t)unit_cmp );

	if ( nitem > 0 )
	{
		pItems = (StrStrPair **)malloc( nitem * sizeof(StrStrPair *) );
		i = 0;
		while ( ( item = (StrStrPair *)PtrList_remove_head( &conf->items )) != NULL )
			pItems[i++] = item;

		for (i=0; i<nitem; i++)
			PtrList_insert_1to9( &conf->items, pItems[i] );
		free( pItems );
	}

	/* then the subconf */
	if ( nconf > 0 )
	{
		pConf = (ConfUnit **)malloc( nconf * sizeof(ConfUnit *) );
		i = 0;
		while ( ( subconf = (ConfUnit *)PtrList_remove_head( &conf->subunits )) != NULL )
			pConf[i++] = subconf;

		for (i=0; i<nconf; i++)
			PtrList_insert_1to9( &conf->subunits, pConf[i] );
		free( pConf );
	}

	/* then sort the content of subconf */
	for ( pos=conf->subunits.head; pos!=NULL; pos=pos->next )
	{
		subconf = (ConfUnit *)pos->ptr;
		ConfUnit_sort( subconf );
	}
}

/* ----------------------------- Function Demo -------------------------

int main(int argc, char* argv[])
{
	ConfUnit *cf, *subcf;
	int port;

	cf = ConfUnit_new(argv[1]);
	ConfUnit_load_from(cf, argv[1]);

	subcf = ConfUnit_get_subunit(cf, "mod_file");
	if(! subcf) {
		subcf = ConfUnit_new("mod_file");
		ConfUnit_add_subunit(cf, subcf);
	}

	ConfUnit_add_item(cf, "Port", "8080");
	ConfUnit_add_item(subcf, "DocumentRoot", "/home/xlm");

	port = ConfUnit_get_item_as_int(cf, "Port", 8080);

	ConfUnit_save_to(cf, argv[1]);

	ConfUnit_delete(cf);

	return 0;
}

------------------------------------------------------------------------- */
