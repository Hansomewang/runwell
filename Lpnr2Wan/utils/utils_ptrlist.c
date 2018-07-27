/***************************************************************************
                          utils_ptrlist.c  -  description
                             -------------------
 ***************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "utils_ptrlist.h"

void PtrList_print(PtrList *list)
{
	POSITION node;
	for(node=list->head; node!=NULL; node=node->next) {
		fprintf(stderr, "%p\n", node->ptr);
	}
	fprintf(stderr, "\n");
}

void PtrList_initialize( PtrList *list )
{
	memset(list, 0, sizeof(*list) );
}

// compatible API with function in hp_ptrlist.c
void PtrList_terminate( PtrList *list )
{
	PtrList_remove_all(list);
}

PtrList* PtrList_new( void )
{
	PtrList *list;

	list = (PtrList *)malloc( sizeof(PtrList) );
	if(! list) return NULL;
	memset(list, 0, sizeof(*list) );

	return list;
}

void PtrList_set_compare( PtrList *list, __compar_fn_t cmpfunc )
{
	list->ptrcmp = cmpfunc;
}

int PtrList_get_count(PtrList *list)
{
	return list->count;
}

POSITION PtrList_get_head(PtrList *list)
{
	return list->head;
}

POSITION PtrList_get_rear(PtrList *list)
{
	return list->rear;
}

POSITION PtrList_get_next(PtrList *list, POSITION node)
{
	return node->next;
}

POSITION PtrList_get_prev(PtrList *list, POSITION node)
{
	return node->prev;
}

void* PtrList_get(PtrList *list, POSITION node)
{
	return node->ptr;
}

int PtrList_get_all(PtrList *list, void* *buf, int buf_size)
{
	POSITION node;
	void* *p;
	int n;

	n = 0;
	p = buf;
	for( node = list->head; node != NULL; node = node->next ) {
		*p = node->ptr;
		p ++;
		n ++;
		if(n >= buf_size) break;
	}
	return n;
}

POSITION PtrList_find(PtrList *list, void *ptr)
{
	POSITION node;

	for(node=list->head; node!=NULL; node=node->next) {
		if( node->ptr == ptr ) return node;
	}
	return NULL;
}

POSITION PtrList_insert_before(PtrList *list, POSITION node, void *ptr)
{
	POSITION newnode, prev_node;

	newnode =( POSITION) malloc( sizeof(PtrNode) );
	if(! newnode) return NULL;
	//memset( newnode, 0, sizeof(PtrNode));
	newnode->ptr = ptr;

	if(NULL == node) { /* insert before all node */
		newnode->prev = NULL;
		newnode->next = list->head;
		if( NULL != list->head ) {
			list->head->prev = newnode;
		}

		list->head = newnode;
		if(NULL == list->rear) { /* there is no node before */
			list->rear = newnode;
		}
	} else {
		prev_node = node->prev;

		node->prev = newnode;
		newnode->next = node;

		if( NULL == prev_node ) { /* if this is the first node */
			list->head = newnode;
			newnode->prev = NULL;
		} else {
			prev_node->next = newnode;
			newnode->prev = prev_node;
		}
	}

	list->count ++;

	return newnode;
}

POSITION PtrList_insert_after(PtrList *list, POSITION node, void *ptr)
{
	POSITION newnode, next_node;

	newnode = (POSITION)malloc( sizeof(PtrNode) );
	if(! newnode) return NULL;
	//memset( newnode, 0, sizeof(PtrNode));
	newnode->ptr = ptr;

	if(NULL == node) { /* insert after all node */
		newnode->next = NULL;
		newnode->prev = list->rear;
		if( NULL != list->rear ) {
			list->rear->next = newnode;
		}

		if(NULL == list->head) { /* there is no node before */
			list->head = newnode;
		}
		list->rear = newnode;
	} else {
		next_node = node->next;

		node->next = newnode;
		newnode->prev = node;

		if( NULL == next_node ) { /* if this is the last node */
			list->rear = newnode;
			newnode->next = NULL;
		} else {
			next_node->prev = newnode;
			newnode->next = next_node;
		}
	}

	list->count ++;

	return newnode;
}

POSITION PtrList_append(PtrList *list, void *ptr)
{
	return PtrList_insert_after( list, NULL, ptr );
}

POSITION PtrList_insert_1to9(PtrList *list, void *ptr)
{
	POSITION node = NULL;
	if ( list->ptrcmp == NULL ) {
		for( node=list->head;
		     node!=NULL;
		     node=node->next ) {
			if(ptr < node->ptr)
				return PtrList_insert_before(list, node, ptr);
		}
	} else {
		for( node=list->head;
		     node!=NULL;
		     node=node->next ) {
			if( (*list->ptrcmp)( ptr, node->ptr) < 0 )
				return PtrList_insert_before(list, node, ptr);
		}
	}
	return PtrList_insert_after(list, node, ptr);
}

POSITION PtrList_insert_9to1(PtrList *list, void *ptr)
{
	POSITION node = NULL;
	if ( list->ptrcmp == NULL ) {
		for( node=list->head;
		     node!=NULL;
		     node=node->next ) {
			if(ptr > node->ptr)
				return PtrList_insert_before(list, node, ptr);
		}
	} else {
		for( node=list->head;
		     node!=NULL;
		     node=node->next ) {
			if( (*list->ptrcmp)( ptr, node->ptr) > 0 )
				return PtrList_insert_before(list, node, ptr);
		}
	}
	return PtrList_insert_after(list, node, ptr);
}

void* PtrList_remove(PtrList *list, POSITION node)
{
	POSITION prev_node, next_node;
	void *ptr;

	if ( node == NULL )	return NULL;
	prev_node = node->prev;
	next_node = node->next;

	if(NULL == prev_node) { /* this node is the first one */
		list->head = next_node; //node->next;
	} else {
		prev_node->next = next_node;
	}

	if(NULL == next_node) { /* this node is the last one */
		list->rear = prev_node; //node->prev;
	} else {
		next_node->prev = prev_node;
	}

	list->count --;

	ptr = node->ptr;
	free(node);

	return ptr;
}

void* PtrList_remove_head(PtrList *list)
{
	return PtrList_remove(list, list->head);
}

void* PtrList_remove_rear(PtrList *list)
{
	return PtrList_remove(list, list->rear);
}

// remove and free and double linking structure. The contents are remained.
void PtrList_remove_all( PtrList *list )
{
	POSITION node, del;

	node = list->head;
	while( node ) {
		del = node;
		node = node->next;
		free(del);
	}
	list->head = list->rear = NULL;
	list->count = 0;
}

/*
 * delete - remove + free the content of node->ptr
 */
void PtrList_delete(PtrList *list, POSITION node)
{
	POSITION prev_node, next_node;

	if ( node == NULL ) return;

	prev_node = node->prev;
	next_node = node->next;

	if(NULL == prev_node) { /* this node is the first one */
		list->head = next_node; //node->next;
	} else {
		prev_node->next = next_node;
	}

	if(NULL == next_node) { /* this node is the last one */
		list->rear = prev_node; //node->prev;
	} else {
		next_node->prev = prev_node;
	}

	list->count--;

	free( node->ptr );
	free(node);
}

void PtrList_delete_head(PtrList *list)
{
	PtrList_delete(list, list->head);
}

void PtrList_delete_rear(PtrList *list)
{
	PtrList_delete(list, list->rear);
}

// delete the structure and content. After delete_all, PtrList is fresh new.
void PtrList_delete_all( PtrList *list )
{
	POSITION node, del;

	node = list->head;
	while( node ) {
		del = node;
		node = node->next;
		free( del->ptr );
		free(del);
	}
	list->head = list->rear = NULL;
	list->count = 0;
}

/* ---------------------------
 * string operation
 * ---------------------------*/

POSITION StrList_find(PtrList *list, const char *string)
{
	POSITION node;

	for(node=list->head; node!=NULL; node=node->next) {
		if( 0 == strcasecmp(( const char *)node->ptr, string) ) return node;
	}
	return NULL;
}

POSITION StrList_exact_find(PtrList *list, const char *string)
{
	POSITION node;

	for(node=list->head; node!=NULL; node=node->next) {
		if( 0 == strcmp(( const char *)node->ptr, string) ) return node;
	}
	return NULL;
}

POSITION StrList_insert_string_atoz(PtrList *list, const char *string)
{
	POSITION node = NULL;
	for( node=list->head; node!=NULL; node=node->next ) {
		if( strcmp(string,( const char *)node->ptr) < 0 ) {
			return PtrList_insert_before(list, node, (void*)string);
		}
	}
	return PtrList_insert_after(list, node, (void*)string);
}

POSITION StrList_insert_string_ztoa(PtrList *list, const char *string)
{
	POSITION node = NULL;
	for( node=list->head; node!=NULL; node=node->next ) {
		if( strcmp(string,( const char *)node->ptr) > 0 ) {
			return PtrList_insert_before(list, node, (void*)string);
		}
	}
	return PtrList_insert_after(list, node, (void*)string);
}

void		StrList_remove_all( StrList *list )
{
	char		* item;

	while( list->count > 0 ) {
		item = (char *)PtrList_remove_head(list);
		free(item);
	}
}

/* ---------------------------
 * string mapping operation
 * ---------------------------*/

/* return: NULL if not found */
const char * StrMap_get( StrMap *strmap, const char * key )
{
	PtrList		* list = strmap;
	POSITION 	pos;
	StrStrPair	* item;

	if ( key != NULL )
	{
		for(pos=list->head; pos!=NULL; pos=pos->next)
		{
			item = (StrStrPair *)pos->ptr;
			if( 0 == strcasecmp(item->key, key) )
				return item->value;
		}
	}
	return NULL;
}

/* if not found, return default value instead of NULL pointer */
const char * StrMap_safe_get( StrMap * strmap, const char * key, const char * default_value )
{
	PtrList		* list = strmap;
	POSITION 	pos;
	StrStrPair	* item;

	if ( key != NULL )
	{
		for(pos=list->head; pos!=NULL; pos=pos->next)
		{
			item = (StrStrPair *)pos->ptr;
			if( 0 == strcasecmp(item->key, key) )
				return item->value;
		}
	}
	return default_value ? default_value : "";
}

/* param: 'value' can be NULL, to remove specified item;
 * return: number of items */
int StrMap_set( StrMap *strmap, const char *key, const char * value )
{
	StrStrPair	* item;
	POSITION	pos;
	PtrList		* list = strmap;

	if ( key == NULL ) return list->count;

	for(pos=list->head; pos!=NULL; pos=pos->next) {
		item = (StrStrPair *)pos->ptr;
		if( 0 == strcasecmp(item->key, key) ) {
			if( value == NULL ) {
				item = (StrStrPair *)PtrList_remove(list,pos);
				free(item->key);
				free(item->value);
				free(item);
			} else {
				free(item->value);
				item->value = strdup(value);
			}
			break;
		}
	}

	if( (!pos) && (value != NULL) )
	{ 	/* not found, append new map item */
		item = (StrStrPair *)malloc( sizeof(StrStrPair) );
		if(item)
		{
			item->key = strdup(key);
			item->value = strdup(value);
			if( item->key && item->value ) {
				PtrList_append(list, item);
			} else {
				if( item->key ) free( item->key );
				if( item->value ) free( item->value );
				free( item );
			}
		}
	}
	return list->count;
}

// prepend a key/value pair into Map. It will hide the existing duplicated key.
// used by core_dwp.c
void StrMap_prepend( StrMap *strmap, const char *key, const char *value )
{
	StrStrPair	* item;

	item = (StrStrPair *)malloc( sizeof(StrStrPair) );
	if(item)
	{
		item->key = strdup(key);
		item->value = strdup(value);
		if( item->key && item->value ) {
			PtrList_insert_before(strmap, NULL, item);
		} else {
			if( item->key ) free( item->key );
			if( item->value ) free( item->value );
			free( item );
		}
	}
}

void StrMap_remove( StrMap * strmap, const char * key )
{
	StrStrPair	* item;
	POSITION	pos;
	PtrList		* list = strmap;

	for(pos=list->head; pos!=NULL; pos=pos->next)
	{
		item = (StrStrPair *)pos->ptr;
		if( 0 == strcasecmp(item->key, key) )
		{
			item = (StrStrPair *)PtrList_remove(list,pos);
			free(item->key);
			free(item->value);
			free(item);
			break;
		}
	}
}

void StrMap_remove_all( StrMap * strmap )
{
	StrStrPair	* item;
	PtrList		* list = strmap;

	while( list->count > 0 )
	{
		item = (StrStrPair *)PtrList_remove_head(list);
		free(item->key);
		free(item->value);
		free(item);
	}
}


/* -------------------------------------------------------------------------- *
#include <stdio.h>

int main( int argc, char* argv[] )
{
	PtrList a;
	POSITION pos;
	int i;

	memset( &a, 0, sizeof(a) );

	fprintf(stderr, "sizeof(int)=%d, sizeof(void*)=%d\n", sizeof(int), sizeof(void*) );


	for(i=0; i<20; i++) {
		PtrList_insert_1to9( &a, (void*) (rand()%100) );
	}
	for( pos = PtrList_get_head( &a ); pos != NULL; pos = PtrList_get_next(&a, pos) ) {
		fprintf(stderr, "%02d ", (int)PtrList_get(&a, pos) );
	}
	fprintf(stderr, "\n" );
	PtrList_remove_all( &a );


	for(i=0; i<20; i++) {
		PtrList_insert_1to9( &a, (void*) (rand()%100) );
	}
	while( PtrList_get_count(&a) > 0 ) {
		fprintf(stderr, "%02d ", (int)PtrList_remove_head(&a) );
	}
	fprintf(stderr, "\n" );


	for(i=0; i<20; i++) {
		PtrList_insert_9to1( &a, (void*) (rand()%100) );
	}
	while( PtrList_get_count(&a) > 0 ) {
		fprintf(stderr, "%02d ", (int)PtrList_remove_rear(&a) );
	}
	fprintf(stderr, "\n" );

	return 0;
}
* -------------------------------------------------------------------------- */
