#ifndef _WIN32

#include <sys/time.h>
#include <thrd/thrd.h>

#ifdef THRD_NO_TIMESPEC_GET
int timespec_get(struct timespec *ts, int base) {
	struct timeval tv;

	if (base != TIME_UTC) {
		return 0;
	}

	if (gettimeofday(&tv, 0) == -1) {
		return 0;
	}

	ts->tv_sec  = tv.tv_sec;
	ts->tv_nsec = tv.tv_usec * 1000;
	return base;
}
#endif

#endif /* !_WIN32 */
