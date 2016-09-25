#ifndef TIMER_H
#define TIMER_H

#include <sys/time.h>

extern struct timeval start_time;
extern void init_timer();
extern unsigned long long curr_time();

#endif
