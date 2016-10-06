#include "timer.h"

struct timeval start_time;

void init_timer()
{
	gettimeofday(&start_time, 0);
}

unsigned long long curr_time()
{
	static struct timeval curr;
	gettimeofday(&curr, 0);
	return (curr.tv_sec * 1000 + (curr.tv_usec / 1000.0));
}


