#ifndef UTILS_TIMER_INCLUDED
#define UTILS_TIMER_INCLUDED

#include <time.h>

typedef void (*tmr_hander)(int id);
#define TMR_INFINITE	(-1)

// NOTE - all function return 0 for success, -1 for failure.
int TMR_initialize();
void TMR_terminate();
// start a repeating timer, period is 't_period' (in msec), first fire time is 
// 't_period' msec after added. When timer is fired, user supplied handle function 'handle_fxc'
// will be invoked with the 'id' of this timer as argument.
int TMR_add_repeat(int id, tmr_hander handle_fxc, int t_period, int count);
// add one-shot timer fired at specified time
int TMR_add_absolute(int id, tmr_hander handle_fxc, time_t t_start);
// remove specified timer
int TMR_kill(int id);
// check if specified timer is added
int TMR_has(int id);

#endif
