/***************************************************************************
                          utils_conf.h  -  description
                             -------------------
	An XML syntax alike configuration tools
 ***************************************************************************/

#ifndef _UTILS_CONF_H_
#define _UTILS_CONF_H_

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------------
<section>
key1	value1	# comment1
key2	value2	# comment2
key3	value3

<subsection1>
key1	value1
key2	value2
</subsection1>

</section>
------------------------------------------------------------------------- */

#include "utils_ptrlist.h"

#define 	MAX_CONFUNIT_TEXT	0x10000	/* 64K bytes */

#define	STRSTRMAP_INITIALIZER	{NULL,NULL}

typedef struct tagConfUnit {
	char* 		key;
	PtrList		items;		/* pointer list of StrStrMap */
	PtrList		subunits;	/* pointer list of ConfUnit */
} ConfUnit;

#define CONFUNIT_INITIALIZER	{NULL,PTRLIST_INITIALIZER,PTRLIST_INITIALIZER}

extern ConfUnit* ConfUnit_new( const char *key );
extern void ConfUnit_delete( ConfUnit *cf );		/* remove all elemnets and free cf */

extern void ConfUnit_initialize( ConfUnit *cf, const char *key );
extern void ConfUnit_terminate( ConfUnit *cf );		/* remove all elements but keep cf */
extern void ConfUnit_clean( ConfUnit *cf );		/* remove all elements except cf->key */

/* get item value */
extern const char* ConfUnit_get_item( ConfUnit *cf, const char *key );
extern const char* ConfUnit_get_item_safe( ConfUnit *cf, const char *key, const char *def_value );
#define 	   ConfUnit_get_item_as_string(cf,key) ConfUnit_get_item(cf,key)
#define 	   ConfUnit_get_item_as_string_safe(cf,key,def) ConfUnit_get_item_safe(cf,key,def)
extern int ConfUnit_get_item_as_int( ConfUnit *cf, const char *key, int defval );
extern int ConfUnit_get_item_as_enum( ConfUnit *cf, const char *key, int defval, const char **EnumString );
extern int ConfUnit_get_item_as_bool( ConfUnit *cf, const char *key, int defval );
extern double ConfUnit_get_item_as_double( ConfUnit *cf, const char *key, double defval );

/* get sub unit */
extern ConfUnit* ConfUnit_get_subunit( ConfUnit *cf, const char *key );

/* add item & subunit */
extern void ConfUnit_add_item( ConfUnit *cf, const char *key, const char *value );
extern void ConfUnit_add_subunit( ConfUnit *cf, ConfUnit *subcf );

/* set item,
 * param: 'value' can be NULL, to delete the item */
extern void ConfUnit_set_item( ConfUnit *cf, const char *key, const char *value );
#define		ConfUnit_set_item_as_string(cf,key,value) ConfUnit_set_item(cf,key,value)
#define		ConfUnit_set_item_as_int(cf,key,value) \
{ \
	char str[32]; \
	sprintf(str, "%ld", (long)value); \
	ConfUnit_set_item(cf,key,str); \
}
extern void ConfUnit_set_item_as_enum( ConfUnit *cf, const char *key, int value, const char **EnumString );

/* delete item,
 * return: 0 when done, non-zero when not found */
extern int ConfUnit_delete_item( ConfUnit *cf, const char *key );
extern int ConfUnit_delete_subunit( ConfUnit *cf, const char *key );

/* serialize string */
extern const char* ConfUnit_parse_text( ConfUnit *cf, const char* text );
extern int ConfUnit_calculate_text_size( ConfUnit *cf );
extern int ConfUnit_to_text( ConfUnit *cf, char * buffer, int buf_size );

/* free the return buffer after use */
extern char* ConfUnit_get_text( ConfUnit *cf );

/* serialize file */
extern int ConfUnit_load_from( ConfUnit *cf, const char* filename );
extern void ConfUnit_save_to_fp( ConfUnit *cf, FILE *fp );
extern int ConfUnit_save_to( ConfUnit *cf, const char* filename );
extern void ConfUnit_dump( ConfUnit *cf, int deepth, FILE *fp );

/* create a string array contains all subunit name */
extern char **ConfUnit_subunit_list( ConfUnit *conf );
/* create a string array contains all items name */
extern char **ConfUnit_item_list( ConfUnit *conf );
/* create a string array contains all items value */
extern char **ConfUnit_itemvalue_list( ConfUnit *conf );

/* sort */
extern void ConfUnit_sort( ConfUnit *conf );
extern void ConfUnit_sort_enable( ConfUnit *conf );	// enable the sort (shall be enabled before load element into it)
extern void ConfUnit_sort_disable( ConfUnit *conf );	// disable the sort (default)

#ifdef __cplusplus
};
#endif

#endif /* _UTILS_CONF_H_ */
