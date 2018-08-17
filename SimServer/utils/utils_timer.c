/*
 * utils_timer.c
 *
 * implement a simple easy to used timer mechanism. Application can easily add multiple timer
 * (one-shot or periodic) each one with same or different handler function.
 * all timer are identified by an unique Id specified by user when added.. 
 * handler function can identify which timer is fired by this 'id' when same fucntion handle
 * multiple timer.
 *
 * Author: Thomas Chang
 * Date: 2016-09-12
 *
 * Version 1.1
 * Date: 2017-02-28
 * Description:
 *   Set working thread priority at high level
 *   add function 'TMR_has' to detect whether specified timer_id is in list
 */
 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include "utils_timer.h"
#include "utils_ptrlist.h"
#include "longtime.h"
 
static pthread_mutex_t	list_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t	w_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t		w_cond = PTHREAD_COND_INITIALIZER;
static PtrList		tmr_list = PTRLIST_INITIALIZER;

static pthread_t	tmr_thread;
static int				in_service = 1;

#define list_lock()			pthread_mutex_lock(&list_lock)
#define list_unlock()		pthread_mutex_unlock(&list_lock)

typedef struct {
	int		id;
	tmr_hander	handle;
	int 	count;
	_longtime	lt_fire;		// time to fired
	int		t_period;				// 0 for one-shot timer
} tmr_entry;


static int tmr_find(int id)
{
	POSITION pos;
	list_lock();
	for(pos=tmr_list.head; pos!=NULL; pos=pos->next)
	{
		tmr_entry *member = (tmr_entry *)pos->ptr;
		if ( member->id == id ) break;
	}
	list_unlock();
	return pos!=NULL;
}


static void tmr_insert( tmr_entry *entry)
{
	POSITION pos;
	list_lock();
	for(pos=tmr_list.head; pos!=NULL; pos=pos->next)
	{
		tmr_entry *member = (tmr_entry *)pos->ptr;
		if ( entry->lt_fire < member->lt_fire )
		{
			PtrList_insert_before(&tmr_list, pos, entry);
			break;
		}
	}
	if ( pos==NULL )
		PtrList_append(&tmr_list, entry);
	list_unlock();
}


static void *timer_server_fxc(void *lparam)
{
	struct timespec ts;
	tmr_entry  *tmr_head;
	
	for(;in_service;)
	{
		list_lock();
		_longtime lt_now = timeGetLongTime();
		clock_gettime( CLOCK_REALTIME, &ts ); 
		if ( tmr_list.count == 0 )
		{
			ts.tv_sec += 60;		// for empty list, sleep 60 second
			tmr_head = NULL;
		}
		else
		{
			tmr_head = (tmr_entry *)tmr_list.head->ptr;
			if ( tmr_head->lt_fire <= lt_now )
			{
				// head entry fire time is due, remove it out of list
				PtrList_remove_head(&tmr_list);
			}
			else
			{
				// head entry fire time later than now. calculate time to wait.
				int delta_ms;
				ts.tv_sec += (int)(tmr_head->lt_fire - lt_now)/1000;
				delta_ms = (int)((tmr_head->lt_fire%1000) - (lt_now%1000) );
				ts.tv_nsec += delta_ms * 1000000;
				if ( ts.tv_nsec < 0 )
				{
					ts.tv_sec--;
					ts.tv_nsec += 1000000000;
				}
				tmr_head = NULL;		// set as NULL when no one shall be fired.
			}			
		}
		list_unlock();
		
		// if head entry need to be fired
		if ( tmr_head != NULL )
		{
			// if this is periodic timer, calculate next fired time before invoke handler
			if ( tmr_head->t_period > 0 )
				tmr_head->lt_fire = lt_now + tmr_head->t_period;
			tmr_head->handle(tmr_head->id);
			// check to delete or insert back to list for next time to fire
			if ( tmr_head->count==TMR_INFINITE || --tmr_head->count > 0 )
			{
				tmr_insert(tmr_head);
			}
			else
				free(tmr_head);
		}
		else
		{
			// empty list or fire time of head timer not due yet. wait
			pthread_mutex_lock( &w_lock );
			pthread_cond_timedwait( &w_cond, &w_lock, &ts );
			pthread_mutex_unlock( &w_lock );
		}
	}
	return NULL;
}


int TMR_initialize()
{
	pthread_attr_t thread_attr;
	struct sched_param sch_param;
	int pri_min, pri_max, pri_middle;

	pthread_attr_init(&thread_attr);
	// we have to create a gpio thread with higher priority to prevent lost data
	pri_max = sched_get_priority_max(SCHED_RR);
	pri_min = sched_get_priority_min(SCHED_RR);
	pri_middle = (pri_max + pri_min) / 2;
	sch_param.__sched_priority = (pri_middle+pri_max)/2;
	pthread_attr_setschedpolicy(&thread_attr,SCHED_RR);
	pthread_attr_setschedparam(&thread_attr, &sch_param);
		
	in_service = 1;
	return pthread_create(&tmr_thread, &thread_attr, timer_server_fxc, NULL);
}


void TMR_terminate()
{
	// stop working thread
	in_service = 0;
	pthread_mutex_lock(&w_lock);
	pthread_cond_signal(&w_cond);
	pthread_mutex_unlock(&w_lock);
	pthread_join(tmr_thread,NULL);
	// delete all timer entries in list
	PtrList_delete_all(&tmr_list);
}


// start repeating timer, period is t_period (in msec), first fire time is after 't_period' after added
int TMR_add_repeat(int id, tmr_hander handle_fxc, int t_period, int count)
{
	if ( tmr_find(id)==0 && t_period>0 && handle_fxc )
	{
		tmr_entry *entry = malloc(sizeof(tmr_entry));
		if ( entry==NULL )  // out of heap memory
			return -1;
		entry->id = id;
		entry->handle = handle_fxc;
		entry->count = count;
		entry->t_period = t_period;
		entry->lt_fire = timeGetLongTime() + t_period;
		tmr_insert(entry);
		// wake up working thread
		pthread_mutex_lock(&w_lock);
		pthread_cond_signal(&w_cond);
		pthread_mutex_unlock(&w_lock);
		return 0;
	}
	return -1;
}


// add one-short timer fired at specified time
int TMR_add_absolute(int id, tmr_hander handle_fxc, time_t t_start)
{
	if ( tmr_find(id)==0 && t_start > time(NULL) && handle_fxc )
	{
		tmr_entry *entry = malloc(sizeof(tmr_entry));
		if ( entry==NULL )  // out of heap memory
			return -1;
		entry->id = id;
		entry->handle = handle_fxc;
		entry->count = 1;
		entry->t_period = 0;
		entry->lt_fire = timeLongTimeFromTime(t_start);
		tmr_insert(entry);
		// wake up working thread
		pthread_mutex_lock(&w_lock);
		pthread_cond_signal(&w_cond);
		pthread_mutex_unlock(&w_lock);
		return 0;
	}
	return -1;
}

int TMR_kill(int id)
{
	POSITION pos;
	list_lock();
	for(pos=tmr_list.head; pos!=NULL; pos=pos->next)
	{
		tmr_entry *entry = (tmr_entry *)pos->ptr;
		if ( entry->id == id )
		{
			PtrList_delete(&tmr_list,pos);
			break;
		}
	}
	list_unlock();
	// no need to wake up working thread for one timer entry deleted. 
	return pos==NULL ? -1 : 0;
}

int TMR_has(int id)
{
	POSITION pos;
	list_lock();
	for(pos=tmr_list.head; pos!=NULL; pos=pos->next)
	{
		tmr_entry *entry = (tmr_entry *)pos->ptr;
		if ( entry->id == id )
			break;
	}
	list_unlock();
	return pos!=NULL;
}
